// ==WindhawkMod==
// @id              taskbar-split
// @name            Taskbar Split: Running Left, Pinned Right
// @description     Places running apps on the left and closed pinned apps on the right, with flexible empty space between them (Windows 11).
// @version         0.1.0
// @author          Arkadiusz + OpenAI Codex
// @github          https://github.com/Artllex
// @homepage        https://github.com/Artllex/taskbar-split
// @include         explorer.exe
// @architecture    x86-64
// @compilerOptions -lcomctl32 -lole32 -loleaut32 -lruntimeobject
// @license         GPL-3.0
// ==/WindhawkMod==

// Copyright (C) 2026 Arkadiusz and OpenAI Codex.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Layout-hooking and taskbar-XAML access patterns are adapted from
// "Taskbar Start Button Centered Origin" by rick/rycalvo and Windhawk mods
// by Michael Maltsev (m417z), under GPL-3.0.

// ==WindhawkModReadme==
/*
# Taskbar Split

Creates two dynamic application zones on the Windows 11 taskbar:

`[Start/System] [Running apps]  <flexible empty space>  [Closed pinned apps] [Tray/Clock]`

Launching a pinned app moves it to the left zone. Closing it returns it to
the right zone. The persistent Windows pin list is not changed; only the live
XAML layout is rearranged.

Version 0.1.0 targets the horizontal primary taskbar on Windows 11 x64.
Disable the mod to immediately return to the standard Windows layout.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- leftPadding: 8
  $name: Left edge padding
  $description: Empty space before the first system button, in pixels.
- runningGap: 8
  $name: Gap after system buttons
  $description: Space between Start/Search/Widgets/Task View and running apps.
- trayGap: 8
  $name: Gap before tray
  $description: Space between closed pinned apps and the notification area.
- middleGap: 48
  $name: Minimum middle gap
  $description: Preferred minimum empty space between running and closed pinned groups. When the taskbar is crowded, icon spacing is compressed before this gap is reduced.
- systemButtonsLeft: true
  $name: Keep system buttons on the left
  $description: Put Start, Search, Widgets and Task View at the left edge. Recommended for the intended split layout.
*/
// ==/WindhawkModSettings==

#include <windhawk_utils.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <commctrl.h>

#undef GetCurrentTime

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Xaml.Automation.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <winrt/Windows.UI.Xaml.Shapes.h>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/base.h>

#define WH_WINRT_WINUI2
#include <winrt/Microsoft.UI.Xaml.Controls.h>

using namespace winrt::Windows::UI::Xaml;

struct ModSettings {
    std::atomic<int> leftPadding;
    std::atomic<int> runningGap;
    std::atomic<int> trayGap;
    std::atomic<int> middleGap;
    std::atomic<bool> systemButtonsLeft;
};

ModSettings g_settings;
std::atomic<bool> g_unloading;
std::atomic<HWND> g_taskbarWnd;
std::atomic<bool> g_taskbarSubclassed;
std::atomic<bool> g_taskbarViewLoaded;
std::atomic<bool> g_layoutRequestPending;
thread_local bool g_inLayoutPass;

void LoadSettings() {
    g_settings.leftPadding = std::max(0, Wh_GetIntSetting(L"leftPadding"));
    g_settings.runningGap = std::max(0, Wh_GetIntSetting(L"runningGap"));
    g_settings.trayGap = std::max(0, Wh_GetIntSetting(L"trayGap"));
    g_settings.middleGap = std::max(0, Wh_GetIntSetting(L"middleGap"));
    g_settings.systemButtonsLeft =
        Wh_GetIntSetting(L"systemButtonsLeft") != 0;
}

FrameworkElement EnumChildElements(
    FrameworkElement element,
    const std::function<bool(FrameworkElement)>& callback) {
    int count = Media::VisualTreeHelper::GetChildrenCount(element);
    for (int i = 0; i < count; i++) {
        auto child = Media::VisualTreeHelper::GetChild(element, i)
                         .try_as<FrameworkElement>();
        if (child && callback(child)) {
            return child;
        }
    }
    return nullptr;
}

