#!/bin/bash
set -e

# Directories
BIN_DIR="$HOME/.local/bin"
APP_DIR="$HOME/.local/share/applications"
ICON_DIR="$HOME/.local/share/icons/hicolor/512x512/apps"
PIXMAP_DIR="$HOME/.local/share/pixmaps"

# Create directories
mkdir -p "$BIN_DIR"
mkdir -p "$APP_DIR"
mkdir -p "$ICON_DIR"
mkdir -p "$PIXMAP_DIR"

echo "Installing BetterWallpaper..."

# Install CSS Style (Critical)
mkdir -p "$HOME/.local/share/betterwallpaper/ui"
if [ -f "data/ui/style.css" ]; then
    echo "Installing style.css..."
    cp data/ui/style.css "$HOME/.local/share/betterwallpaper/ui/style.css"
else
    echo "Warning: data/ui/style.css not found!"
fi

# Build project if needed
if [ ! -f "build/src/gui/betterwallpaper" ]; then
    echo "Building project..."
    cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release
fi

# Install Binary
echo "Installing binary..."
cp build/src/gui/betterwallpaper "$BIN_DIR/betterwallpaper"
chmod +x "$BIN_DIR/betterwallpaper"

# Install Daemon Binary
if [ -f "build/src/daemon/betterwallpaper-daemon" ]; then
    echo "Installing daemon binary..."
    cp build/src/daemon/betterwallpaper-daemon "$BIN_DIR/betterwallpaper-daemon"
    chmod +x "$BIN_DIR/betterwallpaper-daemon"

    # Also install to /usr/bin for systemd service compatibility
    if [ -w /usr/bin ] || command -v sudo &>/dev/null; then
        echo "Installing daemon to /usr/bin (requires sudo)..."
        sudo cp build/src/daemon/betterwallpaper-daemon /usr/bin/betterwallpaper-daemon
        sudo chmod +x /usr/bin/betterwallpaper-daemon
    fi
else
    echo "Warning: Daemon binary not found. Build with: cmake --build build"
fi

# Install Icon (to multiple locations to be safe)
if [ -f "betterwallpaper_icon.png" ]; then
    echo "Installing icon..."
    cp betterwallpaper_icon.png "$ICON_DIR/betterwallpaper.png"
    cp betterwallpaper_icon.png "$PIXMAP_DIR/betterwallpaper.png"
fi

# Create Desktop File with ABSOLUTE PATHS
echo "Creating desktop entry..."
cat > betterwallpaper.desktop <<EOF
[Desktop Entry]
Type=Application
Name=BetterWallpaper
Comment=Manage and set animated wallpapers
Exec=$BIN_DIR/betterwallpaper
Icon=$ICON_DIR/betterwallpaper.png
Terminal=false
Categories=Utility;GTK;
Keywords=wallpaper;background;animated;live;
StartupNotify=true
EOF

# Install Desktop File
cp betterwallpaper.desktop "$APP_DIR/"

# Update icon cache
gtk-update-icon-cache "$HOME/.local/share/icons/hicolor" 2>/dev/null || true
update-desktop-database "$HOME/.local/share/applications" 2>/dev/null || true

echo "Installation complete!"
echo "Exec path: $BIN_DIR/betterwallpaper"
echo "Icon path: $ICON_DIR/betterwallpaper.png"
