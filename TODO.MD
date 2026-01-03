# Pulsrr Sequencer — TODO / Roadmap

> Central TODO list for tracking progress, bugs, features, and technical debt

---

## [HIGH] Critical Bugs
- [ ] Speed / FPS drop when adding a new sequence
- [ ] Sequence preview thumbnails sometimes fail to display

## [MEDIUM] Core Features
- [ ] Display accurate sequence information (duration, frames, size, FPS)
- [ ] Implement Contrast FX in layer effects modal
- [ ] Implement Invert FX in layer effects modal
- [ ] Visual UI for Live / Playback mode toggle (buttons + indicators)
- [ ] Allow deleting / emptying a layer while in Live mode
- [ ] Allow deleting individual sequences
- [ ] FX modal: grab focus and set parent window insensitive (gtk_window_set_sensitive(FALSE))
- [ ] Disable Playback mode button when no sequences are loaded

## [MEDIUM] Refactoring & Code Quality
- [ ] Remove redundant sequence existence check in `sdl.c` — use existing `get_number_of_sequences()`
- [ ] Unify frame advance logic between Live and Playback modes:
  - Use same FPS reference
  - Use same speed multiplier math
  - Avoid direct `SDL_GetTicks()` calls inside render loop (use pre-computed delta)
- [ ] Route remaining `g_print()` debug statements in `sdl.c` to `add_main_log()` for consistency
- [ ] Implement stubbed `on_layer_menu_label_click()` in `component_layer.c`

## [LOW] Nice-to-Have / Experiments
- [ ] Cut / crop frames directly in Live mode
- [ ] Proper looping implementation (start/end markers, seamless loop)
- [ ] Fullscreen mode (F11 or menu)
- [ ] Prepare and push a public release (v0.1)
- [ ] Debounce Live / Playback mode toggle buttons
- [ ] Reduce width of layer preview thumbnails for better layout

## [IN PROGRESS]
- [ ] Fix sequence preview thumbnail display issue

## [PENDING FOR REVIEW]
- [ ] Unify frame advance logic between Live and Playback (see refactoring section)
- [x] Clear all sequences (disk + memory + UI)

## [DONE / READY TO COMMIT]
- [x] Sequence preview thumbnail display issue (initial fix)

## Technical Debt & Observations
### Memory & Resource Management
- Good use of frees observed, but run **Valgrind** to confirm no leaks in frame loading loops

### Concurrency
- No mutex locking around shared `g_sdl.sequence` or `layers` during read/write
- Currently safe because rendering runs on main thread — monitor if background threads increase

### UI / Styling
- `style.css` is comprehensive
- Some selectors use dynamic IDs (e.g., `#sequence-preview-css`) — verify no conflicts

### Rendering
- `update_sequence_texture()` loads all sequence frames into memory sequentially
  - Works for small sequences
  - Becomes memory-heavy with long/high-res sequences — consider streaming or limits later

### Build & Release
- Makefile not yet reviewed — ensure it correctly handles GTK-x11, SDL2, SDL2_image, SDL2_ttf dependencies

### Architecture
- Consider moving singleton accessor (`get_app_ctx()`) to a dedicated file (e.g., `app_context.c/h`) when stabilizing

---

*Last updated: January 03, 2026*
