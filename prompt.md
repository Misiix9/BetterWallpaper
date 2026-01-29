# BetterWallpaper - AI Development Prompt

> **Purpose**: This prompt provides a comprehensive overview of the BetterWallpaper project for AI assistants to understand the codebase, identify areas for improvement, and implement enhancements.

---

## 📋 FIRST TASK: Create `plan2.md`

**Before doing anything else**, create a new file called `plan2.md` in the project root. This file should:

1. **Follow the exact structure** of the existing `plan.md` file (phases, numbered tasks, checkboxes)
2. **Include ALL unfinished tasks** from `plan.md` (marked with `[ ]`), but **completely rethought and restructured**
3. **Add NEW optimizations and improvements** not covered in the original plan
4. **Focus on these key areas**:
   - Visual enhancements (animations, transitions, styles, modern UI patterns)
   - Performance optimizations (startup speed, loading times, memory usage)
   - Code quality improvements (better abstractions, error handling, testing)
   - User experience improvements (smoother interactions, better feedback)
   - Feature completions and enhancements

---

## 🏗️ Project Overview

### Technology Stack
| Component | Technology | Version |
|-----------|------------|---------|
| Language | C++ | C++20 |
| GUI Framework | GTK4 + libadwaita | 4.10+ / 1.4+ |
| Build System | CMake | 3.20+ |
| Video Playback | libmpv | Latest |
| Layer Shell | gtk4-layer-shell | Latest |
| JSON | nlohmann/json | 3.11+ |
| HTTP | libcurl | 7.0+ |

### Project Structure
```
BetterWallpaper/
├── src/
│   ├── core/                    # Shared library code
│   │   ├── config/              # ConfigManager, ProfileManager, SettingsSchema
│   │   ├── hyprland/            # HyprlandIPC, HyprlandManager  
│   │   ├── ipc/                 # DBusClient, DBusService
│   │   ├── monitor/             # MonitorInfo, MonitorManager, WaylandMonitor
│   │   ├── steam/               # SteamWorkshopClient
│   │   ├── transition/          # TransitionEngine, effects/
│   │   ├── utils/               # Logger, FileUtils, StringUtils
│   │   └── wallpaper/           # WallpaperManager, LibraryScanner, renderers/
│   ├── gui/                     # GTK4 Application
│   │   ├── views/               # LibraryView, MonitorsView, SettingsView, etc.
│   │   ├── widgets/             # Sidebar, WallpaperCard, WallpaperGrid, etc.
│   │   ├── dialogs/             # TagDialog
│   │   ├── Application.cpp
│   │   └── MainWindow.cpp
│   ├── daemon/                  # Background daemon (STUB - needs implementation)
│   ├── cli/                     # Command-line tool (STUB - needs implementation)
│   └── tray/                    # System tray (STUB - needs implementation)
├── data/
│   └── ui/style.css             # Application styling
├── plan.md                      # Original implementation plan (1783 lines)
└── CMakeLists.txt               # Build configuration
```

---

## ✅ Currently Implemented Features

### Core
- [x] Logger system with colored output
- [x] FileUtils (path expansion, file operations)
- [x] StringUtils (case conversion, trimming)
- [x] ConfigManager (JSON-based configuration)
- [x] ProfileManager (basic structure)
- [x] MonitorManager (Wayland monitor detection)
- [x] DBusService/DBusClient (basic structure)
- [x] TransitionEngine with FadeEffect, SlideEffect, WipeEffect

### GUI
- [x] MainWindow with AdwOverlaySplitView
- [x] Sidebar navigation (Library, Favorites, Recent, Workshop, Folders, Monitors, Settings)
- [x] LibraryView with SearchBar, WallpaperGrid, PreviewPanel
- [x] WallpaperCard with thumbnail, type badge, favorite button
- [x] WallpaperGrid with FlowBox (4-12 columns)
- [x] PreviewPanel with monitor selector and settings controls
- [x] MonitorsView with MonitorLayout and MonitorConfigPanel
- [x] SettingsView with Library Sources, General, Appearance, Performance groups
- [x] WorkshopView with Installed/Browse tabs
- [x] FavoritesView, RecentView, FolderView (basic)
- [x] TagDialog for managing wallpaper tags

### Wallpaper Rendering
- [x] WallpaperWindow using gtk4-layer-shell
- [x] StaticRenderer for images
- [x] VideoRenderer using libmpv
- [x] WallpaperEngineRenderer (spawns linux-wallpaperengine process)
- [x] LibraryScanner (scans Steam Workshop and local paths)
- [x] WallpaperLibrary (in-memory storage)

