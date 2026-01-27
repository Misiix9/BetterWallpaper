# BetterWallpaper

A modern, high-performance animated wallpaper manager for Linux.

> [!WARNING]
> **WORK IN PROGRESS**: This project is currently in active development. Some features (like Transitions, Settings UI, and full Wallpaper Engine compatibility) may be incomplete or unstable.

## Features
- **Responsive Library**: Grid layout that adapts to your screen.
- **Preview Panel**: Settings and adjustments for your wallpapers (WIP).
- **Wallpaper Engine Support**: Compatible with `linux-wallpaperengine` (Experimental on Wayland).
- **System Tray**: Quick access and control.

## Installation

Run the installation script:
```bash
chmod +x install.sh
./install.sh
```

## Window Manager Configuration

### Hyprland
To make BetterWallpaper open as a floating window (recommended), add these rules to your `~/.config/hypr/hyprland.conf`:

```ini
windowrulev2 = float,class:^(com.github.betterwallpaper)$
windowrulev2 = center,class:^(com.github.betterwallpaper)$
windowrulev2 = size 1280 720,class:^(com.github.betterwallpaper)$
```

## Dependencies
- GTK4 / LibAdwaita
- linux-wallpaperengine (for animated wallpapers)
