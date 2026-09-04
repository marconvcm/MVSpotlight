import QtQuick
import QtQuick.Controls

Item {
    id: root
    width: parent.width
    height: 62

    property bool isSelected: false
    property string itemTitle: ""
    property string itemSubtitle: ""
    property string itemIcon: ""
    property string itemType: ""
    property string itemProvider: ""
    property string secondaryActionLabel: ""

    signal itemClicked()
    signal secondaryClicked()

    Rectangle {
        id: highlightPill
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        anchors.topMargin: 2
        anchors.bottomMargin: 2
        radius: 14

        color: {
            if (root.isSelected) {
                return themeService.isDark ? "#383842" : "#E2E2E8";
            }
            if (mouseArea.containsMouse) {
                return themeService.isDark ? "#28282E" : "#ECECED";
            }
            return "transparent";
        }

        border.width: root.isSelected ? 1 : 0
        border.color: themeService.isDark ? "#484856" : "#D0D0D8"

        Behavior on color {
            ColorAnimation { duration: 100 }
        }

        Row {
            anchors.fill: parent
            anchors.leftMargin: 14
            anchors.rightMargin: 14
            spacing: 14

            // Result Icon
            ResultIcon {
                anchors.verticalCenter: parent.verticalCenter
                iconSource: root.itemIcon
                iconSize: 38
            }

            // Title & Subtitle column
            Column {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - 38 - 14 - rightBadge.width - (rightBadge.visible ? 10 : 0)
                spacing: 3

                Text {
                    id: titleText
                    width: parent.width
                    text: root.itemTitle
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                    font.family: Qt.application.font.family
                    color: themeService.textColor
                    elide: Text.ElideRight
                }

                Text {
                    id: subtitleText
                    width: parent.width
                    text: root.itemSubtitle
                    font.pixelSize: 13
                    font.weight: Font.Normal
                    font.family: Qt.application.font.family
                    color: themeService.secondaryTextColor
                    elide: Text.ElideRight
                    opacity: 0.85
                }
            }

            // Right side badge and hint
            Row {
                id: rightBadge
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8

                // Badge for provider/type
                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    height: 22
                    width: typeLabel.implicitWidth + 14
                    radius: 6
                    color: themeService.isDark ? "#2E2E36" : "#E4E4EA"
                    visible: root.itemType.length > 0

                    Text {
                        id: typeLabel
                        anchors.centerIn: parent
                        text: root.itemType
                        font.pixelSize: 11
                        font.weight: Font.Medium
                        color: themeService.secondaryTextColor
                    }
                }

                // Return key hint when selected
                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    height: 22
                    width: 26
                    radius: 6
                    color: themeService.accentColor
                    visible: root.isSelected

                    Text {
                        anchors.centerIn: parent
                        text: "↵"
                        font.pixelSize: 13
                        font.weight: Font.Bold
                        color: "#FFFFFF"
                    }
                }
            }
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onClicked: function(mouse) {
            if (mouse.button === Qt.RightButton) {
                root.secondaryClicked();
            } else {
                root.itemClicked();
            }
        }
    }
}
