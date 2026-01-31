# BetterWallpaper - Final Master Plan (v3)

> **The definitive roadmap for BetterWallpaper v1.0.0.**
> consolidated from previous plans and user decisions from `answers.md`.

---

## ✅ Phase 1: Core Foundation & Basic Architecture
*Status: Completed*

- [x] **1.1 Project Structure**
  - [x] Directory hierarchy (`src/core`, `src/gui`, `src/daemon`)
  - [x] CMake build system setup
  - [x] Dependency integration (libmpv, gtk4, libadwaita, nlohmann_json)
- [x] **1.2 Core Libraries**
  - [x] `Logger` system
  - [x] `ConfigManager` (JSON based)
  - [x] `FileUtils` / `StringUtils`
- [x] **1.3 Daemon Architecture**
  - [x] Single instance checks
  - [x] Main event loop
  - [x] Wallpaper Renderer interfaces
- [x] **1.4 Rendering Engine Basics**
  - [x] `StaticRenderer` (Images)
  - [x] `VideoRenderer` (MPV)
  - [x] `WallpaperEngineRenderer` (IPC wrapper)
- [x] **1.5 Basic GUI Shell**
  - [x] Main Window with Sidebar
  - [x] Basic Styling (CSS)
  - [x] Wallpaper Grid View

## ✅ Phase 2: Visual Polish & User Experience
*Status: Completed*

- [x] **2.1 Animations & Transitions**
  - [x] Crossfade page transitions
  - [x] Card hover effects (Scale/Shadow)
  - [x] Sidebar selection animations
- [x] **2.2 Asynchronous Loading**
  - [x] Background `LibraryScanner`
  - [x] Lazy-loaded thumbnails (Cache)
  - [x] Skeleton loading states
- [x] **2.3 UI Refinement**
  - [x] Consistent spacing & border radius
  - [x] Dark mode color palette
  - [x] Typography hierarchy

## ✅ Phase 3: Settings & System Integration
*Status: Completed*

- [x] **3.1 Settings Overhaul**
  - [x] Split into sub-pages (General, Graphics, Audio, etc.)
  - [x] **Inputs:** `InputManager` for global keybinds (Ctrl+Arrows)
  - [x] **Power:** `PowerManager` for Battery detection
  - [x] **Autostart:** `AutostartManager` (XDG Desktop Entry)
- [x] **3.2 Controls Logic**
  - [x] Pause/Resume/Next/Prev logic
  - [x] "Pause on Battery" implementation
  - [x] "Start Minimized" configuration hook

---

## 🚧 Phase 4: Final Feature Implementation (The "Polish" Phase)
*Current Focus Area - Derived from `answers.md`*

