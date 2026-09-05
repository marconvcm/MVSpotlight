import QtQuick
import QtQuick.Layouts

Item {
    id: root

    property string title: ""
    property string subtitle: ""
    property string iconText: ""
    property bool showDivider: true
    default property alias control: controlContainer.data

    width: parent ? parent.width : 500
    implicitHeight: Math.max(54, rowLayout.implicitHeight + 20)

    RowLayout {
        id: rowLayout
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 16

        // Optional Icon
        Rectangle {
            visible: root.iconText.length > 0
            Layout.preferredWidth: 32
            Layout.preferredHeight: 32
            Layout.alignment: Qt.AlignVCenter
            radius: 8
            color: themeService.isDark ? "#14FFFFFF" : "#0F000000"

            Text {
                anchors.centerIn: parent
                text: root.iconText
                font.pixelSize: 15
            }
        }

        // Title and Subtitle (Takes all remaining width!)
        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 2

            Text {
                Layout.fillWidth: true
                text: root.title
                font.pixelSize: 13
                font.weight: Font.Medium
                color: themeService.textColor
                elide: Text.ElideRight
            }

            Text {
                Layout.fillWidth: true
                text: root.subtitle
                font.pixelSize: 11
                color: themeService.isDark ? "#9A9996" : "#77767B"
                wrapMode: Text.WordWrap
                visible: root.subtitle.length > 0
            }
        }

        // Right-aligned Control Container (Strictly on the right, never overlaps!)
        RowLayout {
            id: controlContainer
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            spacing: 10
        }
    }

    // Divider Line between rows in a card
    Rectangle {
        visible: root.showDivider
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: root.iconText.length > 0 ? 56 : 16
        height: 1
        color: themeService.isDark ? "#12FFFFFF" : "#12000000"
    }
}
