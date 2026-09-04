#!/usr/bin/env bash
set -e

echo "=== Configuring GNOME 50 Alt+Space Shortcut for MVSpotlight ==="

# 1. Unbind Alt+Space from the legacy GNOME window menu
CURRENT_WM_BINDING=$(gsettings get org.gnome.desktop.wm.keybindings activate-window-menu 2>/dev/null || echo "")
if [[ "$CURRENT_WM_BINDING" == *"space"* ]]; then
    echo "Reassigning conflicting GNOME window menu shortcut..."
    gsettings set org.gnome.desktop.wm.keybindings activate-window-menu "['<Super>space']" || true
fi

# 2. Clean up old custom-spotlight binding if present
OLD_BINDING_PATH="/org/gnome/settings-daemon/plugins/media-keys/custom-keybindings/custom-spotlight/"
EXISTING_BINDINGS=$(gsettings get org.gnome.settings-daemon.plugins.media-keys custom-keybindings 2>/dev/null || echo "@as []")
if [[ "$EXISTING_BINDINGS" == *"$OLD_BINDING_PATH"* ]]; then
    echo "Cleaning up legacy custom-spotlight keybinding..."
    gsettings reset-recursively org.gnome.settings-daemon.plugins.media-keys.custom-keybinding:$OLD_BINDING_PATH 2>/dev/null || true
    CLEANED=$(echo "$EXISTING_BINDINGS" | sed "s|'$OLD_BINDING_PATH', ||g" | sed "s|, '$OLD_BINDING_PATH'||g" | sed "s|'$OLD_BINDING_PATH'||g")
    gsettings set org.gnome.settings-daemon.plugins.media-keys custom-keybindings "$CLEANED" 2>/dev/null || true
fi

# 3. Setup custom keybinding for MVSpotlight
BINDING_PATH="/org/gnome/settings-daemon/plugins/media-keys/custom-keybindings/custom-mvspotlight/"
EXISTING_BINDINGS=$(gsettings get org.gnome.settings-daemon.plugins.media-keys custom-keybindings 2>/dev/null || echo "@as []")

if [[ "$EXISTING_BINDINGS" != *"$BINDING_PATH"* ]]; then
    if [[ "$EXISTING_BINDINGS" == "@as []" || "$EXISTING_BINDINGS" == "[]" ]]; then
        NEW_BINDINGS="['$BINDING_PATH']"
    else
        # Strip trailing ']' and append
        NEW_BINDINGS="${EXISTING_BINDINGS%]}, '$BINDING_PATH']"
    fi
    echo "Registering custom keybinding slot in GNOME..."
    gsettings set org.gnome.settings-daemon.plugins.media-keys custom-keybindings "$NEW_BINDINGS"
fi

# Configure properties of the custom keybinding
gsettings set org.gnome.settings-daemon.plugins.media-keys.custom-keybinding:$BINDING_PATH name "MVSpotlight"
gsettings set org.gnome.settings-daemon.plugins.media-keys.custom-keybinding:$BINDING_PATH command "$HOME/.local/bin/mvspotlight --toggle"
gsettings set org.gnome.settings-daemon.plugins.media-keys.custom-keybinding:$BINDING_PATH binding "<Alt>space"

echo "✓ Shortcut configured successfully: Press Alt+Space to toggle MVSpotlight!"
