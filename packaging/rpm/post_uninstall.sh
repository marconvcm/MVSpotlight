#!/bin/sh
/bin/touch --no-create /usr/share/icons/hicolor &>/dev/null || :
/usr/bin/gtk-update-icon-cache /usr/share/icons/hicolor &>/dev/null || :
/usr/bin/update-desktop-database &>/dev/null || :
