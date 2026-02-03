# AGENT HANDOVER PROTOCOL: THE ENCYCLOPEDIA
**Classification:** TYPE-1 (Full Context Transfer)
**Status:** COMPLETE
**Last Update:** 2026-02-03
**Target Audience:** Autonomous Agents, Maintainers, and Legacy Historians.

---

# TABLE OF CONTENTS

1.  **INTRODUCTION & PHILOSOPHY**
    *   1.1 The Mission
    *   1.2 The "Monochrome Glass" Design System
    *   1.3 Project Structure Overview
2.  **BUILD SYSTEM & DEPLOYMENT**
    *   2.1 Dependencies & Requirements
    *   2.2 CMake Configuration Deep Dive
    *   2.3 Packaging (AUR, Debian, Flatpak)
    *   2.4 The Versioning Strategy
3.  **CORE ARCHITECTURE (The "Brain")**
    *   3.1 The Singleton Pattern usage
    *   3.2 `ConfigManager`: The Source of Truth
    *   3.3 `MonitorManager`: Wayland vs X11 vs Windows
    *   3.4 `PowerManager`: Raw Filesystem vs DBus
    *   3.5 `InputManager`: GTK Event Controllers
4.  **WALLPAPER MANAGEMENT (The "Heart")**
    *   4.1 The Library Database (`WallpaperLibrary`)
    *   4.2 The Scanner Logic (`LibraryScanner`)
    *   4.3 Virtual Folders & Organization
5.  **RENDERING ENGINE (The "Eyes")**
    *   5.1 Supported Types Breakdown
    *   5.2 `WallpaperEngineRenderer`: The External Bindings
    *   5.3 `VideoRenderer`: MPV Integration
    *   5.4 `NativeWallpaperSetter`: DE Integration
6.  **USER INTERFACE (The "Face")**
    *   6.1 GTK4 & LibAdwaita Hierarchy
    *   6.2 Styling Guide (CSS & Classes)
    *   6.3 View Controller Breakdown
7.  **AUTOMATION & QA (The "Hands")**
    *   7.1 `hand_of_god.py`: The Harness
    *   7.2 CI/CD Pipelines
8.  **KNOWN ISSUES & TECHNICAL DEBT**
    *   8.1 The "Hack" List
    *   8.2 Security Considerations
    *   8.3 Performance Bottlenecks

---

# 1. INTRODUCTION & PHILOSOPHY

## 1.1 The Mission
**BetterWallpaper** is a chaotic-good, maximum-effort attempt to bring a first-class "Wallpaper Engine" experience to the Linux Desktop, specifically targeting the modern Wayland stack (Hyprland, Sway) while maintaining compatibility with GNOME and KDE.

Unlike simple wrappers (like `swww` or `mpvpaper`), this project aims to be a **Full Ecosystem Manager**. It doesn't just display an image; it manages:
*   **Discovery:** Scanning and importing Steam Workshop libraries.
*   **Lifecycle:** Pausing when you game, stopping when you're on battery.
*   **Aesthetics:** Providing a UI that feels like a native piece of glass on your desktop.

## 1.2 The "Monochrome Glass" Design System
We follow a strict, opinionated design language. If you add a feature that breaks this, you have failed.

### The Palette (No-Color Policy)
The interface is strictly grayscale. Color serves **only** two purposes:
1.  **The Content:** The wallpapers themselves provide the color.
2.  **Semantic Action:** Red for error, Green for success.

| Variable | Value | Usage |
| :--- | :--- | :--- |
| `void` | `#0A0A0A` | The deepest background layer. Not black, but "void". |
| `carbon` | `#121212` | Sidebars, panels, opaque overlay bases. |
| `glass` | `rgba(30, 30, 30, 0.6)` | Cards, floating docks. Always combined with `blur`. |
| `text-primary` | `#EDEDED` | Headers, distinct data. |
| `text-secondary` | `#A1A1AA` | Body text, descriptions. |

