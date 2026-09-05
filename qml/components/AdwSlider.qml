import QtQuick
import QtQuick.Layouts

Item {
    id: root

    property real from: 0
    property real to: 100
    property real stepSize: 1
    property real value: 50
    property string unit: ""
    property bool showValueBadge: true
    property real sliderWidth: 150

    signal moved()

    implicitWidth: sliderWidth + (showValueBadge ? 56 : 0)
    implicitHeight: 32

    function updateValueFromPos(mouseX) {
        var range = root.to - root.from;
        if (range <= 0 || trackArea.width <= 0) return;
        var ratio = Math.max(0, Math.min(1, mouseX / trackArea.width));
        var rawVal = root.from + ratio * range;
        if (root.stepSize > 0) {
            rawVal = Math.round(rawVal / root.stepSize) * root.stepSize;
        }
        root.value = Math.max(root.from, Math.min(root.to, rawVal));
        root.moved();
    }

    RowLayout {
        anchors.fill: parent
        spacing: 12

        // Slider track and knob
        Item {
            id: trackArea
            Layout.preferredWidth: root.sliderWidth
            Layout.fillHeight: true

            // Background Track
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                height: 4
                radius: 2
                color: themeService.isDark ? "#48484E" : "#D4D4DA"

                // Highlighted / Filled Portion
                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: handle.x + handle.width / 2
                    radius: 2
                    color: themeService.accentColor
                }
            }

            // Handle Knob
            Rectangle {
                id: handle
                width: 18
                height: 18
                radius: 9
                anchors.verticalCenter: parent.verticalCenter
                x: {
                    var range = root.to - root.from;
                    if (range <= 0) return 0;
                    var ratio = (root.value - root.from) / range;
                    return Math.max(0, Math.min(trackArea.width - width, ratio * (trackArea.width - width)));
                }
                color: "#FFFFFF"
                border.width: 1
                border.color: "#2E000000"

                scale: dragArea.pressed ? 1.15 : (dragArea.containsMouse ? 1.08 : 1.0)
                Behavior on scale {
                    NumberAnimation { duration: 100 }
                }
            }

            MouseArea {
                id: dragArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor

                onPressed: function(mouse) {
                    root.updateValueFromPos(mouse.x);
                }

                onPositionChanged: function(mouse) {
                    if (pressed) {
                        root.updateValueFromPos(mouse.x);
                    }
                }
            }
        }

        // Value Badge Pill
        Rectangle {
            visible: root.showValueBadge
            Layout.preferredWidth: Math.max(46, badgeText.width + 14)
            Layout.preferredHeight: 24
            Layout.alignment: Qt.AlignVCenter
            radius: 6
            color: themeService.isDark ? "#1AFFFFFF" : "#0F000000"

            Text {
                id: badgeText
                anchors.centerIn: parent
                text: (root.stepSize < 1 ? root.value.toFixed(2) : Math.round(root.value)) + (root.unit ? " " + root.unit : "")
                font.pixelSize: 11
                font.bold: true
                color: themeService.textColor
            }
        }
    }
}
