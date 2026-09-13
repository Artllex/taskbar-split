"""Source guards for review fixes; these do not replace Windows runtime tests."""

from pathlib import Path
import unittest

SOURCE = (Path(__file__).resolve().parents[1] / "taskbar-split.wh.cpp").read_text()


def section(start, end):
    return SOURCE.split(start, 1)[1].split(end, 1)[0]


class ReviewRegressionTests(unittest.TestCase):
    def test_unloading_is_set_before_restoration(self):
        body = section("void Wh_ModBeforeUninit()", "void Wh_ModUninit()")
        self.assertLess(body.index("g_unloading = true"), body.index("SendMessageW"))

    def test_loader_callback_does_not_refresh_or_subclass_windows(self):
        body = section("void TryHookLoadedTaskbarView", "using LoadLibraryExW_t")
        self.assertNotIn("RequestRefresh", body)
        self.assertNotIn("EnsureTaskbarWindow", body)

    def test_implementation_pointer_is_not_used_as_abi_pointer(self):
        body = section("void WINAPI TaskListButton_UpdateVisualStates_Hook", "bool HookTaskbarHostSymbols")
        self.assertNotIn("TaskListButton_GetIsRunning_Original(self", body)
        self.assertIn("winrt::copy_from_abi", body)
        self.assertIn("ButtonIsRunning(element)", body)

    def test_no_strong_composition_visual_in_global_state(self):
        self.assertNotIn("composition::Visual", SOURCE)
        self.assertNotIn("GetElementVisual", SOURCE)

    def test_scale_is_left_anchored(self):
        body = section("void ScaleElement", "void RestoreVisualStates")
        self.assertIn("centerPoint.x = 0;", body)
        for scale in (0.5, 0.75, 1.0):
            width, tray_edge, gap = 44, 1000, 8
            target = tray_edge - gap - width * scale
            rendered_right = target + width * scale
            self.assertEqual(rendered_right, tray_edge - gap)

    def test_arm64_prologue_has_a_real_parser(self):
        self.assertIn("instructions[0] == 0xD503237F", SOURCE)
        self.assertIn("elementOffset = (instructions[3] >> 12) & 0xFF", SOURCE)

    def test_layout_requires_restore_channel(self):
        body = section("HRESULT WINAPI ArrangeOverride_Hook", "UINT RefreshMessage")
        self.assertLess(body.index("!g_taskbarSubclassed"), body.index("BuildLayoutPlan()"))

    def test_full_size_buttons_skip_scale_writes(self):
        body = section("void ScaleElement", "void RestoreElementState")
        full_size = body.split("if (scaleValue == 1.0)", 1)[1].split(
            "if (!applied.scaleApplied)", 1)[0]
        self.assertIn("if (applied.scaleApplied)", full_size)
        self.assertIn("applied.scaleApplied = false;", full_size)
        self.assertIn("return;", full_size)

    def test_detached_entries_are_restored_before_erasure(self):
        body = section("void PruneVisualStates", "void RestoreVisualStates")
        self.assertIn("!live.count(it->first)", body)
        self.assertLess(body.index("RestoreElementState"), body.index("g_visualStates.erase"))
        layout = section("bool BuildLayoutPlan", "using ArrangeOverride_t")
        self.assertIn("PruneVisualStates(children)", layout)

    def test_window_discovery_does_not_write_weak_cache(self):
        body = section("HWND EnsureTaskbarWindow() {", "void RequestRefresh")
        self.assertNotIn("g_repeaterCache =", body)
        self.assertIn("g_repeaterCacheInvalidated = true", body)

    def test_positioning_changes_arrange_rect_not_translation(self):
        self.assertNotIn("element.Translation(", SOURCE)
        hook = section("HRESULT WINAPI ElementArrange_Hook", "bool EnsureArrangeHook")
        self.assertIn("rect.X = found->second.x", hook)
        self.assertNotIn("VisualTreeHelper", hook)
        self.assertNotIn("BuildLayoutPlan", hook)

    def test_plan_is_ready_before_native_arrange(self):
        body = section("HRESULT WINAPI ArrangeOverride_Hook", "UINT RefreshMessage")
        plan_index = body.index("BuildLayoutPlan()")
        native_index = body.index("HRESULT result = ArrangeOverride_Original", plan_index)
        self.assertLess(plan_index, native_index)
        self.assertGreater(body.index("ScaleElement"), native_index)

    def test_late_subclass_install_is_undone_on_unload(self):
        for begin, end in (("HWND EnsureTaskbarWindow() {", "void RequestRefresh"),
                           ("void Wh_ModAfterInit()", "void Wh_ModBeforeUninit()")):
            body = section(begin, end)
            self.assertIn("if (g_unloading && g_taskbarSubclassed.exchange(false))", body)
            self.assertIn("RemoveWindowSubclassFromAnyThread", body)

    def test_overflow_is_appended_to_running_group(self):
        body = section("bool BuildLayoutPlan", "using ArrangeOverride_t")
        self.assertIn('L"Taskbar.OverflowToggleButton"', body)
        self.assertIn("running.push_back(&overflowInfo)", body)

    def test_restore_requests_native_layout(self):
        body = section("if (message == RestoreMessage())", "HWND EnsureTaskbarWindow() {")
        self.assertIn("repeater.InvalidateArrange()", body)
        self.assertIn("repeater.UpdateLayout()", body)


if __name__ == "__main__":
    unittest.main()