FrameworkElement FindChildByName(FrameworkElement element, PCWSTR name) {
    return EnumChildElements(element, [name](FrameworkElement child) {
        return child.Name() == name;
    });
}

FrameworkElement FindChildByClassName(FrameworkElement element,
                                       PCWSTR className) {
    return EnumChildElements(element, [className](FrameworkElement child) {
        return winrt::get_class_name(child) == className;
    });
}

std::vector<FrameworkElement> GetRepeaterChildren(FrameworkElement element) {
    std::vector<FrameworkElement> result;
    auto repeater =
        element.try_as<winrt::Microsoft::UI::Xaml::Controls::ItemsRepeater>();
    if (!repeater) {
        return result;
    }

    auto source = repeater.ItemsSourceView();
    int count = source ? source.Count() : 0;
    for (int i = 0; i < count; i++) {
        auto item = repeater.TryGetElement(i);
        auto child = item ? item.try_as<FrameworkElement>() : nullptr;
        if (child) {
            result.push_back(child);
        }
    }
    return result;
}

HWND FindPrimaryTaskbarWindow() {
    HWND result = nullptr;
    EnumWindows(
        [](HWND hwnd, LPARAM value) -> BOOL {
            DWORD pid = 0;
            WCHAR className[32]{};
            if (GetWindowThreadProcessId(hwnd, &pid) &&
                pid == GetCurrentProcessId() &&
                GetClassName(hwnd, className, ARRAYSIZE(className)) &&
                _wcsicmp(className, L"Shell_TrayWnd") == 0) {
                *reinterpret_cast<HWND*>(value) = hwnd;
                return FALSE;
            }
            return TRUE;
        },
        reinterpret_cast<LPARAM>(&result));
    return result;
}

void* CTaskBand_ITaskListWndSite_vftable;
using CTaskBand_GetTaskbarHost_t = void*(WINAPI*)(void*, void**);
CTaskBand_GetTaskbarHost_t CTaskBand_GetTaskbarHost_Original;
void* TaskbarHost_FrameHeight_Original;
using RefCount_Decref_t = void(WINAPI*)(void*);
RefCount_Decref_t RefCount_Decref_Original;

XamlRoot XamlRootFromTaskbarHost(void* sharedPtr[2]) {
    if (!sharedPtr[0] && !sharedPtr[1]) {
        return nullptr;
    }

    struct DecrefGuard {
        void* value;
        ~DecrefGuard() {
            if (value && RefCount_Decref_Original) {
                RefCount_Decref_Original(value);
            }
        }
    } guard{sharedPtr[1]};

    if (!sharedPtr[0]) {
        return nullptr;
    }

    size_t elementOffset = 0;
    bool matched = false;
#if defined(_M_X64)
    const BYTE* bytes =
        reinterpret_cast<const BYTE*>(TaskbarHost_FrameHeight_Original);
    if (bytes[0] == 0x48 && bytes[1] == 0x83 && bytes[2] == 0xEC &&
        bytes[4] == 0x48 && bytes[5] == 0x83 && bytes[6] == 0xC1 &&
        bytes[7] <= 0x7F) {
        elementOffset = bytes[7];
        matched = true;
    }
#endif
    if (!matched) {
        Wh_Log(L"Unsupported TaskbarHost::FrameHeight prologue");
        return nullptr;
    }

    auto unknown = *reinterpret_cast<IUnknown**>(
        reinterpret_cast<BYTE*>(sharedPtr[0]) + elementOffset);
    if (!unknown) {
        return nullptr;
    }

    FrameworkElement taskbarElement = nullptr;
    unknown->QueryInterface(winrt::guid_of<FrameworkElement>(),
                            winrt::put_abi(taskbarElement));
    return taskbarElement ? taskbarElement.XamlRoot() : nullptr;
}