### The Physics
*   **Transitions:** All UI movements use `cubic-bezier(0.2, 0, 0, 1)` (Power Ease).
*   **Duration:** 200ms is the standard tick.
*   **Response:** Buttons check `mousedown`, not `click` (mouseup), for perceived responsiveness.

## 1.3 Project Structure Overview
The codebase is strictly separated into modules to prevent "Header Spaghetti".

*   **`src/core/`**: The Headless Logic.
    *   *Rules:* No GTK headers allowed here (mostly). Pure C++20.
    *   *Submodules:* `config`, `ipc`, `monitor`, `power`, `wallpaper`, `utils`.
*   **`src/gui/`**: The Visuals.
    *   *Rules:* All GTK/Adwaita code lives here. Consumes `src/core`.
    *   *Submodules:* `views`, `widgets`, `dialogs`.
*   **`src/daemon/`**: The Service.
    *   *Role:* Runs in the background if the GUI is closed (optional).
*   **`src/tray/`**: The Indicator.
    *   *Role:* System tray icon logic (separate executable in some build configs).
*   **`scripts/`**: Automation.
    *   *Role:* Python scripts for QA and maintenance.

---

# 2. BUILD SYSTEM & DEPLOYMENT

## 2.1 Dependencies & Requirements
We bleed edge. We require C++20 and modern Linux libraries.

### Core Libraries
*   **`gtk4`** & **`libadwaita-1`**: The UI toolkit.
*   **`nlohmann_json`**: Formatting configuration and IPC messages.
*   **`libGLEW`** & **`libGL`**: OpenGL context management for WebGL/Scenes.
*   **`wayland-client`** & **`wayland-protocols`**: Native display server communication.
*   **`mpv`**: The video backend engine.

### Runtime Requirements
*   **`linux-wallpaperengine`**: This is an *external binary requirement*. We do not link it; we spawn it. It must be in the `$PATH` or in the executable directory.
*   **`ffmpeg`**: Required for thumbnail generation (invoked via CLI).

## 2.2 CMake Configuration Deep Dive
The `CMakeLists.txt` is the source of truth for the build pipeline.

### Flags
*   `CMAKE_CXX_STANDARD 20`: Non-negotiable.
*   `CMAKE_POSITION_INDEPENDENT_CODE ON`: Required for `libbwp_core.a` static linking.

### Targets
1.  **`bwp_core`**: A static library containing all logic from `src/core`.
2.  **`betterwallpaper`**: The GUI executable. Links `bwp_core` + `gtk4`.
3.  **`betterwallpaper-daemon`**: The headless service. Links `bwp_core` (no GTK).
4.  **`unit_tests`**: The GoogleTest binary.

## 2.3 Packaging

### Arch Linux (AUR)
*   **File:** `packaging/aur/PKGBUILD`
*   **Package Name:** `betterwallpaper-git`
*   **Maintainer Strategy:** Checks out the latest git commit.
*   **Quirk:** Manual installation of `linux-wallpaperengine` is listed as an `optdepend`, but practically it is a hard requirement for Scene wallpapers.

### Debian / Ubuntu
*   **File:** `packaging/debian/control`
*   **Status:** Experimental.
*   **Issue:** Older Ubuntu LTS releases (20.04/22.04) often lack the required `libadwaita` version (1.2+ recommended).

---

# 3. CORE ARCHITECTURE (The "Brain")

This module (`src/core`) contains the business logic. It is designed to be potentially portable to Windows/macOS in the future by isolating platform-specific code behind interfaces.

## 3.1 The Singleton Pattern
We misuse Singletons. I admit it.
Almost every Manager class is a `Meyers Singleton` (Static local variable).
*   **Pros:** Easy access from anywhere (`Manager::getInstance()`).
*   **Cons:** Destruction order is undefined-ish.
*   **Fix:** We manually orchestrate initialization in `main.cpp`.

## 3.2 `ConfigManager`: The Source of Truth
Located in `src/core/config/ConfigManager.hpp`.

### The recursive_mutex Decision
We faced massive deadlocks in `Phase 1`. The UI thread would ask for a config value, which would trigger a callback, which would try to read another config value.
*   **Solution:** `std::recursive_mutex m_mutex`.
*   **Implication:** A single thread can lock the config multiple times safely.

