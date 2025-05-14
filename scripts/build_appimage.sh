#!/bin/bash
set -e

# Paths
SCRIPT_DIR="$(dirname "$(realpath "$0")")"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
APPDIR="$ROOT_DIR/build/AppDir"
BIN="$ROOT_DIR/build/dist/Tablecruncher"
DESKTOP_FILE="$ROOT_DIR/assets/linux/tablecruncher.desktop"
ICON_DIR="$ROOT_DIR/assets/linux/icons"
VERSION_FILE="$SCRIPT_DIR/version.txt"
APPIMAGETOOL="$SCRIPT_DIR/appimagetool-x86_64.AppImage"

# Read version
if [ ! -f "$VERSION_FILE" ]; then
  echo "version.txt not found!"
  exit 1
fi
VERSION=$(cat "$VERSION_FILE" | tr -d '[:space:]')

# Clean old AppDir
rm -rf "$APPDIR"

# Create AppDir structure
mkdir -p "$APPDIR/usr/bin"
mkdir -p "$APPDIR/usr/share/applications"
mkdir -p "$APPDIR/usr/share/icons/hicolor/64x64/apps"
mkdir -p "$APPDIR/usr/share/icons/hicolor/128x128/apps"
mkdir -p "$APPDIR/usr/share/icons/hicolor/256x256/apps"

# Copy files
cp "$BIN" "$APPDIR/usr/bin/"
cp "$ICON_DIR/64x64/tablecruncher.png" "$APPDIR/usr/share/icons/hicolor/64x64/apps/"
cp "$ICON_DIR/128x128/tablecruncher.png" "$APPDIR/usr/share/icons/hicolor/128x128/apps/"
cp "$ICON_DIR/256x256/tablecruncher.png" "$APPDIR/usr/share/icons/hicolor/256x256/apps/"
# Place top-level icon for AppImage
cp "$ICON_DIR/256x256/tablecruncher.png" "$APPDIR/tablecruncher.png"
cp "$DESKTOP_FILE" "$APPDIR/usr/share/applications/"
# Symlink .desktop file into AppDir root
ln -sf usr/share/applications/$(basename "$DESKTOP_FILE") "$APPDIR/$(basename "$DESKTOP_FILE")"


# Create AppRun
ln -sf usr/bin/tablecruncher "$APPDIR/AppRun"

# Patch .desktop file to include version
# Patch .desktop file to include version
DESKTOP_TARGET="$APPDIR/usr/share/applications/$(basename "$DESKTOP_FILE")"
if [ ! -f "$DESKTOP_TARGET" ]; then
  echo "Desktop file not found at $DESKTOP_TARGET, aborting."
  exit 1
fi
# Append version line only if not present
if ! grep -q "^X-AppImage-Version=" "$DESKTOP_TARGET"; then
  echo "X-AppImage-Version=$VERSION" >> "$DESKTOP_TARGET"
fi



# Download appimagetool if needed
if [ ! -f "$APPIMAGETOOL" ]; then
  wget -O "$APPIMAGETOOL" https://github.com/AppImage/AppImageKit/releases/latest/download/appimagetool-x86_64.AppImage
  chmod +x "$APPIMAGETOOL"
fi

# Build AppImage
OUTPUT="Tablecruncher_v${VERSION}_Linux_x86_64.AppImage"
echo "$APPIMAGETOOL" "$APPDIR" "$ROOT_DIR/$OUTPUT"
"$APPIMAGETOOL" "$APPDIR" "$ROOT_DIR/$OUTPUT"

echo "Created $OUTPUT"