XamlRoot GetTaskbarXamlRoot(HWND hwnd) {
    HWND taskbandWindow =
        reinterpret_cast<HWND>(GetProp(hwnd, L"TaskbandHWND"));
    if (!taskbandWindow) {
        return nullptr;
    }

    void* taskBand = reinterpret_cast<void*>(
        GetWindowLongPtr(taskbandWindow, 0));
    if (!taskBand) {
        return nullptr;
    }

    void* taskListSite = taskBand;
    for (int i = 0;
         *reinterpret_cast<void**>(taskListSite) !=
             CTaskBand_ITaskListWndSite_vftable;
         i++) {
        if (i == 20) {
            return nullptr;
        }
        taskListSite = reinterpret_cast<void**>(taskListSite) + 1;
    }

    void* sharedPtr[2]{};
    CTaskBand_GetTaskbarHost_Original(taskListSite, sharedPtr);
    return XamlRootFromTaskbarHost(sharedPtr);
}

FrameworkElement FindTaskbarRepeater(FrameworkElement root) {
    FrameworkElement child = root;
    if (child &&
        (child = FindChildByClassName(child, L"Taskbar.TaskbarFrame")) &&
        (child = FindChildByName(child, L"RootGrid")) &&
        (child = FindChildByName(child, L"TaskbarFrameRepeater"))) {
        return child;
    }
    return nullptr;
}

winrt::weak_ref<FrameworkElement> g_cachedRepeater;

FrameworkElement GetTaskbarRepeater() {
    if (FrameworkElement cached = g_cachedRepeater.get()) {
        if (cached.XamlRoot()) {
            return cached;
        }
        g_cachedRepeater = nullptr;
    }

    HWND hwnd = g_taskbarWnd;
    XamlRoot root = hwnd ? GetTaskbarXamlRoot(hwnd) : nullptr;
    if (!root) {
        return nullptr;
    }
    auto content = root.Content().try_as<FrameworkElement>();
    auto repeater = content ? FindTaskbarRepeater(content) : nullptr;
    if (repeater) {
        g_cachedRepeater = repeater;
    }
    return repeater;
}

double FullWidth(FrameworkElement element) {
    Thickness margin = element.Margin();
    return margin.Left + element.ActualWidth() + margin.Right;
}

double ElementLeftRelativeTo(FrameworkElement element,
                             FrameworkElement ancestor) {
    auto transform = element.TransformToVisual(ancestor);
    auto point = transform.TransformPoint(
        winrt::Windows::Foundation::Point{0, 0});
    return point.X;
}

double TaskbarWidthLocal() {
    HWND hwnd = g_taskbarWnd;
    RECT rect{};
    if (!hwnd || !GetWindowRect(hwnd, &rect)) {
        return 0;
    }
    UINT dpi = GetDpiForWindow(hwnd);
    double scale = dpi ? 96.0 / dpi : 1.0;
    return (rect.right - rect.left) * scale;
}

enum class SystemButton { None, Start, Widgets, Search, TaskView };

SystemButton IdentifySystemButton(FrameworkElement element) {
    auto className = winrt::get_class_name(element);
    if (className == L"Taskbar.ExperienceToggleButton") {
        auto id = Automation::AutomationProperties::GetAutomationId(element);
        if (id == L"StartButton") {
            return SystemButton::Start;
        }
        if (id == L"TaskViewButton") {
            return SystemButton::TaskView;
        }
    } else if (className == L"Taskbar.AugmentedEntryPointButton") {
        return SystemButton::Widgets;
    } else if (className == L"Taskbar.TaskbarExtensionElement") {
        return SystemButton::Search;
    }
    return SystemButton::None;
}

bool IsTaskListButton(FrameworkElement element) {
    return winrt::get_class_name(element) == L"Taskbar.TaskListButton";
}

using TaskListButton_get_IsRunning_t = HRESULT(WINAPI*)(void*, bool*);
TaskListButton_get_IsRunning_t TaskListButton_get_IsRunning_Original;