### Hyprland Integration
- [x] HyprlandIPC (socket connection)
- [x] HyprlandManager (workspace event handling, basic workspace wallpapers)

### Steam Workshop
- [x] SteamWorkshopClient (MOCK implementation - returns fake data)
- [x] Download via steamcmd (basic implementation)

---

## ❌ Incomplete/Stub Features (FROM plan.md)

### Daemon & CLI (Phase 11-12) - COMPLETELY STUB
- The `daemon/main.cpp` only prints "BetterWallpaper Daemon starting..."
- The `cli/main.cpp` only prints "BetterWallpaper CLI (bwp)"
- No actual functionality implemented

### System Tray (Phase 11)
- `tray/main.cpp` is a stub with no implementation
- No TrayIcon or TrayMenu classes exist

### Settings View Panels (Phase 4 - Section 5.7)
- [ ] **5.7.2** GeneralSettings - mostly incomplete
- [ ] **5.7.3** AppearanceSettings - only theme info row
- [ ] **5.7.4** PerformanceSettings - only FPS limit dropdown
- [ ] **5.7.5** NotificationSettings - not implemented
- [ ] **5.7.6** TransitionSettings - not implemented
- [ ] **5.7.7** ShortcutsSettings - not implemented
- [ ] **5.7.8** ThemingSettings - not implemented
- [ ] **5.7.9** BackupSettings - not implemented
- [ ] **5.7.10** AboutPanel - not implemented

### Multi-Monitor Modes (Phase 5)
- [ ] **6.3.2** Clone mode
- [ ] **6.3.3** Span mode (single wallpaper across all monitors)

### Transition Effects (Phase 7)
- [ ] **8.3.1** ExpandingCircleTransition
- [ ] **8.3.2** ExpandingSquareTransition
- [ ] **8.3.5** DissolveTransition
- [ ] **8.3.6** ZoomTransition
- [ ] **8.3.7** MorphTransition
- [ ] **8.4.2** TransitionDialog (preview and configure transitions)

### Profiles System (Phase 10)
- [ ] **11.1.1** ProfilesView
- [ ] **11.1.2** ProfileEditDialog
- [ ] **11.1.3** Profile triggers (time, monitor, power, application)

### Scheduling (Phase 10)
- [ ] **11.2.1** Scheduler class
- [ ] **11.2.2** ScheduleView (visual timeline)
- [ ] **11.2.3** ScheduleDialog

### Slideshow Mode (Phase 10)
- [ ] **11.3.1** SlideshowManager
- [ ] **11.3.2** Slideshow controls

### Color Extraction & Theming (Phase 10)
- [ ] **11.4.1** ColorExtractor (k-means clustering)
- [ ] **11.4.2** ThemeGenerator
- [ ] **11.4.3** ThemeApplier (pywal, matugen integration)

### Notifications (Phase 11)
- [ ] **12.2.1** NotificationManager
- [ ] **12.2.2** Toast widget

### Autostart (Phase 11)
- [ ] **12.3.1** Autostart methods (systemd, XDG, Hyprland)
- [ ] **12.3.2** Autostart UI

### Backup & Restore (Phase 11)
- [ ] **12.4.1** BackupManager
- [ ] **12.4.2** Backup UI

### Import Features (Phase 11)
- [ ] **12.5.1** ImportManager
- [ ] **12.5.2** ImportDialog

### Packaging (Phase 12)
- [ ] **13.1.x** Desktop integration files
- [ ] **13.2.x** Arch Linux PKGBUILD
- [ ] **13.3.x** Flatpak manifest
- [ ] **13.4.x** AppImage config
- [ ] **13.5.x** Debian package
- [ ] **13.6.x** Fedora RPM spec

---

## 🐛 Known Bugs & Issues

### Critical
1. **CSS Loading**: `Application.cpp` uses relative path `"data/ui/style.css"` which may fail
2. **Steam Workshop**: Uses MOCK data, not real Steam API
3. **Daemon Not Functional**: All wallpaper operations happen in GUI process

### Performance  
1. **Library Loading**: Blocks UI during initial scan
2. **Thumbnail Loading**: Synchronous, causes stuttering
3. **No Lazy Loading**: All wallpapers loaded at once in grid
4. **No Virtual Scrolling**: Memory grows with library size

### UX Issues
1. **Window Not Resizable**: Set to `gtk_window_set_resizable(FALSE)` 
2. **No Loading Indicators**: No skeleton loaders or spinners
3. **No Error Handling UI**: Errors only logged, not shown to user
4. **Workshop Search**: Blocks on search, returns mock data