### JSON Storage
*   **Path:** `~/.config/betterwallpaper/config.json`.
*   **Format:**
    ```json
    {
      "ui": { "sidebar_width": 240 },
      "performance": { "fps_limit": 60 }
    }
    ```
*   **Atomic Saves:** We write to a temp file and rename it to avoid corruption on crash.

## 3.3 `MonitorManager`: Wayland vs X11 vs Windows
Located in `src/core/monitor/MonitorManager.cpp`.

### The Wayland Implementation (`WaylandMonitor`)
We do not use GDK to get monitor info because GDK often lies about monitor names (e.g., calling everything "Wayland-0").
*   **Mechanism:** We bind directly to the `wl_output` global registry.
*   **Data:** We extract the *EDID name* output name (e.g., `DP-1`, `HDMI-A-1`).
*   **Why?** Because `hyprctl` and `wlr-randr` use these names. We need to match them exactly to set wallpapers.

## 3.4 `PowerManager`: Raw Filesystem vs DBus
Located in `src/core/power/PowerManager.cpp`.

### The "Sysfs" Hack
Instead of depending on `upower` (which adds a DBus dependency and complexity), we read `/sys/class/power_supply/BAT*/status` directly.
*   **Frequency:** Checks every 10 seconds.
*   **Logic:** If *any* battery reports "Discharging", we trigger the `onBattery` callback.
*   **Effect:** Sets `WallpaperEngineRenderer` to PAUSE state to save GPU cycles.

## 3.5 `InputManager`: GTK Event Controllers
Located in `src/core/input/InputManager.cpp`.

### The Event Trap
We attach a `GtkEventControllerKey` to the top-level window.
*   **Key Matching:** We do not hardcode keybinds in the C++ logic (mostly). We ask `KeybindManager` "What does `Ctrl+Shift+P` do?".
*   **Action Dispatch:** If `KeybindManager` returns `"toggle_pause"`, `InputManager` calls `SlideshowManager::getInstance().toggle()`.

---

# 4. WALLPAPER MANAGEMENT (The "Heart")

This module handles the discovery, storage, and organization of content.

## 4.1 The Library Database (`WallpaperLibrary`)
Located in `src/core/wallpaper/WallpaperLibrary.cpp`.

### The Schema (`WallpaperInfo`)
Every wallpaper is a struct:
*   `std::string id`: Unique ID (Hash of path, or Workshop ID).
*   `std::string path`: Absolute filesystem path.
*   `WallpaperType type`: Enum (`Scene`, `Video`, `Web`, `Image`).
*   `Settings`: Per-wallpaper overrides (FPS, alignment, volume).

### Threading Model
*   **Read-Heavy:** methods like `getAllWallpapers()` are called constantly by the UI.
*   **Locking:** Uses a coarse-grained `std::mutex`. Optimization opportunity: `std::shared_mutex` (Reader-Writer lock) in Phase 5.

## 4.2 The Scanner Logic (`LibraryScanner`)
Located in `src/core/wallpaper/LibraryScanner.cpp`.