bool IsRunning(FrameworkElement element) {
    // Fail left: if a future Windows build loses the optional state symbol,
    // don't strand a genuinely running app in the launcher-only right zone.
    if (!TaskListButton_get_IsRunning_Original) {
        return true;
    }
    bool running = false;
    HRESULT result = TaskListButton_get_IsRunning_Original(
        winrt::get_abi(
            element.as<winrt::Windows::Foundation::IUnknown>()),
        &running);
    return FAILED(result) ? true : running;
}

struct ButtonEntry {
    FrameworkElement element;
    double width;
    bool running;
};

std::unordered_map<void*, double> g_targetX;

double CompressedStepScale(const std::vector<ButtonEntry*>& group,
                           double allocatedWidth,
                           bool anchoredLeft) {
    if (group.size() < 2) {
        return 1.0;
    }

    double total = 0;
    for (auto* item : group) {
        total += item->width;
    }
    if (total <= allocatedWidth) {
        return 1.0;
    }

    // The edge-most icon remains completely inside its anchored edge.
    double fixedOuterWidth = anchoredLeft ? group.back()->width
                                          : group.front()->width;
    double compressible = total - fixedOuterWidth;
    double room = allocatedWidth - fixedOuterWidth;
    return compressible > 0 ? std::clamp(room / compressible, 0.0, 1.0)
                            : 1.0;
}

void BuildLayoutPlan() {
    HWND hwnd = g_taskbarWnd;
    if (!hwnd ||
        GetWindowThreadProcessId(hwnd, nullptr) != GetCurrentThreadId()) {
        return;
    }

    try {
        FrameworkElement repeater = GetTaskbarRepeater();
        if (!repeater) {
            return;
        }
        auto content =
            repeater.XamlRoot().Content().try_as<FrameworkElement>();
        if (!content) {
            return;
        }

        auto children = GetRepeaterChildren(repeater);
        std::unordered_map<void*, double> plan;
        std::vector<ButtonEntry> entries;
        std::vector<FrameworkElement> systemButtons;

        for (auto& child : children) {
            if (IsTaskListButton(child)) {
                entries.push_back({child, FullWidth(child), IsRunning(child)});
            } else if (IdentifySystemButton(child) != SystemButton::None) {
                systemButtons.push_back(child);
            }
        }

        double leftStart = g_settings.leftPadding.load();
        if (g_settings.systemButtonsLeft.load()) {
            for (auto& button : systemButtons) {
                if (button.ActualWidth() <= 0) {
                    continue;
                }
                plan[winrt::get_abi(button)] = leftStart;
                leftStart += FullWidth(button);
            }
        } else {
            // Find the native end of the system-button cluster and start
            // running apps after it without changing those buttons.
            leftStart = g_settings.leftPadding.load();
            for (auto& button : systemButtons) {
                if (button.ActualWidth() <= 0) {
                    continue;
                }
                double nativeLeft = ElementLeftRelativeTo(button, content);
                leftStart = std::max(leftStart, nativeLeft + FullWidth(button));
            }
        }
        leftStart += g_settings.runningGap.load();

        FrameworkElement tray =
            FindChildByClassName(content, L"SystemTray.SystemTrayFrame");
        double rightEdge = tray ? ElementLeftRelativeTo(tray, content)
                                : TaskbarWidthLocal();
        rightEdge -= g_settings.trayGap.load();

        std::vector<ButtonEntry*> running;
        std::vector<ButtonEntry*> pinned;
        for (auto& entry : entries) {
            (entry.running ? running : pinned).push_back(&entry);
        }

        double runningTotal = 0;
        for (auto* item : running) {
            runningTotal += item->width;
        }
        double pinnedTotal = 0;
        for (auto* item : pinned) {
            pinnedTotal += item->width;
        }

        double available = std::max(0.0,
            rightEdge - leftStart - g_settings.middleGap.load());
        double requested = runningTotal + pinnedTotal;
        double runningAllocation = runningTotal;
        double pinnedAllocation = pinnedTotal;
        if (requested > available && requested > 0) {
            runningAllocation = available * runningTotal / requested;
            pinnedAllocation = available - runningAllocation;
        }

        double runningScale =
            CompressedStepScale(running, runningAllocation, true);
        double pinnedScale =
            CompressedStepScale(pinned, pinnedAllocation, false);

        double x = leftStart;
        for (auto* item : running) {
            if (item->element.ActualWidth() > 0) {
                void* key = winrt::get_abi(item->element);
                plan[key] = x;
            }
            x += item->width * runningScale;
        }

        x = rightEdge;
        for (auto it = pinned.rbegin(); it != pinned.rend(); ++it) {
            ButtonEntry* item = *it;
            x -= item->width;
            if (item->element.ActualWidth() > 0) {
                void* key = winrt::get_abi(item->element);
                plan[key] = x;
            }
            // Re-add the uncompressed width, then advance by the compressed
            // step. This keeps the rightmost icon fully inside the tray edge.
            x += item->width;
            x -= item->width * pinnedScale;
        }

        g_targetX = std::move(plan);
    } catch (...) {
        Wh_Log(L"BuildLayoutPlan: exception; keeping previous safe plan");
    }
}