### Code Quality
1. **Memory Leaks**: Some GTK callbacks use `new` without proper cleanup
2. **Inconsistent Error Handling**: Some functions return bool, others throw
3. **No Unit Tests**: Test directory exists but no actual tests
4. **Hardcoded Paths**: Steam paths hardcoded in LibraryScanner

---

## 🎨 Visual Enhancement Opportunities

### Animations to Add
1. **Page Transitions**: Crossfade/slide when switching sidebar views
2. **Card Hover**: Subtle lift animation with shadow
3. **Grid Loading**: Staggered fade-in for wallpaper cards
4. **Preview Panel**: Smooth slide-in when wallpaper selected
5. **Sidebar Selection**: Animated indicator sliding between items
6. **Button Press**: Micro-animations for feedback
7. **Toast Notifications**: Slide-in/out animations
8. **Modal Dialogs**: Scale + fade entrance/exit

### Style Improvements
1. **Glassmorphism**: Frosted glass effects for panels
2. **Gradient Overlays**: Subtle gradients on card images
3. **Better Typography**: Variable font weights, letter-spacing
4. **Color Palette**: Refined accent colors, better dark mode contrast
5. **Shadows**: Layered shadows for depth hierarchy
6. **Border Radius**: Consistent radius scale (4, 8, 12, 16, 24)
7. **Icon Consistency**: Ensure all icons use same style/weight

### Layout Improvements
1. **Responsive Grid**: Adjust columns based on content width
2. **Masonry Layout**: Consider for varied aspect ratios
3. **Collapsible Sidebar**: Toggle to maximize grid space
4. **Floating Preview**: Option to position preview differently
5. **Full-Screen Preview**: Lightbox mode for selected wallpaper

---

## ⚡ Performance Optimization Opportunities

### Startup Speed
1. **Lazy Initialization**: Don't scan library until view accessed
2. **Config Caching**: Preload config during splash/startup
3. **Parallel Loading**: Load components concurrently
4. **Deferred Rendering**: Don't render invisible views

### Loading Times
1. **Thumbnail Cache**: Pre-generate and cache thumbnails on disk
2. **Progressive Loading**: Load visible thumbnails first
3. **Background Scanning**: Don't block UI during library scan
4. **Async Everything**: All file I/O should be async

### Memory Usage
1. **Virtual Scrolling**: Only render visible grid items
2. **Texture Pooling**: Reuse GdkTexture objects
3. **Thumbnail Eviction**: LRU cache for thumbnails
4. **Smart Unloading**: Unload off-screen resources

### Rendering Performance
1. **Hardware Acceleration**: Ensure GPU compositing enabled
2. **Animation Optimization**: Use GPU-accelerated transforms
3. **Reduce Redraws**: Only redraw changed regions
4. **FPS Limiting**: Configurable frame rate cap

---

## 📝 Implementation Guidelines

### Code Style
- Use C++20 features (concepts, ranges, coroutines where appropriate)
- Follow existing namespace pattern: `bwp::<module>`
- Use snake_case for filenames, PascalCase for classes
- Document public APIs with Doxygen-style comments

### Error Handling
- Return `std::expected<T, Error>` or `std::optional<T>` instead of exceptions
- Log all errors with appropriate level (ERROR, WARNING)
- Show user-friendly error messages in UI

### Testing
- Add unit tests for core utilities
- Add integration tests for major workflows
- Mock external dependencies (Steam, D-Bus)

### UI/UX Principles
- Follow GNOME HIG (Human Interface Guidelines)
- Use libadwaita widgets and patterns
- Provide keyboard shortcuts for all actions
- Show loading states for async operations
- Confirm destructive actions

---

## 🎯 Priority Order for plan2.md

When creating `plan2.md`, prioritize in this order:

1. **Critical Bug Fixes** - CSS loading, error handling
2. **Performance Foundations** - Async loading, thumbnail caching
3. **Core Missing Features** - Daemon, CLI, System Tray
4. **Visual Polish** - Animations, refined styling
5. **Feature Completions** - Settings panels, profiles
6. **Advanced Features** - Scheduling, color extraction
7. **Packaging & Distribution** - AUR, Flatpak

---

## 📎 Reference Files

When implementing changes, refer to these key files:

