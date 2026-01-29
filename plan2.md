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

- [ ] **1.2.2** Add error display UI
  > ⚠️ DEFERRED: Depends on core UI refactoring in Phase 4
  - Create `ErrorBanner` widget for inline errors
  - Create `ErrorDialog` for critical errors
  - Add error indicator in status bar

- [ ] **1.2.3** Improve logging robustness
  > ⚠️ DEFERRED: Lower priority, existing logging is functional
  - Fix potential log file rotation issues
  - Add crash log capture
  - Add debug mode toggle in settings

### 1.3 Window Resizability
- [x] **1.3.1** Enable window resizing
  - Remove `gtk_window_set_resizable(FALSE)` from MainWindow
  - Add minimum size constraints (800x600)
  - Save/restore window size and position
  - Handle responsive layout on resize

### 1.4 Memory Safety Audit
- [ ] **1.4.1** Fix memory leaks in GTK callbacks
  > ⚠️ DEFERRED: Requires codebase-wide audit, GtkPtr wrappers now available
  - Audit all `new` allocations in signal handlers
  - Replace raw pointers with smart pointers where possible
  - Use proper G_OBJECT reference counting

- [x] **1.4.2** Implement RAII wrappers for GTK objects
  - Create `GtkPtr<T>` smart pointer template
  - Ensure proper cleanup on widget destruction

*Phase 1 completed: 2026-01-28 - Core fixes done, some UI tasks deferred to Phase 4*

---

## Phase 2: Performance Optimization

> **Priority: HIGH** - Critical for smooth user experience.

### 2.1 Async Library Loading
- [x] **2.1.1** Make library scanning non-blocking
  - Move `LibraryScanner` to background thread
  - Use `GTask` or `std::async` for async operations
  - Show skeleton loaders during scan
  - Emit progress signals for UI updates

- [ ] **2.1.2** Implement incremental loading
  - Load library metadata first (fast)
  - Defer thumbnail loading until visible
  - Cache library index in SQLite for instant startup

### 2.2 Thumbnail System Overhaul
- [x] **2.2.1** Implement persistent thumbnail cache
  - Cache location: `~/.cache/betterwallpaper/thumbnails/`
  - Use wallpaper hash as cache key
  - Store as WebP for smaller size
  - Add cache size limit with LRU eviction

- [x] **2.2.2** Async thumbnail loading
  - [x] Load thumbnails in worker thread pool
  - [x] Priority queue: visible items first (Implicit in GTK main loop handling)
  - [x] Cancel loading for scrolled-away items (Handled by cache)
  - [x] Placeholder image while loading

- [ ] **2.2.3** Thumbnail pregeneration
  - Generate thumbnails on library scan
  - Background regeneration for missing thumbnails
  - Multiple sizes: 128, 256, 512 for different views

### 2.3 Virtual Scrolling for Grid
- [x] **2.3.1** Replace FlowBox with virtualized container
  - Only render visible grid items + buffer
  - Recycle card widgets as user scrolls
  - Maintain scroll position on data changes

- [x] **2.3.2** Implement efficient data model
  - Use `GListModel` for wallpaper list
  - Support filtering without full re-render
  - Support sorting without full re-render

### 2.4 Lazy View Initialization
- [x] **2.4.1** Defer view creation until accessed
  - Don't create SettingsView until user clicks Settings
  - Don't create WorkshopView until user clicks Workshop
  - Use factory pattern for view instantiation

- [ ] **2.4.2** Implement view caching
  - Keep views in memory after first creation
  - Optional: Unload views after timeout

### 2.5 Startup Optimization
- [x] **2.5.1** Profile and optimize startup time
  - Measure time for each initialization step
  - Parallelize independent operations
  - Defer non-critical initialization

- [ ] **2.5.2** Implement splash screen (optional)
  - Show loading progress for slow startups
  - Display recent wallpaper as splash background

---

## Phase 3: Core Services (Daemon, CLI, Tray)

> **Priority: HIGH** - Essential for proper architecture.

### 3.1 Daemon Implementation
- [x] **3.1.1** Create daemon architecture
  - Single-instance enforcement via lockfile/socket
  - Proper daemonization (fork, setsid)
  - Signal handling (SIGTERM, SIGHUP for reload)
  - Clean shutdown with resource cleanup

- [x] **3.1.2** Implement core daemon loop
  - GMainLoop for event processing
  - D-Bus service registration
  - Wayland connection management
  - Hyprland IPC listener

- [x] **3.1.3** Move wallpaper rendering to daemon
  - Create WallpaperWindow instances
  - Manage per-monitor renderers
  - Handle monitor connect/disconnect
  - Support hot-reload of wallpapers

