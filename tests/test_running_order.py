"""Compile the production ordering function with lightweight identity stubs.

This exercises C++ ordering, not XAML, drag/drop, or Windows compilation.
"""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


class RunningOrderTests(unittest.TestCase):
    def test_drag_anchor_survives_multiple_neighbour_swaps(self):
        compiler = shutil.which("g++")
        if not compiler:
            self.skipTest("g++ unavailable")
        source = (Path(__file__).resolve().parents[1] / "taskbar-split.wh.cpp").read_text()
        anchor = "void KeepDraggedSlot" + source.rsplit("void KeepDraggedSlot", 1)[1].split(
            "void UpdateDragPreview", 1)[0]
        harness = r'''
#include <optional>
#include <cassert>
#include <cmath>
using FrameworkElement = int;
namespace winrt::Windows::Foundation { struct Rect { float X; }; }
struct Weak { int value; int get() const { return value; } };
struct SectionGesture { bool dragged; Weak source; float originalSlotX; };
std::optional<SectionGesture> g_sectionGesture;
''' + anchor + r'''
int main() {
    const FrameworkElement held = 1, neighbour = 2;
    // The initial slot, existing translation, and point of grab differ.
    const double slot = 300, originalTranslation = 5, grabOffset = 17;
    const double visualStart = slot + originalTranslation;
    const double pointerStart = visualStart + grabOffset;
    g_sectionGesture = SectionGesture{true, {held}, float(slot)};
    for (double pointer : {330., 380., 430., 475., 430., 380., 330.}) {
        const double desired = visualStart + pointer - pointerStart;
        // Repeated layout passes propose different slots after reordering.
        for (float proposedSlot : {300.f, 348.f, 396.f, 444.f}) {
            winrt::Windows::Foundation::Rect rect{proposedSlot};
            KeepDraggedSlot(held, rect);
            assert(rect.X == slot);
            const double translation = DragTranslationX(originalTranslation, visualStart, desired);
            assert(std::abs(rect.X + translation + grabOffset - pointer) < 1e-9);
            // Repeated frames must not accumulate displacement.
            assert(translation == DragTranslationX(originalTranslation, visualStart, desired));
            rect.X = proposedSlot;
            KeepDraggedSlot(neighbour, rect);
            assert(rect.X == proposedSlot);
        }
    }
    g_sectionGesture.reset();
    winrt::Windows::Foundation::Rect finalSlot{396.f};
    KeepDraggedSlot(held, finalSlot);
    assert(finalSlot.X == 396.f);
}
'''
        with tempfile.TemporaryDirectory() as directory:
            executable = str(Path(directory) / "anchor-test")
            subprocess.run([compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror",
                            "-x", "c++", "-", "-o", executable],
                           input=harness, text=True, check=True, capture_output=True)
            subprocess.run([executable], check=True, capture_output=True)

    def test_production_cpp_ordering(self):
        compiler = shutil.which("g++")
        if not compiler:
            self.skipTest("g++ unavailable")
        source = (Path(__file__).resolve().parents[1] / "taskbar-split.wh.cpp").read_text()
        reorder = "template<typename T>\nbool ReorderWithinSection" + source.split(
            "bool ReorderWithinSection", 1)[1].split("void FinishSectionReorder", 1)[0]
        harness = r'''
#include <algorithm>
#include <vector>
#include <cassert>
#include <iterator>
#include <cmath>
''' + reorder + r'''
int main() {
    // Dragging reorders only one section, in either direction.
    std::vector<int> left{1, 2, 3}, right{4, 5, 6};
    assert(ReorderWithinSection(right, 6, 4, false));
    assert((right == std::vector<int>{6, 4, 5}));
    assert(ReorderWithinSection(right, 6, 5, true));
    assert((right == std::vector<int>{4, 5, 6}));
    assert(ReorderWithinSection(left, 1, 3, true));
    assert((left == std::vector<int>{2, 3, 1}));
    // Full-size and 50% right-group icons must reach BOTH end slots.
    assert(DragDestination({48,48,48}, 2, 24) == 0);
    assert(DragDestination({48,48,48}, 0, 120) == 2);
    assert(DragDestination({24,24,24}, 2, 12) == 0);
    assert(DragDestination({24,24,24}, 0, 60) == 2);
    assert(DragDestination({24,24,24}, 0, 35) == 0);
    assert(DragDestination({24,24,24}, 0, 36) == 1);
    // Landing on a target midpoint remains stable after the swap.
    assert(DragDestination({24,24,24}, 1, 36) == 1);
    assert(DragDestination({24}, 0, 12) == 0);
    // Reproduce the unreachable endpoint at 70%: exact centre crossing
    // fails after subtraction of the right section's large absolute X.
    double w70 = 44 * 0.7;
    double leftCentre = (1300.0 + w70 / 2) - 1300.0;
    assert(leftCentre > w70 / 2);
    assert(DragDestination({w70, w70}, 1, leftCentre) == 1);
    assert(PinnedDragDestination({w70, w70}, 1, leftCentre) == 0);
    // Fractional scales: both extremes and retention after reorder.
    for (double scale : {0.5, 0.65, 0.7, 0.75, 0.8, 0.9, 1.0}) {
        double w = 44 * scale;
        double low = (1300.0 + w / 2) - 1300.0;
        double high = (1300.0 + 2.5 * w) - 1300.0;
        assert(PinnedDragDestination({w,w,w}, 2, low) == 0);
        assert(PinnedDragDestination({w,w,w}, 0, high) == 2);
        assert(PinnedDragDestination({w,w,w}, 2, high) == 2);
        assert(PinnedDragDestination({w,w}, 0, w) == 0);
        assert(PinnedDragDestination({w,w}, 1, w) == 1);
        assert(PinnedDragDestination({w,w}, 0, w + 1) == 1);
    }
    // Neither a target from the opposite section nor an absent source works.
    assert(!ReorderWithinSection(right, 4, 2, true));
    assert(!ReorderWithinSection(left, 4, 2, true));
    assert(!ReorderWithinSection(right, 4, 4, true));
    assert((right == std::vector<int>{4, 5, 6}));
    assert((left == std::vector<int>{2, 3, 1}));
}
'''
        with tempfile.TemporaryDirectory() as directory:
            executable = str(Path(directory) / "order-test")
            subprocess.run([compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror",
                            "-x", "c++", "-", "-o", executable],
                           input=harness, text=True, check=True, capture_output=True)
            subprocess.run([executable], check=True, capture_output=True)
