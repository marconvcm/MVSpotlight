import QtQuick
import QtQuick.Controls

Item {
    id: root
    height: 74

    property alias text: input.text
    property alias inputItem: input
    signal downPressed()
    signal upPressed()
    signal enterPressed()
    signal secondaryActionPressed()
    signal escapePressed()

    Row {
        id: row
        anchors.fill: parent
        anchors.leftMargin: 24
        anchors.rightMargin: 24
        spacing: 16
        opacity: 1

        // Spotlight Magnifying Glass Icon / AI Badge
        Item {
            width: 28
            height: 28
            anchors.verticalCenter: parent.verticalCenter

            Image {
                id: searchIcon
                anchors.fill: parent
                source: "qrc:/assets/icons/mvspotlight.svg"
                sourceSize.width: 56
                sourceSize.height: 56
                fillMode: Image.PreserveAspectFit
                smooth: true
                visible: !searchController.isAiMode
            }

            Rectangle {
                id: aiBadge
                anchors.fill: parent
                radius: 8
                color: configService.aiAccentColor
                visible: searchController.isAiMode

                Text {
                    anchors.centerIn: parent
                    text: "✨"
                    font.pixelSize: 15
                }
            }
        }

        // Search Text Input
        TextInput {
            id: input
            width: parent.width - 28 - (parent.spacing * 2) - (clearBtn.visible ? clearBtn.width + 8 : 0) - prefBtn.width
            height: parent.height
            anchors.verticalCenter: parent.verticalCenter
            verticalAlignment: TextInput.AlignVCenter

            font.pixelSize: Math.round(22 * configService.fontSizeScale)
            font.weight: Font.Normal
            font.family: Qt.application.font.family
            color: themeService.textColor
            selectionColor: searchController.isAiMode ? configService.aiAccentColor : themeService.accentColor
            selectedTextColor: "#FFFFFF"
            selectByMouse: true
            clip: true

            focus: true

            Text {
                id: placeholder
                anchors.fill: parent
                verticalAlignment: Text.AlignVCenter
                text: "Search with MVSpotlight... (or type > to ask AI)"
                color: themeService.secondaryTextColor
                font: input.font
                visible: input.text.length === 0 && !input.inputMethodComposing
                opacity: 0.65
            }

            Keys.onPressed: function(event) {
                if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_Comma) {
                    searchController.openPreferences();
                    event.accepted = true;
                    return;
                }
                if (event.key === Qt.Key_Down) {
                    root.downPressed();
                    event.accepted = true;
                } else if (event.key === Qt.Key_Up) {
                    root.upPressed();
                    event.accepted = true;
                } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                    if (event.modifiers & Qt.ControlModifier || event.modifiers & Qt.ShiftModifier) {
                        root.secondaryActionPressed();
                    } else {
                        root.enterPressed();
                    }
                    event.accepted = true;
                } else if (event.key === Qt.Key_Escape) {
                    root.escapePressed();
                    event.accepted = true;
                } else if (event.key === Qt.Key_Tab) {
                    root.downPressed();
                    event.accepted = true;
                } else if (event.key === Qt.Key_Backtab) {
                    root.upPressed();
                    event.accepted = true;
                }
            }
        }

        // Clear button (appears when input has text)
        Rectangle {
            id: clearBtn
            width: 22
            height: 22
            radius: 11
            anchors.verticalCenter: parent.verticalCenter
            color: clearMouse.containsMouse ? (themeService.isDark ? "#55555C" : "#CCCCCC") : (themeService.isDark ? "#3A3A40" : "#E2E2E6")
            visible: input.text.length > 0

            Text {
                anchors.centerIn: parent
                text: "✕"
                font.pixelSize: 11
                font.weight: Font.Bold
                color: themeService.textColor
            }

            MouseArea {
                id: clearMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    input.text = "";
                    input.forceActiveFocus();
                }
            }
        }

        // Preferences Gear Button
        Rectangle {
            id: prefBtn
            width: 28
            height: 28
            radius: 14
            anchors.verticalCenter: parent.verticalCenter
            color: prefMouse.containsMouse ? (themeService.isDark ? "#3A3A42" : "#E2E2E6") : "transparent"

            Text {
                anchors.centerIn: parent
                text: "⚙"
                font.pixelSize: 16
                color: prefMouse.containsMouse ? themeService.accentColor : themeService.secondaryTextColor
            }

            ToolTip.visible: prefMouse.containsMouse
            ToolTip.text: "Preferences (Ctrl+,)"
            ToolTip.delay: 400

            MouseArea {
                id: prefMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    searchController.openPreferences();
                }
            }
        }
    }
}