- [x] **3.1.4** Implement daemon-GUI communication
  - GUI connects to daemon via D-Bus
  - Daemon emits status updates to GUI
  - Handle daemon restart gracefully

### 3.2 CLI Implementation
- [x] **3.2.1** Create CLI argument parser
  - Use argparse or similar library
  - Support all commands from CLI Reference
  - JSON output option for scripting
  - Shell completion scripts (bash, zsh, fish)

- [x] **3.2.2** Implement wallpaper commands
  - `bwp set <path> [-m monitor] [-w workspace]`
  - `bwp next`, `bwp prev`, `bwp random`
  - `bwp pause`, `bwp resume`, `bwp toggle`
  - `bwp status` - show current state

- [ ] **3.2.3** Implement library commands
  - `bwp library scan` - rescan library
  - `bwp library list` - list wallpapers
  - `bwp library search <query>`
  - `bwp library add <path>`

- [ ] **3.2.4** Implement monitor commands
  - `bwp monitors` - list all monitors
  - `bwp monitor <name> info`
  - `bwp monitor <name> set <path>`

- [ ] **3.2.5** Implement profile commands
  - `bwp profiles` - list profiles
  - `bwp profile <name>` - activate
  - `bwp profile create/delete <name>`

### 3.3 System Tray Implementation
- [x] **3.3.1** Create TrayIcon using StatusNotifierItem
  - App icon in system tray
  - Tooltip showing current wallpaper
  - Support different tray protocols (SNI, XEmbed fallback)

- [x] **3.3.2** Create TrayMenu
  - Current wallpaper thumbnail + name
  - Pause/Resume toggle
  - Next/Previous (if slideshow)
  - Quick profile switcher submenu
  - Open main window
  - Quit application

- [ ] **3.3.3** Tray preferences
  - Option to minimize to tray
  - Option to start minimized
  - Close button behavior (minimize/quit)

---

## Phase 4: Visual Polish & Animations

> **Priority: MEDIUM-HIGH** - Makes the app feel premium.

### 4.1 Page Transitions
- [ ] **4.1.1** Add crossfade between views
  - Fade out current view, fade in new view
  - Duration: 200-300ms
  - Use CSS transitions or AdwNavigationView

- [ ] **4.1.2** Add slide transitions for sub-pages
  - Slide from right for drill-down
  - Slide from left for back navigation

### 4.2 Card Hover Effects
- [ ] **4.2.1** Enhance WallpaperCard hover
  - Subtle scale transform (1.02x)
  - Elevated shadow on hover
  - Smooth transition (150ms ease-out)
  - Border highlight with accent color

- [ ] **4.2.2** Add card press feedback
  - Scale down slightly on press (0.98x)
  - Ripple effect on click (optional)

### 4.3 Grid Loading Animations
- [ ] **4.3.1** Staggered card fade-in
  - Cards fade in sequentially on load
  - 50ms delay between cards
  - Use intersection observer pattern

- [ ] **4.3.2** Skeleton loading states
  - Show placeholder cards during load
  - Pulsing animation on placeholders
  - Transition smoothly to real content

### 4.4 Preview Panel Animations
- [ ] **4.4.1** Slide-in animation on selection
  - Panel slides in from right
  - Content fades in after slide
  - Crossfade when changing selection

- [ ] **4.4.2** Image transition effects
  - Crossfade between preview images
  - Optional zoom reveal effect

### 4.5 Sidebar Enhancements
- [ ] **4.5.1** Animated selection indicator
  - Selection bar slides between items
  - Smooth 200ms transition
  - Background highlight follows along

- [ ] **4.5.2** Hover animations
  - Background color transition
  - Icon scale/color change

### 4.6 CSS Styling Improvements
- [ ] **4.6.1** Refine color palette
  - Improved dark mode contrast
  - Subtle accent color variations
  - Better distinction between states

- [ ] **4.6.2** Consistent spacing scale
  - Define spacing tokens (4, 8, 12, 16, 24, 32)
  - Apply consistently across all widgets

- [ ] **4.6.3** Consistent border radius
  - Define radius scale (4, 8, 12, 16, 24)
  - Cards: 12px, Buttons: 8px, Inputs: 6px

- [ ] **4.6.4** Layered shadows
  - Subtle shadows for depth hierarchy
  - Elevated shadows for modals/popovers

- [ ] **4.6.5** Typography refinement
  - Use variable font weights
  - Improve letter-spacing
  - Better text hierarchy (size/weight/color)

### 4.7 Micro-Interactions
- [ ] **4.7.1** Button feedback
  - Scale animation on press
  - Color transition on hover/active
  - Loading spinner for async actions

