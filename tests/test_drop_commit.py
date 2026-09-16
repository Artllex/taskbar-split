"""Exercise the actual commit function against the captured stale-offset case.

The compositor is a stub: this verifies writes/lifetime, not Windows rendering.
"""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


class DropCommitTests(unittest.TestCase):
    def test_recorded_stale_compositor_offset_is_explicitly_committed(self):
        compiler = shutil.which("g++")
        if not compiler:
            self.skipTest("g++ unavailable")
        source = (Path(__file__).resolve().parents[1] / "taskbar-split.wh.cpp").read_text()
        signature = "void CompletePendingDrop(FrameworkElement const& element) {"
        function = signature + source.split(signature, 1)[1].split("void SettlePendingDrop", 1)[0]
        harness = r'''
#include <optional>
#include <cassert>
#include <cmath>
struct Vec { double x, y, z; };
Vec baseOffset{399.2, 7, 2};
Vec translation{-58, 0, 0};
struct FrameworkElement {
    int id;
    bool operator!=(FrameworkElement const& other) const { return id != other.id; }
    Vec ActualOffset() const { return {355.2, 7, 2}; }
    void Translation(Vec value) const { translation = value; }
};
struct Weak { FrameworkElement value; FrameworkElement get() const { return value; } };
struct Gesture { Weak source; Vec translation; };
struct PendingDrop { Gesture gesture; bool arranged = false; };
std::optional<PendingDrop> g_pendingDrop;
struct Properties { void StopAnimation(wchar_t const*) {} };
struct Visual {
    void StopAnimation(wchar_t const*) {} // Stop leaves the old base, as observed.
    ::Properties Properties() { return {}; }
    Vec Offset() { return baseOffset; }
    void Offset(Vec value) { baseOffset = value; }
};
namespace Hosting {
struct ElementCompositionPreview {
    static Visual GetElementVisual(FrameworkElement const&) { return {}; }
};
}
''' + function + r'''
int main() {
    FrameworkElement held{1}, other{2};
    g_pendingDrop = PendingDrop{Gesture{Weak{held}, {0, 0, 0}}};
    CompletePendingDrop(other);
    assert(baseOffset.x == 399.2 && translation.x == -58);
    CompletePendingDrop(held);
    assert(std::abs(baseOffset.x - 355.2) < 1e-9);
    assert(baseOffset.y == 7 && baseOffset.z == 2);
    assert(translation.x == 0);
    assert(g_pendingDrop && g_pendingDrop->arranged);
    // A late shell write to the old offset can be corrected idempotently.
    baseOffset.x = 399.2;
    CompletePendingDrop(held);
    assert(std::abs(baseOffset.x - 355.2) < 1e-9);
    assert(g_pendingDrop); // Animation settings remain guarded after Arrange.
}
'''
        with tempfile.TemporaryDirectory() as directory:
            executable = str(Path(directory) / "drop-commit")
            subprocess.run([compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror",
                            "-x", "c++", "-", "-o", executable],
                           input=harness, text=True, check=True, capture_output=True)
            subprocess.run([executable], check=True, capture_output=True)