### 4.1 Error Handling Strategy
- [x] **4.1.1** Implement Robust File Validation
  - [x] Auto-remove missing files from Library DB on startup [Answer #4]
  - [x] Detect corrupt image headers before loading
- [x] **4.1.2** User Notification for Failures [Answer #5]
  - [x] Show **Popup Dialog** when a wallpaper fails to set
  - [x] Show "Failed" status badge in sidebar/preview
  - [x] Implement exponential back-off for rendering crashes [Answer #6]

### 4.2 Audio Management [Answer #3]
- [x] **4.2.1** Smart Mute Logic
  - [x] Listen for window focus events
  - [x] Mute wallpaper audio when **any** other window has focus (or when user is not on desktop)
  - [x] Ensure Mute persists correctly across wallpaper changes

### 4.3 Setup Wizard (First Run) [Answer #53, #65]
- [x] **4.3.1** Create `SetupDialog` (Modal)
  - [x] **Step 1: Welcome**: Brief intro
  - [x] **Step 2: Library**: Select initial folder (default `~/Pictures/Wallpapers`)
  - [x] **Step 3: Theme**: Choose Light/Dark preference [Answer #15]
  - [x] **Step 4: Performance**: Set FPS limit / Quality preset
  - [x] **Step 5: Finish**: Scan library immediately
- [x] **4.3.2** Logic
  - [x] Check `first_run` flag in config
  - [x] Force dialog on first launch before Main Window is usable

### 4.4 Advanced Resource Management [Answer #33, #63]
- [x] **4.4.1** High RAM Safeguard
  - [x] Monitor Process RAM usage
  - [x] If usage > Limit (e.g. 2GB):
    - [x] Stop Video/Scene Renderer
    - [x] Render **Static First Frame** of video [Answer #63]
    - [x] Send Notification: "Wallpaper paused due to high RAM"

### 4.5 Hyprland Integration (Static Rules) [Answer #39, #61, #64]
- [x] **4.5.1** Workspace Configuration UI
  - [x] In future WorkspacesView: dropdown per workspace
- [x] **4.5.2** Persistence
  - [x] Save to `config.json` under `hyprland.workspaces`
- [x] **4.5.3** Global Binds Generator
  - [x] Button to generate sample hyprland.conf snippet using `bwp` CLI
- [x] Switch wallpaper instantly when workspace changes (via HyprlandManager event listener)

### 4.6 Steam Workshop (Polished) [Answer #21, #22, #62]
- [ ] **4.6.1** Download Strategy
  - [ ] Default: Try **Anonymous** download via `steamcmd`
  - [ ] Failure Handler:
    - [ ] If 403/License error: Show "Login Required" Popup
    - [ ] Popup explains: "This wallpaper requires a license. Please login."
- [ ] **4.6.2** Authentication UI
  - [ ] "Log In" button in Workshop View
  - [ ] Store session securly (or leverage Steam's existing session if possible)
- [ ] **4.6.3** Dependency Check
  - [ ] Check for `steamcmd` on startup
  - [ ] If missing: Prompt user with distro-specific install command (pacman/apt/dnf) [Answer #22]

---

## 🚧 Phase 5: Library & Management Refinement

### 5.1 Tagging & Search [Answer #20, #41]
- [ ] **5.1.1** Default Tags
  - [ ] Seed DB with standard tags (Anime, Nature, Sci-Fi, Abstract)
- [ ] **5.1.2** Advanced Search
  - [ ] Implement Fuzzy Search (typo tolerant) [Answer #18]
  - [ ] Filter by Tag (Dropdown/Tags Input)

### 5.2 Favorites & Organizing [Answer #17]
- [ ] **5.2.1** Favorites UI Hints
  - [ ] Add tooltip/hint text explaining the "Star" icon usage
  - [ ] "Empty State" for Favorites view (Button -> "Go to Library")

---

## 🚧 Phase 6: Packaging & Distribution [Answer #50, #52, #60]

### 6.1 Dependency Bundling
- [ ] **6.1.1** Identify Stable Dependencies
  - [ ] `libmpv` (Static or Bundled for AppImage/Flatpak)
  - [ ] `ffmpeg` (Bundled where possible for stability) [Answer #52]

### 6.2 Package Creation
- [ ] **6.2.1** Arch Linux (AUR)
  - [ ] Create `PKGBUILD`
- [ ] **6.2.2** AppImage
  - [ ] Create `AppImageBuilder.yml`
- [ ] **6.2.3** Debian (.deb)
  - [ ] Create `control` files
- [ ] **6.2.4** Flatpak
  - [ ] Create Manifest `com.github.betterwallpaper.yml`

### 6.3 Licensing
- [ ] **6.3.1** Finalize License
  - [ ] Apply **GPL-3.0** headers to all source files [Answer #60]

---

## 📊 Summary of Remaining Work
1.  **Safety/Stability**: Error popups, RAM limits, Audio focus.
2.  **Onboarding**: Setup Wizard.
3.  **Steam**: Robust auth handling & prompts.
4.  **Integration**: Hyprland workspace rules.
5.  **Release**: Packaging for all distros.
