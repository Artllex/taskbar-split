"""Deterministic checks for Taskbar Split's group-placement math."""

import unittest


def scale(widths, allocation, anchored_left):
    if len(widths) < 2 or sum(widths) <= allocation:
        return 1.0
    fixed = widths[-1] if anchored_left else widths[0]
    compressible = sum(widths) - fixed
    room = allocation - fixed
    return max(0.0, min(1.0, room / compressible)) if compressible else 1.0


def positions(left, right, left_start, right_edge, middle_gap):
    available = max(0.0, right_edge - left_start - middle_gap)
    requested = sum(left) + sum(right)
    if requested > available and requested:
        left_allocation = available * sum(left) / requested
        right_allocation = available - left_allocation
    else:
        left_allocation = sum(left)
        right_allocation = sum(right)

    left_scale = scale(left, left_allocation, True)
    right_scale = scale(right, right_allocation, False)

    left_x = []
    x = left_start
    for width in left:
        left_x.append(x)
        x += width * left_scale

    right_x = [None] * len(right)
    x = right_edge
    for index in range(len(right) - 1, -1, -1):
        width = right[index]
        x -= width
        right_x[index] = x
        x += width
        x -= width * right_scale
    return left_x, right_x


class LayoutTests(unittest.TestCase):
    def test_normal_layout_anchors_both_edges(self):
        left, right = positions([48, 48], [48, 48, 48], 100, 1000, 48)
        self.assertEqual(left, [100, 148])
        self.assertEqual(right, [856, 904, 952])

    def test_group_order_is_preserved(self):
        left, right = positions([40, 50, 60], [35, 45, 55], 80, 900, 40)
        self.assertEqual(left, [80, 120, 170])
        self.assertEqual(right, [765, 800, 845])

    def test_crowded_layout_keeps_outer_icons_inside_edges(self):
        left, right = positions([60] * 5, [60] * 5, 100, 500, 48)
        self.assertGreaterEqual(left[0], 100)
        self.assertLessEqual(left[-1] + 60, 500)
        self.assertGreaterEqual(right[0], 100)
        self.assertLessEqual(right[-1] + 60, 500)

    def test_empty_groups(self):
        left, right = positions([], [], 100, 1000, 48)
        self.assertEqual(left, [])
        self.assertEqual(right, [])


if __name__ == "__main__":
    unittest.main()
