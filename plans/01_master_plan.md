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
    - [ ] **Experience Optimization** (Priority Request)
        - [ ] **Faster Switching**: Optimize renderer loading pipeline.
        - [ ] **Seamless Transition**: Ensure old wallpaper stays until new one is ready.
        - [ ] **Persistence**: Detach wallpaper process on app exit so it stays running.
- [ ] **Final Polish**
    - [x] Verify "No Full Black/White" Rule compliance.
    - [x] Performance Profiling (Memory/CPU).
    - [x] Transition Animations (Crossfades) [Spec #26]

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
