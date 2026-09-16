## Response to the latest review (0.3.17)

**Items 3 and 4:** App ranks now use the taskbar's `Appid: ` automation identity, read afresh rather than cached by XAML address. No caption/HWND/pointer fallback is persisted. Separate bounded, versioned lists are stored with `Wh_SetStringValue` after a completed drag and read on initialization of the primary layout. Unknown identities retain native input; separate windows of the same app share a rank. Reordering a visible subset preserves entries outside that subset. Live weak element lists are only gesture views rebuilt from these IDs, not authoritative ranks.

The Arrange hook now compares the caller's canonical Layout identity with the primary ItemsRepeater's Layout before BuildLayoutPlan or post-layout scaling. The Widgets lookup is scoped to TaskbarFrame and weak-cached with attachment validation; a miss is throttled, settings invalidate the cache, and current size/position are still read each pass.

**Item 1:** We acknowledge the overlap and leave catalog consolidation to the maintainer. This request specifically targets running/closed classification, edge anchors, independent section order and reduced right-side icons. Integrating those as modes in Centered Origin is technically possible, but has not been proposed to that author; this PR is not claiming otherwise. We retain the separate implementation pending the maintainer's decision.

**Item 2:** Replacing native left-drag is intentional for this layout: native pin-list dragging crosses the visual section boundary and previously produced incorrect placement. The submitter explicitly requested dragging only within the current section and verified that interaction after iterative fixes. Moving it to right-drag would change the requested interaction. The README now explicitly states the replacement; app order is mod-owned and persistent, while Windows' own pin list is untouched. We are not claiming native left-drag remains active on supported buttons.

**Validation:** Local production-function tests cover container replacement/recycling, storage reload, malformed storage, hidden entries, canceled-order snapshots and transition from pinned to running. The new 0.3.17 integration still requires Windows verification, including the primary Layout identity check and Appid availability. The existing screenshot and runtime confirmations apply to 0.3.14/0.3.15, not a claim that 0.3.17 has been runtime-tested.

---

Taskbar Split 0.3.17 replaces the earlier test revision.

## Why a separate mod
The goal is an edge-anchored launcher/workspace split: running apps at the left, closed pinned apps at the right, independent ordering within each section, and an optional smaller pinned group. Taskbar Start Button Centered Origin organizes windows around a centered Start button. Adding a running-state mode there was considered; this submission keeps the configuration focused on the edge-anchored workflow. This is a product-scope preference, not a claim that integration is technically impossible. The maintainer can decide whether to consolidate. The README documents that the positioning mods cannot be combined.

## Changes and evidence
- Real Arrange positioning; section-local mouse drag with live neighbour reordering and Escape cancellation.
- A bounded post-drop compositor correction addresses an observed delayed return animation. It uses a window-message timer, is flushed on a new press/unload/window destruction, and has a 2-second upper bound.
- Direct rank-based ordering for the right group; native Widgets clearance when system button placement is disabled.
- Defaults: 0 / 0 / 8 / 48 DIP; right icons 90%.
- Removed all per-drag diagnostic file writes and the diagnostic sampling timer.
- 30 local tests pass, including selected production C++ functions compiled with stubs. These are not full Windows integration tests.

The submitter confirmed on Windows that left/right dragging works (0.3.14) and Widgets clearance works with system-button placement disabled (0.3.15). 0.3.17 is the publication cleanup of that tested implementation. Exact Windows build, reboot/Explorer restart, both taskbar alignments, all hidden-system-button combinations, full thumbnail/jump-list checks and ARM64 runtime coverage have not been confirmed. We do not claim that full matrix has passed.

## Screenshot

![Taskbar Split: running applications on the left and closed pinned applications on the right](https://raw.githubusercontent.com/Artllex/taskbar-split/main/assets/taskbar-split.png)

Actual screenshot supplied by the submitter, including the small margin above the taskbar.

---

<!-- ⚠️ Please keep the template below intact and fill in the relevant sections. Any additional content can be placed above the template. -->

## Changelog

If this pull request updates an existing mod, describe the changes below:

New mod submission. Current version: 0.3.17; changes and verification are described above.

## Mod authorship

If this pull request introduces a new mod, please complete the section below.

This mod was created by:

- [ ] The submitter, without AI assistance
- [x] The submitter, with AI assistance
- [ ] Claude
- [x] ChatGPT
- [ ] Gemini
- [ ] Another AI (please specify):
- [ ] Other (please specify):

Please select the options that best apply. Your selection does not affect the acceptance criteria, but it helps reviewers understand the context of the code and provide relevant feedback.

