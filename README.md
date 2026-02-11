<div align="center">

# BetterWallpaper

**High-Performance Animated Wallpaper Manager for Linux**

![Version](https://img.shields.io/badge/version-0.5.5%20Beta-blue?style=for-the-badge&logo=none)
![License](https://img.shields.io/badge/license-GPL3-green?style=for-the-badge&logo=none)
![Platform](https://img.shields.io/badge/platform-Wayland-orange?style=for-the-badge&logo=linux)
[![AUR](https://img.shields.io/aur/version/betterwallpaper-git?style=for-the-badge&logo=arch-linux)](https://aur.archlinux.org/packages/betterwallpaper-git)

BetterWallpaper is a feature-rich, modern wallpaper manager built specifically for **Wayland** (Hyprland, Sway, River). It brings the power of animated wallpapers to your Linux desktop with a beautiful, "Liquid Glass" GTK4 interface.

</div>

---

## ✨ Features

*   **Diverse Wallpaper Support**:
    *   🎥 **Video Wallpapers**: Smooth playback driven by `mpv`.
    *   ⚙️ **Wallpaper Engine Scenes**: Full support for interactive 2D/3D scenes via `linux-wallpaperengine`.
    *   🖼️ **Static Images**: High-quality rendering for classic wallpapers.
    *   🎞️ **GIFs**: Animated GIF support.
*   **Modern UI**: Built with **GTK4 + Libadwaita**, offering a sleek, dark, glass-morphism aesthetic.
*   **Multi-Monitor Control**: Set different wallpapers, volumes, and mute states for each monitor independently.
*   **Smart Scheduling**: Create playlists and schedule wallpapers to change based on time of day or logic.
*   **Resource Efficient**: 
    *   Headless C++ daemon handles rendering.
    *   Smart pausing when windows are fullscreen (Hyprland integration).
*   **System Integration**:
    *   Tray icon for quick controls.
    *   CLI for scripting and external control.
    *   Pywal / Matugen color extraction support.

## 📦 Installation

### Arch Linux (AUR)
The recommended way to install on Arch Linux is via the AUR.

```bash
# Using yay
yay -S betterwallpaper-git

# Using paru
paru -S betterwallpaper-git
```

### Manual Installation (Build from Source)
For other distributions (Fedora, Debian, Pop!_OS), you will need to build from source.

**Dependencies:**
*   `cmake`, `gcc/clang`, `make`, `git`
*   `gtk4`, `libadwaita`
*   `gtk4-layer-shell`
*   `mpv`, `libcurl`
*   `wayland`, `wayland-protocols`
*   `linux-wallpaperengine` (Runtime dependency for WE scenes)

1.  **Clone the repository:**
    ```bash
    git clone https://github.com/Misiix9/BetterWallpaper.git
    cd BetterWallpaper
    ```

2.  **Install Dependencies & Build:**
    We provide a helper script for Arch/Pop!_OS:
    ```bash
    # Install system deps and build
    sudo ./install.sh
    ```

    **Or manually:**
    ```bash
    mkdir build && cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    cmake --build . --config Release -j$(nproc)
    
    # Install binaries
    sudo make install
    ```

## 🚀 Usage

### Graphical Interface
Launch the app from your application launcher or terminal:
```bash
betterwallpaper
```
*   **Library**: View your local collection. Import files or folders.
*   **Monitors**: Configure specific settings (Scale, Mute, Volume) per monitor.
*   **Profiles**: Save your favorite monitor layouts.

### CLI (Command Line Interface)
BetterWallpaper includes a CLI tool `bwp` (or via `betterwallpaper-cli` if aliased) to control the daemon.

```bash
# Set a wallpaper on the default monitor
bwp set /path/to/video.mp4

# Set wallpaper on a specific monitor
bwp set /path/to/image.png --monitor DP-1

# Playback controls
bwp pause --monitor HDMI-A-1
bwp resume
bwp stop

# Navigation (for playlists)
bwp next
bwp prev

# Get current wallpaper path
bwp get
```

### System Tray
A tray icon is available (`betterwallpaper-tray`) that provides quick access to:
*   Open UI
*   Pause/Resume all
*   Mute/Unmute
*   Recall saved profiles

## ⚙️ Configuration

The configuration files are located at:
*   Config: `~/.config/betterwallpaper/config.json`
*   Library Database: `~/.config/betterwallpaper/library.json`
*   Local State: `~/.local/share/betterwallpaper/`

### Hyprland Integration
To enable smart pausing when windows obscure the wallpaper, ensure you are running Hyprland. The daemon automatically detects the environment.

**Hyprland Config (`hyprland.conf`)**:
No specific config is required, but you can set window rules if needed. The wallpaper window uses the Layer Shell `background` layer.

## 🤝 Contributing

Contributions are welcome!

## 📄 License

This project is licensed under the **GPL-3.0 License** - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgements

*   **[linux-wallpaperengine](https://github.com/AlpyneDreams/linux-wallpaperengine)**: The magic behind rendering WE scenes on Linux.
*   **MPV**: For robust video playback.
*   **GTK4 & Libadwaita**: For the beautiful UI toolkit.
