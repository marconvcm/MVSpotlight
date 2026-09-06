import QtQuick
import QtQuick.Controls

Item {
    id: root
    width: parent.width

    property bool isSelected: false
    property string itemTitle: ""
    property string itemSubtitle: ""
    property string itemIcon: ""
    property string itemType: ""
    property string itemProvider: ""
    property string secondaryActionLabel: ""

    signal itemHovered()
    signal itemClicked()
    signal secondaryClicked()

    property bool isAiItem: root.itemType === "AI"
    property bool isAiLongResponse: isAiItem && root.itemTitle.length > 60 && root.itemTitle !== "Thinking..."

    height: isAiLongResponse ? Math.min(180, Math.max(62, titleText.implicitHeight + subtitleText.implicitHeight + 26)) : 62

    Behavior on height {
        NumberAnimation { duration: 120 }
    }

    Rectangle {
        id: highlightPill
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        anchors.topMargin: 2
        anchors.bottomMargin: 2
        radius: 14

        color: root.isSelected 
            ? (root.isAiItem ? (themeService.isDark ? "#38234E" : "#EDE9FE") : (themeService.isDark ? "#383842" : "#E2E2E8")) 
            : "transparent"
        border.width: root.isSelected ? 1 : 0
        border.color: root.isSelected 
            ? (root.isAiItem ? configService.aiAccentColor : (themeService.isDark ? "#484856" : "#D0D0D8")) 
            : "transparent"

        Row {
            anchors.fill: parent
            anchors.leftMargin: 14
            anchors.rightMargin: 14
            spacing: 14

            // Result Icon
            Item {
                width: 38
                height: 38
                anchors.verticalCenter: isAiLongResponse ? undefined : parent.verticalCenter
                anchors.top: isAiLongResponse ? parent.top : undefined
                anchors.topMargin: isAiLongResponse ? 12 : 0

                ResultIcon {
                    anchors.fill: parent
                    iconSource: root.itemIcon
                    iconSize: 38
                }

                // Pulsing animation for Thinking...
                SequentialAnimation on opacity {
                    running: root.itemTitle === "Thinking..."
                    loops: Animation.Infinite
                    NumberAnimation { from: 1.0; to: 0.3; duration: 500; easing.type: Easing.InOutQuad }
                    NumberAnimation { from: 0.3; to: 1.0; duration: 500; easing.type: Easing.InOutQuad }
                }
            }

            // Title & Subtitle column
            Column {
                anchors.verticalCenter: isAiLongResponse ? undefined : parent.verticalCenter
                anchors.top: isAiLongResponse ? parent.top : undefined
                anchors.topMargin: isAiLongResponse ? 10 : 0
                width: Math.max(100, parent.width - 38 - rightBadge.width - 28)
                spacing: 4

                Text {
                    id: titleText
                    width: parent.width
                    text: root.itemTitle
                    textFormat: isAiLongResponse ? Text.MarkdownText : Text.AutoText
                    font.pixelSize: isAiLongResponse ? 13 : 16
                    font.weight: isAiLongResponse ? Font.Normal : Font.DemiBold
                    font.family: Qt.application.font.family
                    color: themeService.textColor
                    wrapMode: isAiLongResponse ? Text.WordWrap : Text.NoWrap
                    maximumLineCount: isAiLongResponse ? 6 : 1
                    elide: Text.ElideRight
                    lineHeight: isAiLongResponse ? 1.25 : 1.0
                }

                Text {
                    id: subtitleText
                    width: parent.width
                    text: root.itemSubtitle
                    font.pixelSize: 12
                    font.weight: Font.Normal
                    font.family: Qt.application.font.family
                    color: root.isAiItem ? configService.aiAccentColor : themeService.secondaryTextColor
                    elide: Text.ElideRight
                    opacity: 0.85
                }
            }

            // Right side badges and formatted action buttons
            Row {
                id: rightBadge
                anchors.verticalCenter: isAiLongResponse ? undefined : parent.verticalCenter
                anchors.top: isAiLongResponse ? parent.top : undefined
                anchors.topMargin: isAiLongResponse ? 12 : 0
                spacing: 8
                z: 2

                // Secondary action pill button (e.g. "Copy Response")
                Rectangle {
                    id: secondaryBtn
                    anchors.verticalCenter: parent.verticalCenter
                    height: 26
                    width: secActionRow.implicitWidth + 16
                    radius: 13
                    visible: root.secondaryActionLabel.length > 0 && (root.isSelected || secMouse.containsMouse)
                    color: secMouse.containsMouse 
                        ? (themeService.isDark ? "#454552" : "#D2D2DC") 
                        : (themeService.isDark ? "#32323C" : "#E4E4EC")
                    border.width: 1
                    border.color: themeService.isDark ? "#25FFFFFF" : "#1A000000"

                    Row {
                        id: secActionRow
                        anchors.centerIn: parent
                        spacing: 5
                        Text {
                            text: root.isAiItem ? "📋" : "⚙"
                            font.pixelSize: 11
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text: root.secondaryActionLabel
                            font.pixelSize: 11
                            font.weight: Font.Medium
                            color: themeService.textColor
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    MouseArea {
                        id: secMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.secondaryClicked();
                        }
                    }
                }

                // Badge for provider/type
                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    height: 24
                    width: typeLabel.implicitWidth + 14
                    radius: 12
                    color: root.isAiItem 
                        ? (themeService.isDark ? "#341B4D" : "#EDE9FE") 
                        : (themeService.isDark ? "#2E2E36" : "#E4E4EA")
                    visible: root.itemType.length > 0 && (!secondaryBtn.visible || !root.isAiItem)

                    Text {
                        id: typeLabel
                        anchors.centerIn: parent
                        text: root.itemType
                        font.pixelSize: 11
                        font.weight: Font.Medium
                        color: root.isAiItem ? configService.aiAccentColor : themeService.secondaryTextColor
                    }
                }

                // Primary Action Button / Hint when selected
                Rectangle {
                    id: enterBadge
                    anchors.verticalCenter: parent.verticalCenter
                    height: 26
                    width: enterRow.implicitWidth + (root.isAiItem ? 16 : 14)
                    radius: 13
                    color: root.isAiItem ? configService.aiAccentColor : themeService.accentColor
                    visible: root.isSelected

                    Row {
                        id: enterRow
                        anchors.centerIn: parent
                        spacing: 4
                        Text {
                            text: "↵"
                            font.pixelSize: 12
                            font.weight: Font.Bold
                            color: "#FFFFFF"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text: root.isAiItem ? "Copy" : ""
                            visible: root.isAiItem
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                            color: "#FFFFFF"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.itemClicked();
                        }
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

        onPositionChanged: function(mouse) {
            root.itemHovered();
        }

        onClicked: function(mouse) {
            if (mouse.button === Qt.RightButton) {
                root.secondaryClicked();
            } else {
                root.itemClicked();
            }
        }
    }
}
