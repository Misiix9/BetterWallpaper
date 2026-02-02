# DEEP SPECIFICATION AUDIT: QUESTIONS

## A. Broken Windows (The Fixes)
1.  **MainWindow.cpp:33**: `loadWindowState()` call is commented out with `// DEBUG: Disable state loading`. Should we re-enable this for production?
2.  **MainWindow.cpp:46**: `onCloseRequest` signal connection is commented out, meaning window state isn't saved on close. Enable?
3.  **ConfigManager.hpp:54**: The `std::lock_guard` in `get<T>()` is commented out. This seems like a critical race condition given the multi-threaded nature (GUI + Daemon). Must we enable it?
4.  **ConfigManager.hpp:61**: The `catch (...)` block swallows all exceptions and silently returns a default constructed object. Should we at least log the error?
5.  **ConfigManager.hpp:92**: `set()` logic for creating missing keys is commented as "simplified". If a key path doesn't exist, the write fails silently. Should we implement recursive key creation?
6.  **WallpaperLibrary.cpp:94**: If a wallpaper file is missing on scan, it is automatically removed from the library and the DB is saved (`m_dirty = true`). Should we warn the user or keep a "missing" state instead of destructive deletion (e.g., in case of unmounted external drive)?
7.  **WallpaperEngineRenderer.hpp:50**: `m_fpsLimit` defaults to 0. Is 0 "Uncapped" or "Not Set"?
8.  **MainWindow.cpp:62**: `m_weRenderer->pause()` is called with comment `// Assuming pause() exists`. I verified it does, but is the implementation safe to call if the process is already dead?
9.  **Daemon/main.cpp:75**: If DBus init fails, `g_application_quit` is called. Does this cleanly shut down any initialized singletons (WallpaperManager)?
10. **Global**: usage of hardcoded strings like "com.github.betterwallpaper.daemon". Should these be moved to a `Constants.hpp`?

## B. Business Logic & Edge Cases
11. **Config Path**: `WallpaperLibrary::getDatabasePath` relies on `HOME` or `XDG_DATA_HOME`. If both are unset (e.g. strange system service env), it falls back to relative `library.json`. Should we enforce absolute paths?
12. **DB Corruption**: If `library.json` is malformed, `load()` clears the in-memory library. On next save, the disk file is overwritten with empty data. Should we backup corrupt files (`library.json.bak`) before overwriting?
13. **Power Management**: If `pause_on_battery` is true, and the user *manually* pauses while on AC, then switches to Battery, then back to AC... does it auto-resume (overriding the manual pause)?
14. **Process Crash**: If the external wallpaper engine process crashes, `WallpaperEngineRenderer` has a `m_crashCount`. proper restart backoff strategy (e.g., wait 5s after 3 crashes) implemented in the .cpp?
15. **Instance Conflict**: If a second instance of the daemon starts, it quits. Should it instead try to signal the existing instance to open its window?
16. **Slideshow**: What is the behavior if the playlist is empty? Does the timer busy-loop or stop?
17. **Offline Mode**: If `WorkshopView` is accessed without internet, does it show a friendly error or crash/hang?
18. **Thumbnail Gen**: If `ffmpeg` fails to generate a thumbnail, do we use a static placeholder image?
19. **File Monitor**: If the user deletes a wallpaper file via OS file manager, does the `FolderView` update immediately (inotify) or only on restart?
20. **Disk Full**: If `save()` fails due to disk space, do we notify the user?
21. **Hotkeys**: `InputManager` is initialized. If a global hotkey conflicts with a system shortcut, who wins?
22. **Sleep/Wake**: Does the wallpaper pause *before* system sleep to save state, and resume *after* wake?

## C. UI/UX Micro-Interactions
23. **Closing**: Does closing the MainWindow quit the application or minimize to the System Tray?
24. **Resize**: `MIN_WINDOW_WIDTH` is 800px. Is this small enough for vertical monitors or tiling split-views?
25. **Feedback**: In `folder.new`, if `createFolder` fails (e.g. name exists), the dialog currently just closes. Should we show an error toast/shake animation?
26. **Transitions**: When switching wallpapers, is there a crossfade transition? If so, how long?
27. **Loading**: Does `ensureWorkshopView` show a skeleton loader or a spinner while initializing?
28. **Sidebar**: Is the 200px width fixed, or should it be draggable?
29. **Hierarchy**: Can folders be nested? (Code scanned suggests flat list).
30. **Drag & Drop**: Can I drag a file from my file manager into the "Library" view to add it?
31. **Dark Mode**: Does the app follow the system theme (Libadwaita default) or enforce its own?
32. **Double Click**: What does double-clicking a wallpaper in the library do? Apply it? Or open details?
33. **Hover**: Do video wallpapers preview/play on hover in the library grid?

## D. Data & Schema
34. **Rating**: Is the `rating` field 0-5 stars? Or 0-10?
35. **Tags**: Are tags case-sensitive? (e.g., "Nature" vs "nature").
36. **IDs**: How are UUIDs generated? Are they guaranteed unique across machines (for sync)?
37. **Play Count**: Is `play_count` incremented on *select*, or after a duration (e.g. 1 min)?
38. **Settings Defaults**: Where are the default values for `fps`, `volume`, etc. defined? Hardcoded in C++?
39. **Video Formats**: What extensions are strictly supported? (mp4, webm, mkv, gif?)
40. **Max Size**: Is there a file size limit for adding wallpapers (e.g. > 1GB)?
41. **Schema Versioning**: If we update `library.json` structure, is there a migration strategy?

## E. Permissions & Security
42. **Execution**: Does `WallpaperEngineRenderer` launch arbitrary binaries? If so, is there a sandbox?
43. **Path Traversal**: Can a user import a generic file (e.g. `/etc/passwd`) as a wallpaper?
44. **Sudo**: Does any part of the daemon require root (e.g. for `pkexec`)?
45. **Network**: Does the app verify SSL certificates for Workshop downloads?
46. **Integrity**: Do we verify SHA checksums of downloaded workshop items?

## F. System Integration
47. **Wayland**: Does the `NativeWallpaperSetter` support `wlr-layer-shell` AND `hyprpaper` IPC? Or just one?
48. **Monitors**: How do we handle monitor hot-plugging? Does the daemon re-apply wallpapers automatically?
49. **Hyprland**: Does the `HyprlandWorkspacesView` require the IPC socket to be open? What if it's closed?
50. **Dependencies**: Is `mpv` a hard dependency? Can it run without it (falling back to static images)?
51. **Localization**: I verified no `_()` wrapping in `MainWindow.cpp`. Is i18n planned?
52. **Distribution**: Does `make install` put files in standard FHS locations?
