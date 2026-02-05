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
    > **IMPORTANT**: The Cairo-based transitions (FadeEffect, etc.) work with our internal renderers (StaticRenderer, VideoRenderer). 
    > For linux-wallpaperengine (external process), transitions work differently:
    > 
    > **How it works:**
    > 1. When user sets a wallpaper via linux-wallpaperengine, the OLD wallpaper process keeps running
    > 2. NEW linux-wallpaperengine process starts IN FRONT of it (higher layer, opacity 0)
    > 3. NEW wallpaper fades IN (opacity 0 → 1) over the old one
    > 4. Old process is killed after transition completes
    > 
    > **Implementation tasks:**
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
    > Create documentation file: `documentation/WALLPAPER_SWITCHING_PROCESS.md`
    > 
    > **Process Flow:**
    > ```
    > 1. User has Wallpaper A displayed on desktop
    > 2. User selects Wallpaper B in the library (click on card)
    >    → PreviewPanel shows Wallpaper B info
    >    → Background: Wallpaper B begins preloading
    > 3. User clicks "Set as Wallpaper" button
    >    → If preload complete: Proceed immediately
    >    → If preload not complete: Wait (show subtle indicator)
    > 4. Wallpaper B process starts IN FRONT of A (invisible - opacity 0)
    >    → linux-wallpaperengine spawns with Wallpaper B at higher z-index
    >    → OR internal renderer loads Wallpaper B above A
    > 5. Wallpaper B signals "ready" (first frame rendered)
    > 6. Transition begins:
    >    → B fades IN (opacity 0 → 1) over A
    >    → Duration: user's setting (default 500ms)
    >    → Easing: user's setting (default easeInOut)
    > 7. Transition completes:
    >    → Wallpaper A process killed/cleaned up (now hidden behind B)
    >    → Wallpaper B now sole wallpaper
    > 8. State saved to config:
    >    → `current_wallpaper.path` = B's path
    >    → `current_wallpaper.monitor` = target monitor
    > ```

### 4.5.4 Wallpaper Persistence (Survives App Close & Reboot)
- [x] **Wallpaper stays after closing BetterWallpaper**
    - [x] linux-wallpaperengine process is DETACHED (not child of our app)
    - [x] Use `setsid()` or `nohup` equivalent when spawning
    - [x] Closing BetterWallpaper GUI does NOT kill the wallpaper process
    
- [x] **Wallpaper restored after system reboot**
    - [x] Save last wallpaper info to config: `state.last_wallpaper.path`, `state.last_wallpaper.monitor`, `state.last_wallpaper.type`
    - [x] Create systemd user service: `betterwallpaper-restore.service`
        ```ini
        [Unit]
        Description=Restore BetterWallpaper on login
        After=graphical-session.target
        
        [Service]
        Type=oneshot
        ExecStart=/usr/bin/bwp restore-wallpaper
        
        [Install]
        WantedBy=default.target
        ```
    - [ ] CLI command `bwp restore-wallpaper` reads config and sets last wallpaper
    - [ ] Alternative: XDG autostart desktop file that runs restore on login

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
    - [ ] Validate settings before applying
    
- [x] **Improve Monitor detection**
    - [x] Faster monitor enumeration
    - [x] Handle monitor hot-plug more gracefully
    - [x] Remember per-monitor wallpaper assignments
    
- [ ] **Improve Search**
    - [ ] Instant search-as-you-type (debounced)
    - [ ] Search history / recent searches
    - [ ] Highlight matching text in results
    
- [x] **Improve Favorites**
    - [x] Sync favorites instantly (no delay after clicking star)
    - [ ] Show favorite count in sidebar
    - [ ] Quick unfavorite from preview panel

---

## Phase 5: Linux Universal Standardization
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

## Phase 6: The Windows Invasion
*Goal: Native `.exe` with installer.*
- [/] **Core Porting**
    - [x] Create `WindowsWallpaperSetter` (using User32.dll / SystemParametersInfo).
    - [x] Abstract `TrayIcon` (GTK works, but check backend).
    - [x] Set up `CMake` logic for MSVC (Visual Studio Compiler) support.
- [x] **Installer**
    - [x] Create a WiX Toolset or NSIS installer config for `.msi/.exe` generation.

## Phase 7: MacOS Ecosystem
*Goal: Native `.app` bundle.*
- [ ] **Core Porting**
    - [ ] Create `MacWallpaperSetter` (using Cocoa/ScriptingBridge).
    - [ ] Adjust UI for Mac Menu Bar behavior (Global Menu vs In-Window).
- [ ] **Packaging**
    - [ ] Package as `.dmg` (Disk Image).
