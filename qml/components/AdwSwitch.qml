import QtQuick

Item {
    id: root

    property bool checked: false
    signal toggled(bool checked)

    implicitWidth: 46
    implicitHeight: 26

    Rectangle {
        id: track
        anchors.fill: parent
        radius: 13
        color: root.checked ? themeService.accentColor : (themeService.isDark ? "#484850" : "#CDCDD4")
        border.width: 1
        border.color: themeService.isDark ? "#14FFFFFF" : "#14000000"

        Behavior on color {
            ColorAnimation { duration: 150 }
        }

        // Animated Knob
        Rectangle {
            id: knob
            width: 20
            height: 20
            radius: 10
            y: 3
            x: root.checked ? (parent.width - width - 3) : 3
            color: "#FFFFFF"

            border.width: 1
            border.color: "#1F000000"

            Behavior on x {
                NumberAnimation {
                    duration: 160
                    easing.type: Easing.OutCubic
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                root.checked = !root.checked;
                root.toggled(root.checked);
            }
        }
    }
}
