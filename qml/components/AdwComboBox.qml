import QtQuick
import QtQuick.Controls

Item {
    id: root

    property var model: []
    property int currentIndex: 0
    property string currentText: (model && model.length > currentIndex && currentIndex >= 0) ? String(model[currentIndex]) : ""
    property real preferredWidth: 200

    signal activated(int index)

    implicitWidth: preferredWidth
    implicitHeight: 34

    Rectangle {
        id: buttonBox
        anchors.fill: parent
        radius: 8
        color: btnMouse.pressed ? (themeService.isDark ? "#424248" : "#D4D4D8") : (btnMouse.containsMouse ? (themeService.isDark ? "#3B3B42" : "#E0E0E6") : (themeService.isDark ? "#323238" : "#E8E8EE"))
        border.width: 1
        border.color: popup.visible ? themeService.accentColor : (themeService.isDark ? "#14FFFFFF" : "#14000000")

        Row {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 10

            Text {
                text: root.currentText
                font.pixelSize: 12
                font.weight: Font.Medium
                color: themeService.textColor
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - 24
                elide: Text.ElideRight
            }

            Text {
                text: "▾"
                font.pixelSize: 13
                color: themeService.secondaryTextColor
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        MouseArea {
            id: btnMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                if (popup.visible) {
                    popup.close();
                } else {
                    popup.open();
                }
            }
        }
    }

    Popup {
        id: popup
        y: root.height + 4
        width: Math.max(root.width, 180)
        padding: 4
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            radius: 10
            color: themeService.isDark ? "#2C2C32" : "#FFFFFF"
            border.width: 1
            border.color: themeService.isDark ? "#1FFFFFFF" : "#1F000000"
        }

        contentItem: Column {
            spacing: 2
            width: parent.width

            Repeater {
                model: root.model

                Rectangle {
                    width: popup.width - 8
                    height: 32
                    radius: 6
                    color: itemMouse.containsMouse ? (themeService.isDark ? "#1AFFFFFF" : "#0F000000") : (index === root.currentIndex ? (themeService.isDark ? "#0DFFFFFF" : "#08000000") : "transparent")

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        spacing: 8

                        Text {
                            text: String(modelData)
                            font.pixelSize: 12
                            font.weight: index === root.currentIndex ? Font.Bold : Font.Normal
                            color: index === root.currentIndex ? themeService.accentColor : themeService.textColor
                            anchors.verticalCenter: parent.verticalCenter
                            width: parent.width - 24
                            elide: Text.ElideRight
                        }

                        Text {
                            text: "✓"
                            font.pixelSize: 12
                            font.bold: true
                            color: themeService.accentColor
                            visible: index === root.currentIndex
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    MouseArea {
                        id: itemMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.currentIndex = index;
                            root.activated(index);
                            popup.close();
                        }
                    }
                }
            }
        }
    }
}
