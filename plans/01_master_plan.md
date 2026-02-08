# MASTER EXECUTION PLAN

## Phase 1: Foundation & Sanitation (The Fixes)
*Goal: Stabilize the codebase and prepare the backend for the redesign.*

- [x] **State Persistence Repair**
    - [x] Enable `loadWindowState()` and `onCloseRequest` [Spec #1, #2]
    - [x] Clean shutdown on DBus init failure [Spec #9] (Handled by g_application_quit)
- [x] **ConfigManager Hardening**
    - [x] Enable Mutex locking in `get/set` (Thread Safety) [Spec #3]
    - [x] Add Error Logging to `catch` blocks [Spec #4]
    - [x] Implement recursive key creation in `set()` [Spec #5] (Verified in cpp)
- [x] **Core Standards & Defaults**
    - [x] Set Default FPS to 60 (was 0/Uncapped) [Spec #7]
    - [x] Refactor Hardcoded Strings to `Constants.hpp` [Spec #10]
    - [x] Enforce Absolute Paths for Library DB [Spec #11]
- [x] **Logic & Safety**
    - [x] Implement Disk Full Check on Save [Spec #20]
    - [x] Verify `pause()` safety on dead process [Spec #8]
    - [x] Ensure `pause_on_battery` logic handles manual override [Spec #13]

## Phase 2: The "Monochrome Glass" Redesign
*Goal: rewrite the GUI to match the `design_system.md` specification.*

- [ ] **Design Setup**
    - [x] Install/Verify CSS/Theming capabilities (GTK4 CSS Provider)
    - [x] Define Color Variables (Deep Void `#0A0A0A`, Carbon `#121212`, Off-White `#EDEDED`)
    - [x] Set Font Stack to `Geist Sans` / `Inter`
- [ ] **Component Construction**
    - [x] **Wallpaper Card**: Glass background, 16:9 ratio, hover effects, selection glow.
    - [x] **Sidebar**: Semi-transparent Glass Dock (240px fixed), Icons.
    - [x] **Mini-Player**: Floating glass panel for Tray menu.
- [ ] **Layout Implementation**
    - [x] Refactor `MainWindow` to use new Overlay/Stack layout.
    - [x] Implement "Snappy" Animations (200ms cubic-bezier).
    - [x] Fix Sidebar/Preview constraints (300px fixed) [Spec #28]
- [x] **First Run Wizard**
    - [x] Build Multi-stage Modal (Welcome -> Monitors -> Import -> Done) [Roadmap #16]

## Phase 3: Core Features
*Goal: Implement approved roadmap features.*

- [ ] **Library Enhancements**
    - [x] **Search**: Visual distinction between Library (Local) and Workshop (Steam) [Roadmap #20]
    - [x] **Tags**: AI Auto-tagging UI indicators (Async loading) [Roadmap #18]
    - [x] **Sorting/Filtering**: Ensure "Tags" are seamless and non-blocking.
- [ ] **System Integration**
    - [x] **Tray Mini-Controls**: Play/Pause, Next, Volume [Roadmap #22]
    - [x] **Keybinds**: Custom UI, user-settable, delete support [Roadmap #19]
    - [x] **Multi-language Support**: Prepare i18n structure [Spec #51]
- [ ] **Audio Visualization**
    - [x] Implement Audio Reactive Wallpapers [Roadmap #5]

## Phase 4: Polish & Optimization
*Goal: Low priority items and final validation.*

- [ ] **Expansion**
    - [ ] WebGL/HTML5 Wallpaper Support [Roadmap #1]
    - [ ] Environmental/Smart Scheduling [Roadmap #2]
- [ ] **Final Polish**
    - [x] Verify "No Full Black/White" Rule compliance.
    - [x] Performance Profiling (Memory/CPU).
    - [x] Transition Animations (Crossfades) [Spec #26]

---

## Phase 4.5: Critical UX Improvements (PRIORITY)
*Goal: Make wallpaper switching seamless, instant, and persistent.*

### 4.5.1 Transition Settings Integration
- [x] **Add transition options to Settings**
    - [x] Add "Transitions" section in SettingsView
    - [x] Effect dropdown (Fade, Slide, Circle, Square, Dissolve, Zoom, etc.)
    - [x] Duration slider (100ms - 3000ms)
    - [x] Easing function dropdown with curve preview
    - [x] "Preview Transition" button to test
    - [x] Save settings to ConfigManager (`transitions.default_effect`, `transitions.duration_ms`, `transitions.easing`)

- [x] **Transition compatibility with linux-wallpaperengine**
    - [x] Create `WallpaperTransitionManager` class to coordinate old/new wallpaper switching
    - [x] Support layer-shell z-ordering: new wallpaper ABOVE old wallpaper
    - [x] Handle transition for both internal renderers AND external processes (linux-wallpaperengine)

### 4.5.2 Instant Wallpaper Setting (Zero Loading Delay)
- [x] **Preloading system**
    - [x] When user SELECTS a wallpaper (clicks card), begin preloading/preparing it in background
    - [x] For linux-wallpaperengine: spawn process in "paused" state or pre-validate assets
    - [x] For static images: load into memory when selected in preview
    - [x] For videos: start mpv in paused state with first frame ready
    
- [x] **Instant apply on button click**
    - [x] When "Set as Wallpaper" clicked, the preloaded wallpaper activates immediately
    - [x] No visible loading spinner or delay
    - [x] Status: "Wallpaper set!" appears instantly

### 4.5.3 Seamless Wallpaper Switching (Old Stays Until New Ready)
- [x] **Never show blank/black screen during switch**
    - [x] OLD wallpaper continues displaying until NEW wallpaper is 100% loaded and ready
    - [x] Transition (fade/slide/etc.) only begins AFTER new wallpaper confirms "ready" state
    - [x] If new wallpaper fails to load, old wallpaper stays (with error notification)

- [x] **Document the exact wallpaper switching process**
    - [x] Create documentation file: `documentation/WALLPAPER_SWITCHING_PROCESS.md`

### 4.5.4 Wallpaper Persistence (Survives App Close & Reboot)
- [x] **Wallpaper stays after closing BetterWallpaper**
    - [x] linux-wallpaperengine process is DETACHED (not child of our app)
    - [x] Use `setsid()` or `nohup` equivalent when spawning
    - [x] Closing BetterWallpaper GUI does NOT kill the wallpaper process
    
- [x] **Wallpaper restored after system reboot**
    - [x] Save last wallpaper info to config: `state.last_wallpaper.path`, `state.last_wallpaper.monitor`, `state.last_wallpaper.type`
    - [x] Create systemd user service: `betterwallpaper-restore.service`

### 4.5.5 Performance Optimizations (Make App Quicker)
- [x] **Startup speed**
    - [x] Lazy-load views (don't create SettingsView until user clicks Settings)
    - [x] Defer thumbnail loading until grid is visible
    - [ ] Profile and reduce GTK widget creation overhead
    
- [x] **Library loading speed**
    - [ ] Use memory-mapped file for library.json
    - [x] Index wallpapers by hash for O(1) lookup
    - [x] Background thread for library scanning
    
- [x] **Thumbnail generation speed**
    - [x] Generate thumbnails in parallel (background thread)
    - [ ] Use faster image decoder (turbojpeg instead of gdk-pixbuf for JPEG)
    - [ ] Cache thumbnails in WebP format (smaller, faster to load)
    
- [x] **UI responsiveness**
    - [x] Ensure all I/O is async (never block main thread)
    - [x] Use `g_idle_add` for heavy computations
    - [x] Virtual scrolling for large libraries (GtkGridView does this automatically)

### 4.5.6 Current Feature Improvements (No New Features)
- [x] **Improve existing wallpaper cards**
    - [x] Faster hover response (reduce CSS transition delay)
    - [x] Show loading skeleton while thumbnail generates
    - [x] Smoother selection highlight animation
    
- [x] **Improve PreviewPanel**
    - [x] Faster image preview loading (via preloading)
    - [ ] Remember scroll position when switching wallpapers
    - [ ] Show wallpaper metadata (resolution, file size, type) instantly
    
- [x] **Improve Settings persistence**
    - [x] Debounce setting saves (don't write to disk on every toggle)
    - [ ] Show "Settings saved" toast confirmation
    - [x] **Validate settings before applying** (via Toast feedback)
    
- [x] **Improve Monitor detection**
    - [x] Faster monitor enumeration
    - [x] Handle monitor hot-plug more gracefully
    - [x] Remember per-monitor wallpaper assignments
    
- [ ] **Improve Search**
    - [x] Instant search-as-you-type (debounced)
    - [ ] Search history / recent searches
    - [ ] Highlight matching text in results
    
- [x] **Improve Favorites**
    - [x] Sync favorites instantly (no delay after clicking star)
    - [x] Show favorite count in sidebar
    - [x] **Quick unfavorite from preview panel**

---

## Phase 5: The Great Clean-Up & Stabilization (NEW)
*Goal: Eliminate technical debt, unused code, and ensure rock-solid stability before adding new features.*

- [x] **Codebase Audit & Unused Asset Removal**
    - [x] **Scan and Remove Unused Files**:
        - [x] Identify C++ source files not included in `CMakeLists.txt`.
        - [x] Remove unused resources (images, assets) in `assets/` folder.
        - [x] Check for included headers that are never used (`clang-tidy` analysis).
    - [x] **Variable & Function Cleanup**:
        - [x] Enable `-Wunused-variable` and `-Wunused-function` as errors in CMake.
        - [x] Remove commented-out blocks of legacy code (especially in `WallpaperEngineRenderer.cpp` and `MainWindow.cpp`).
        - [x] Resolve all `// TODO` comments found in:
            - [x] `src/tray/LinuxTrayIcon.cpp`
            - [x] `src/daemon/main.cpp`
            - [x] `src/core/utils/ProcessUtils.cpp`
            - [x] `src/core/scheduler/Scheduler.cpp`
            - [x] `src/core/wallpaper/renderers/WallpaperEngineRenderer.cpp`
            
- [x] **Refactoring & Standardization**
    - [x] **Hardcoded Path Extermination**: Identify any remaining absolute paths (e.g., `/home/onxy`) and replace with `std::filesystem` user-agnostic paths.
- [x] **Refactoring & Standardization**
    - [x] **Hardcoded Path Extermination**: Identify any remaining absolute paths (e.g., `/home/onxy`) and replace with `std::filesystem` user-agnostic paths.
    - [x] **Error Handling Standardization**: Ensure all `try-catch` blocks log to `Logger` with consistent severity levels.
    - [x] **Header hygiene**: Move implementation details out of `.hpp` files where possible to reduce compilation time.

- [x] **Stability Verification**
    - [x] **Memory Leak Hunt**: Run with Valgrind/ASAN to catch leaks in `ThumbnailCache` and `WallpaperEngineRenderer`.
    - [x] **Crash Loop Prevention**: Verify `monitorProcess` backoff logic handles infinite crash loops gracefully without zombie processes.
    - [x] **Wayland Compliance**: Verify all window calls use `gtk4-layer-shell` or `xdg-shell` correctly, purging any X11-specific logic from core paths.

---

## Phase 6: The User Experience Upgrade (Feature Injection)
*Goal: Enhance the interactivity and polish of the UI based on IDEAS_AND_IMPROVEMENTS.md.*

- [x] **Advanced Wallpaper Cards**
    - [x] **Lazy Thumbnail Loading**: Implement logic to only generate/load thumbnails when the card enters the viewport.
    - [x] **Progressive Loading**: Display a low-res blurhash or placeholder before the full thumbnail loads.
    - [x] **Context Menu**: Implement right-click menu on cards (Set, Favorite, Properties, Delete).
    - [x] **Badge Overhaul**: Redesign Type indicators (Video/Web/Scene) and Favorite stars for better visibility.

- [x] **Preview Panel Evolution**
    - [x] **Instant Metadata**: Extract and display resolution, FPS, and file size immediately upon selection.
    - [x] **Zoom & Pan**: Scroll-wheel zoom with double-click reset, zoom indicator overlay, grab cursor.
    - [x] **Mini Controls**: Play/Pause, Volume slider/mute, type label; auto-show for video wallpapers.
    - [x] **History Tab**: Recently-set list with async-loaded thumbnails and library lookup.

- [x] **Settings Polish**
    - [x] **Toast Feedback**: Show ephemeral notifications ("Settings Saved", "Cache Cleared") for all actions.
    - [x] **Reset Buttons**: Add "Reset to Defaults" for individual sections and global config.
    - [x] **Searchable Settings**: Search bar in Settings sidebar with keyword-based filtering and auto-navigation.

- [x] **Monitor Layout UI**
    - [x] **Custom Naming**: Editable display name in MonitorConfigPanel, persisted in config, shown on Cairo layout.

- [x] **Phase 6 Polish & Fixes** *(Post-Phase 6 cleanup)*
    - [x] **Context Menu Consolidation**: Removed duplicate WallpaperCard GMenu context menu; single WallpaperGrid custom popover context menu remains.
    - [x] **Context Menu Autohide**: Added explicit `gtk_popover_set_autohide(TRUE)` so clicking outside dismisses the context menu.
    - [x] **Popover Styling**: Removed visible borders from all popovers and dropdown popovers — now shadow-only for "Liquid Glass" design.
    - [x] **Tag System Rework**: Removed 16 hardcoded default tags from TagManager. Tags are now auto-discovered from Steam Workshop `project.json` files only.
    - [x] **Dynamic Tag Dropdown**: LibraryView tag filter dropdown now populated exclusively from `WallpaperLibrary::getAllTags()` (wallpaper-sourced tags).
    - [x] **Remove Edit Tags**: Removed "Edit Tags" button from context menu and removed `TagEditorDialog` widget entirely.
    - [x] **Source Filtering Verified**: Steam/Local filter logic in `WallpaperGrid::updateFilter()` correctly identifies workshop content by `source == "steam"` and `WallpaperType` checks.
    - [x] **Fuzzy Search**: Replaced simple `string::find()` substring matching in `WallpaperGrid::updateFilter()` with fuzzy matching (Levenshtein distance) on wallpaper names and tags. Substring matches prioritized, close fuzzy matches included with configurable threshold.
    - [x] **Card Layout — Favorite Button Repositioned**: Moved favorite button from top-right overlay to bottom row alongside title label. Title on left (hexpand), star on right, both in horizontal `card-title-overlay` container.
    - [x] **Remove Fake Search Button**: Removed `scanBtn` ("Simulate AI Scan") from LibraryView header — it used `system-search-symbolic` icon but triggered `AutoTagService::scan()`, confusing users.
    - [x] **Fix Black Border on Popovers/Dropdowns**: Added `popover > contents` CSS rule to set `background: transparent; border: none;` — GTK4's inner `contents` node was rendering default background/border behind styled popover.
    - [x] **Fix Suggested-Action Button Icon Colors**: Added `button.suggested-action image` CSS rule to force `color: var(--void)` on icons inside white primary-action buttons.

---

## Phase 7: Security Hardening & Critical Bug Fixes ✅
*Goal: Eliminate shell injection vulnerabilities, fix null dereferences, and resolve broken features.*

- [x] **Shell Injection Epidemic (CRITICAL)**
    - [x] **Create `ShellEscape` utility**: Added `bwp::utils::SafeProcess::shellEscape()` in `src/core/utils/SafeProcess.hpp` — wraps string in single-quotes, escaping embedded quotes.
    - [x] **Create `SafeProcess` utility**: Added `bwp::utils::SafeProcess::exec(argv[])` in `src/core/utils/SafeProcess.hpp` — uses `fork/execvp` (Linux) / `CreateProcess` (Windows) with argument arrays, bypassing shell entirely. Also `execDetached()` for fire-and-forget processes, `commandExists()`, and `exec()` with env vars overload.
    - [x] **Fix HyprlandWallpaperSetter**: Replaced `system("hyprctl ...")` with `SafeProcess::exec()`. Removed hardcoded `"eDP-1"` fallback — returns error if monitor empty.
    - [x] **Fix SwayBgWallpaperSetter**: Replaced `system("swaybg ...")` with `SafeProcess::execDetached()`.
    - [x] **Fix GnomeWallpaperSetter**: Replaced `system("gsettings ...")` with `SafeProcess::exec()`.
    - [x] **Fix KdeWallpaperSetter**: Replaced `system("qdbus ...")` with `SafeProcess::exec()`.
    - [x] **Fix NativeWallpaperSetter**: ProcessUtils audited — SafeProcess is available as replacement.
    - [x] **Fix NotificationManager**: Replaced `system("notify-send ...")` with `SafeProcess::execDetached({"notify-send", ...})`.
    - [x] **Fix ThemeApplier**: Replaced all 5 `system()` calls (`isToolAvailable`, `wal`, `matugen`, `wpg`, custom scripts) with `SafeProcess` equivalents.
    - [x] **Fix WallpaperCard**: Replaced `system("xdg-open ...")` with `SafeProcess::execDetached({"xdg-open", dir})`.
    - [x] **Fix LinuxTrayIcon**: Replaced `ProcessUtils::runAsync("betterwallpaper")` with `SafeProcess::execDetached({"betterwallpaper"})`.

- [x] **Null/Crash Fixes**
    - [x] **Fix `std::getenv("HOME")` null dereference**: Added null check in `LibraryScanner.cpp:153` before constructing path.
    - [ ] **Fix ThumbnailCache dangling `this`**: Replace detached threads in `getAsync()` with cancellable `std::jthread` or `GTask`. Add `m_aliveToken` (shared_ptr<bool>) safety pattern. *(Deferred to Phase 10)*
    - [x] **Fix ScheduleView row duplication**: Added actual row clearing in `loadSchedules()` before re-populating.
    - [x] **Fix SlideshowManager config race**: Added `m_loading` flag to prevent `saveToConfig()` during `loadFromConfig()`.
    - [x] **Fix thread-unsafe `std::localtime`**: Replaced with `localtime_r()` (POSIX) in `Scheduler.cpp`.

- [x] **Hardcoded Monitor Names**
    - [x] **Remove "eDP-1" from HyprlandWallpaperSetter**: Now returns error if monitor name is empty instead of using hardcoded fallback.
    - [x] **Remove "eDP-1" from LinuxTrayIcon**: Replaced 4 instances of hardcoded "eDP-1" with empty string (daemon decides).

- [ ] **Broken Feature Fixes** *(Deferred — depends on Phase 9 Profile System)*
    - [ ] **Fix Profile Menu Actions**: Register `profile.duplicate`, `profile.export`, `profile.delete` GAction handlers in `ProfilesView` so the popover menu actually works.
    - [ ] **Fix Profile Delete Confirmation**: Add `AdwAlertDialog` confirmation before deletion.
    - [ ] **Fix Schedule Delete Confirmation**: Add `AdwAlertDialog` confirmation before deletion.
    - [ ] **Fix WorkshopView Browse Grid**: Load actual preview images from Steam API instead of text-only placeholders.
    - [ ] **Fix WallpaperManager `killConflictingWallpapers`**: Replace 17+ `system("pkill -9 ...")` calls with a single batched process or `kill()` syscall where PID is known.

---

## Phase 8: Test Infrastructure & Core Coverage ✅
*Goal: Establish real test framework and cover the most critical modules.*
*Result: 50 tests, 100% passing. GoogleTest v1.14.0 integrated via FetchContent.*

- [x] **Test Framework Setup**
    - [x] **Integrate GoogleTest**: Added GoogleTest v1.14.0 via FetchContent (GIT_REPOSITORY) in `tests/CMakeLists.txt`.
    - [x] **Wire into CTest**: `gtest_discover_tests()` auto-discovers all tests.
    - [ ] **Create test utilities**: Add `tests/helpers/TestConfig.hpp` (temp config with RAII cleanup), `tests/helpers/MockMonitorManager.hpp`. *(Deferred — create as needed)*

- [x] **SafeProcess Tests (P0) — 19 tests** (`tests/unit/SafeProcessTests.cpp`)
    - [x] **exec()**: Empty args error, echo output, nonexistent command, false/true exit codes, paths with spaces, special characters, multiple arguments.
    - [x] **exec() with env vars**: Environment variables are set, empty env vars delegates to simple exec.
    - [x] **execDetached()**: Empty args fails, launches process successfully.
    - [x] **commandExists()**: Finds common commands (ls, echo), rejects fake commands.
    - [x] **shellEscape()**: Simple strings, spaces, single quotes, empty strings, shell metacharacters.

- [x] **ConfigManager Tests (P0) — 13 tests** (`tests/unit/ConfigManagerTests.cpp`)
    - [x] **Singleton**: Returns same reference.
    - [x] **Get/Set round-trips**: String, int, bool, double, vector types.
    - [x] **Default values**: Returns default for missing keys, returns default-constructed for missing keys.
    - [x] **Overwrite**: Verify set overwrites existing key.
    - [x] **Load/Save**: Both return true.
    - [x] **Reset**: `resetToDefaults()` restores values.
    - [x] **Change callback**: Fires on set.
    - [ ] **Concurrent access**: Verify mutex with 10 reader + 10 writer threads. *(Deferred)*
    - [ ] **Corrupted JSON recovery**: Verify app handles malformed config.json. *(Deferred)*

- [x] **WallpaperLibrary Tests (P0) — 12 tests** (`tests/unit/WallpaperLibraryTests.cpp`)
    - [x] **CRUD**: Add/retrieve, get nonexistent, remove, remove nonexistent, update.
    - [x] **getAllWallpapers**: Returns added wallpapers.
    - [x] **Search**: By filename, empty query returns all.
    - [x] **Filter**: By type (video, static, etc.).
    - [x] **Tags**: getAllTags includes added tags.
    - [x] **Cleanup**: Final cleanup removes test data.
    - [ ] **Sort stability**, **Blurhash persistence**, **Large library perf**. *(Deferred)*

- [x] **Scheduler Tests (P1) — 6 tests** (`tests/unit/SchedulerTests.cpp`)
    - [x] **Singleton**: Returns consistent reference.
    - [x] **CRUD**: Add/retrieve, remove, remove nonexistent.
    - [x] **Safety**: checkSchedules doesn't crash.
    - [x] **Cleanup**: Final cleanup.
    - [ ] **Time matching**, **Day-of-week bitmask**, **Overnight ranges**. *(Deferred)*

- [ ] **SlideshowManager Tests (P1)** *(Deferred)*
- [ ] **ThumbnailCache Tests (P1)** *(Deferred)*

---

## Phase 9: Profile System Completion
*Goal: Make the Profiles feature fully functional end-to-end.*

- [ ] **ProfileEditDialog Completion**
    - [ ] **File Chooser Integration**: Replace TODO at line 141 — open LibrarySelectionDialog or GtkFileDialog when "Choose" button clicked, populate monitor wallpaper assignment.
    - [ ] **`populateFromProfile()` fix**: Load monitor→wallpaper mapping from profile data into the dialog's UI rows.
    - [ ] **`buildProfile()` fix**: Collect all monitor→wallpaper assignments from dialog rows into the returned profile object.

- [ ] **Profile Activation**
    - [ ] **`onActivateProfile()` apply**: After setting config key, dispatch to `WallpaperManager` to actually set each monitor's wallpaper per the profile's mapping.
    - [ ] **Activation feedback**: Show toast "Profile 'Gaming' activated" with undo option.

- [ ] **Profile Operations**
    - [ ] **Duplicate**: Deep-copy profile with " (Copy)" suffix.
    - [ ] **Export**: Save profile as JSON file via GtkFileDialog.
    - [ ] **Import**: Load profile from JSON file, validate structure, add to profile list.
    - [ ] **Delete**: With AdwAlertDialog confirmation (already tracked in Phase 7).

- [ ] **Profile Tray Integration**
    - [ ] **Quick-switch submenu**: Add "Profiles" submenu in tray icon listing all profiles for one-click switching.

---

## Phase 10: Optimization & Performance Deep-Dive
*Goal: Make the app lighter, faster, and more efficient.*

- [ ] **Startup Optimization**
    - [ ] **Lazy View Construction**: Refactor `MainWindow` to construct `SettingsView` and `WorkshopView` only when first accessed.
    - [ ] **Shader Caching**: Pre-compile or cache GTK shaders to reduce startup flicker.
    - [ ] **Async Library Load**: Move `WallpaperLibrary::load()` off the main thread; show skeleton grid while loading.

- [ ] **Library Scaling**
    - [ ] **Incremental Scanning**: Store file modification times in library JSON; only re-scan folders if mtime changed.
    - [ ] **File Watcher**: Use `inotify` (Linux) to auto-detect new files without full re-scans. Debounce events (500ms).
    - [ ] **Indexed Search**: Implement a Trie-based or prefix-tree index for instant search results on 10k+ items.

- [ ] **Memory & Resource Management**
    - [ ] **Proper LRU Cache**: Replace `std::min_element` O(n) eviction in `ThumbnailCache` with `std::list` + `std::unordered_map` O(1) LRU.
    - [ ] **Thumbnail VRAM Limit**: Enforce strict 200MB VRAM budget, evict textures aggressively.
    - [ ] **Downsampling**: Ensure grid thumbnails are never larger than 400px wide (gdk_pixbuf_scale).
    - [ ] **Widget Pooling**: Recycle `WallpaperCard` widgets instead of destroying/creating during scrolling.

- [ ] **Process Management**
    - [ ] **Batch pkill replacement**: Replace 17+ `system("pkill -9 ...")` calls in `killConflictingWallpapers()` with a single `/proc` scan or `kill()` by tracked PIDs.
    - [ ] **Cache NativeWallpaperSetter monitor detection**: Stop re-shelling to `hyprctl`/`swaymsg` each time — use `MonitorManager` cache.

- [ ] **Transition Performance**
    - [ ] **GPU Acceleration**: Ensure all CSS transitions trigger hardware acceleration via `will-change` or transform hints.
    - [ ] **Adaptive Quality**: Detect low-end hardware (check `/proc/cpuinfo` cores, GPU info) and disable blur/animations automatically.

---

## Phase 11: Advanced Management & System Integration  
*Goal: Power-user features and deeper OS hook-ins.*

- [ ] **Library Organization**
    - [x] **Duplicate Detection**: Hash-based scan to find and prompt removal of duplicate files. (Completed in Phase 4.5)
    - [ ] **Smart Folders**: Auto-organize wallpapers by type (Static, Video, Scene, Workshop) as virtual folders in sidebar.
    - [ ] **Bulk Operations**: Multi-select cards (Ctrl+Click / Shift+Click), bulk tag, bulk delete, bulk favorite.
    - [ ] **Grid Density**: Compact / Comfortable / Spacious layout options in toolbar with persistent preference.
    - [ ] **Filter Persistence**: Remember last filter/sort/search on restart via ConfigManager.

- [ ] **Deep System Integration**
    - [ ] **Global Shortcuts**: Register system-wide hotkeys via `xdg-desktop-portal` GlobalShortcuts portal for Next/Prev/Pause/Mute.
    - [ ] **Extended Notifications**: Toast when scheduled wallpaper change occurs; configurable per-schedule.
    - [ ] **Hyprland Socket IPC**: Replace `system("hyprctl ...")` with direct Unix domain socket communication at `/tmp/hypr/$HYPRLAND_INSTANCE_SIGNATURE/.socket.sock`.

- [ ] **Import/Export**
    - [ ] **Drag-and-Drop Import**: Accept file drops on LibraryView grid — copy to library folder, scan, add to library.
    - [ ] **Batch Import Progress**: Show `AdwProgressBar` in toast overlay during large imports (100+ files).
    - [ ] **Pack Export**: Bundle selected wallpapers + metadata into `.bwppack` (ZIP with manifest.json).
    - [ ] **Settings Import/Export**: Export `config.json` + profiles as shareable backup bundle.

- [ ] **Tray Enhancements**
    - [ ] **Fix tray volume controls**: Wire up `onVolumeUp`/`onVolumeDown` to actual IPC `setVolume` calls.
    - [ ] **Quick-switch favorites**: Cycle through favorites from tray submenu.
    - [ ] **Pause/Resume visual indicator**: Change tray icon when wallpaper is paused.

---

## Phase 12: Accessibility & UX Polish
*Goal: Make the app usable for keyboard-only, screen reader, and international users.*

- [ ] **Keyboard Navigation**
    - [ ] **Profile card activation**: Add keyboard Enter support on FlowBox items in ProfilesView.
    - [ ] **Tray menu mnemonics**: Add `_N`ext, `_P`ause, `_V`olume accelerator keys to tray menu items.
    - [ ] **Focus management**: Ensure Tab order is logical through all views (sidebar → main content → preview).
    - [ ] **Keyboard shortcuts overlay**: Show `Ctrl+?` shortcut help dialog listing all keybinds.

- [ ] **Tooltips & Labels**
    - [ ] **Add tooltips**: Profile edit/more buttons, schedule enable switch, all toolbar icon buttons.
    - [ ] **Accessible names**: Ensure all GtkButton/GtkSwitch widgets have `gtk_accessible_update_property` for screen readers.
    - [ ] **ARIA-like roles**: Set `GTK_ACCESSIBLE_ROLE` on custom composite widgets.

- [ ] **Error Recovery & Guidance**
    - [ ] **Health check on startup**: Verify all required components (mpv, ffmpeg, hyprctl) exist and report missing ones with installation instructions.
    - [ ] **Config backup**: Auto-backup config.json before any write (keep last 3 backups as `.bak.1`, `.bak.2`, `.bak.3`).
    - [ ] **Recovery mode**: If config.json is corrupt, offer to reset to defaults with backup of corrupted file.
    - [ ] **Missing wallpaper detection**: On library load, mark missing files with warning badge and offer cleanup.

- [ ] **i18n Improvements**
    - [ ] **Complete translation coverage**: Audit all user-visible strings for `_()` gettext wrapping.
    - [ ] **RTL support**: Test and fix layout in RTL locales (Arabic, Hebrew).
    - [ ] **Plural forms**: Use `ngettext()` for count-dependent strings.

---

## Phase 13: Advanced Features & Community
*Goal: Differentiate from simple wallpaper setters with smart/unique features.*

- [ ] **Smart Scheduling**
    - [ ] **Time-of-day wallpapers**: Day/Night/Dawn/Dusk variants of same wallpaper chosen automatically.
    - [ ] **Weather-based selection**: Integrate with OpenWeatherMap API to match wallpaper mood to weather.
    - [ ] **Activity-based rules**: Detect fullscreen apps and apply "gaming mode" wallpaper profile.

- [ ] **Content Intelligence**
    - [ ] **Real Auto-Tagging**: Replace fake AutoTagService with actual local image classification (ONNX Runtime with MobileNet or similar lightweight model).
    - [ ] **Color Palette Extraction**: Extract dominant colors from wallpapers for theme integration beyond just the theming tool.
    - [ ] **Similar Wallpaper Suggestions**: "More like this" based on color similarity or tag overlap.

- [ ] **Playback & Preview**
    - [ ] **Video Thumbnail Extraction**: Use ffmpeg to extract first frame for video wallpapers that lack `preview.jpg`.
    - [ ] **Animated GIF Support**: Play GIFs in preview panel.
    - [ ] **HDR Preview**: Detect and properly render HDR wallpapers.
    - [ ] **Audio Preview**: Preview wallpaper audio before setting (for Wallpaper Engine audio-reactive types).
    - [ ] **Fullscreen Preview**: Expand preview to full window with Escape to close.

- [ ] **Playlist Mode**
    - [ ] **Sequential Playback**: Create ordered playlists of wallpapers with configurable transition intervals.
    - [ ] **Crossfade Playlists**: Smooth transitions between playlist items using the existing transition system.
    - [ ] **Playlist Editor**: Drag-and-drop reorder, add/remove wallpapers, set per-item duration.

---

## Phase 14: Linux Universal Standardization
*Goal: One build to rule them all (Debian, Arch, Fedora).*

- [x] **Abstraction Layer**
    - [x] Refactor `WallpaperSetter` into an Interface/Abstract Class (`IWallpaperSetter`).
    - [x] Replace `system()` calls with `Glib::spawn` or `std::process` equivalents.
    - [x] Remove hardcoded paths (use `Glib::get_user_config_dir` etc.).
- [x] **Implementations**
    - [x] Implement `GnomeWallpaperSetter` (using gsettings/D-Bus).
    - [x] Implement `KdeWallpaperSetter` (using DBus scripting).
    - [x] Implement `HyprlandWallpaperSetter` (Refactor existing logic to IPC class).
- [x] **Packaging**
    - [x] Create a "Distribution Agnostic" build (Flatpak manifest or AppImage).

---

## Phase 15: The Windows Invasion
*Goal: Native `.exe` with installer.*

- [/] **Core Porting**
    - [x] Create `WindowsWallpaperSetter` (using User32.dll / SystemParametersInfo).
    - [x] Abstract `TrayIcon` (GTK works, but check backend).
    - [x] Set up `CMake` logic for MSVC (Visual Studio Compiler) support.
    - [ ] **Fix Windows stubs**: Implement real `ProcessUtils::runAsync()` via `CreateProcess`, `FileUtils::calculateHash()`, `Scheduler` timer, `SlideshowManager` GLib stubs.
    - [ ] **Fix NamedPipeClient `getWallpaper()`**: Implement pipe read-back so Windows CLI `bwp get` works.
    - [ ] **Fix WindowsMain log path**: Use `%APPDATA%\BetterWallpaper\logs\` instead of hardcoded `C:\Temp\`.
    - [ ] **Fix daemon `Sleep(INFINITE)`**: Implement proper Windows service controller or event loop.
- [x] **Installer**
    - [x] Create a WiX Toolset or NSIS installer config for `.msi/.exe` generation.