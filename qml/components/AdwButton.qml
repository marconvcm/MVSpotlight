import QtQuick

Item {
    id: root

    property string text: ""
    property string iconText: ""
    property string styleType: "normal" // "normal", "suggested", "destructive", "flat", "pill"

    signal clicked()

    implicitWidth: rowContent.width + (styleType === "pill" ? 28 : 22)
    implicitHeight: 34

    Rectangle {
        id: bg
        anchors.fill: parent
        radius: root.styleType === "pill" ? 17 : 8
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
                if (mouseArea.pressed) return Qt.darker(themeService.accentColor, 1.15);
                if (mouseArea.containsMouse) return Qt.lighter(themeService.accentColor, 1.1);
                return themeService.accentColor;
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
            // "normal" / "pill"
            if (mouseArea.pressed) return themeService.isDark ? "#424248" : "#D4D4D8";
            if (mouseArea.containsMouse) return themeService.isDark ? "#3B3B42" : "#E0E0E6";
            return themeService.isDark ? "#323238" : "#E8E8EE";
        }

        border.width: (root.styleType === "flat" || root.styleType === "suggested") ? 0 : 1
        border.color: {
            if (root.styleType === "destructive") {
                return themeService.isDark ? "#7A1C1C" : "#F5C2C2";
            }
            return themeService.isDark ? "#14FFFFFF" : "#14000000";
        }

        Row {
            id: rowContent
            anchors.centerIn: parent
            spacing: 6

            Text {
                text: root.iconText
                visible: root.iconText.length > 0
                font.pixelSize: 13
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                text: root.text
                font.pixelSize: 12
                font.weight: (root.styleType === "suggested" || root.styleType === "destructive") ? Font.DemiBold : Font.Medium
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
