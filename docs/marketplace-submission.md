Taskbar Split 0.3.16 replaces the earlier test revision.

## Why a separate mod
The goal is an edge-anchored launcher/workspace split: running apps at the left, closed pinned apps at the right, independent ordering within each section, and an optional smaller pinned group. Taskbar Start Button Centered Origin organizes windows around a centered Start button. Adding a running-state mode there was considered; this submission keeps the configuration focused on the edge-anchored workflow. This is a product-scope preference, not a claim that integration is technically impossible. The maintainer can decide whether to consolidate. The README documents that the positioning mods cannot be combined.

## Changes and evidence
- Real Arrange positioning; section-local mouse drag with live neighbour reordering and Escape cancellation.
- A bounded post-drop compositor correction addresses an observed delayed return animation. It uses a window-message timer, is flushed on a new press/unload/window destruction, and has a 2-second upper bound.
- Direct rank-based ordering for the right group; native Widgets clearance when system button placement is disabled.
- Defaults: 0 / 0 / 8 / 48 DIP; right icons 90%.
- Removed all per-drag diagnostic file writes and the diagnostic sampling timer.
- 30 local tests pass, including selected production C++ functions compiled with stubs. These are not full Windows integration tests.

The submitter confirmed on Windows that left/right dragging works (0.3.14) and Widgets clearance works with system-button placement disabled (0.3.15). 0.3.16 is the publication cleanup of that tested implementation. Exact Windows build, reboot/Explorer restart, both taskbar alignments, all hidden-system-button combinations, full thumbnail/jump-list checks and ARM64 runtime coverage have not been confirmed. We do not claim that full matrix has passed.

## Screenshot

![Taskbar Split: running applications on the left and closed pinned applications on the right](https://raw.githubusercontent.com/Artllex/taskbar-split/main/assets/taskbar-split.png)

Actual screenshot supplied by the submitter, including the small margin above the taskbar.

---

<!-- ⚠️ Please keep the template below intact and fill in the relevant sections. Any additional content can be placed above the template. -->

## Changelog

If this pull request updates an existing mod, describe the changes below:

New mod submission. Current version: 0.3.16; changes and verification are described above.

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

