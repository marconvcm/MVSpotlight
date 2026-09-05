import QtQuick

Column {
    id: root

    property string title: ""
    property string description: ""
    default property alias content: cardColumn.data

    width: parent ? parent.width : 500
    spacing: 6

    // Group Header Title (GNOME HIG uppercase label)
    Text {
        visible: root.title.length > 0
        text: root.title.toUpperCase()
        font.pixelSize: 11
        font.weight: Font.Bold
        color: themeService.isDark ? "#9A9996" : "#5E5C64"
        anchors.left: parent.left
        anchors.leftMargin: 12
    }

    // Optional Group Description
    Text {
        visible: root.description.length > 0
        text: root.description
        font.pixelSize: 11
        color: themeService.isDark ? "#9A9996" : "#77767B"
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.right: parent.right
        anchors.rightMargin: 12
        wrapMode: Text.WordWrap
    }

    // Card Container (Rounded rectangle)
    Rectangle {
        id: cardRect
        width: parent.width
        implicitHeight: cardColumn.implicitHeight
        radius: 12
        color: themeService.isDark ? "#303030" : "#FFFFFF"
        border.width: 1
        border.color: themeService.isDark ? "#14FFFFFF" : "#14000000"
        clip: true

        Column {
            id: cardColumn
            width: parent.width
        }
    }
}
