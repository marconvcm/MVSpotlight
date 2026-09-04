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

        // Spotlight Magnifying Glass Icon
        Item {
            width: 28
            height: 28
            anchors.verticalCenter: parent.verticalCenter

            Canvas {
                id: searchIcon
                anchors.fill: parent
                renderTarget: Canvas.Image

                onPaint: {
                    var ctx = getContext("2d");
                    ctx.reset();
                    ctx.clearRect(0, 0, width, height);

                    var color = themeService.secondaryTextColor;
                    ctx.strokeStyle = color;
                    ctx.lineWidth = 2.4;
                    ctx.lineCap = "round";

                    // Circle
                    var radius = 7.5;
                    var cx = 11.5;
                    var cy = 11.5;
                    ctx.beginPath();
                    ctx.arc(cx, cy, radius, 0, 2 * Math.PI, false);
                    ctx.stroke();

                    // Handle
                    ctx.beginPath();
                    ctx.moveTo(17.5, 17.5);
                    ctx.lineTo(24.5, 24.5);
                    ctx.stroke();
                }

                Connections {
                    target: themeService
                    function onThemeChanged() {
                        searchIcon.requestPaint();
                    }
                }
            }
        }

        // Search Text Input
        TextInput {
            id: input
            width: parent.width - 28 - parent.spacing - (clearBtn.visible ? clearBtn.width + 12 : 0)
            height: parent.height
            anchors.verticalCenter: parent.verticalCenter
            verticalAlignment: TextInput.AlignVCenter

            font.pixelSize: 22
            font.weight: Font.Normal
            font.family: Qt.application.font.family
            color: themeService.textColor
            selectionColor: themeService.accentColor
            selectedTextColor: "#FFFFFF"
            selectByMouse: true
            clip: true

            focus: true

            Text {
                id: placeholder
                anchors.fill: parent
                verticalAlignment: Text.AlignVCenter
                text: "Search with MVSpotlight..."
                color: themeService.secondaryTextColor
                font: input.font
                visible: input.text.length === 0 && !input.inputMethodComposing
                opacity: 0.65
            }

            Keys.onPressed: function(event) {
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
    }
}
