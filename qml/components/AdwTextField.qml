import QtQuick

Rectangle {
    id: root

    property alias text: input.text
    property string placeholderText: ""
    property int echoMode: TextInput.Normal
    property real preferredWidth: 240

    signal editingFinished()

    implicitWidth: preferredWidth
    implicitHeight: 34
    radius: 8
    color: themeService.isDark ? "#26262B" : "#F0F0F5"
    border.width: input.activeFocus ? 2 : 1
    border.color: input.activeFocus ? themeService.accentColor : (themeService.isDark ? "#1AFFFFFF" : "#1A000000")

    Behavior on border.color {
        ColorAnimation { duration: 120 }
    }

    TextInput {
        id: input
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        verticalAlignment: TextInput.AlignVCenter
        font.pixelSize: 12
        color: themeService.textColor
        selectionColor: themeService.accentColor
        selectedTextColor: "#FFFFFF"
        clip: true
        echoMode: root.echoMode

        onEditingFinished: {
            root.editingFinished();
        }

        Text {
            anchors.fill: parent
            verticalAlignment: Text.AlignVCenter
            text: root.placeholderText
            font.pixelSize: 12
            color: themeService.secondaryTextColor
            visible: input.text.length === 0 && !input.inputMethodComposing
            opacity: 0.6
        }
    }
}
