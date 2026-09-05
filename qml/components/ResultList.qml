import QtQuick
import QtQuick.Controls

Item {
    id: root
    width: parent.width

    property alias listView: view
    property bool keyboardNavigating: false

    // Mouse movement inside results area reenables mouse hover selection
    MouseArea {
        anchors.fill: parent
        z: -1
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
        onPositionChanged: {
            root.keyboardNavigating = false;
        }
    }

    // Divider line at top of result list
    Rectangle {
        id: divider
        width: parent.width - 24
        height: 1
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        color: themeService.borderColor
    }

    ListView {
        id: view
        anchors.top: divider.bottom
        anchors.topMargin: 6
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 6
        clip: true

        model: searchController.model
        currentIndex: searchController.selectedIndex
        highlight: null
        highlightFollowsCurrentItem: false
        keyNavigationEnabled: false

        ScrollBar.vertical: ScrollBar {
            id: scrollBar
            policy: view.contentHeight > view.height ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
            width: 6
            contentItem: Rectangle {
                radius: 3
                color: themeService.secondaryTextColor
                opacity: 0.35
            }
        }

        delegate: ResultItem {
            width: view.width
            isSelected: index === searchController.selectedIndex
            itemTitle: model.title || ""
            itemSubtitle: model.subtitle || ""
            itemIcon: model.icon || ""
            itemType: model.type || ""
            itemProvider: model.provider || ""
            secondaryActionLabel: model.secondaryActionLabel || ""

            onItemHovered: {
                if (!root.keyboardNavigating) {
                    if (searchController.selectedIndex !== index) {
                        searchController.selectedIndex = index;
                    }
                }
            }

            onItemClicked: {
                searchController.selectedIndex = index;
                searchController.executeIndex(index);
            }

            onSecondaryClicked: {
                searchController.selectedIndex = index;
                searchController.executeSecondaryIndex(index);
            }
        }

        Connections {
            target: searchController
            function onSelectedIndexChanged() {
                if (searchController.selectedIndex >= 0) {
                    view.currentIndex = searchController.selectedIndex;
                    view.positionViewAtIndex(searchController.selectedIndex, ListView.Contain);
                }
            }
        }
    }

    // Empty state
    Item {
        id: emptyState
        anchors.fill: parent
        visible: searchController.resultCount === 0 && searchController.query.length > 0 && !searchController.isSearching

        Column {
            anchors.centerIn: parent
            spacing: 8

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "No results found"
                font.pixelSize: 16
                font.weight: Font.Medium
                color: themeService.secondaryTextColor
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Try a different search term"
                font.pixelSize: 13
                color: themeService.secondaryTextColor
                opacity: 0.7
            }
        }
    }
}
