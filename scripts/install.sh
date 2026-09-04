#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "=== Installing MVSpotlight for GNOME 50 ==="

# Build project if binary doesn't exist
if [ ! -f "$PROJECT_ROOT/build/mvspotlight" ]; then
    echo "Building MVSpotlight..."
    cmake -B "$PROJECT_ROOT/build" -S "$PROJECT_ROOT" -DCMAKE_BUILD_TYPE=Release
    cmake --build "$PROJECT_ROOT/build" -j$(nproc)
fi

# Directories
BIN_DIR="$HOME/.local/bin"
DESKTOP_DIR="$HOME/.local/share/applications"
AUTOSTART_DIR="$HOME/.config/autostart"
ICON_DIR="$HOME/.local/share/icons/hicolor/scalable/apps"
PLUGIN_DIR="$HOME/.local/share/mvspotlight/plugins"

mkdir -p "$BIN_DIR" "$DESKTOP_DIR" "$AUTOSTART_DIR" "$ICON_DIR" "$PLUGIN_DIR"

# Terminate any running instance of old spotlight-qt or mvspotlight
systemctl --user stop spotlight-qt.service 2>/dev/null || true
systemctl --user disable spotlight-qt.service 2>/dev/null || true
rm -f "$HOME/.config/systemd/user/spotlight-qt.service" 2>/dev/null || true
rm -f "/run/user/$(id -u)/systemd/transient/spotlight-qt.service" 2>/dev/null || true

pkill -9 -x "mvspotlight" 2>/dev/null || true
pkill -9 -x "spotlight-qt" 2>/dev/null || true
sleep 0.2

# Install binary and compatibility symlink
echo "Installing binary to $BIN_DIR/mvspotlight..."
cp --remove-destination "$PROJECT_ROOT/build/mvspotlight" "$BIN_DIR/mvspotlight"
chmod +x "$BIN_DIR/mvspotlight"
ln -sf "$BIN_DIR/mvspotlight" "$BIN_DIR/spotlight-qt"

# Install Icon
echo "Installing application icons..."
cp "$PROJECT_ROOT/assets/icons/mvspotlight.svg" "$ICON_DIR/mvspotlight.svg"
cp "$PROJECT_ROOT/assets/icons/spotlight-qt.svg" "$ICON_DIR/spotlight-qt.svg"

# Install Desktop Entry
echo "Installing desktop entry..."
sed "s|Exec=mvspotlight|Exec=$BIN_DIR/mvspotlight|g" "$PROJECT_ROOT/data/mvspotlight.desktop" > "$DESKTOP_DIR/mvspotlight.desktop"
rm -f "$DESKTOP_DIR/spotlight-qt.desktop"
update-desktop-database "$DESKTOP_DIR" 2>/dev/null || true

# Install Autostart
echo "Installing autostart entry..."
sed "s|Exec=mvspotlight|Exec=$BIN_DIR/mvspotlight|g" "$PROJECT_ROOT/data/autostart/mvspotlight.desktop" > "$AUTOSTART_DIR/mvspotlight.desktop"
rm -f "$AUTOSTART_DIR/spotlight-qt.desktop"

# Install Example Plugins
echo "Installing example plugins to $PLUGIN_DIR..."
cp -r "$PROJECT_ROOT/plugins/examples/"* "$PLUGIN_DIR/"

# Setup GNOME shortcut
"$SCRIPT_DIR/setup-shortcut.sh"

# Install Systemd user service
SYSTEMD_USER_DIR="$HOME/.config/systemd/user"
mkdir -p "$SYSTEMD_USER_DIR"
echo "Installing systemd user service..."
cp "$PROJECT_ROOT/data/mvspotlight.service" "$SYSTEMD_USER_DIR/mvspotlight.service"
rm -f "/run/user/$(id -u)/systemd/transient/mvspotlight.service" 2>/dev/null || true

systemctl --user daemon-reload
systemctl --user stop mvspotlight.service 2>/dev/null || true
pkill -9 -x "mvspotlight" 2>/dev/null || true
sleep 0.2
systemctl --user enable --now mvspotlight.service

echo ""
echo "=== Installation Complete! ==="
echo "The MVSpotlight daemon is now active as a systemd user service."
echo "Press Alt + Space to open MVSpotlight!"
