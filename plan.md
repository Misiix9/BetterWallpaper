# BetterWallpaper - Complete Implementation Plan

> **A feature-rich wallpaper manager for Linux with Wallpaper Engine support, optimized for Hyprland.**

---

## Table of Contents

1. [Project Overview](#1-project-overview)
2. [Phase 1: Project Setup](#2-phase-1-project-setup)
3. [Phase 2: Core Library Foundation](#3-phase-2-core-library-foundation)
4. [Phase 3: Wallpaper Rendering](#4-phase-3-wallpaper-rendering)
5. [Phase 4: GUI Application](#5-phase-4-gui-application)
6. [Phase 5: Multi-Monitor Support](#6-phase-5-multi-monitor-support)
7. [Phase 6: Library Management](#7-phase-6-library-management)
8. [Phase 7: Transition System](#8-phase-7-transition-system)
9. [Phase 8: Hyprland Integration](#9-phase-8-hyprland-integration)
10. [Phase 9: Steam Workshop](#10-phase-9-steam-workshop)
11. [Phase 10: Advanced Features](#11-phase-10-advanced-features)
12. [Phase 11: System Integration](#12-phase-11-system-integration)
13. [Phase 12: Packaging](#13-phase-12-packaging)
14. [Technical Specifications](#14-technical-specifications)
15. [Configuration Reference](#15-configuration-reference)
16. [CLI Reference](#16-cli-reference)

---

## 1. Project Overview

### 1.1 Vision Statement
BetterWallpaper is a comprehensive wallpaper management application that brings Wallpaper Engine functionality to Linux, with native Hyprland support and cross-distribution compatibility.

### 1.2 Target Users
- Linux users on Hyprland/Wayland
- Former Windows users wanting Wallpaper Engine
- Power users wanting advanced wallpaper management

### 1.3 Technology Stack

| Component | Technology | Version |
|-----------|------------|---------|
| Language | C++ | C++20 |
| GUI Framework | GTK4 + libadwaita | 4.10+ / 1.4+ |
| Build System | CMake | 3.20+ |
| Video Playback | libmpv | Latest |
| IPC | D-Bus | - |
| JSON | nlohmann/json | 3.11+ |
| HTTP | libcurl | 7.0+ |
| Wayland | wlr-layer-shell | v1 |

### 1.4 Supported Wallpaper Types

| Type | Formats | Rendering Engine |
|------|---------|------------------|
| Static Images | PNG, JPG, JPEG, WEBP, BMP, TIFF | GdkPixbuf |
| Animated Images | GIF, APNG | GdkPixbuf |
| Videos | MP4, WEBM, MKV, AVI, MOV | libmpv |
| Wallpaper Engine Scene | .pkg | linux-wallpaperengine |
| Wallpaper Engine Video | .pkg | linux-wallpaperengine |
| Wallpaper Engine Web | .pkg | **BLOCKED** (error shown) |

---

## 2. Phase 1: Project Setup

### 2.1 Directory Structure Creation
- [x] **2.1.1** Create root project directory structure
  - Create `src/` directory for all source code
  - Create `src/core/` for the shared library
  - Create `src/daemon/` for background process
  - Create `src/gui/` for GTK4 application
  - Create `src/cli/` for command-line tool
  - Create `src/tray/` for system tray component

- [x] **2.1.2** Create build configuration directories
  - Create `cmake/` for CMake modules
  - Create `protocols/` for Wayland protocols
  - Create `data/` for assets (icons, desktop files)
  - Create `data/icons/` for app icons
  - Create `data/ui/` for CSS styles

- [x] **2.1.3** Create packaging directories
  - Create `packaging/arch/` for PKGBUILD
  - Create `packaging/flatpak/` for Flatpak manifest
  - Create `packaging/appimage/` for AppImage config
  - Create `packaging/debian/` for .deb files
  - Create `packaging/fedora/` for .rpm spec

- [x] **2.1.4** Create documentation directories
  - Create `docs/` for documentation
  - Create `docs/man/` for man pages
  - Create `tests/` for unit/integration tests

### 2.2 Build System Configuration
- [x] **2.2.1** Create root `CMakeLists.txt`
  - Set minimum CMake version to 3.20
  - Set project name "BetterWallpaper"
  - Set C++ standard to C++20
  - Enable position independent code
  - Configure build types (Debug, Release)

- [x] **2.2.2** Configure dependency detection
  - Create `cmake/FindWaylandProtocols.cmake`
  - Add GTK4 and libadwaita detection
  - Add libmpv detection
  - Add nlohmann_json detection
  - Add libcurl detection
  - Add wayland-client detection

- [x] **2.2.3** Configure subdirectory builds
  - Add `src/core/CMakeLists.txt` for libbwp.so
  - Add `src/daemon/CMakeLists.txt`
  - Add `src/gui/CMakeLists.txt`
  - Add `src/cli/CMakeLists.txt`
  - Add `src/tray/CMakeLists.txt`

### 2.3 Project Configuration Files
- [x] **2.3.1** Create `.gitignore`
  - Ignore build directories
  - Ignore IDE files
  - Ignore compiled objects
  - Ignore temporary files

- [x] **2.3.2** Create `README.md`
  - Project description
  - Features list
  - Installation instructions
  - Build instructions
  - Usage examples

- [x] **2.3.3** Create `LICENSE` file
  - Choose appropriate license (GPL-3.0 recommended)

- [x] **2.3.4** Create `CHANGELOG.md`
  - Initial unreleased section
  - Version format documentation

---

## 3. Phase 2: Core Library Foundation

### 3.1 Utility Classes
- [x] **3.1.1** Create Logger system (`src/core/utils/Logger.hpp/cpp`)
  - Log levels: DEBUG, INFO, WARNING, ERROR, FATAL
  - File output with rotation
  - Console output with colors
  - Timestamp formatting
  - Thread-safe logging

- [x] **3.1.2** Create FileUtils (`src/core/utils/FileUtils.hpp/cpp`)
  - File existence checking
  - Directory creation (recursive)
  - File reading/writing
  - Path expansion (~, $HOME)
  - MIME type detection
  - File hash calculation (SHA256)

- [x] **3.1.3** Create StringUtils (`src/core/utils/StringUtils.hpp/cpp`)
  - String trimming
  - Case conversion
  - String splitting/joining
  - URL encoding/decoding
  - Path operations

### 3.2 Configuration Management
- [x] **3.2.1** Create SettingsSchema (`src/core/config/SettingsSchema.hpp`)
  - Define all configuration keys as constants
  - Define default values for each setting
  - Define value types and constraints
  - Define setting categories

- [x] **3.2.2** Create ConfigManager (`src/core/config/ConfigManager.hpp/cpp`)
  - **Load configuration** from `~/.config/betterwallpaper/config.json`
  - **Save configuration** with atomic writes
  - **Get/Set methods** for all setting types
  - **Validation** of values against schema
  - **Migration** from older config versions
  - **Default creation** if config doesn't exist
  - **Watch for changes** and reload

- [x] **3.2.3** Create ProfileManager (`src/core/config/ProfileManager.hpp/cpp`)
  - **List profiles** from `~/.config/betterwallpaper/profiles/`
  - **Load profile** by name
  - **Save profile** to disk
  - **Create profile** with default settings
  - **Delete profile** with confirmation
  - **Duplicate profile** with new name
  - **Export profile** to archive
  - **Import profile** from archive
  - **Set active profile**

### 3.3 Monitor Management
- [x] **3.3.1** Create MonitorInfo struct (`src/core/monitor/MonitorInfo.hpp`)
  ```cpp
  struct MonitorInfo {
      std::string name;        // e.g., "DP-1"
      std::string description; // e.g., "LG Ultragear"
      int32_t x, y;           // Position in layout
      int32_t width, height;   // Resolution
      int32_t refresh_rate;    // In mHz (144000 = 144Hz)
      double scale;            // 1.0, 1.25, 1.5, 2.0
      bool enabled;
      bool primary;
  };
  ```

- [x] **3.3.2** Create WaylandMonitor (`src/core/monitor/WaylandMonitor.hpp/cpp`)
  - Connect to Wayland display
  - Bind to wl_output interface
  - Parse output geometry events
  - Parse output mode events
  - Handle output done events
  - Maintain list of monitors

- [x] **3.3.3** Create MonitorManager (`src/core/monitor/MonitorManager.hpp/cpp`)
  - **Initialize** Wayland connection
  - **Get all monitors** as vector
  - **Get monitor by name**
  - **Get primary monitor**
  - **Watch for changes** (connect/disconnect)
  - **Emit signals** on monitor change
  - **Calculate layout** for visual display

### 3.4 D-Bus Service Interface
- [x] **3.4.1** Create D-Bus interface definition
  - Create `data/com.github.betterwallpaper.xml`
  - Define methods: SetWallpaper, GetStatus, Pause, Resume
  - Define signals: WallpaperChanged, MonitorChanged
  - Define properties: CurrentWallpaper, IsPaused

- [x] **3.4.2** Create DBusService (`src/core/ipc/DBusService.hpp/cpp`)
  - Register D-Bus name
  - Export methods to bus
  - Handle method calls
  - Emit signals
  - Handle property queries

- [x] **3.4.3** Create DBusClient (`src/core/ipc/DBusClient.hpp/cpp`)
  - Connect to daemon service
  - Call remote methods
  - Subscribe to signals
  - Get properties
  - Handle timeouts

---

## 4. Phase 3: Wallpaper Rendering

### 4.1 Wallpaper Data Structures
- [x] **4.1.1** Create WallpaperType enum
  ```cpp
  enum class WallpaperType {
      Unknown,
      StaticImage,    // PNG, JPG, etc.
      AnimatedImage,  // GIF, APNG
      Video,          // MP4, WEBM, etc.
      WEScene,        // Wallpaper Engine Scene
      WEVideo,        // Wallpaper Engine Video
      WEWeb           // Wallpaper Engine Web (blocked)
  };
  ```

- [x] **4.1.2** Create ScalingMode enum
  ```cpp
  enum class ScalingMode {
      Stretch,  // Fill screen, distort if needed
      Fill,     // Crop to fill, maintain aspect ratio
      Fit,      // Show entire image, letterbox if needed
      Center,   // No scaling, center on screen
      Tile,     // Repeat pattern
      Zoom      // User-defined zoom level
  };
  ```

- [x] **4.1.3** Create WallpaperInfo struct
  ```cpp
  struct WallpaperInfo {
      std::string id;           // Unique identifier
      std::string path;         // Full path to file
      WallpaperType type;
      std::string format;       // File extension
      int32_t width, height;    // Dimensions
      int64_t size_bytes;
      std::string hash;         // SHA256 hash
      std::string source;       // "local" or "workshop"
      std::optional<uint64_t> workshop_id;
      std::vector<std::string> tags;
      int rating;               // 1-5 stars
      bool favorite;
      int play_count;
      std::chrono::system_clock::time_point last_used;
      std::chrono::system_clock::time_point added;
      std::vector<std::string> colors;  // Extracted colors
  };
  ```

### 4.2 Renderer Interface
- [x] **4.2.1** Create WallpaperRenderer base class
  ```cpp
  class WallpaperRenderer {
  public:
      virtual ~WallpaperRenderer() = default;
      virtual bool load(const std::string& path) = 0;
      virtual void render(cairo_t* cr, int width, int height) = 0;
      virtual void setScalingMode(ScalingMode mode) = 0;
      virtual void play() = 0;
      virtual void pause() = 0;
      virtual void setVolume(float volume) = 0;  // 0.0 - 1.0
      virtual void setPlaybackSpeed(float speed) = 0;
      virtual bool isPlaying() const = 0;
      virtual bool hasAudio() const = 0;
  };
  ```

### 4.3 Static Image Renderer
- [x] **4.3.1** Create StaticRenderer (`src/core/wallpaper/renderers/StaticRenderer.cpp`)
  - **Load image** using GdkPixbuf
  - **Support formats**: PNG, JPG, JPEG, WEBP, BMP, TIFF
  - **Cache loaded image** in memory
  - **Apply scaling mode** during render
  - **Calculate crop/letterbox** based on mode
  - **Render to Cairo context**

### 4.4 Video Renderer
- [x] **4.4.1** Create VideoRenderer (`src/core/wallpaper/renderers/VideoRenderer.cpp`)
  - **Initialize libmpv** handle
  - **Configure mpv options**:
    - `vo=libmpv` for rendering
    - `hwdec=auto` for hardware acceleration
    - `loop=yes` for looping
    - `audio=yes/no` based on setting
  - **Load video file**
  - **Render to texture/surface**
  - **Control playback**: play, pause, seek
  - **Control volume**: 0-100%
  - **Control speed**: 0.25x - 4x
  - **Handle video end** (loop or stop)

### 4.5 Wallpaper Engine Renderer
- [x] **4.5.1** Create WallpaperEngineRenderer (`src/core/wallpaper/renderers/WallpaperEngineRenderer.cpp`)
  - **Detect linux-wallpaperengine** installation
  - **Parse .pkg files** to detect type (Scene/Video/Web)
  - **Block Web wallpapers** with error:
    ```
    "Web-based Wallpaper Engine wallpapers are not supported.
    This wallpaper uses HTML/JavaScript which requires a browser engine.
    Please choose a Scene or Video type wallpaper instead."
    ```
  - **Launch linux-wallpaperengine** subprocess
  - **Pass configuration** (monitor, scaling, etc.)
  - **Monitor process** health
  - **Restart on crash**

### 4.6 Layer Shell Window
- [x] **4.6.1** Create WallpaperWindow (`src/core/wallpaper/WallpaperWindow.cpp`)
  - **Create GDK Wayland window**
  - **Set layer shell properties**:
    - Layer: Background
    - Anchor: All edges
    - Exclusive zone: -1 (fill entire screen)
  - **Assign to specific output** (monitor)
  - **Handle resize events**
  - **Render wallpaper content**

### 4.7 Wallpaper Manager
- [x] **4.7.1** Create WallpaperManager (`src/core/wallpaper/WallpaperManager.cpp`)
  - **Initialize** for each monitor
  - **Set wallpaper** by path or ID
  - **Get current wallpaper** per monitor
  - **Handle monitor changes**
  - **Apply scaling mode**
  - **Control playback** globally and per-monitor
  - **Emit signals** on wallpaper change

---

## 5. Phase 4: GUI Application

### 5.1 Application Setup
- [x] **5.1.1** Create Application class (`src/gui/Application.cpp`)
  - Inherit from Gtk::Application
  - Register application ID: `com.github.betterwallpaper`
  - Handle command-line arguments
  - Create main window on activate
  - Handle single instance

- [x] **5.1.2** Create main entry point (`src/gui/main.cpp`)
  - Initialize GTK4
  - Load CSS stylesheet
  - Create and run application
  - Handle exit codes

### 5.2 Main Window
- [x] **5.2.1** Create MainWindow class (`src/gui/MainWindow.cpp`)
  - **Window setup**:
    - Default size: 1400 x 900
    - Minimum size: 800 x 600
    - Title: "BetterWallpaper"
    - Enable transparency for blur
  - **Create layout**:
    - Horizontal paned (sidebar + content)
    - Sidebar: 250px default width
    - Content area: Stack for pages
  - **Connect to D-Bus** daemon

- [x] **5.2.2** Create header bar
  - Application icon
  - Title
  - Menu button (hamburger)
  - Window controls (minimize, maximize, close)

### 5.3 Sidebar Navigation
- [x] **5.3.1** Create Sidebar widget (`src/gui/widgets/Sidebar.cpp`)
  - **Navigation items**:
    - 📁 Library (default)
    - ⭐ Favorites
    - 🕐 Recent
    - 🏪 Workshop
    - ─── (separator)
    - 📂 Folders (expandable)
    - ─── (separator)
    - 🖥️ Monitors
    - 📊 Profiles
    - ⏰ Schedule
    - ⚙️ Settings
  - **Hover animations**: Background slide
  - **Selection indicator**: Left border accent
  - **Emit navigation signal** on click

### 5.4 Wallpaper Grid View
- [x] **5.4.1** Create WallpaperCard widget (`src/gui/widgets/WallpaperCard.cpp`)
  - **Display thumbnail** (256x144 default)
  - **Overlay badge** for type (video, GIF, WE)
  - **Hover effect**: Scale 1.02x, shadow
  - **Click handler**: Select wallpaper
  - **Double-click**: Apply immediately
  - **Right-click**: Context menu
  - **Context menu items**:
    - Apply to all monitors
    - Apply to monitor → (submenu)
    - Add to favorites
    - Add tags
    - View details
    - Open file location
    - Remove from library

- [ ] **5.4.2** Create WallpaperGrid widget (`src/gui/widgets/WallpaperGrid.cpp`)
  - **FlowBox-based layout**
  - **Responsive columns**: Adjust based on width
  - **Lazy loading**: Load visible items only
  - **Virtual scrolling**: Memory efficient
  - **Selection handling**: Single select
  - **Keyboard navigation**: Arrow keys

- [ ] **5.4.3** Create SearchBar widget (`src/gui/widgets/SearchBar.cpp`)
  - **Search input** with icon
  - **View toggle**: Grid / List
  - **Sort dropdown**: Name, Date, Size, Rating
  - **Filter button**: Opens filter popover
  - **Filter options**:
    - Type: All, Static, Video, GIF, Wallpaper Engine
    - Resolution: All, 1080p, 1440p, 4K, Ultrawide
    - Tags: (selectable list)
    - Rating: 1-5 stars minimum

### 5.5 Preview Panel
- [ ] **5.5.1** Create PreviewPanel widget (`src/gui/widgets/PreviewPanel.cpp`)
  - **Large preview area** (fills available space)
  - **Animated preview** for videos/GIFs
  - **Wallpaper info section**:
    - Name (editable)
    - Resolution
    - Type
    - Size
    - Tags (editable)
  - **Action buttons**:
    - Apply to All Monitors
    - Apply to Monitor (dropdown)
    - Add to Favorites (heart icon)
    - Settings (gear icon)
  - **Crossfade animation** on selection change

### 5.6 View Pages
- [ ] **5.6.1** Create LibraryView (`src/gui/views/LibraryView.cpp`)
  - Combine SearchBar + WallpaperGrid + PreviewPanel
  - Load all wallpapers from library
  - Handle search/filter

- [ ] **5.6.2** Create FavoritesView (`src/gui/views/FavoritesView.cpp`)
  - Same layout as LibraryView
  - Filter to favorites only

- [ ] **5.6.3** Create RecentView (`src/gui/views/RecentView.cpp`)
  - Same layout as LibraryView
  - Sort by last_used descending
  - Limit to last 50 wallpapers

### 5.7 Settings View
- [ ] **5.7.1** Create SettingsView (`src/gui/views/SettingsView.cpp`)
  - **Sidebar categories**:
    - General
    - Appearance
    - Performance
    - Notifications
    - Transitions
    - Shortcuts
    - Theming
    - Backup
    - About

- [ ] **5.7.2** Create GeneralSettings panel
  - **Start on boot** toggle
  - **Autostart method** dropdown (Systemd, XDG, Hyprland)
  - **Start minimized to tray** toggle
  - **Close button behavior** dropdown (Minimize/Quit)
  - **Default audio** toggle (Enabled/Muted)
  - **Default loop** toggle
  - **Duplicate handling** dropdown
  - **Library paths** list with Add/Remove buttons

- [ ] **5.7.3** Create AppearanceSettings panel
  - **Theme** follows system (info text)
  - **Enable blur** toggle (for Hyprland)
  - **Grid card size** slider
  - **Show type badges** toggle
  - **Animation speed** dropdown (Fast/Normal/Slow)

- [ ] **5.7.4** Create PerformanceSettings panel
  - **FPS limit** dropdown (15/30/60/120/Unlimited)
  - **Pause on battery** toggle
  - **Pause on fullscreen** toggle
  - **Fullscreen exceptions** app list
  - **Max memory (MB)** number input
  - **Max VRAM (MB)** number input
  - **GPU acceleration** toggle

- [ ] **5.7.5** Create NotificationSettings panel
  - **Enable notifications** master toggle
  - **System notifications** toggle
  - **In-app toasts** toggle
  - **On wallpaper change** toggle
  - **On schedule change** toggle
  - **On error** toggle

- [ ] **5.7.6** Create TransitionSettings panel
  - **Enable transitions** toggle
  - **Default effect** dropdown with preview
  - **Duration** slider (100ms - 5000ms)
  - **Easing** dropdown
  - **Preview button** to test current settings

- [ ] **5.7.7** Create ShortcutsSettings panel
  - List of actions with key bindings
  - Click to change binding
  - Reset to defaults button
  - **Shortcuts**:
    - Next wallpaper
    - Previous wallpaper
    - Pause/Resume
    - Open window

- [ ] **5.7.8** Create ThemingSettings panel
  - **Enable color extraction** toggle
  - **Auto-apply on change** toggle
  - **Theming tool** dropdown (Auto/pywal/matugen/wpgtk/Custom)
  - **Palette size** dropdown (8/16/32)
  - **Custom script path** input (if Custom selected)
  - **Manual apply** button
  - **Color preview** showing current palette

- [ ] **5.7.9** Create BackupSettings panel
  - **Export configuration** button → file picker
  - **Import configuration** button → file picker
  - **Include wallpaper files** checkbox
  - **Last backup** info text
  - **Restore defaults** button (with confirmation)

- [ ] **5.7.10** Create AboutPanel
  - **App icon** (large)
  - **App name**: BetterWallpaper
  - **Version** number
  - **Description** text
  - **Links**: GitHub, Documentation, Report bug
  - **License**: GPL-3.0
  - **Credits**: Dependencies used
  - **Check for updates** button

---

## 6. Phase 5: Multi-Monitor Support

### 6.1 Monitor Layout Widget
- [ ] **6.1.1** Create MonitorLayout widget (`src/gui/widgets/MonitorLayout.cpp`)
  - **Visual representation** of monitor arrangement
  - **Proportional sizing** based on resolutions
  - **Position matching** to actual physical layout
  - **Monitor labels** inside each rectangle:
    - Monitor name (e.g., "DP-1")
    - Resolution (e.g., "2560×1440")
    - Refresh rate (e.g., "144Hz")
  - **Click to select** monitor
  - **Selected indicator** (border highlight)
  - **Thumbnail preview** of current wallpaper

- [ ] **6.1.2** Create MonitorCard widget (`src/gui/widgets/MonitorCard.cpp`)
  - Individual monitor in layout
  - Hover tooltip with full specs
  - Current wallpaper thumbnail
  - Quick set button

### 6.2 Monitor Configuration Panel
- [ ] **6.2.1** Create MonitorConfigPanel (`src/gui/widgets/MonitorConfigPanel.cpp`)
  - **Selected monitor info**
  - **Current wallpaper** with change button
  - **Scaling mode** dropdown:
    - Stretch, Fill, Fit, Center, Tile, Zoom
  - **Playback speed** slider (0.25x - 4x)
  - **Volume** slider (0-100%) with mute toggle
  - **Workspace wallpapers** section:
    - Grid of workspace numbers (1-10)
    - Click to assign wallpaper
    - Thumbnail preview for each
  - **Special workspace** toggle and config

### 6.3 Multi-Monitor Modes
- [ ] **6.3.1** Implement Independent mode
  - Each monitor has separate wallpaper
  - Separate settings per monitor
  - Save per-monitor in profile

- [ ] **6.3.2** Implement Clone mode
  - Single wallpaper for all monitors
  - Single set of settings
  - Apply to all monitors

- [ ] **6.3.3** Implement Span mode
  - Single wallpaper stretched across all
  - Calculate total resolution
  - Position wallpaper sections per monitor

### 6.4 MonitorsView Page
- [ ] **6.4.1** Create MonitorsView (`src/gui/views/MonitorsView.cpp`)
  - **Header**: "Monitor Configuration"
  - **Mode selector**: Radio buttons for Independent/Clone/Span
  - **Monitor layout widget**: Visual representation
  - **Config panel**: Selected monitor settings
  - **Apply button**: Confirm changes

---

## 7. Phase 6: Library Management

### 7.1 Library Database
- [ ] **7.1.1** Create WallpaperLibrary class (`src/core/wallpaper/WallpaperLibrary.cpp`)
  - **Database file**: `~/.local/share/betterwallpaper/library.json`
  - **Load library** on startup
  - **Save library** on changes (debounced)
  - **CRUD operations** for wallpapers
  - **Search** by name, tags, path
  - **Filter** by type, resolution, rating
  - **Sort** by name, date, size, rating, usage

### 7.2 Library Scanning
- [ ] **7.2.1** Create LibraryScanner class (`src/core/wallpaper/LibraryScanner.cpp`)
  - **Scan directories** recursively
  - **Detect file type** by extension and magic bytes
  - **Extract metadata**:
    - Dimensions (for images/videos)
    - File size
    - Hash (SHA256)
  - **Generate thumbnails** (256x144)
  - **Store in cache**: `~/.cache/betterwallpaper/thumbnails/`
  - **Progress reporting** via signals
  - **Background threading** for non-blocking scan

- [ ] **7.2.2** Handle duplicates
  - **Detect by hash** comparison
  - **User preference**:
    - Keep first found
    - Keep newest
    - Keep highest resolution
    - Ask each time
  - **Show dialog** when duplicates found
  - **Store preference** in config

- [ ] **7.2.3** Handle missing files
  - **Detect on access** or periodic scan
  - **Remove from library** automatically
  - **Optional notification** to user

### 7.3 Tagging System
- [ ] **7.3.1** Create TagManager class (`src/core/wallpaper/TagManager.cpp`)
  - **Store tag list** in library.json
  - **Tag colors** (optional, user-defined)
  - **Assign tags** to wallpapers
  - **Remove tags** from wallpapers
  - **Auto-suggest** existing tags
  - **Tag usage count** tracking

- [ ] **7.3.2** Create TagDialog (`src/gui/dialogs/TagDialog.cpp`)
  - **Current tags** display with remove buttons
  - **Tag input** with autocomplete
  - **Existing tags** list (click to add)
  - **Create new tag** option
  - **Apply/Cancel** buttons

### 7.4 Favorites & Rating
- [ ] **7.4.1** Implement favorites system
  - **Toggle favorite** per wallpaper
  - **Heart icon** on cards
  - **Filter favorites** in views
  - **Favorites count** in sidebar

- [ ] **7.4.2** Implement rating system
  - **1-5 star rating** per wallpaper
  - **Star display** on cards
  - **Click to rate** interaction
  - **Filter by minimum rating**
  - **Sort by rating**

### 7.5 Folder Organization
- [ ] **7.5.1** Create FolderManager class
  - **Virtual folders** (not file system)
  - **Create folder** with name
  - **Rename folder**
  - **Delete folder** (wallpapers remain)
  - **Add wallpaper** to folder
  - **Remove wallpaper** from folder
  - **Wallpaper can be in multiple** folders

- [ ] **7.5.2** Create FolderView (`src/gui/views/FolderView.cpp`)
  - **Folder tree** in sidebar
  - **Folder contents** in grid
  - **Drag-drop** to add wallpapers

---

## 8. Phase 7: Transition System

### 8.1 Transition Engine
- [ ] **8.1.1** Create TransitionEngine class (`src/core/transition/TransitionEngine.cpp`)
  - **Manage transitions** between wallpapers
  - **Hold current and next** wallpaper surfaces
  - **Calculate progress** (0.0 - 1.0)
  - **Apply easing functions**:
    - Linear
    - Ease-in (accelerate)
    - Ease-out (decelerate)
    - Ease-in-out
    - Bounce
  - **Composite frames** based on effect
  - **60fps rendering** during transition

### 8.2 Transition Effects Base
- [ ] **8.2.1** Create TransitionEffect interface
  ```cpp
  class TransitionEffect {
  public:
      virtual ~TransitionEffect() = default;
      virtual std::string getName() const = 0;
      virtual void render(
          cairo_t* cr,
          cairo_surface_t* from,
          cairo_surface_t* to,
          double progress,  // 0.0 to 1.0
          int width,
          int height,
          const TransitionParams& params
      ) = 0;
  };
  ```

### 8.3 Transition Effect Implementations
- [ ] **8.3.1** Create ExpandingCircleTransition
  - Random origin point (or configurable)
  - Circle expands from point
  - Reveals new wallpaper inside circle
  - Smooth antialiased edge

- [ ] **8.3.2** Create ExpandingSquareTransition
  - Same as circle but square shape
  - Rounded corners option

- [ ] **8.3.3** Create FadeTransition
  - Simple crossfade
  - Alpha blending

- [ ] **8.3.4** Create SlideTransition
  - Slide from direction:
    - Left, Right, Up, Down
  - Old slides out, new slides in
  - Optional: Push vs Cover

- [ ] **8.3.5** Create WipeTransition
  - Linear wipe at configurable angle
  - Options: 0°, 45°, 90°, etc.
  - Sharp or gradient edge

- [ ] **8.3.6** Create DissolveTransition
  - Pixelated dissolve effect
  - Random order pixel reveal
  - Configurable block size

- [ ] **8.3.7** Create ZoomTransition
  - Zoom in on old wallpaper
  - Fade to new
  - Or zoom out from center

- [ ] **8.3.8** Create MorphTransition
  - Color morphing effect
  - Blend colors smoothly
  - Abstract interpolation

### 8.4 Transition Settings
- [ ] **8.4.1** Create TransitionSettings struct
  ```cpp
  struct TransitionSettings {
      std::string effect = "expanding_circle";
      int duration_ms = 500;
      std::string easing = "ease_out";
      bool enabled = true;
      // Effect-specific params
      std::optional<std::pair<int,int>> origin;  // null = random
  };
  ```

- [ ] **8.4.2** Create TransitionDialog (`src/gui/dialogs/TransitionDialog.cpp`)
  - **Effect selector** with preview
  - **Duration slider** (100ms - 5000ms)
  - **Easing dropdown**
  - **Live preview** in dialog
  - **Apply/Cancel** buttons

---

## 9. Phase 8: Hyprland Integration

### 9.1 Hyprland IPC
- [ ] **9.1.1** Create HyprlandIPC class (`src/core/hyprland/HyprlandIPC.cpp`)
  - **Socket connection**: `$XDG_RUNTIME_DIR/hypr/$HYPRLAND_INSTANCE_SIGNATURE/.socket2.sock`
  - **Send commands** to Hyprland
  - **Parse JSON responses**
  - **Subscribe to events**
  - **Handle reconnection** on socket loss

- [ ] **9.1.2** Implement event subscriptions
  - **workspace** events (workspace changes)
  - **activewindow** events
  - **monitoradded/monitorremoved** events
  - **createworkspace/destroyworkspace** events

### 9.2 Workspace Manager
- [ ] **9.2.1** Create WorkspaceManager class (`src/core/hyprland/WorkspaceManager.cpp`)
  - **Track current workspace** per monitor
  - **Store wallpaper mappings**: workspace → wallpaper
  - **Handle workspace change** event:
    1. Get new workspace ID
    2. Look up assigned wallpaper
    3. Trigger transition to new wallpaper
  - **Default wallpaper** if workspace not configured

- [ ] **9.2.2** Implement workspace wallpaper UI
  - **Workspace grid** (1-10+)
  - **Click to assign** wallpaper
  - **Right-click to clear**
  - **Thumbnail preview** per workspace
  - **Enable/disable** workspace wallpapers toggle

### 9.3 Special Workspace
- [ ] **9.3.1** Implement special workspace support
  - **Detect special workspaces** (scratchpads)
  - **Separate configuration** option
  - **Enable/disable toggle**
  - **Assign wallpaper** to special workspace

### 9.4 Hyprctl CLI Integration
- [ ] **9.4.1** Add Hyprland commands to CLI
  - `bwp hyprland status` - Show Hyprland connection status
  - `bwp hyprland workspace <num> set <path>` - Set workspace wallpaper
  - `bwp hyprland workspace <num> clear` - Clear workspace wallpaper
  - `bwp hyprland workspaces list` - List workspace assignments

## 10. Phase 9: Steam Workshop

### 10.1 Steam API Client
- [ ] **10.1.1** Create SteamWorkshopClient class (`src/core/workshop/SteamWorkshopClient.cpp`)
  - **API base URL**: `https://api.steampowered.com/`
  - **Wallpaper Engine App ID**: 431960
  - **HTTP requests** via libcurl
  - **JSON parsing** of responses
  - **Error handling** with retry logic
  - **Rate limiting** respect

- [ ] **10.1.2** Implement authentication
  - **Detect Steam session** from `~/.steam/`
  - **Read Steam login info**
  - **Use session cookies** for authenticated requests
  - **Fall back to anonymous** if not logged in
  - **Show limited features notice** for anonymous users

### 10.2 Workshop Search
- [ ] **10.2.1** Create WorkshopSearch class (`src/core/workshop/WorkshopSearch.cpp`)
  - **Search by text query**
  - **Pagination** (page, items per page)
  - **Sort options**:
    - Most Popular
    - Most Subscribed
    - Recent
    - Trending
    - Top Rated
  - **Return WorkshopItem list**

- [ ] **10.2.2** Implement filters
  - **Resolution filters**:
    - 1920x1080 (1080p)
    - 2560x1440 (1440p)
    - 3840x2160 (4K)
    - Ultrawide (21:9)
    - Any
  - **Type filters**:
    - Scene ✓
    - Video ✓
    - Web ✗ (disabled with tooltip)
  - **Tag/Category filters**:
    - Abstract, Anime, Games, Nature, etc.
  - **Rating filter**: Minimum star rating
  - **Age filter**: All time, This year, This month, This week

### 10.3 Workshop Item Data
- [ ] **10.3.1** Create WorkshopItem struct
  ```cpp
  struct WorkshopItem {
      uint64_t id;
      std::string title;
      std::string description;
      std::string preview_url;     // Thumbnail URL
      std::string author_name;
      uint64_t author_id;
      std::string type;            // "scene", "video", "web"
      int64_t subscriptions;
      int64_t favorites;
      float rating;                // 0.0 - 5.0
      int64_t file_size;
      std::vector<std::string> tags;
      std::chrono::system_clock::time_point created;
      std::chrono::system_clock::time_point updated;
  };
  ```

### 10.4 Download Manager
- [ ] **10.4.1** Create WorkshopDownloader class (`src/core/workshop/WorkshopDownloader.cpp`)
  - **Primary download**: Direct HTTP download
  - **Fallback download**: Trigger Steam via steamcmd
  - **Download location**: `~/.steam/steam/steamapps/workshop/content/431960/<item_id>/`
  - **Progress tracking**: bytes downloaded / total
  - **Cancel support**: Abort download
  - **Resume support**: Continue partial downloads
  - **Verify integrity**: Check file hash after download

- [ ] **10.4.2** Implement download queue
  - **Queue multiple downloads**
  - **Sequential or parallel** (configurable)
  - **Priority ordering**
  - **Pause/resume queue**
  - **Persistent queue** (survive restart)

### 10.5 Workshop View
- [ ] **10.5.1** Create WorkshopView (`src/gui/views/WorkshopView.cpp`)
  - **Search bar** with autocomplete
  - **Filter panel** (collapsible sidebar)
  - **Results grid**: WorkshopCard items
  - **Pagination controls**: Previous/Next, page numbers
  - **Loading state**: Skeleton cards

- [ ] **10.5.2** Create WorkshopCard widget
  - **Preview image** (lazy loaded)
  - **Title** (truncated)
  - **Author name**
  - **Rating** (stars)
  - **Subscriptions count**
  - **Type badge** (Scene/Video)
  - **Web type**: Disabled state with strikethrough
  - **Click**: Open detail view
  - **Download button**: Icon overlay

- [ ] **10.5.3** Create WorkshopDetailDialog
  - **Large preview** with animation
  - **Full title and description**
  - **Author info** with link to profile
  - **Stats**: Subscriptions, Favorites, Rating
  - **Tags list**
  - **File size**
  - **Created/Updated dates**
  - **Download button** (large, prominent)
  - **For Web type**: Error message explaining incompatibility

### 10.6 Download Progress UI
- [ ] **10.6.1** Create DownloadProgressWidget
  - **Current download**: Thumbnail + Title
  - **Progress bar**: Percentage
  - **Speed indicator**: MB/s
  - **ETA display**
  - **Cancel button**
  - **Queue count**: "3 remaining"

- [ ] **10.6.2** Integrate into main window
  - **Bottom panel** (appears during download)
  - **Minimize to notification** option
  - **Complete notification** when finished

---

## 11. Phase 10: Advanced Features

### 11.1 Profiles System
- [ ] **11.1.1** Create ProfilesView (`src/gui/views/ProfilesView.cpp`)
  - **Profile list**: Cards for each profile
  - **Active profile indicator**
  - **Create new profile** button
  - **Profile card actions**:
    - Activate
    - Edit
    - Duplicate
    - Export
    - Delete

- [ ] **11.1.2** Create ProfileEditDialog
  - **Profile name** input
  - **Profile icon** selection
  - **Trigger settings**:
    - Time ranges
    - Connected monitors
    - Power state
    - Running applications
  - **Save/Cancel** buttons

- [ ] **11.1.3** Implement profile triggers
  - **Time-based trigger**:
    - Start time, End time
    - Days of week
  - **Monitor trigger**:
    - Specific monitor connected
    - Number of monitors
  - **Power trigger**:
    - Plugged in
    - On battery
  - **Application trigger**:
    - When app is running
    - When app is focused

### 11.2 Scheduling System
- [ ] **11.2.1** Create Scheduler class (`src/core/scheduler/Scheduler.cpp`)
  - **Store schedule entries** in config
  - **Check schedules** on timer (every minute)
  - **Trigger profile/wallpaper** changes
  - **Handle overlapping schedules** (priority)
  - **Time zone handling**

- [ ] **11.2.2** Create ScheduleView (`src/gui/views/ScheduleView.cpp`)
  - **Visual timeline**: 24-hour view
  - **Colored blocks** for schedules
  - **Click to add** schedule
  - **Drag to resize** duration
  - **Schedule list** view (alternative)

- [ ] **11.2.3** Create ScheduleDialog
  - **Time range** pickers (start, end)
  - **Days of week** toggles
  - **Action**: Change wallpaper or profile
  - **Wallpaper/Profile** selector
  - **Repeat** options (daily, weekly)

### 11.3 Slideshow Mode
- [ ] **11.3.1** Create SlideshowManager class (`src/core/scheduler/SlideshowManager.cpp`)
  - **Enable/disable** toggle
  - **Interval** setting (seconds to hours)
  - **Order**: Sequential or Random
  - **Wallpaper list**: All, Favorites, Folder, Tag
  - **Per-monitor slideshows** option
  - **Transition on change** using TransitionEngine

- [ ] **11.3.2** Add slideshow controls to UI
  - **Slideshow panel** in settings
  - **Quick toggle** in tray menu
  - **Next/Previous** buttons
  - **Status indicator** in main window

### 11.4 Color Extraction & Theming
- [ ] **11.4.1** Create ColorExtractor class (`src/core/color/ColorExtractor.cpp`)
  - **Extract dominant colors** using k-means clustering
  - **Palette sizes**: 8, 16, 32 colors
  - **Color sorting**: By luminance, saturation, hue
  - **Special colors**: Primary, secondary, accent, background

- [ ] **11.4.2** Create ThemeGenerator class (`src/core/color/ThemeGenerator.cpp`)
  - **Export formats**:
    - Shell variables (colors.sh)
    - JSON (colors.json)
    - CSS variables (colors.css)
    - SCSS variables (colors.scss)
  - **Template support** for custom formats

- [ ] **11.4.3** Create ThemeApplier class (`src/core/color/ThemeApplier.cpp`)
  - **Detect installed tools**: pywal, matugen, wpgtk
  - **Auto-select best tool** or user preference
  - **Apply theme** by running tool
  - **Reload applications** after theme change
  - **Custom script support** for other tools

- [ ] **11.4.4** Add theming UI
  - **Settings section** for theming
  - **Enable/disable** auto-theming
  - **Tool selection** dropdown
  - **Preview extracted colors**
  - **Manual apply** button

---

## 12. Phase 11: System Integration

### 12.1 System Tray
- [ ] **12.1.1** Create TrayIcon class (`src/tray/TrayIcon.cpp`)
  - **App icon** in system tray
  - **Left-click**: Open main window
  - **Right-click**: Context menu

- [ ] **12.1.2** Create TrayMenu
  - **Current wallpaper** (thumbnail + name)
  - **Separator**
  - **⏸ Pause/▶ Resume** animations
  - **⏭ Next** (if slideshow active)
  - **⏮ Previous**
  - **Separator**
  - **📊 Profiles** submenu:
    - List all profiles
    - Checkmark on active
  - **Separator**
  - **⚙️ Preferences**
  - **❌ Quit**

### 12.2 Notifications
- [ ] **12.2.1** Create NotificationManager class
  - **System notifications** via libnotify
  - **In-app toasts** via custom widget
  - **Notification types**:
    - Info
    - Success
    - Warning
    - Error
  - **User preferences**: Enable/disable each type

- [ ] **12.2.2** Create Toast widget (`src/gui/widgets/Toast.cpp`)
  - **Slide in** from top
  - **Auto-dismiss** after timeout
  - **Close button**
  - **Action button** (optional)
  - **Queue multiple** toasts

### 12.3 Autostart
- [ ] **12.3.1** Implement autostart methods
  - **Systemd user service**:
    - Create `~/.config/systemd/user/betterwallpaper.service`
    - Enable/disable via systemctl
  - **XDG autostart**:
    - Create `~/.config/autostart/betterwallpaper.desktop`
  - **Hyprland exec-once**:
    - Detect hyprland.conf location
    - Add/remove exec-once line
  - **User preference** for method

- [ ] **12.3.2** Add autostart UI
  - **Enable autostart** toggle
  - **Method selection** dropdown
  - **Start minimized** toggle

### 12.4 Backup & Restore
- [ ] **12.4.1** Create BackupManager class
  - **Export to ZIP archive**:
    - `config.json`
    - All profiles
    - Library metadata
    - Tags
    - Favorites
  - **Optional**: Include wallpaper files
  - **Import from ZIP**:
    - Validate archive structure
    - Merge or replace existing
    - Handle conflicts

- [ ] **12.4.2** Add backup UI
  - **Export button** → file picker
  - **Import button** → file picker
  - **Include wallpapers** checkbox
  - **Confirmation dialogs**

### 12.5 Import Features
- [ ] **12.5.1** Create ImportManager class
  - **Import from local file**: Drag-drop or picker
  - **Import from URL**: Download and add
  - **Bulk import folder**: Recursive scan
  - **Copy vs Reference**: User choice

- [ ] **12.5.2** Create ImportDialog
  - **Drop zone** for files
  - **URL input** field
  - **Folder picker** button
  - **Options**:
    - Copy to library folder
    - Reference in place
    - Apply default tags
  - **Progress indicator**
  - **Results summary**

## 13. Phase 12: Packaging & Distribution

### 13.1 Desktop Integration Files
- [ ] **13.1.1** Create desktop entry (`data/betterwallpaper.desktop`)
  ```desktop
  [Desktop Entry]
  Name=BetterWallpaper
  Comment=Wallpaper Manager with Wallpaper Engine Support
  Exec=betterwallpaper
  Icon=betterwallpaper
  Terminal=false
  Type=Application
  Categories=Utility;Settings;
  Keywords=wallpaper;background;desktop;
  StartupWMClass=betterwallpaper
  ```

- [ ] **13.1.2** Create autostart entry (`data/betterwallpaper-autostart.desktop`)
  - Same as desktop entry
  - Add `X-GNOME-Autostart-enabled=true`
  - Add `Hidden=false`

- [ ] **13.1.3** Create app icons
  - SVG source (`data/icons/betterwallpaper.svg`)
  - Symbolic icon (`data/icons/betterwallpaper-symbolic.svg`)
  - Export to hicolor sizes: 16, 24, 32, 48, 64, 128, 256, 512

- [ ] **13.1.4** Create systemd service (`systemd/betterwallpaper.service`)
  ```ini
  [Unit]
  Description=BetterWallpaper Daemon
  After=graphical-session.target
  PartOf=graphical-session.target

  [Service]
  Type=simple
  ExecStart=/usr/bin/betterwallpaper-daemon
  Restart=on-failure
  RestartSec=5

  [Install]
  WantedBy=graphical-session.target
  ```

### 13.2 Arch Linux (AUR)
- [ ] **13.2.1** Create PKGBUILD (`packaging/arch/PKGBUILD`)
  ```bash
  # Maintainer: Your Name <email@example.com>
  pkgname=betterwallpaper
  pkgver=1.0.0
  pkgrel=1
  pkgdesc="Feature-rich wallpaper manager with Wallpaper Engine support"
  arch=('x86_64')
  url="https://github.com/username/BetterWallpaper"
  license=('GPL3')
  depends=(
      'gtk4'
      'libadwaita'
      'mpv'
      'curl'
      'wayland'
      'linux-wallpaperengine'
  )
  makedepends=('cmake' 'git')
  source=("$pkgname-$pkgver.tar.gz")
  sha256sums=('SKIP')

  build() {
      cmake -B build -S "$pkgname-$pkgver" \
          -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_INSTALL_PREFIX=/usr
      cmake --build build
  }

  package() {
      DESTDIR="$pkgdir" cmake --install build
  }
  ```

- [ ] **13.2.2** Create PKGBUILD for git version (`packaging/arch/PKGBUILD-git`)
  - `pkgname=betterwallpaper-git`
  - Source from git repository
  - Include `provides=('betterwallpaper')`
  - Include `conflicts=('betterwallpaper')`

- [ ] **13.2.3** Test AUR package
  - Build with makepkg
  - Install and verify
  - Submit to AUR

### 13.3 Flatpak
- [ ] **13.3.1** Create Flatpak manifest (`packaging/flatpak/com.github.BetterWallpaper.yml`)
  ```yaml
  app-id: com.github.BetterWallpaper
  runtime: org.gnome.Platform
  runtime-version: '45'
  sdk: org.gnome.Sdk
  command: betterwallpaper
  finish-args:
    - --share=ipc
    - --socket=wayland
    - --socket=fallback-x11
    - --filesystem=home
    - --filesystem=~/.steam:ro
    - --talk-name=org.freedesktop.Notifications
    - --device=dri
  modules:
    - name: betterwallpaper
      buildsystem: cmake-ninja
      sources:
        - type: git
          url: https://github.com/username/BetterWallpaper.git
          tag: v1.0.0
  ```

- [ ] **13.3.2** Create Flathub metadata files
  - `com.github.BetterWallpaper.metainfo.xml` (AppStream)
  - Screenshots for Flathub listing
  - Release notes

- [ ] **13.3.3** Test Flatpak build
  - Build locally with flatpak-builder
  - Install and verify
  - Submit to Flathub

### 13.4 AppImage
- [ ] **13.4.1** Create AppImage config (`packaging/appimage/AppImageBuilder.yml`)
  ```yaml
  version: 1
  AppDir:
    path: ./AppDir
    app_info:
      id: com.github.BetterWallpaper
      name: BetterWallpaper
      icon: betterwallpaper
      version: 1.0.0
      exec: usr/bin/betterwallpaper
    apt:
      arch: amd64
      sources:
        - sourceline: 'deb http://archive.ubuntu.com/ubuntu/ jammy main'
      include:
        - libgtk-4-1
        - libadwaita-1-0
        - libmpv1
        - libcurl4
  AppImage:
    arch: x86_64
    update-information: guess
  ```

- [ ] **13.4.2** Create desktop integration script
  - Integrate with system on first run
  - Create menu entry
  - Handle app updates

- [ ] **13.4.3** Test AppImage
  - Build with appimage-builder
  - Test on multiple distributions
  - Publish to GitHub releases

### 13.5 Debian/Ubuntu
- [ ] **13.5.1** Create debian control files
  - `packaging/debian/control`:
    - Package name, description
    - Dependencies
    - Build-dependencies
  - `packaging/debian/rules`:
    - Build commands
    - dh integration
  - `packaging/debian/changelog`:
    - Version history

- [ ] **13.5.2** Create .deb package
  - Build with dpkg-buildpackage
  - Test installation on Ubuntu/Debian
  - Publish to PPA (optional)

### 13.6 Fedora/RPM
- [ ] **13.6.1** Create RPM spec file (`packaging/fedora/betterwallpaper.spec`)
  ```spec
  Name:           betterwallpaper
  Version:        1.0.0
  Release:        1%{?dist}
  Summary:        Wallpaper manager with Wallpaper Engine support

  License:        GPLv3+
  URL:            https://github.com/username/BetterWallpaper
  Source0:        %{name}-%{version}.tar.gz

  BuildRequires:  cmake >= 3.20
  BuildRequires:  gcc-c++
  BuildRequires:  gtk4-devel
  BuildRequires:  libadwaita-devel
  BuildRequires:  mpv-libs-devel
  BuildRequires:  libcurl-devel

  Requires:       gtk4
  Requires:       libadwaita
  Requires:       mpv-libs

  %description
  Feature-rich wallpaper manager for Linux with Wallpaper Engine support.

  %build
  %cmake
  %cmake_build

  %install
  %cmake_install

  %files
  %license LICENSE
  %{_bindir}/betterwallpaper
  %{_bindir}/betterwallpaper-daemon
  %{_bindir}/bwp
  %{_datadir}/applications/betterwallpaper.desktop
  %{_datadir}/icons/hicolor/*/apps/betterwallpaper.*
  ```

- [ ] **13.6.2** Test RPM build
  - Build with rpmbuild
  - Test on Fedora
  - Publish to COPR (optional)

---

## 14. Technical Specifications

### 14.1 Daemon Process Architecture
```
┌─────────────────────────────────────────────────────────────┐
│                  betterwallpaper-daemon                      │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────┐  │
│  │   D-Bus      │  │   Wayland    │  │   Hyprland IPC   │  │
│  │   Listener   │  │   Monitor    │  │   Listener       │  │
│  └──────┬───────┘  └──────┬───────┘  └────────┬─────────┘  │
│         │                 │                    │             │
│         └─────────────────┼────────────────────┘             │
│                           │                                  │
│                   ┌───────▼───────┐                         │
│                   │  Event Loop   │                         │
│                   │   (GMainLoop) │                         │
│                   └───────┬───────┘                         │
│                           │                                  │
│  ┌────────────────────────┼────────────────────────────┐    │
│  │                        │                             │    │
│  │  ┌─────────────┐ ┌─────▼─────┐ ┌─────────────────┐ │    │
│  │  │ Wallpaper   │ │Transition │ │    Scheduler    │ │    │
│  │  │ Renderer    │ │  Engine   │ │                 │ │    │
│  │  └─────────────┘ └───────────┘ └─────────────────┘ │    │
│  │                                                     │    │
│  └─────────────────────────────────────────────────────┘    │
│                                                              │
│  ┌──────────────────────────────────────────────────────┐   │
│  │                  Layer Shell Windows                  │   │
│  │   [Monitor 1]      [Monitor 2]      [Monitor N]      │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### 14.2 Error Codes

| Code | Name | Description |
|------|------|-------------|
| 0 | SUCCESS | Operation completed successfully |
| 1 | ERR_UNKNOWN | Unknown error |
| 10 | ERR_FILE_NOT_FOUND | Wallpaper file not found |
| 11 | ERR_INVALID_FORMAT | Unsupported file format |
| 12 | ERR_WEB_WALLPAPER | Web wallpaper not supported |
| 20 | ERR_MONITOR_NOT_FOUND | Monitor not found |
| 21 | ERR_WAYLAND_CONNECT | Cannot connect to Wayland |
| 30 | ERR_DAEMON_NOT_RUNNING | Daemon is not running |
| 31 | ERR_DBUS_CONNECT | Cannot connect to D-Bus |
| 40 | ERR_CONFIG_INVALID | Invalid configuration |
| 50 | ERR_WORKSHOP_API | Steam Workshop API error |
| 51 | ERR_DOWNLOAD_FAILED | Download failed |

### 14.3 D-Bus Interface

**Service Name**: `com.github.BetterWallpaper`
**Object Path**: `/com/github/BetterWallpaper`

**Methods**:
- `SetWallpaper(path: string, monitor: string) → success: bool`
- `GetCurrentWallpaper(monitor: string) → path: string`
- `Pause() → void`
- `Resume() → void`
- `NextWallpaper() → void`
- `PreviousWallpaper() → void`
- `SetProfile(name: string) → success: bool`
- `GetStatus() → status: dict`

**Signals**:
- `WallpaperChanged(monitor: string, path: string)`
- `ProfileChanged(name: string)`
- `MonitorConnected(name: string)`
- `MonitorDisconnected(name: string)`

**Properties**:
- `IsPaused: bool` (read)
- `CurrentProfile: string` (read)
- `DaemonVersion: string` (read)

---

## 15. Configuration Reference

### 15.1 Main Config File
**Location**: `~/.config/betterwallpaper/config.json`

```json
{
  "version": "1.0.0",
  "general": {
    "autostart": true,
    "autostart_method": "systemd",
    "start_minimized": true,
    "close_to_tray": true,
    "check_updates": true
  },
  "library": {
    "paths": [
      "~/Pictures/Wallpapers",
      "~/.steam/steam/steamapps/workshop/content/431960"
    ],
    "scan_recursive": true,
    "duplicate_handling": "ask",
    "auto_remove_missing": true,
    "thumbnail_size": 256
  },
  "defaults": {
    "scaling_mode": "fill",
    "audio_enabled": false,
    "audio_volume": 50,
    "loop_enabled": true,
    "playback_speed": 1.0
  },
  "performance": {
    "fps_limit": 60,
    "pause_on_battery": true,
    "pause_on_fullscreen": true,
    "fullscreen_exceptions": [],
    "max_memory_mb": 512,
    "max_vram_mb": 256
  },
  "transitions": {
    "enabled": true,
    "default_effect": "expanding_circle",
    "duration_ms": 500,
    "easing": "ease_out"
  },
  "notifications": {
    "enabled": true,
    "system_notifications": true,
    "in_app_toasts": true,
    "on_wallpaper_change": false,
    "on_error": true
  },
  "theming": {
    "enabled": true,
    "auto_apply": true,
    "tool": "auto",
    "palette_size": 16
  },
  "hyprland": {
    "workspace_wallpapers": true,
    "special_workspace_enabled": false
  },
  "current_profile": "default"
}
```

### 15.2 Profile File
**Location**: `~/.config/betterwallpaper/profiles/<name>.json`

```json
{
  "name": "default",
  "monitors": {
    "DP-1": {
      "wallpaper_id": "abc123",
      "scaling_mode": "fill",
      "volume": 80,
      "playback_speed": 1.0,
      "workspaces": {
        "1": "wallpaper_id_1",
        "2": "wallpaper_id_2"
      }
    }
  },
  "multi_monitor_mode": "independent",
  "slideshow": {
    "enabled": false,
    "interval_seconds": 300,
    "order": "random"
  },
  "triggers": []
}
```

### 15.3 Library Database
**Location**: `~/.local/share/betterwallpaper/library.json`

---

## 16. CLI Reference

### 16.1 General Usage
```bash
bwp [command] [options]

Global Options:
  --help, -h         Show help message
  --version, -v      Show version
  --json             Output in JSON format
  --quiet, -q        Suppress output
  --verbose          Verbose output
```

### 16.2 Commands

#### Wallpaper Commands
```bash
# Set wallpaper
bwp set <path>                      # Set on all monitors
bwp set <path> -m <monitor>         # Set on specific monitor
bwp set <path> -w <workspace>       # Set for workspace

# Navigation (slideshow)
bwp next                            # Next wallpaper
bwp prev                            # Previous wallpaper
bwp random                          # Random from library

# Playback control
bwp pause                           # Pause animations
bwp resume                          # Resume animations
bwp toggle                          # Toggle pause/resume
```

#### Monitor Commands
```bash
bwp monitors                        # List all monitors
bwp monitors -j                     # JSON output
bwp monitor <name>                  # Show monitor info
bwp monitor <name> set <path>       # Set wallpaper
bwp monitor <name> scaling <mode>   # Set scaling mode
```

#### Profile Commands
```bash
bwp profiles                        # List profiles
bwp profile <name>                  # Activate profile
bwp profile create <name>           # Create profile
bwp profile delete <name>           # Delete profile
bwp profile export <name> <file>    # Export to file
bwp profile import <file>           # Import from file
```

#### Library Commands
```bash
bwp library scan                    # Rescan library
bwp library list                    # List wallpapers
bwp library search <query>          # Search
bwp library add <path>              # Add wallpaper/folder
bwp library remove <id>             # Remove from library
bwp library info <id>               # Show wallpaper info
```

#### Workshop Commands
```bash
bwp workshop search <query>         # Search workshop
bwp workshop download <id>          # Download wallpaper
bwp workshop info <id>              # Show item info
bwp workshop queue                  # Show download queue
```

#### Slideshow Commands
```bash
bwp slideshow start                 # Start slideshow
bwp slideshow stop                  # Stop slideshow
bwp slideshow status                # Show status
bwp slideshow interval <seconds>    # Set interval
```

#### Theme Commands
```bash
bwp theme extract                   # Extract colors
bwp theme apply                     # Apply theme
bwp theme export <format>           # Export (json/sh/css)
bwp theme colors                    # Show current colors
```

#### Daemon Commands
```bash
bwp daemon status                   # Show daemon status
bwp daemon restart                  # Restart daemon
bwp daemon logs                     # Show recent logs
bwp daemon logs -f                  # Follow logs
```

---

## 17. Success Criteria Checklist

### 17.1 Core Functionality
- [ ] Static image wallpapers working (PNG, JPG, WEBP, etc.)
- [ ] Video wallpapers working (MP4, WEBM, MKV)
- [ ] GIF/APNG animated images working
- [ ] Wallpaper Engine Scene wallpapers working
- [ ] Wallpaper Engine Video wallpapers working
- [ ] Web wallpapers properly blocked with clear error
- [ ] All scaling modes implemented and working

### 17.2 Multi-Monitor
- [ ] Auto-detect all connected monitors
- [ ] Visual monitor layout accurate
- [ ] Independent wallpaper per monitor
- [ ] Clone mode working
- [ ] Span mode working
- [ ] Dynamic monitor connect/disconnect handling

### 17.3 Hyprland Integration
- [ ] Workspace change detection working
- [ ] Per-workspace wallpaper switching
- [ ] Special workspace support
- [ ] Smooth transitions on workspace change
- [ ] Hyprctl CLI commands working

### 17.4 Transitions
- [ ] Expanding circle transition
- [ ] All other transition effects
- [ ] Configurable duration and easing
- [ ] Smooth 60fps during transition

### 17.5 Steam Workshop
- [ ] Search and browse working
- [ ] All filters functional
- [ ] Download working (direct)
- [ ] Download fallback to Steam working
- [ ] Download progress display

### 17.6 GUI
- [ ] Responsive window resizing
- [ ] System theme integration (dark/light)
- [ ] All animations smooth
- [ ] All views implemented and functional

### 17.7 System Integration
- [ ] System tray working
- [ ] Notifications working
- [ ] Autostart working (all methods)
- [ ] Backup/restore working
- [ ] CLI fully functional

### 17.8 Packaging
- [ ] AUR package building and installing
- [ ] Flatpak building and running
- [ ] AppImage created and tested
- [ ] .deb package building
- [ ] .rpm package building

---

*This plan was last updated: 2026-01-22*
*Mark tasks with [x] as they are completed*
