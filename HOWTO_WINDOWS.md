# How to Run BetterWallpaper on Windows

## Prerequisites
- **CMake** (3.20+)
- **Compiler**: Visual Studio 2022 (MSVC) OR MinGW64.
- **Git**

## Build Instructions

1.  **Open PowerShell** in the project root.
2.  **Generate Build Files**:
    ```powershell
    cmake -B build
    ```
3.  **Compile**:
    ```powershell
    cmake --build build --config Release
    ```

## Running the App

You need to run two components: the **Daemon** (Background Service) and the **GUI** (Controller).

1.  **Start the Daemon** (Open a terminal):
    ```powershell
    .\build\src\daemon\Release\betterwallpaper-daemon.exe
    ```
    *Keep this window open to see logs.*

2.  **Start the GUI** (Open another terminal or double-click):
    ```powershell
    .\build\src\gui\Release\betterwallpaper.exe
    ```

3.  **Controls**:
    - Click **Next/Previous** to cycle wallpapers.
    - Click **Pause/Resume** to control the slideshow.
    - Click **Mute** to silence audio wallpapers.

## Troubleshooting
- If the GUI says "Daemon not found", ensure `betterwallpaper-daemon.exe` is running.
- Logs are typically saved to `%APPDATA%\BetterWallpaper\`.