- [ ] **4.7.2** Toggle switch animations
  - Smooth slide animation
  - Color transition for on/off

- [ ] **4.7.3** Dropdown animations
  - Fade/slide in dropdown content
  - Arrow rotation animation

---

## Phase 5: Settings & Configuration UI

> **Priority: MEDIUM** - Complete the settings panels.

### 5.1 Settings View Structure
- [ ] **5.1.1** Implement SettingsView with sidebar navigation
  - General, Appearance, Performance, Notifications
  - Transitions, Shortcuts, Theming, Backup, About
  - Use AdwNavigationSplitView or similar

### 5.2 General Settings Panel
- [ ] **5.2.1** Implement GeneralSettings
  - Start on boot toggle + method dropdown
  - Start minimized to tray toggle
  - Close button behavior dropdown
  - Default audio toggle (enabled/muted)
  - Default loop toggle
  - Duplicate handling dropdown

- [ ] **5.2.2** Library paths management
  - List of configured paths
  - Add path button (folder picker)
  - Remove path button
  - Edit path (inline or dialog)
  - Scan status indicator per path

### 5.3 Appearance Settings Panel
- [ ] **5.3.1** Implement AppearanceSettings
  - Theme follows system (info text)
  - Enable blur toggle (Hyprland)
  - Grid card size slider (small/medium/large)
  - Show type badges toggle
  - Animation speed dropdown (Fast/Normal/Slow)
  - Preview panel position dropdown

### 5.4 Performance Settings Panel
- [ ] **5.4.1** Implement PerformanceSettings
  - FPS limit dropdown (15/30/60/120/Unlimited)
  - Pause on battery toggle
  - Pause on fullscreen toggle
  - Fullscreen exceptions list (add/remove apps)
  - Max memory usage slider
  - Max VRAM usage slider
  - GPU acceleration toggle

### 5.5 Notification Settings Panel
- [ ] **5.5.1** Implement NotificationSettings
  - Enable notifications master toggle
  - System notifications toggle
  - In-app toasts toggle
  - Notify on wallpaper change toggle
  - Notify on schedule change toggle
  - Notify on error toggle

### 5.6 Transition Settings Panel
- [ ] **5.6.1** Implement TransitionSettings
  - Enable transitions toggle
  - Default effect dropdown with preview
  - Duration slider (100ms - 5000ms)
  - Easing dropdown preview
  - Preview/test button

### 5.7 Shortcuts Settings Panel
- [ ] **5.7.1** Implement ShortcutsSettings
  - List of actions with current keybindings
  - Click to edit binding
  - Conflict detection
  - Reset to defaults button
  - Available actions: Next, Previous, Pause/Resume, Open window

### 5.8 Theming Settings Panel
- [ ] **5.8.1** Implement ThemingSettings
  - Enable color extraction toggle
  - Auto-apply on wallpaper change toggle
  - Theming tool dropdown (Auto/pywal/matugen/wpgtk/Custom)
  - Palette size dropdown (8/16/32)
  - Custom script path input
  - Manual apply button
  - Color preview palette widget

### 5.9 Backup Settings Panel
- [ ] **5.9.1** Implement BackupSettings
  - Export configuration button → file picker
  - Import configuration button → file picker
  - Include wallpaper files checkbox
  - Last backup info text
  - Restore defaults button (with confirmation)

### 5.10 About Panel
- [ ] **5.10.1** Implement AboutPanel
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
- [ ] **6.1.1** Improve MonitorLayout rendering
  - Accurate proportional sizing based on resolution
  - Correct physical positioning
  - Current wallpaper thumbnail in each monitor
  - Selection highlight with animation

- [ ] **6.1.2** Monitor interaction
  - Click to select monitor
  - Hover tooltip with full specs
  - Double-click to configure
  - Right-click context menu

### 6.2 Monitor Configuration Panel
- [ ] **6.2.1** Enhance MonitorConfigPanel
  - Selected monitor info header
  - Current wallpaper with change button
  - Scaling mode dropdown
  - Playback speed slider (0.25x - 4x)
  - Volume slider with mute toggle

- [ ] **6.2.2** Workspace wallpapers section
  - Grid of workspace numbers (1-10+)
  - Thumbnail preview per workspace
  - Click to assign wallpaper
  - Right-click to clear
  - Special workspace toggle + config

### 6.3 Multi-Monitor Modes
- [ ] **6.3.1** Complete Independent mode
  - Each monitor fully independent
  - Save per-monitor in profile
  - Different settings per monitor

- [ ] **6.3.2** Implement Clone mode
  - Single wallpaper for all monitors
  - Single settings control
  - Scale appropriately per resolution

