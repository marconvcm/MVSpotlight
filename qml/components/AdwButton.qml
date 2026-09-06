import QtQuick

Item {
    id: root

    property string text: ""
    property string iconText: ""
    property string styleType: "normal" // "normal", "suggested", "destructive", "flat", "pill"
    property color accentColor: themeService.accentColor

    signal clicked()

    implicitWidth: Math.max(styleType === "pill" ? 36 : 28, rowContent.width + (styleType === "pill" ? 24 : 20))
    implicitHeight: 32

    width: implicitWidth
    height: implicitHeight

    Rectangle {
        id: bg
        anchors.fill: parent
        radius: root.styleType === "pill" ? Math.round(root.height / 2) : 8
        scale: mouseArea.pressed ? 0.98 : 1.0

        Behavior on scale {
            NumberAnimation { duration: 80 }
        }

        Behavior on color {
            ColorAnimation { duration: 120 }
        }

        color: {
            if (!root.enabled) {
                return themeService.isDark ? "#2A2A2E" : "#E8E8EC";
            }
            if (root.styleType === "suggested") {
                if (mouseArea.pressed) return Qt.darker(root.accentColor, 1.15);
                if (mouseArea.containsMouse) return Qt.lighter(root.accentColor, 1.1);
                return root.accentColor;
            }
            if (root.styleType === "destructive") {
                if (mouseArea.pressed) return themeService.isDark ? "#481212" : "#F4D2D2";
                if (mouseArea.containsMouse) return themeService.isDark ? "#621818" : "#FCDCDC";
                return themeService.isDark ? "#521414" : "#FCE8E8";
            }
            if (root.styleType === "flat") {
                if (mouseArea.pressed) return themeService.isDark ? "#1FFFFFFF" : "#1A000000";
                if (mouseArea.containsMouse) return themeService.isDark ? "#12FFFFFF" : "#0D000000";
                return "transparent";
            }
            if (root.styleType === "pill") {
                if (mouseArea.pressed) return root.accentColor + "38";
                if (mouseArea.containsMouse) return root.accentColor + "2C";
                return root.accentColor + "20";
            }
            // "normal"
            if (mouseArea.pressed) return themeService.isDark ? "#424248" : "#D4D4D8";
            if (mouseArea.containsMouse) return themeService.isDark ? "#3B3B42" : "#E0E0E6";
            return themeService.isDark ? "#323238" : "#E8E8EE";
        }

        border.width: (root.styleType === "flat" || root.styleType === "suggested") ? 0 : 1
        border.color: {
            if (root.styleType === "destructive") {
                return themeService.isDark ? "#7A1C1C" : "#F5C2C2";
            }
            if (root.styleType === "pill") {
                return root.accentColor + "60";
            }
            return themeService.isDark ? "#1AFFFFFF" : "#1A000000";
        }

        Row {
            id: rowContent
            anchors.centerIn: parent
            spacing: 6

            Text {
                text: root.iconText
                visible: root.iconText.length > 0
                font.pixelSize: 12
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                text: root.text
                font.pixelSize: 12
                font.weight: (root.styleType === "suggested" || root.styleType === "destructive" || root.styleType === "pill") ? Font.DemiBold : Font.Medium
                anchors.verticalCenter: parent.verticalCenter
                color: {
                    if (!root.enabled) {
                        return themeService.secondaryTextColor;
                    }
                    if (root.styleType === "suggested") {
                        return "#FFFFFF";
                    }
                    if (root.styleType === "destructive") {
                        return themeService.isDark ? "#FFA0A0" : "#C01C28";
                    }
                    if (root.styleType === "pill") {
                        return root.accentColor;
                    }
                    return themeService.textColor;
                }
            }
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: root.enabled
            enabled: root.enabled
            cursorShape: root.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
            onClicked: root.clicked()
        }
    }
}