| Purpose | File |
|---------|------|
| Main entry | `src/gui/main.cpp` |
| App setup | `src/gui/Application.cpp` |
| Window layout | `src/gui/MainWindow.cpp` |
| Styling | `data/ui/style.css` |
| Config | `src/core/config/ConfigManager.cpp` |
| Library scan | `src/core/wallpaper/LibraryScanner.cpp` |
| Wallpaper apply | `src/gui/widgets/PreviewPanel.cpp` |
| Build | `CMakeLists.txt` |
| Original plan | `plan.md` |

---

## 🚀 Getting Started

1. Read through this entire prompt
2. Create `plan2.md` following the structure outlined above
3. Begin with Phase 1 items from `plan2.md`
4. Test each change before moving to the next
5. Update `plan2.md` checkboxes as you complete items
6. Run `cmake --build build` to verify compilation after changes

**Good luck improving BetterWallpaper!** 🖼️

//////////////////////////////////////////////////////////////////////////////

📋 Working with plan2.md - AI Instructions
Overview
You will be implementing features from plan2.md in the BetterWallpaper project. This file contains a structured implementation plan with phases, sections, and individual tasks. Follow these guidelines to work through it systematically.

✅ When to Mark a Task as Done [x]
Mark a task as complete [x] when ALL of the following are true:

Code is written - The implementation matches the task description
Code compiles - Run cmake --build build with no errors
Basic testing - The feature works as described (test it yourself)
No regressions - Existing features still work properly
Do NOT mark as done if:

Code is partially written but not functional
There are compiler errors or warnings related to the change
The feature has known bugs that need fixing
🔄 Task Workflow
For each individual task:
1. Read the task description carefully
2. Identify which files need to be created/modified
3. Implement the feature
4. Compile and test
5. Fix any issues
6. Mark as [x] in plan2.md
7. Move to next task
Marking progress in plan2.md:
markdown
- [ ] Uncompleted task
- [/] Currently working on (in progress)
- [x] Completed and verified
📦 Completing a Phase
When you finish ALL tasks in a phase:

Review the phase - Go through each [x] and verify nothing was missed
Run full build - cmake --build build --clean-first
Test the phase features - Launch the app and verify functionality
Commit point - This is a good time to suggest the user commits changes
Update summary - Add a brief note at the end of the phase: *Phase X completed: [date]*
Move to next phase - Begin the next phase in order
🚦 Phase Dependencies
Some phases depend on others. Follow this order:

Phase 1: Critical Fixes → Must be done first
Phase 2: Performance Foundation → Before visual work
Phase 3: Core Infrastructure → Daemon/CLI needed for later features
Phase 4: Visual Polish → Can be done after core is stable
Phase 5+: Features → Build on stable foundation
Exception: If a phase is blocked (e.g., waiting for external dependency), you may skip to the next phase and note the blocker.

🛠️ Implementation Guidelines
Before modifying any file:
Read the existing code to understand the pattern
Check if similar functionality exists that you can extend
Follow the existing code style (namespaces, naming conventions)
When creating new files:
Add to appropriate 
CMakeLists.txt
Follow existing file structure (header + implementation)
Use the namespace pattern: bwp::<module>
When modifying existing files:
Minimize changes to unrelated code
Keep functions focused and small
Add comments for complex logic
⚠️ Handling Blockers
If a task is blocked or impossible:

Add a note under the task explaining why:
markdown
- [ ] **8.3.1** Create ExpandingCircleTransition
  > ⚠️ BLOCKED: Requires shader support not available in Cairo
Continue with the next task
Come back when the blocker is resolved
💬 Communication Points
Ask the user before:

Deleting files or removing features
Changing build configuration significantly
Installing new dependencies
Making breaking API changes
Inform the user after:

Completing a phase
Encountering a blocker
Finding bugs in existing code
Finishing the entire plan
📝 Example Workflow
markdown
Starting Phase 4: Visual Polish
Task 4.1.1 - Page transition animations
→ Identify files: MainWindow.cpp, style.css
→ Implement crossfade on view switch
→ Build: cmake --build build ✓
→ Test: Switch between Library/Monitor views ✓
→ Mark [x] in plan2.md
Task 4.1.2 - Card hover animations  
→ Identify files: WallpaperCard.cpp, style.css
→ Add CSS transition for hover state
→ Build: ✓
→ Test: Hover over cards ✓
→ Mark [x] in plan2.md
... continue until phase complete ...
Phase 4 complete! Suggesting user to test and commit.
Moving to Phase 5...
🎯 Success Criteria
The plan is complete when:

All tasks are marked [x] or have documented blockers
The application builds without warnings
All features work as described
The user has reviewed and accepted the changes
Good luck implementing BetterWallpaper! 🚀