### The Steam Integration
We do not use the Steamworks API (requires an AppID, strict usage). Instead, we act as a "parasite".
1.  **Locate Steam:** We check default paths (`~/.steam/steam`, `~/.local/share/Steam`).
2.  **Locate Workshop:** We look for `steamapps/workshop/content/431960` (Wallpaper Engine's App ID).
3.  **Parse:** We read the `project.json` inside every numeric folder to extract Title, Preview Image, and File Type.

### The "Auto-Tag" Heuristic
*   If `project.json` contains tags, we import them.
*   If not, we try to guess based on folder names (unreliable).
*   **Optimization:** We verify if the file *actually exists* before adding it. Ghost files are common in Steam Workshop downloads.

## 4.3 Virtual Folders & Organization
*   **`FolderManager`**: Implements a "Playlist" system.
*   **Logic:** A folder is just a JSON list of IDs.
*   **Complexity:** Dragging and dropping into a folder updates the `SideBar` and the `FolderView` simultaneously via a signal bus.

---

# 5. RENDERING ENGINE (The "Eyes")

## 5.1 Supported Types Breakdown
We support 4 native types, mapped in `WallpaperManager::createRenderer`:

1.  **Static (`.png`, `.jpg`):** Handled by `StaticRenderer`.
2.  **Video (`.mp4`, `.webm`, `.mkv`):** Handled by `VideoRenderer` (mpv).
3.  **Scene (`.pkg`, `.json`):** Handled by `WallpaperEngineRenderer`.
4.  **Web (`.html`):** Handled by `WallpaperEngineRenderer` (Web Mode).

## 5.2 `WallpaperEngineRenderer`: The External Bindings
This is the most complex renderer.

### The "Fork-Exec" Dance
We do not link against a library. We fork `linux-wallpaperengine`.
```cpp
// Pseudocode of launchProcess()
std::vector<std::string> args;
args.push_back("--screen-root");
args.push_back(m_monitorName); // e.g. DP-1
args.push_back("--fps");
args.push_back(std::to_string(m_fpsLimit));
args.push_back(m_pkgPath);     // /path/to/scene.pkg

execvp("linux-wallpaperengine", args);
```

### Crash Recovery
The `monitorProcess` thread waits on the PID.
*   **Exit Code 0:** Clean exit. Do nothing.
*   **Exit Code 139 (Segfault):** Common with anime shaders on NVIDIA.
    *   *Action:* Sleep 1s, Restart.
    *   *Limit:* After 3 fast crashes (<10s), we stop trying and mark the wallpaper as "Broken".

## 5.3 `VideoRenderer`: MPV Integration
We use `mpv` because it is the most robust player on Linux.
*   **Looping:** Seamless looping is tricky. We use the `--loop-file=inf` flag.
*   **Muting:** `mpv` handles volume internally. We just send IPC commands `set volume 0`.
*   **HW Acceleration:** We implicitly trust `mpv`'s config. If the user has `vo=gpu` in their system config, we inherit it.

## 5.4 `NativeWallpaperSetter`: DE Integration
When we can't render live content (e.g. low memory mode), we fall back to "setting the wallpaper" the old fashioned way.

### The Strategy Pattern
`NativeWallpaperSetter` detects the environment:
1.  **Hyprland detected?** -> Use `hyprctl hyprpaper`.
2.  **Sway detected?** -> Use `swaybg`.
3.  **GNOME detected?** -> Use `gsettings`.
4.  **KDE detected?** -> Use DBus `org.kde.Plasma...`.

---

# 6. USER INTERFACE (The "Face")

Because "Command Line is cool, but Glass is cooler."

## 6.1 GTK4 & LibAdwaita Hierarchy
We are strictly a **GTK4 App**. No Legacy GTK3.

*   **`AdwApplication`**: The base.
*   **`AdwWindow`** (`MainWindow`): The frame. Contains the `AdwToolbarView` pattern.
*   **`AdwViewStack`**: The swappable pages (Library, Workshop, Settings).
*   **`AdwToastOverlay`**: Used for non-intrusive notifications ("Wallpaper Applied").

## 6.2 Styling Guide (CSS & Classes)
Located in `src/gui/style.css` (compiled into binary via GResource usually, or loaded relative).

### Key Classes
*   `.glass-panel`: Adds the `backdrop-filter: blur(20px)` and semi-transparent background.
*   `.void-background`: Sets the pure `#0A0A0A` color.
*   `.wallpaper-card`: Handles the aspect ratio and hover scaling.
    *   *Trick:* We use `transform: scale(1.05)` on hover, but we must set `overflow: hidden` on the parent to avoid breaking the grid layout.

## 6.3 View Controller Breakdown

### `LibraryView`
*   **Complexities:**
    *   **Lazy Loading:** `WallpaperGrid` creates widgets for *all* items instantly? No. That would crash.
    *   **Recycling:** We rely on GTK's `FlowBox` smart rendering, but for thumbnails, we use a `ThumbnailCache` thread that loads images asynchronously and signals the main thread when ready. If we didn't, the UI would freeze while scrolling.

### `WorkshopView`
*   **Distinction:** It looks like `LibraryView`, but the "Click" action is different.
    *   *Library Click:* Apply Wallpaper.
    *   *Workshop Click:* Import/Subscribe (Copy to internal library).

### `MonitorConfigPanel`
*   **Role:** The inspector panel on the right side of the "Monitors" tab.
*   **Data Binding:** Changes the `m_scalingModes` map in `WallpaperManager` immediately.

---

# 7. AUTOMATION & QA (The "Hands")

We treat testing as a first-class citizen. Features are useless if they Segfault.

## 7.1 `hand_of_god.py`: The Harness
Located in `scripts/hand_of_god.py`.

### Architecture
It is a Python script that uses `subprocess` to control the C++ app and `pyautogui` / `grim` to see it.
*   **Wayland Challenge:** `pyautogui` cannot move the mouse on Hyprland (security).
*   **Solution:** We use `hyprctl dispatch movecursor` commands to emulate mouse movement for the tests, OR we run in "Blind Mode" where we just verify process life.

### Modes
1.  **`--mode=chaos`**: Randomly sends SIGINT, Clicks, and Window Resizes.
2.  **`--mode=scenario`**: Runs defined "User Stories" (e.g., "Open App -> Search 'Anime' -> Apply").
3.  **`--mode=screenshot`**: Launches, waits 2s, snaps `.agent/artifacts/state_startup.png`, closes.

## 7.2 CI/CD Pipelines
*Currently Local Only.*
To run the full suite:
```bash
./scripts/run_all_tests.sh
```
This compiles the `unit_tests` target (GoogleTest) and runs the python harness.

---

# 8. KNOWN ISSUES & TECHNICAL DEBT

## 8.1 The "Hack" List
Things we did to make it work *now*, that will hurt *later*.

1.  **Hardcoded Steam Paths:**
    *   *Location:* `LibraryScanner.cpp`.
    *   *Issue:* It checks `/home/onxy/`. It works for the dev machine. It fails for everyone else.
    *   *Fix:* Use `std::getenv("HOME")`.

2.  **LD_LIBRARY_PATH Injection:**
    *   *Location:* `WallpaperEngineRenderer.cpp`.
    *   *Issue:* We force the child process to look in the current build dir for `libGLEW`.
    *   *Fix:* Properly install libraries to `/usr/lib` or bundle them with `RPATH`.

3.  **Hyprland IPC Shelling:**
    *   *Location:* `HyprlandWallpaperSetter.cpp`.
    *   *Issue:* `system("hyprctl ...")`. Slow and ugly.
    *   *Fix:* Use the Unix Domain Socket `/tmp/hypr/...` directly.

## 8.2 Security Considerations
*   **Web Wallpapers:** We run `.html` files in a browser context (via `linux-wallpaperengine`'s internal CEF/Web view).
    *   *Risk:* Malicious wallpapers could run JS.
    *   *Mitigation:* currently none. We trust the Steam Workshop curation.

## 8.3 Performance Bottlenecks
*   **Large Libraries:** Loading 5000+ wallpapers makes the `WallpaperLibrary::load()` take ~1.5 seconds.
    *   *Impact:* App launch delay.
    *   *Fix:* SQLITE database or async loading with pagination.
*   **Memory Leaks:** `VideoRenderer` (mpv) seems stable, but `linux-wallpaperengine` can leak VRAM over days. We rely on the `ResourceMonitor` to kill it if it grows too big.

---

# 9. EPILOGUE: A NOTE TO THE MAINTAINER

You have inherited a beast. It is powerful, fast, and beautiful, but it holds itself together with mutexes and sheer will.
*   **Respect the Locking Order:** `Config` -> `WallpaperLibrary` -> `Renderer`. Never the reverse.
*   **Respect the Design:** Do not add color to the UI.
*   **Respect the Logs:** If `application_debug.log` says "ScopeTracer failed", you have a stack overflow.

**Project Status: ALPHA.**
**Handover Status: COMPLETE.**

*End of Transmission.*



