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
