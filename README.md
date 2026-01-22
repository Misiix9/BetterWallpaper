# BetterWallpaper

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![AUR](https://img.shields.io/aur/version/betterwallpaper)](https://aur.archlinux.org/packages/betterwallpaper)

> A feature-rich wallpaper manager for Linux with Wallpaper Engine support, optimized for Hyprland.

![BetterWallpaper Screenshot](docs/screenshot.png)

## ✨ Features

- **Multiple Wallpaper Types**
  - Static images (PNG, JPG, WEBP, BMP, TIFF)
  - Animated images (GIF, APNG)
  - Videos (MP4, WEBM, MKV, AVI, MOV)
  - Wallpaper Engine scenes and videos

- **Multi-Monitor Support**
  - Independent wallpaper per monitor
  - Clone mode (same wallpaper on all)
  - Span mode (stretch across all monitors)
  - Visual monitor layout configuration

- **Hyprland Integration**
  - Per-workspace wallpapers
  - Special workspace support
  - Smooth transitions on workspace change
  - Native layer-shell support

- **Steam Workshop**
  - Browse and search wallpapers
  - Direct download without Steam
  - Filter by type, resolution, rating

- **Beautiful Transitions**
  - Expanding circle/square
  - Fade, slide, wipe, dissolve
  - Zoom and morph effects
  - Configurable duration and easing

- **Color Theming**
  - Automatic color extraction
  - pywal/matugen integration
  - Export to CSS, JSON, shell variables

- **Advanced Features**
  - Profiles with triggers
  - Time-based scheduling
  - Slideshow mode
  - System tray integration
  - Full CLI support

## 📦 Installation

### Arch Linux (AUR)

```bash
yay -S betterwallpaper
```

### Flatpak

```bash
flatpak install flathub com.github.BetterWallpaper
```

### Build from Source

#### Dependencies

- CMake >= 3.20
- C++20 compiler (GCC 11+ or Clang 14+)
- GTK4 >= 4.10
- libadwaita >= 1.4
- libmpv
- libcurl
- wayland-client
- nlohmann-json
- linux-wallpaperengine (optional, for Wallpaper Engine support)

#### Build

```bash
git clone https://github.com/username/BetterWallpaper.git
cd BetterWallpaper
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
sudo cmake --install .
```

## 🚀 Usage

### GUI Application

```bash
betterwallpaper
```

### CLI

```bash
# Set wallpaper
bwp set ~/Pictures/wallpaper.png

# Set for specific monitor
bwp set ~/Pictures/wallpaper.png -m DP-1

# List monitors
bwp monitors

# Start slideshow
bwp slideshow start

# See all commands
bwp --help
```

### Autostart

BetterWallpaper can start automatically on login. Configure in Settings > General > Start on boot.

## ⚙️ Configuration

Configuration files are stored in:
- `~/.config/betterwallpaper/config.json` - Main configuration
- `~/.config/betterwallpaper/profiles/` - Profile configurations
- `~/.local/share/betterwallpaper/library.json` - Wallpaper library

## 🎨 Theming

BetterWallpaper can extract colors from your wallpaper and apply them to your system:

```bash
# Extract and apply theme
bwp theme apply

# Export colors to different formats
bwp theme export json > colors.json
bwp theme export css > colors.css
bwp theme export sh > colors.sh
```

Supports pywal, matugen, wpgtk, and custom scripts.

## 🤝 Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](docs/CONTRIBUTING.md) for guidelines.

## 📄 License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- [linux-wallpaperengine](https://github.com/Almamu/linux-wallpaperengine) - Wallpaper Engine support
- [GTK4](https://gtk.org/) - GUI framework
- [libmpv](https://mpv.io/) - Video playback
