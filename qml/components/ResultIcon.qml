import QtQuick
import QtQuick.Controls

Item {
    id: root
    property string iconSource: ""
    property int iconSize: 38

    width: iconSize
    height: iconSize

    Image {
        id: img
        anchors.centerIn: parent
        width: root.iconSize
        height: root.iconSize
        source: root.iconSource.length > 0 ? "image://icon/" + root.iconSource : ""
        sourceSize.width: root.iconSize * 2
        sourceSize.height: root.iconSize * 2
        fillMode: Image.PreserveAspectFit
        smooth: true
        mipmap: true
        asynchronous: true

        onStatusChanged: {
            if (status === Image.Error) {
                // Fallback icon
                source = "image://icon/application-x-executable"
            }
        }
    }
}