- [ ] **6.3.3** Implement Span mode
  - Single wallpaper across all monitors
  - Calculate combined resolution
  - Position wallpaper sections correctly
  - Handle different monitor heights/offsets

### 6.4 MonitorsView Page
- [ ] **6.4.1** Complete MonitorsView
  - Header with mode selector (radio buttons)
  - Monitor layout widget (large, prominent)
  - Configuration panel for selected monitor
  - Apply/reset buttons

---

## Phase 7: Library & Tag Management

> **Priority: MEDIUM** - Enhanced organization features.

### 7.1 Library Database
- [ ] **7.1.1** Improve WallpaperLibrary performance
  - Use SQLite instead of JSON for large libraries
  - Index frequently searched fields
  - Implement efficient full-text search
  - Debounced auto-save

### 7.2 Advanced Scanning
- [ ] **7.2.1** Enhance LibraryScanner
  - Background thread scanning
  - Progress reporting (files scanned, ETA)
  - Throttling to prevent system slowdown
  - Cancel scan functionality

- [ ] **7.2.2** Implement duplicate detection
  - Detect by file hash (SHA256)
  - User preference for handling:
    - Keep first found
    - Keep newest
    - Keep highest resolution
    - Ask each time
  - Duplicate consolidation dialog

- [ ] **7.2.3** Handle missing files
  - Detect on library load
  - Mark as missing (distinct styling)
  - Auto-remove after X attempts
  - Notification to user

### 7.3 Tagging System
- [ ] **7.3.1** Enhanced TagManager
  - Global tag registry with usage counts
  - Tag colors (user-defined)
  - Tag auto-suggestions
  - Bulk tag operations

- [ ] **7.3.2** Improve TagDialog
  - Current tags with remove buttons
  - Tag input with autocomplete dropdown
  - Existing tags list (click to add)
  - Create new tag inline
  - Color picker for new tags

- [ ] **7.3.3** Tag-based filtering
  - Multiple tag filter (AND/OR logic)
  - Exclude tags filter
  - Tag cloud visualization

### 7.4 Favorites & Rating
- [ ] **7.4.1** Complete favorites system
  - Toggle favorite on card with animation
  - Heart icon in grid and preview
  - Favorites count in sidebar badge
  - Synced favorites indication

- [ ] **7.4.2** Implement rating system
  - 1-5 star rating on cards
  - Click stars to rate
  - Filter by minimum rating
  - Sort by rating

### 7.5 Virtual Folders
- [ ] **7.5.1** Implement FolderManager
  - Create/rename/delete virtual folders
  - Add wallpaper to multiple folders
  - Folder icons (user selectable)
  - Nested folders support

- [ ] **7.5.2** FolderView implementation
  - Folder tree in sidebar (expandable)
  - Folder contents in grid
  - Drag-drop to organize
  - Right-click folder context menu

---

## Phase 8: Advanced Transitions

> **Priority: MEDIUM** - Nice-to-have visual effects.

### 8.1 Transition Engine Improvements
- [ ] **8.1.1** Enhance TransitionEngine
  - Maintain 60fps during transitions
  - GPU-accelerated compositing
  - Preload next wallpaper before transition
  - Cache rendered frames if needed

- [ ] **8.1.2** Implement all easing functions
  - Linear, Ease-in, Ease-out, Ease-in-out
  - Bounce, Elastic, Back
  - Custom cubic-bezier support

### 8.2 Additional Transition Effects
- [ ] **8.2.1** ExpandingCircleTransition
  - Random or configured origin point
  - Circle expands revealing new wallpaper
  - Smooth antialiased edge

- [ ] **8.2.2** ExpandingSquareTransition
  - Same as circle but square shape
  - Rounded corners option

- [ ] **8.2.3** DissolveTransition
  - Pixelated dissolve effect
  - Random pixel reveal order
  - Configurable block size

- [ ] **8.2.4** ZoomTransition
  - Zoom in on old wallpaper
  - Fade to new at zoom peak
  - Zoom out with new wallpaper

- [ ] **8.2.5** MorphTransition
  - Abstract color morphing
  - Smooth color interpolation
  - Blend colors between wallpapers

- [ ] **8.2.6** WipeTransition (improved)
  - Configurable angle (0°, 45°, 90°, etc.)
  - Sharp or gradient edge option
  - Customizable wipe direction

### 8.3 Transition UI
- [ ] **8.3.1** Create TransitionDialog
  - Effect selector with mini preview
  - Duration slider with live feedback
  - Easing dropdown with visualizer
  - Full preview with test button
  - Apply/Cancel buttons

---

## Phase 9: Profiles & Scheduling

> **Priority: MEDIUM** - Power user features.

### 9.1 Profiles View
- [ ] **9.1.1** Create ProfilesView
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
