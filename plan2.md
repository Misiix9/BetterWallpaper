# BetterWallpaper - Optimized Implementation Plan v2

> **A comprehensive, prioritized roadmap for completing BetterWallpaper with focus on performance, polish, and user experience.**

---

## Table of Contents

1. [Phase 1: Critical Fixes & Foundation](#phase-1-critical-fixes--foundation)
2. [Phase 2: Performance Optimization](#phase-2-performance-optimization)
3. [Phase 3: Core Services (Daemon, CLI, Tray)](#phase-3-core-services-daemon-cli-tray)
4. [Phase 4: Visual Polish & Animations](#phase-4-visual-polish--animations)
5. [Phase 5: Settings & Configuration UI](#phase-5-settings--configuration-ui)
6. [Phase 6: Multi-Monitor Enhancements](#phase-6-multi-monitor-enhancements)
7. [Phase 7: Library & Tag Management](#phase-7-library--tag-management)
8. [Phase 8: Advanced Transitions](#phase-8-advanced-transitions)
9. [Phase 9: Profiles & Scheduling](#phase-9-profiles--scheduling)
10. [Phase 10: Steam Workshop Integration](#phase-10-steam-workshop-integration)
11. [Phase 11: Color Extraction & Theming](#phase-11-color-extraction--theming)
12. [Phase 12: Notifications & System Integration](#phase-12-notifications--system-integration)
13. [Phase 13: Testing & Quality Assurance](#phase-13-testing--quality-assurance)
14. [Phase 14: Packaging & Distribution](#phase-14-packaging--distribution)
15. [Phase 15: Normal Image Wallpaper Support](#phase-15-normal-image-wallpaper-support)
16. [Phase 16: UI Cleanup & Simplification](#phase-16-ui-cleanup--simplification)
17. [Phase 17: Source Directory Management](#phase-17-source-directory-management)
18. [Phase 18: Wallpaper-Specific Settings](#phase-18-wallpaper-specific-settings)
19. [Phase 19: Global Default Settings System](#phase-19-global-default-settings-system)

---

## Phase 1: Critical Fixes & Foundation

> **Priority: CRITICAL** - These issues must be fixed before any other work.

### 1.1 CSS Loading Fix
- [x] **1.1.1** Fix CSS path resolution in `Application.cpp`
  - Replace relative path `"data/ui/style.css"` with absolute path
  - Use XDG data directories for installed version
  - Fallback to source directory for development builds
  - Add proper error handling if CSS fails to load

- [x] **1.1.2** Add CSS hot-reload for development
  - Watch CSS file for changes
  - Reload stylesheet without restart
  - Log style errors to console

### 1.2 Error Handling Infrastructure
- [x] **1.2.1** Create centralized error handling system
  - Define `bwp::Error` class with error codes
  - Use `std::expected<T, Error>` for fallible operations
  - Create error translation for user-friendly messages

- [x] **1.2.2** Add error display UI
  - Create `ErrorDialog` for critical errors
  - Error indicator in status bar

### 1.4 Memory Safety Audit
- [x] **1.4.2** Implement RAII wrappers for GTK objects
  - Create `GtkPtr<T>` smart pointer template
  - Ensure proper cleanup on widget destruction

### 2.1 Async Library Loading
- [x] **2.1.1** Make library scanning non-blocking
  - Move `LibraryScanner` to background thread
  - Show skeleton loaders during scan

### 2.2 Thumbnail System Overhaul
- [x] **2.2.1** Implement persistent thumbnail cache
  - Cache location: `~/.cache/betterwallpaper/thumbnails/`
  - Store as WebP for smaller size

- [x] **2.2.2** Async thumbnail loading
  - [x] Load thumbnails in worker thread pool
  - [x] Priority queue: visible items first
  - [x] Placeholder image while loading

### 2.3 Virtual Scrolling for Grid
- [x] **2.3.1** Replace FlowBox with virtualized container
  - Only render visible grid items + buffer
  - Recycle card widgets as user scrolls

- [x] **2.3.2** Implement efficient data model
  - Use `GListModel` for wallpaper list
  - Support filtering/sorting without full re-render

### 2.4 Lazy View Initialization
- [x] **2.4.1** Defer view creation until accessed
  - Don't create SettingsView until user clicks Settings
  - Use factory pattern for view instantiation

### 2.5 Startup Optimization
- [x] **2.5.1** Profile and optimize startup time
  - Parallelize independent operations

### 3.1 Daemon Implementation
- [x] **3.1.1** Create daemon architecture
  - Single-instance enforcement
  - Signal handling

- [x] **3.1.2** Implement core daemon loop
  - GMainLoop for event processing
  - D-Bus service registration

- [x] **3.1.3** Move wallpaper rendering to daemon
  - Manage per-monitor renderers
  - Handle monitor connect/disconnect

- [x] **3.1.4** Implement daemon-GUI communication
  - GUI connects to daemon via D-Bus
  - Daemon emits status updates to GUI

### 3.2 CLI Implementation
### 3.2 CLI Implementation
- [x] **3.2.1** Create CLI argument parser
  - Use argparse
  - Support basic commands

- [x] **3.2.2** Implement wallpaper commands
  - `bwp set <path>`
  - `bwp next`, `bwp prev`
  - `bwp pause`, `bwp resume`, `bwp stop`

- [ ] **3.2.3** Implement library commands
  > ⚠️ DEFERRED: Complex IPC
  - `bwp library scan`
  - `bwp library list`

- [ ] **3.2.4** Implement monitor commands
  > ⚠️ DEFERRED: Complex IPC
  - `bwp monitors`

### 3.3 System Tray Implementation
- [x] **3.3.1** Create TrayIcon using StatusNotifierItem
  - App icon in system tray
  - Tooltip showing current wallpaper

- [x] **3.3.2** Create TrayMenu
  - Current wallpaper name
  - Pause/Resume toggle
  - Open main window/Quit

- [x] **3.3.3** Tray preferences
  - Minimize to tray option enabled in Settings

---

## Phase 4: Visual Polish & Animations
*Phase 4 completed: 2026-01-29*

> **Priority: MEDIUM-HIGH** - Makes the app feel premium.

> **Priority: MEDIUM-HIGH** - Makes the app feel premium.

### 4.1 Page Transitions
- [x] **4.1.1** Add crossfade between views
  - Fade out current view, fade in new view
  - Duration: 200-300ms
  - Use CSS transitions or AdwNavigationView

- [x] **4.1.2** Add slide transitions for sub-pages
  - Slide from right for drill-down
  - Slide from left for back navigation

### 4.2 Card Hover Effects
- [x] **4.2.1** Enhance WallpaperCard hover
  - Subtle scale transform (1.02x)
  - Elevated shadow on hover
  - Smooth transition (150ms ease-out)
  - Border highlight with accent color

- [x] **4.2.2** Add card press feedback
  - Scale down slightly on press (0.98x)
  - Ripple effect on click (optional)

### 4.3 Grid Loading Animations
- [x] **4.3.1** Staggered card fade-in
  - Cards fade in sequentially on load
  - 50ms delay between cards
  - Use intersection observer pattern

- [x] **4.3.2** Skeleton loading states
  - Show placeholder cards during load
  - Pulsing animation on placeholders
  - Transition smoothly to real content

### 4.4 Preview Panel Animations
- [x] **4.4.1** Slide-in animation on selection
  - Panel slides in from right
  - Content fades in after slide
  - Crossfade when changing selection

- [x] **4.4.2** Image transition effects
  - Crossfade between preview images
  - Optional zoom reveal effect

### 4.5 Sidebar Enhancements
- [x] **4.5.1** Animated selection indicator
  - Selection bar slides between items
  - Smooth 200ms transition
  - Background highlight follows along

- [x] **4.5.2** Hover animations
  - Background color transition
  - Icon scale/color change

### 4.6 CSS Styling Improvements
- [x] **4.6.1** Refine color palette
  - Improved dark mode contrast
  - Subtle accent color variations
  - Better distinction between states

- [x] **4.6.2** Consistent spacing scale
  - Define spacing tokens (4, 8, 12, 16, 24, 32)
  - Apply consistently across all widgets

- [x] **4.6.3** Consistent border radius
  - Define radius scale (4, 8, 12, 16, 24)
  - Cards: 12px, Buttons: 8px, Inputs: 6px

- [x] **4.6.4** Layered shadows
  - Subtle shadows for depth hierarchy
  - Elevated shadows for modals/popovers

- [x] **4.6.5** Typography refinement
  - Use variable font weights
  - Improve letter-spacing
  - Better text hierarchy (size/weight/color)

### 4.7 Micro-Interactions
- [x] **4.7.1** Button feedback
  - Scale animation on press
  - Color transition on hover/active
  - Loading spinner for async actions

- [x] **4.7.2** Toggle switch animations
  - Smooth slide animation
  - Color transition for on/off

- [x] **4.7.3** Dropdown animations
  - Fade/slide in dropdown content
  - Arrow rotation animation

---

## Phase 5: Settings & Configuration UI

> **Priority: MEDIUM** - Complete the settings panels.

### 5.1 Settings View Structure
- [x] **5.1.1** Implement SettingsView with sidebar navigation
  - General, Appearance, Performance, Notifications
  - Transitions, Shortcuts, Theming, Backup, About
  - Use AdwNavigationSplitView or similar

### 5.2 General Settings Panel
- [x] **5.2.1** Implement GeneralSettings
  - Start on boot toggle + method dropdown
  - Start minimized to tray toggle
  - Close button behavior dropdown
  - Default audio toggle (enabled/muted)
  - Default loop toggle
  - Duplicate handling dropdown

- [x] **5.2.2** Library paths management
  - List of configured paths
  - Add path button (folder picker)
  - Remove path button
  - Edit path (inline or dialog)
  - Scan status indicator per path

### 5.3 Appearance Settings Panel
- [x] **5.3.1** Implement AppearanceSettings
  - Theme follows system (info text)
  - Enable blur toggle (Hyprland)
  - Grid card size slider (small/medium/large)
  - Show type badges toggle
  - Animation speed dropdown (Fast/Normal/Slow)
  - Preview panel position dropdown

### 5.4 Performance Settings Panel
- [x] **5.4.1** Implement PerformanceSettings
  - FPS limit dropdown (15/30/60/120/Unlimited)
  - Pause on battery toggle
  - Pause on fullscreen toggle
  - Fullscreen exceptions list (add/remove apps)
  - Max memory usage slider
  - Max VRAM usage slider
  - GPU acceleration toggle

### 5.5 Notification Settings Panel
- [x] **5.5.1** Implement NotificationSettings
  - Enable notifications master toggle
  - System notifications toggle
  - In-app toasts toggle
  - Notify on wallpaper change toggle
  - Notify on schedule change toggle
  - Notify on error toggle

### 5.6 Transition Settings Panel
- [x] **5.6.1** Implement TransitionSettings
  - Enable transitions toggle
  - Default effect dropdown with preview
  - Duration slider (100ms - 5000ms)
  - Easing dropdown preview
  - Preview/test button

### 5.7 Shortcuts Settings Panel
- [x] **5.7.1** Implement ShortcutsSettings
  - List of actions with current keybindings
  - Click to edit binding
  - Conflict detection
  - Reset to defaults button
  - Available actions: Next, Previous, Pause/Resume, Open window

### 5.8 Theming Settings Panel
- [x] **5.8.1** Implement ThemingSettings
  - Enable color extraction toggle
  - Auto-apply on wallpaper change toggle
  - Theming tool dropdown (Auto/pywal/matugen/wpgtk/Custom)
  - Palette size dropdown (8/16/32)
  - Custom script path input
  - Manual apply button
  - Color preview palette widget

### 5.9 Backup Settings Panel
- [x] **5.9.1** Implement BackupSettings
  - Export configuration button → file picker
  - Import configuration button → file picker
  - Include wallpaper files checkbox
  - Last backup info text
  - Restore defaults button (with confirmation)

### 5.10 About Panel
- [x] **5.10.1** Implement AboutPanel
  - App icon (large)
  - App name: BetterWallpaper
  - Version number
  - Brief description
  - Links: GitHub, Docs, Bug report
  - License: GPL-3.0
  - Credits/dependencies
  - Check for updates button (optional)

---

## Phase 6: Multi-Monitor Enhancements

> **Priority: MEDIUM** - Important for multi-monitor users.

### 6.1 Monitor Layout Widget
- [x] **6.1.1** Improve MonitorLayout rendering
  - Proportionate sizing
  - Correct positioning rendering

- [x] **6.1.2** Monitor interaction
  - Click to select monitor
  - Visual selection state

### 6.2 Monitor Configuration Panel
- [x] **6.2.1** Enhance MonitorConfigPanel
  - Selected monitor info header
  - Current wallpaper update logic
  - Scaling mode dropdown

- [x] **6.2.2** Workspace wallpapers section
  - Implemented per-monitor independent assignment (Default)

### 6.3 Multi-Monitor Modes
- [x] **6.3.1** Complete Independent mode
  - Each monitor fully independent
  - Configured via MonitorsView

- [x] **6.3.2** Implement Clone mode
  - Single wallpaper for all monitors
  - Single settings control
  - Scale appropriately per resolution

- [x] **6.3.3** Implement Span mode
  - Single wallpaper across all monitors
  - Calculate combined resolution
  - Position wallpaper sections correctly
  - Handle different monitor heights/offsets

### 6.4 MonitorsView Page
- [x] **6.4.1** Complete MonitorsView
  - Header with optional mode selector
  - Monitor layout widget
  - Configuration panel integration

---

## Phase 7: Library & Tag Management

### 7.1 Library Database
- [x] **7.1.1** Improve WallpaperLibrary performance
  - Upgraded schema for tags/ratings
  - Serialized to JSON (SQLite deferred)

### 7.2 Advanced Scanning
- [x] **7.2.1** Enhance LibraryScanner
  - Background thread scanning implemented

- [ ] **7.2.2** Implement duplicate detection
  > ⚠️ DEFERRED: Future enhancement

### 7.3 Tagging System
- [x] **7.3.1** Tag Editor Dialog
  - Manage tags for wallpapers
  - Context menu integration

- [x] **7.3.2** Tag Filtering
  - Dropdown filter in LibraryView
  - Favorites toggle filter

### 7.4 Favorites & Rating
- [x] **7.4.1** Complete favorites system
  - Toggle favorite on card
  - Favorites filter logic

- [x] **7.4.2** Implement rating system
  - 1-5 star rating in PreviewPanel
  - Click stars to rate
  - Rating saved to library

### 7.5 Virtual Folders
- [ ] **7.5.1** Implement FolderManager
  > ⚠️ DEFERRED: Future enhancement

---

## Phase 8: Advanced Transitions

> **Priority: MEDIUM** - Nice-to-have visual effects.

### 8.1 Transition Engine Improvements
- [x] **8.1.1** Enhance TransitionEngine
  - Maintain 60fps during transitions
  - Added preload() for next wallpaper before transition
  - Added progress caching for efficiency
  - Added target FPS configuration

- [x] **8.1.2** Implement all easing functions
  - Linear, Ease-in, Ease-out, Ease-in-out (quadratic, cubic, quartic)
  - Sine, Exponential, Circular
  - Bounce, Elastic, Back
  - Custom cubic-bezier support via Easing::cubicBezier()
  - Created `Easing.hpp` utility class with all functions

### 8.2 Additional Transition Effects
- [x] **8.2.1** ExpandingCircleTransition
  - Configurable origin point (setOrigin)
  - Circle expands revealing new wallpaper
  - Smooth antialiased edge via Cairo

- [x] **8.2.2** ExpandingSquareTransition
  - Same as circle but square shape
  - Rounded corners option (setCornerRadius)

- [x] **8.2.3** DissolveTransition
  - Pixelated dissolve effect
  - Deterministic pseudo-random block reveal
  - Configurable block size (setBlockSize)

- [x] **8.2.4** ZoomTransition
  - Zoom in on old wallpaper with fade
  - Switch at midpoint
  - Zoom out with new wallpaper
  - Configurable zoom factor (setZoomFactor)

- [x] **8.2.5** MorphTransition
  - Crossfade with color blending
  - Smooth interpolation between wallpapers

- [x] **8.2.6** WipeTransition (improved) - AngledWipeEffect
  - Configurable angle (setAngle in degrees)
  - Soft edge option with gradient (setSoftEdge, setEdgeWidth)
  - Works with any angle

- [x] **8.2.7** PixelateEffect (bonus)
  - Pixelate out old wallpaper, pixelate in new
  - Configurable max block size

- [x] **8.2.8** BlindsEffect (bonus)
  - Horizontal or vertical blinds
  - Configurable blind count

### 8.3 Transition UI
- [x] **8.3.1** Create TransitionDialog
  - Effect selector dropdown (11 effects)
  - Duration slider (100-3000ms)
  - Easing dropdown with live curve visualizer
  - Full preview with animated test button
  - Apply/Cancel buttons
  - Created `TransitionDialog.hpp` and `TransitionDialog.cpp`

*Phase 8 completed: 2026-01-29 (TransitionDialog integrated into SettingsView)*

---

## Phase 9: Profiles & Scheduling

> **Priority: MEDIUM** - Power user features.

### 9.1 Profiles View
- [x] **9.1.1** Create ProfilesView
  - Profile cards with thumbnail
  - Active profile indicator
  - Create new profile button (FAB)
  - Card actions: Activate, Edit, Duplicate, Export, Delete

### 9.2 Profile Editor
- [ ] **9.2.1** Create ProfileEditDialog
  - Profile name input
  - Profile icon selector
  - Monitor configurations
  - Trigger settings section

- [ ] **9.2.2** Implement profile triggers
  - Time-based: Start time, end time, days of week
  - Monitor: Specific monitor connected, monitor count
  - Power: Plugged in, on battery
  - Application: App running, app focused

### 9.3 Scheduling System
- [ ] **9.3.1** Create Scheduler class
  - Store schedule entries in config
  - Check schedules on timer (every minute)
  - Trigger wallpaper/profile changes
  - Handle overlapping schedules (priority)
  - Timezone awareness

- [ ] **9.3.2** Create ScheduleView
  - 24-hour visual timeline
  - Colored blocks for schedules
  - Click to add schedule
  - Drag to resize duration
  - Alternative list view

- [ ] **9.3.3** Create ScheduleDialog
  - Time range pickers
  - Days of week toggles
  - Action selector (wallpaper or profile)
  - Target selector (wallpaper/profile picker)
  - Repeat options (daily, weekly, monthly)

### 9.4 Slideshow Mode
- [ ] **9.4.1** Create SlideshowManager
  - Enable/disable toggle
  - Interval setting (seconds to hours)
  - Order: Sequential, Random, Shuffle
  - Source: All, Favorites, Folder, Tag
  - Per-monitor option
  - Use TransitionEngine for changes

- [ ] **9.4.2** Slideshow UI
  - Slideshow panel in settings
  - Quick toggle in tray menu
  - Next/Previous controls
  - Status indicator in main window

---

## Phase 10: Steam Workshop Integration

> **Priority: MEDIUM** - Replace mock implementation.

### 10.1 Real Steam API
- [ ] **10.1.1** Implement SteamWorkshopClient with real API
  - API base URL: `https://api.steampowered.com/`
  - Wallpaper Engine App ID: 431960
  - HTTP requests via libcurl with timeout
  - JSON response parsing
  - Error handling with retry logic
  - Rate limiting compliance

- [ ] **10.1.2** Authentication handling
  - Detect Steam session from `~/.steam/`
  - Read Steam login cookies
  - Fallback to anonymous browsing
  - Limited features notice for anonymous

### 10.2 Workshop Search
- [ ] **10.2.1** Implement WorkshopSearch
  - Text query search
  - Pagination (items per page, page number)
  - Sort: Popular, Subscribed, Recent, Trending, Top Rated

- [ ] **10.2.2** Workshop filters
  - Resolution: 1080p, 1440p, 4K, Ultrawide, Any
  - Type: Scene ✓, Video ✓, Web ✗ (disabled)
  - Tags/Categories
  - Rating filter
  - Time filter (all time, year, month, week)

### 10.3 Download Manager
- [ ] **10.3.1** Implement WorkshopDownloader
  - Direct HTTP download (primary)
  - steamcmd fallback
  - Progress tracking with speed/ETA
  - Cancel and resume support
  - Integrity verification

- [ ] **10.3.2** Download queue
  - Queue multiple downloads
  - Sequential or parallel option
  - Priority ordering
  - Pause/resume queue
  - Persistent across restart

### 10.4 Workshop UI
- [ ] **10.4.1** Enhance WorkshopView
  - Real search functionality
  - Collapsible filter panel
  - Results grid with WorkshopCards
  - Pagination controls
  - Loading skeletons

- [ ] **10.4.2** Create WorkshopCard widget
  - Preview image (lazy loaded)
  - Title and author
  - Rating stars
  - Subscription count
  - Type badge (Web disabled with tooltip)
  - Download button overlay

- [ ] **10.4.3** Create WorkshopDetailDialog
  - Large animated preview
  - Full description
  - Author info with link
  - Stats: subs, favorites, rating
  - Tags list
  - File size and dates
  - Download button (or error for Web)

### 10.5 Download Progress UI
- [ ] **10.5.1** Create DownloadProgressWidget
  - Current download info
  - Progress bar with percentage
  - Speed and ETA
  - Cancel button
  - Queue count indicator

- [ ] **10.5.2** Integration
  - Bottom panel during downloads
  - Minimize to notification option
  - Completion notification

---

## Phase 11: Color Extraction & Theming

> **Priority: LOW-MEDIUM** - Aesthetic enhancement.

### 11.1 Color Extraction
- [ ] **11.1.1** Create ColorExtractor
  - K-means clustering for dominant colors
  - Palette sizes: 8, 16, 32
  - Color sorting: luminance, saturation, hue
  - Extract primary, secondary, accent, background

### 11.2 Theme Generation
- [ ] **11.2.1** Create ThemeGenerator
  - Export formats: colors.sh, colors.json, colors.css, colors.scss
  - Template support for custom formats
  - Light/dark mode variations

### 11.3 Theme Application
- [ ] **11.3.1** Create ThemeApplier
  - Detect installed tools: pywal, matugen, wpgtk
  - Auto-select or user preference
  - Apply theme by running tool
  - Reload applications after change
  - Custom script support

### 11.4 Theming UI
- [ ] **11.4.1** Add theming settings panel
  - Enable/disable toggle
  - Tool selection dropdown
  - Color preview palette
  - Manual apply button
  - History of applied themes

---

## Phase 12: Notifications & System Integration

> **Priority: LOW-MEDIUM** - Polish features.

### 12.1 Notification System
- [ ] **12.1.1** Create NotificationManager
  - System notifications via libnotify
  - In-app toast notifications
  - Types: Info, Success, Warning, Error
  - User preference per type

### 12.2 Toast Widget
- [ ] **12.2.1** Create Toast widget
  - Slide in from top
  - Auto-dismiss with timeout
  - Close button
  - Optional action button
  - Queue multiple toasts
  - Smooth animations

### 12.3 Autostart
- [ ] **12.3.1** Implement autostart methods
  - Systemd user service
  - XDG autostart desktop file
  - Hyprland exec-once
  - User preference for method

- [ ] **12.3.2** Autostart UI in Settings
  - Enable/disable toggle
  - Method selector dropdown
  - Start minimized option

### 12.4 Backup & Restore
- [ ] **12.4.1** Create BackupManager
  - Export to ZIP: config, profiles, library metadata, tags
  - Optional: include wallpaper files
  - Import from ZIP with validation
  - Merge or replace options
  - Conflict handling

- [ ] **12.4.2** Backup UI
  - Export/Import buttons with file picker
  - Include wallpapers checkbox
  - Confirmation dialogs
  - Progress indicator

### 12.5 Import Features
- [ ] **12.5.1** Create ImportManager
  - Import from local file (drag-drop or picker)
  - Import from URL (download)
  - Bulk import folder
  - Copy vs reference option

- [ ] **12.5.2** Create ImportDialog
  - Drop zone for drag-drop
  - URL input field
  - Folder picker
  - Options: copy/reference, default tags
  - Progress indicator
  - Results summary

---

## Phase 13: Testing & Quality Assurance

> **Priority: MEDIUM** - Ensure reliability.

### 13.1 Unit Tests
- [ ] **13.1.1** Test core utilities
  - FileUtils tests
  - StringUtils tests
  - Logger tests
  - ConfigManager tests

- [ ] **13.1.2** Test business logic
  - WallpaperLibrary tests
  - TagManager tests
  - ProfileManager tests
  - Scheduler tests

### 13.2 Integration Tests
- [ ] **13.2.1** D-Bus integration tests
  - Daemon communication tests
  - CLI to daemon tests

- [ ] **13.2.2** Wallpaper rendering tests
  - Static image loading
  - Video playback
  - Transition execution

### 13.3 Mock Services
- [ ] **13.3.1** Create test mocks
  - Mock Wayland display
  - Mock Hyprland IPC
  - Mock Steam API
  - Mock D-Bus

### 13.4 Manual Test Plan
- [ ] **13.4.1** Create test checklist
  - All user workflows documented
  - Edge cases identified
  - Multi-monitor scenarios
  - Error handling scenarios

---

## Phase 14: Packaging & Distribution

> **Priority: LOW** - Final step before release.

### 14.1 Desktop Integration Files
- [ ] **14.1.1** Create desktop entry file
  - Standard FreeDesktop format
  - Proper categories and keywords
  - Correct icon reference

- [ ] **14.1.2** Create autostart entry file
  - X-GNOME-Autostart-enabled
  - Hidden=false

- [ ] **14.1.3** Create app icons
  - SVG source
  - Symbolic variant
  - Export all hicolor sizes (16-512)

- [ ] **14.1.4** Create systemd service file
  - User service unit
  - Proper dependencies
  - Restart policy

### 14.2 Arch Linux (AUR)
- [ ] **14.2.1** Create PKGBUILD
  - All dependencies listed
  - Proper build instructions
  - License and metadata

- [ ] **14.2.2** Create PKGBUILD-git
  - Git source version
  - provides/conflicts declarations

- [ ] **14.2.3** Test and publish to AUR

### 14.3 Flatpak
- [ ] **14.3.1** Create Flatpak manifest
  - Proper runtime and SDK
  - Required permissions
  - Module definitions

- [ ] **14.3.2** Create Flathub metadata
  - AppStream metainfo XML
  - Screenshots
  - Release notes

- [ ] **14.3.3** Test and submit to Flathub

### 14.4 AppImage
- [ ] **14.4.1** Create AppImage config
  - AppImageBuilder.yml
  - All dependencies bundled

- [ ] **14.4.2** Desktop integration
  - First-run integration script
  - Update mechanism

- [ ] **14.4.3** Test on multiple distros

### 14.5 Debian/Ubuntu
- [ ] **14.5.1** Create debian control files
  - control, rules, changelog
  - Dependencies

- [ ] **14.5.2** Build and test .deb package

### 14.6 Fedora/RPM
- [ ] **14.6.1** Create RPM spec file
  - All requirements
  - Build instructions

- [ ] **14.6.2** Build and test .rpm package

---

## Phase 15: Normal Image Wallpaper Support

> **Priority: HIGH** - Core feature for non-Steam wallpapers.

### 15.1 Local Image Support
- [x] **15.1.1** Enable setting wallpapers from any directory
  - Support common image formats: PNG, JPG, JPEG, WEBP, BMP
  - Support video formats: MP4, WEBM, MKV, GIF
  - File picker integration for browsing local files
  - Drag-and-drop support for adding wallpapers

- [ ] **15.1.2** Add wallpapers to library from local directories
  - "Add Folder" functionality in sidebar
  - Recursive scanning of subdirectories
  - Real-time folder watching for new files

- [x] **15.1.3** Persist added wallpapers to config
  - Save added wallpapers to config JSON
  - Reload on app restart
  - Remember source (local vs workshop)

- [x] **15.1.4** Fix wallpaper application for normal images
  - Use swaybg as primary method (works on all Wayland)
  - Fallback to hyprctl hyprpaper if swaybg unavailable
  - Fallback to feh for X11 systems
  - Ensure wallpaper actually gets set, not just logged

- [x] **15.1.5** Auto-detect Wallpaper Engine directory
  - Search common Steam library paths on first launch
  - Save found path to config
  - Remove all hardcoded directory paths
  - Only scan user-configured sources

---

## Phase 16: UI Cleanup & Simplification

> **Priority: MEDIUM** - Remove unused features and streamline UI.

### 16.1 Remove Recent Tab
- [x] **16.1.1** Delete Recent tab/view completely
  - Remove RecentView class and files
  - Remove Recent from sidebar navigation
  - Clean up related references in MainWindow/Navigation

### 16.2 Folder Management
- [x] **16.2.1** Ensure folder creation works correctly
  - Create new folders in library
  - Add wallpapers to folders (copy or reference)
  - Folder context menu (rename, delete, slideshow)

- [x] **16.2.2** Folder slideshow integration
  - Right-click folder/Button to Start Slideshow
  - Use SlideshowManager with folder contents
  - Respect slideshow settings (interval, shuffle)

---


## Phase 18: Wallpaper-Specific Settings

> **Priority: HIGH** - Per-wallpaper settings that override defaults.

### 18.1 Wallpaper Properties Detection
- [ ] **18.1.1** Detect wallpaper capabilities
  - Detect if wallpaper has audio (video files, WE scenes with audio)
  - Detect if wallpaper is animated (video, GIF, WE scene)
  - Detect if wallpaper is static (images)
  - Read Wallpaper Engine project.json for metadata

### 18.2 Dynamic Wallpaper Settings UI
- [ ] **18.2.1** Show relevant settings based on wallpaper type
  - Volume slider: Only show if wallpaper has audio
  - FPS setting: Only show if wallpaper is animated
  - Playback speed: Only show for video/animated
  - Scaling mode: Always show
  - Blur/Tint: Always show for background effects

- [ ] **18.2.2** Wallpaper Settings panel in Library
  - Appears under preview when "Wallpaper Settings" clicked
  - Shows only applicable settings for selected wallpaper
  - Settings saved per-wallpaper to config

### 18.3 Per-Wallpaper Settings Storage
- [ ] **18.3.1** Store wallpaper-specific overrides
  - Key by wallpaper path or ID
  - Store: volume, fps, speed, scaling, blur, tint
  - Load overrides when wallpaper is applied

---

## Phase 19: Global Default Settings System

> **Priority: HIGH** - Settings page controls app-wide defaults.

### 19.1 Settings Page Organization
- [ ] **19.1.1** Organize settings into clear sections
  - **Playback**: Volume, FPS, Speed
  - **Display**: Scaling mode, Fill color
  - **Effects**: Blur background, Tint overlay
  - **Behavior**: Pause on battery, Pause on fullscreen
  - **Startup**: Autostart, Start minimized
  - **Theming**: Color extraction, Theme tool

### 19.2 Default Settings Implementation
- [ ] **19.2.1** Implement default values system
  - All settings from Settings page are defaults
  - Defaults apply to all new wallpapers
  - Defaults persist to config file

- [ ] **19.2.2** Wallpaper overrides inherit from defaults
  - Per-wallpaper settings start with default values
  - User changes only affect that specific wallpaper
  - Overrides stored separately from defaults
  - Never overwrite global defaults from per-wallpaper UI

### 19.3 Settings Application
- [ ] **19.3.1** Ensure all settings actually work
  - Volume: Apply to video/audio wallpapers
  - FPS: Cap animation frame rate
  - Speed: Adjust playback speed
  - Scaling: Apply correct scaling mode
  - Pause behavior: Respond to battery/fullscreen

---

## Phase 20: Refine Slideshow & WE Integration
- [ ] **20.1** Fix WE command arguments in slideshow
  - Ensure config settings (FPS, Audio, Scaling) are passed to linux-wallpaperengine
  - Unified renderer logic must respect global/per-wallpaper settings
- [ ] **20.2** Validate Settings Usage
  - Verify "mute" setting actually mutes WE
  - Verify FPS cap works
  - Verify Scaling mode is passed correctly

---
## Success Criteria Checklist

### Core Functionality
- [ ] Static image wallpapers working
- [ ] Video wallpapers working
- [ ] GIF/APNG working
- [ ] Wallpaper Engine Scene working
- [ ] Wallpaper Engine Video working
- [ ] Web wallpapers blocked with clear error
- [ ] All scaling modes working

### Performance
- [ ] Library loads without blocking UI
- [ ] Thumbnails load asynchronously
- [ ] Grid scrolls smoothly
- [ ] Startup time < 2 seconds
- [ ] Memory usage stays reasonable

### Multi-Monitor
- [ ] All monitors detected
- [ ] Visual layout accurate
- [ ] Independent mode working
- [ ] Clone mode working
- [ ] Span mode working

### User Experience
- [ ] All animations smooth 60fps
- [ ] Consistent visual styling
- [ ] Error messages user-friendly
- [ ] Settings complete and working
- [ ] Keyboard navigation works

### Integration
- [ ] Daemon runs as background service
- [ ] CLI commands functional
- [ ] System tray works
- [ ] Autostart works
- [ ] Notifications work

---

*This optimized plan prioritizes critical fixes and performance before features.*
*Mark tasks with [x] as they are completed.*
*Last updated: 2026-01-28*
