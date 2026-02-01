#!/bin/bash
# BetterWallpaper AppImage build script
# Requires: linuxdeploy, linuxdeploy-plugin-gtk

set -e

APP_NAME="BetterWallpaper"
APP_ID="io.github.misiix9.betterwallpaper"
VERSION="${1:-0.2.0}"

# Build directory
BUILD_DIR="build"
APPDIR="AppDir"

echo "Building $APP_NAME v$VERSION AppImage..."

# Build the project
cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build "$BUILD_DIR" -j$(nproc)

# Install to AppDir
DESTDIR="$APPDIR" cmake --install "$BUILD_DIR"

# Create desktop file in AppDir
mkdir -p "$APPDIR/usr/share/applications"
cat > "$APPDIR/usr/share/applications/$APP_ID.desktop" << EOF
[Desktop Entry]
Name=BetterWallpaper
Comment=Modern animated wallpaper manager for Linux
Exec=betterwallpaper
Icon=$APP_ID
Terminal=false
Type=Application
Categories=Utility;Settings;
Keywords=wallpaper;background;animate;video;
EOF

# Copy icon
mkdir -p "$APPDIR/usr/share/icons/hicolor/256x256/apps"
if [ -f "data/icons/$APP_ID.png" ]; then
    cp "data/icons/$APP_ID.png" "$APPDIR/usr/share/icons/hicolor/256x256/apps/"
else
    # Use a placeholder or generate one
    echo "Warning: Icon not found at data/icons/$APP_ID.png"
fi

# Download linuxdeploy if not present
if [ ! -f "linuxdeploy-x86_64.AppImage" ]; then
    wget -c "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
    chmod +x linuxdeploy-x86_64.AppImage
fi

# Download GTK plugin if not present
if [ ! -f "linuxdeploy-plugin-gtk.sh" ]; then
    wget -c "https://raw.githubusercontent.com/linuxdeploy/linuxdeploy-plugin-gtk/master/linuxdeploy-plugin-gtk.sh"
    chmod +x linuxdeploy-plugin-gtk.sh
fi

# Create AppImage
export DEPLOY_GTK_VERSION=4
export VERSION

./linuxdeploy-x86_64.AppImage \
    --appdir "$APPDIR" \
    --output appimage \
    --desktop-file "$APPDIR/usr/share/applications/$APP_ID.desktop" \
    --plugin gtk

echo "AppImage created: $APP_NAME-$VERSION-x86_64.AppImage"
