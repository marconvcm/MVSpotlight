#!/usr/bin/env bash
set -e

echo "=== Uninstalling MVSpotlight ==="

# Terminate running instances
systemctl --user disable --now mvspotlight.service 2>/dev/null || true
systemctl --user disable --now spotlight-qt.service 2>/dev/null || true
rm -f "$HOME/.config/systemd/user/mvspotlight.service"
rm -f "$HOME/.config/systemd/user/spotlight-qt.service"
systemctl --user daemon-reload 2>/dev/null || true
pkill -9 -x "mvspotlight" 2>/dev/null || true
pkill -9 -x "spotlight-qt" 2>/dev/null || true

# Remove installed files
rm -f "$HOME/.local/bin/mvspotlight"
rm -f "$HOME/.local/bin/spotlight-qt"
rm -f "$HOME/.local/share/applications/mvspotlight.desktop"
rm -f "$HOME/.local/share/applications/spotlight-qt.desktop"
rm -f "$HOME/.config/autostart/mvspotlight.desktop"
rm -f "$HOME/.config/autostart/spotlight-qt.desktop"
rm -f "$HOME/.local/share/icons/hicolor/scalable/apps/mvspotlight.svg"
rm -f "$HOME/.local/share/icons/hicolor/scalable/apps/spotlight-qt.svg"
update-desktop-database "$HOME/.local/share/applications" 2>/dev/null || true

# Remove GNOME shortcuts (both mvspotlight and legacy spotlight)
for BINDING_PATH in "/org/gnome/settings-daemon/plugins/media-keys/custom-keybindings/custom-mvspotlight/" "/org/gnome/settings-daemon/plugins/media-keys/custom-keybindings/custom-spotlight/"; do
    EXISTING_BINDINGS=$(gsettings get org.gnome.settings-daemon.plugins.media-keys custom-keybindings 2>/dev/null || echo "@as []")
    if [[ "$EXISTING_BINDINGS" == *"$BINDING_PATH"* ]]; then
        echo "Removing GNOME shortcut ($BINDING_PATH)..."
        gsettings reset-recursively org.gnome.settings-daemon.plugins.media-keys.custom-keybinding:$BINDING_PATH || true
        CLEANED=$(echo "$EXISTING_BINDINGS" | sed "s|'$BINDING_PATH', ||g" | sed "s|, '$BINDING_PATH'||g" | sed "s|'$BINDING_PATH'||g")
        gsettings set org.gnome.settings-daemon.plugins.media-keys custom-keybindings "$CLEANED" || true
    fi
done

echo "✓ Uninstallation complete."