using IUIElement_Arrange_t = HRESULT(WINAPI*)(
    void*, winrt::Windows::Foundation::Rect);
IUIElement_Arrange_t IUIElement_Arrange_Original;

HRESULT WINAPI IUIElement_Arrange_Hook(
    void* pThis, winrt::Windows::Foundation::Rect rect) {
    auto callOriginal = [&] { return IUIElement_Arrange_Original(pThis, rect); };
    if (!g_inLayoutPass || g_unloading) {
        return callOriginal();
    }

    try {
        FrameworkElement element = nullptr;
        reinterpret_cast<IUnknown*>(pThis)->QueryInterface(
            winrt::guid_of<FrameworkElement>(), winrt::put_abi(element));
        if (!element) {
            return callOriginal();
        }

        auto it = g_targetX.find(winrt::get_abi(element));
        if (it == g_targetX.end()) {
            return callOriginal();
        }
        rect.X = it->second;
        return IUIElement_Arrange_Original(pThis, rect);
    } catch (...) {
        return callOriginal();
    }
}

using ArrangeOverride_t = HRESULT(WINAPI*)(
    void*, void*, winrt::Windows::Foundation::Size,
    winrt::Windows::Foundation::Size*);
ArrangeOverride_t ArrangeOverride_Original;

void RequestLayout();
HWND EnsureTaskbarWindow();

HRESULT WINAPI ArrangeOverride_Hook(
    void* pThis, void* context, winrt::Windows::Foundation::Size size,
    winrt::Windows::Foundation::Size* resultSize) {
    [[maybe_unused]] static bool uiElementHooked = [] {
        Shapes::Rectangle rectangle;
        IUIElement element = rectangle;
        void** vtable = *reinterpret_cast<void***>(winrt::get_abi(element));
        auto arrange = reinterpret_cast<IUIElement_Arrange_t>(vtable[92]);
        WindhawkUtils::SetFunctionHook(arrange, IUIElement_Arrange_Hook,
                                       &IUIElement_Arrange_Original);
        Wh_ApplyHookOperations();
        return true;
    }();

    EnsureTaskbarWindow();
    BuildLayoutPlan();

    struct LayoutScope {
        LayoutScope() { g_inLayoutPass = true; }
        ~LayoutScope() { g_inLayoutPass = false; }
    } scope;

    return ArrangeOverride_Original(pThis, context, size, resultSize);
}

thread_local bool g_invalidating;

void PerformLayoutInvalidation() {
    if (g_invalidating) {
        return;
    }
    g_invalidating = true;
    try {
        auto repeater = GetTaskbarRepeater();
        if (repeater) {
            repeater.InvalidateArrange();
            repeater.InvalidateMeasure();
        }
    } catch (...) {
        Wh_Log(L"PerformLayoutInvalidation: exception");
    }
    g_invalidating = false;
}

