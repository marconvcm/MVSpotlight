import QtQuick

Rectangle {
    id: root

    property var options: [] // Array of { label: "System", value: "auto", icon: "🌓" }
    property string currentValue: ""
    property int currentIndex: {
        for (var i = 0; i < options.length; ++i) {
            if (options[i].value === currentValue) return i;
        }
        return 0;
    }

    signal selected(string val, int index)

    height: 38
    radius: 9
    color: themeService.isDark ? "#242424" : "#E4E4EA"
    border.width: 1
    border.color: themeService.isDark ? "#0FFFFFFF" : "#0F000000"

    Row {
        id: rowSegments
        anchors.fill: parent
        anchors.margins: 3
        spacing: 3

        Repeater {
            model: root.options

            Rectangle {
                id: segItem
                width: (rowSegments.width - (root.options.length - 1) * rowSegments.spacing) / Math.max(1, root.options.length)
                height: rowSegments.height
                radius: 7

                property bool isSelected: (modelData.value === root.currentValue)

                color: isSelected 
                    ? (themeService.isDark ? "#3A3A40" : "#FFFFFF")
                    : (segMouse.containsMouse ? (themeService.isDark ? "#0FFFFFFF" : "#0A000000") : "transparent")

                border.width: isSelected ? 1 : 0
                border.color: themeService.isDark ? "#1FFFFFFF" : "#14000000"

                Behavior on color {
                    ColorAnimation { duration: 120 }
                }

                Row {
                    anchors.centerIn: parent
                    spacing: 6

                    Text {
                        text: modelData.icon || ""
                        font.pixelSize: 13
                        visible: (modelData.icon || "").length > 0
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text {
                        text: modelData.label || ""
                        font.pixelSize: 12
                        font.weight: segItem.isSelected ? Font.DemiBold : Font.Normal
                        color: segItem.isSelected 
                            ? themeService.textColor 
                            : (themeService.isDark ? "#9A9996" : "#5E5C64")
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                MouseArea {
                    id: segMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        root.currentValue = modelData.value;
                        root.selected(modelData.value, index);
                    }
                }
            }
        }
    }
}
