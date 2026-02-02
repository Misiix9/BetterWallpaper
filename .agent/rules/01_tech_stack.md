# Technology Stack & Architecture Rules

## Core Frameworks
- **Language**: C++20
- **GUI**: GTK4 + LibAdwaita (via gtkmm-4.0)
- **Build System**: CMake (>3.20)
- **Data**: JSON (nlohmann)

## Cross-Platform Strategy
1.  **Abstraction First**:
    - All system interactions (processes, paths, execution) must NOT use raw `system()` calls or hardcoded paths.
    - Use `std::filesystem` for all path operations.
    - Use `Glib::spawn_async` or `GSubprocess` for executing external commands, wrapped in a cross-platform helper class.

2.  **Wallpaper Engine Abstraction**:
    - Wallpaper logic must be abstracted behind the `IWallpaperSetter` interface.
    - `NativeWallpaperSetter` should be a factory that returns the OS-specific implementation (`WindowsSetter`, `HyprlandSetter`, `MacSetter`).

3.  **Path Resolution**:
    - Use dynamic path finding for assets.
    - Linux: `XDG_DATA_DIRS`, `/usr/share/betterwallpaper`
    - Windows: `AppData/Local`, Execution Directory.
    - MacOS: App Bundle Resources.