UINT InvalidateMessage() {
    static UINT message =
        RegisterWindowMessage(L"Windhawk_TaskbarSplit_Invalidate_" WH_MOD_ID);
    return message;
}

LRESULT CALLBACK TaskbarSubclassProc(HWND hwnd, UINT message, WPARAM wParam,
                                     LPARAM lParam, DWORD_PTR) {
    if (message == InvalidateMessage()) {
        g_layoutRequestPending = false;
        PerformLayoutInvalidation();
        return 0;
    }
    return DefSubclassProc(hwnd, message, wParam, lParam);
}

HWND EnsureTaskbarWindow() {
    HWND hwnd = g_taskbarWnd;
    if (hwnd && !IsWindow(hwnd)) {
        hwnd = nullptr;
        g_taskbarWnd = nullptr;
        g_taskbarSubclassed = false;
        g_cachedRepeater = nullptr;
    }

    if (!hwnd && !g_unloading) {
        hwnd = FindPrimaryTaskbarWindow();
        if (hwnd) {
            g_taskbarWnd = hwnd;
            Wh_Log(L"Primary taskbar resolved: %p", hwnd);
        }
    }

    if (hwnd && !g_taskbarSubclassed && !g_unloading &&
        WindhawkUtils::SetWindowSubclassFromAnyThread(
            hwnd, TaskbarSubclassProc, 0)) {
        g_taskbarSubclassed = true;
    }
    return hwnd;
}

void RequestLayout() {
    HWND hwnd = EnsureTaskbarWindow();
    if (hwnd && g_taskbarSubclassed) {
        if (g_layoutRequestPending.exchange(true)) {
            return;
        }
        if (!PostMessage(hwnd, InvalidateMessage(), 0, 0)) {
            g_layoutRequestPending = false;
        }
    }
}

using TaskListButton_UpdateVisualStates_t = void(WINAPI*)(void*);
TaskListButton_UpdateVisualStates_t TaskListButton_UpdateVisualStates_Original;

void WINAPI TaskListButton_UpdateVisualStates_Hook(void* pThis) {
    TaskListButton_UpdateVisualStates_Original(pThis);

    // This method also fires for hover/press/focus changes. RequestLayout
    // coalesces a burst into one queued asynchronous layout pass; that pass
    // reads the authoritative IsRunning value for every button.
    RequestLayout();
}

bool HookTaskbarDllSymbols() {
    HMODULE module =
        LoadLibraryEx(L"taskbar.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!module) {
        return false;
    }

    WindhawkUtils::SYMBOL_HOOK taskbarDllHooks[] = {
        {{LR"(const CTaskBand::`vftable'{for `ITaskListWndSite'})"},
         &CTaskBand_ITaskListWndSite_vftable},
        {{LR"(public: virtual class std::shared_ptr<class TaskbarHost> __cdecl CTaskBand::GetTaskbarHost(void)const )"},
         &CTaskBand_GetTaskbarHost_Original},
        {{LR"(public: int __cdecl TaskbarHost::FrameHeight(void)const )"},
         &TaskbarHost_FrameHeight_Original},
        {{LR"(public: void __cdecl std::_Ref_count_base::_Decref(void))"},
         &RefCount_Decref_Original},
    };

    return WindhawkUtils::HookSymbols(module, taskbarDllHooks,
                                      ARRAYSIZE(taskbarDllHooks));
}

