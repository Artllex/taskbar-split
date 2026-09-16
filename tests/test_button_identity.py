"""Exercise production ordering with distinct interface views of one object."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


class ButtonIdentityTests(unittest.TestCase):
    def test_weak_and_tree_views_keep_selected_order(self):
        compiler = shutil.which("g++")
        if not compiler:
            self.skipTest("g++ unavailable")
        source = (Path(__file__).resolve().parents[1] / "taskbar-split.wh.cpp").read_text()
        production = "bool SameTaskButton" + source.split("bool SameTaskButton", 1)[1].split("struct Placement", 1)[0]
        harness = r'''
#include <algorithm>
#include <vector>
#include <cassert>
#include <cstdint>
namespace winrt::Windows::Foundation { struct IUnknown { int id; }; }
struct FrameworkElement {
    int id, view;
    explicit operator bool() const { return id != 0; }
    template<class T> T as() const { return T{id}; }
};
namespace winrt {
void* get_abi(FrameworkElement e) { return reinterpret_cast<void*>(uintptr_t(e.id * 16 + e.view)); }
void* get_abi(Windows::Foundation::IUnknown e) { return reinterpret_cast<void*>(uintptr_t(e.id * 16)); }
template<class T> struct weak_ref {
    T value;
    weak_ref(T v): value(v) {}
    T get() const { return T{value.id, 2}; }
};
}
struct ButtonInfo { FrameworkElement element; };
void TracePinnedOrder(char const*, std::vector<ButtonInfo*> const*) {}
std::vector<winrt::weak_ref<FrameworkElement>> g_runningOrder, g_pinnedOrder, g_pinnedUserOrder;
''' + production + r'''
int main() {
    ButtonInfo a{{1,1}}, b{{2,1}}, c{{3,1}};
    assert(SameTaskButton({1,1}, {1,2}));
    assert(!SameTaskButton({1,1}, {2,2}));
    assert(!SameTaskButton({0,0}, {0,0}));
    g_pinnedUserOrder = {b.element, c.element, a.element};
    for (int pass=0; pass<88; ++pass) {
        std::vector<ButtonInfo*> items{&a, &b, &c};
        OrderPinnedButtons(items);
        assert((items == std::vector<ButtonInfo*>{&b, &c, &a}));
        assert(g_pinnedOrder[2].get().id == 1);
    }
    // Every selected/native order combination, with weak views distinct
    // from tree views. Layout must publish precisely the selected sequence.
    std::vector<int> selected{1,2,3};
    ButtonInfo* byId[] = {nullptr, &a, &b, &c};
    do {
        std::vector<int> native{1,2,3};
        do {
            g_pinnedUserOrder.clear();
            for (int id : selected) g_pinnedUserOrder.emplace_back(byId[id]->element);
            g_pinnedOrder.clear();
            std::vector<ButtonInfo*> items;
            for (int id : native) {
                items.push_back(byId[id]);
                g_pinnedOrder.emplace_back(byId[id]->element);
            }
            OrderPinnedButtons(items);
            for (int i=0; i<3; ++i) {
                assert(items[i]->element.id == selected[i]);
                assert(g_pinnedOrder[i].get().id == selected[i]);
            }
        } while (std::next_permutation(native.begin(), native.end()));
    } while (std::next_permutation(selected.begin(), selected.end()));
}
'''
        with tempfile.TemporaryDirectory() as directory:
            executable = str(Path(directory) / "identity-test")
            subprocess.run([compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror",
                            "-x", "c++", "-", "-o", executable],
                           input=harness, text=True, check=True, capture_output=True)
            subprocess.run([executable], check=True, capture_output=True)