bool HookTaskbarViewSymbols(HMODULE module) {
    // Taskbar.View.dll, ExplorerExtensions.dll
    WindhawkUtils::SYMBOL_HOOK hooks[] = {
        {{LR"(public: virtual int __cdecl winrt::impl::produce<struct winrt::Taskbar::implementation::TaskbarCollapsibleLayout,struct winrt::Microsoft::UI::Xaml::Controls::IVirtualizingLayoutOverrides>::ArrangeOverride(void *,struct winrt::Windows::Foundation::Size,struct winrt::Windows::Foundation::Size *))"},
         &ArrangeOverride_Original, ArrangeOverride_Hook},
        {{LR"(public: virtual int __cdecl winrt::impl::produce<struct winrt::Taskbar::implementation::TaskListButton,struct winrt::Taskbar::ITaskListButton>::get_IsRunning(bool *))"},
         &TaskListButton_get_IsRunning_Original, nullptr, true},
        {{LR"(private: void __cdecl winrt::Taskbar::implementation::TaskListButton::UpdateVisualStates(void))"},
         &TaskListButton_UpdateVisualStates_Original,
         TaskListButton_UpdateVisualStates_Hook, true},
    };

    bool ok = WindhawkUtils::HookSymbols(module, hooks, ARRAYSIZE(hooks));
    if (!TaskListButton_get_IsRunning_Original) {
        Wh_Log(L"Warning: IsRunning symbol missing; task buttons stay left");
    }
    return ok;
}

HMODULE GetTaskbarViewModule() {
    HMODULE module = GetModuleHandle(L"Taskbar.View.dll");
    return module ? module : GetModuleHandle(L"ExplorerExtensions.dll");
}

void HandleLoadedTaskbarView(HMODULE module, LPCWSTR name) {
    if (!g_taskbarViewLoaded && GetTaskbarViewModule() == module &&
        !g_taskbarViewLoaded.exchange(true)) {
        Wh_Log(L"Loaded taskbar view module: %s", name);
        HookTaskbarViewSymbols(module);
        Wh_ApplyHookOperations();
    }
}

using LoadLibraryExW_t = decltype(&LoadLibraryExW);
LoadLibraryExW_t LoadLibraryExW_Original;

HMODULE WINAPI LoadLibraryExW_Hook(LPCWSTR fileName, HANDLE file,
                                   DWORD flags) {
    HMODULE module = LoadLibraryExW_Original(fileName, file, flags);
    if (module) {
        HandleLoadedTaskbarView(module, fileName);
    }
    return module;
}

BOOL Wh_ModInit() {
    LoadSettings();
    if (!HookTaskbarDllSymbols()) {
        Wh_Log(L"Failed to hook taskbar.dll symbols");
        return FALSE;
    }

    if (HMODULE module = GetTaskbarViewModule()) {
        g_taskbarViewLoaded = true;
        if (!HookTaskbarViewSymbols(module) || !ArrangeOverride_Original) {
            return FALSE;
        }
    } else {
        HMODULE kernelBase = GetModuleHandle(L"kernelbase.dll");
        auto loadLibrary = kernelBase
            ? reinterpret_cast<decltype(&LoadLibraryExW)>(
                  GetProcAddress(kernelBase, "LoadLibraryExW"))
            : nullptr;
        if (!loadLibrary) {
            return FALSE;
        }
        WindhawkUtils::SetFunctionHook(loadLibrary, LoadLibraryExW_Hook,
                                       &LoadLibraryExW_Original);
    }
    return TRUE;
}

void Wh_ModAfterInit() {
    if (!g_taskbarViewLoaded) {
        if (HMODULE module = GetTaskbarViewModule()) {
            if (!g_taskbarViewLoaded.exchange(true)) {
                HookTaskbarViewSymbols(module);
                Wh_ApplyHookOperations();
            }
        }
    }
    EnsureTaskbarWindow();
    RequestLayout();
}

void Wh_ModBeforeUninit() {
    g_unloading = true;
    HWND hwnd = g_taskbarWnd;
    if (hwnd && g_taskbarSubclassed) {
        SendMessage(hwnd, InvalidateMessage(), 0, 0);
    }
    if (hwnd && g_taskbarSubclassed.exchange(false)) {
        WindhawkUtils::RemoveWindowSubclassFromAnyThread(
            hwnd, TaskbarSubclassProc);
    }
}

void Wh_ModUninit() {
    g_targetX.clear();
    g_cachedRepeater = nullptr;
}

void Wh_ModSettingsChanged() {
    LoadSettings();
    RequestLayout();
}
