import QtQuick
import QtQuick.Window
import QtQuick.Controls
import "components"

Window {
    id: window
    title: "MVSpotlight"
    width: cardContainer.width + 40
    height: cardContainer.height + 40
    visible: searchController.windowVisible
    color: "transparent"

    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint

    // Reposition window on target screen
    function reposition() {
        if (Screen.width > 0 && Screen.height > 0) {
            window.x = (Screen.width - window.width) / 2;
            window.y = Math.round(Screen.height * configService.windowPositionRatio);
        }
    }

    Component.onCompleted: {
        reposition();
    }

    PreferencesWindow {
        id: preferencesWindow
    }

    Shortcut {
        sequence: "Ctrl+,"
        onActivated: {
            searchController.openPreferences();
        }
    }

    Connections {
        target: searchController
        function onOpenPreferencesRequested() {
            preferencesWindow.openPreferences();
        }
        function onWindowVisibleChanged() {
            if (searchController.windowVisible) {
                reposition();
                cardContainer.opacity = 1.0;
                searchBar.inputItem.forceActiveFocus();
                openAnim.start();
            } else {
                closeAnim.start();
                if (configService.clearOnHide) {
                    searchController.query = "";
                }
            }
        }
    }

    onActiveChanged: {
        if (!active && searchController.windowVisible) {
            searchController.hideWindow();
        }
    }

    onActiveFocusItemChanged: {
        if (visible && !activeFocusItem) {
            searchBar.inputItem.forceActiveFocus();
        }
    }

    // Dismiss on clicking outside the card
    MouseArea {
        anchors.fill: parent
        z: -1
        onClicked: {
            searchController.hideWindow();
        }
    }

    // Animated container
    Item {
        id: cardContainer
        anchors.centerIn: parent
        width: configService.cardWidth
        opacity: 1.0
        transform: Translate { id: containerTranslate; y: 0 }
        height: {
            if (searchController.resultCount === 0 && searchController.query.length === 0) {
                return 76;
            } else if (searchController.resultCount === 0) {
                return 180; // Empty state height
            } else {
                var visibleItems = Math.min(configService.maxResults, searchController.resultCount);
                return Math.min(76 + 12 + (configService.maxResults * 62), 76 + 12 + (visibleItems * 62));
            }
        }

        transformOrigin: Item.Center

        Behavior on height {
            NumberAnimation {
                duration: 160
                easing.type: Easing.OutCubic
            }
        }

        // Broad soft shadow
        Rectangle {
            id: shadow
            anchors.fill: surface
            anchors.margins: -10
            radius: themeService.cornerRadius + 10
            color: themeService.shadowColor
            opacity: 0.6
            z: 0
        }

        // Main Translucent Card Surface
        Rectangle {
            id: surface
            anchors.fill: parent
            radius: themeService.cornerRadius
            color: themeService.backgroundColor
            border.width: 1
            border.color: themeService.borderColor
            clip: true
            z: 1

            Column {
                anchors.fill: parent

                // Search Bar
                SearchBar {
                    id: searchBar
                    width: parent.width
                    text: searchController.query

                    onTextChanged: {
                        searchController.query = text;
                    }

                    onDownPressed: {
                        resultList.keyboardNavigating = true;
                        searchController.selectNext();
                    }

                    onUpPressed: {
                        resultList.keyboardNavigating = true;
                        searchController.selectPrevious();
                    }

                    onEnterPressed: {
                        searchController.executeSelected();
                    }

                    onSecondaryActionPressed: {
                        searchController.executeSecondarySelected();
                    }

                    onEscapePressed: {
                        if (searchBar.text.length > 0) {
                            searchBar.text = "";
                        } else {
                            searchController.hideWindow();
                        }
                    }
                }

                // Results List
                ResultList {
                    id: resultList
                    width: parent.width
                    height: parent.height - searchBar.height
                    visible: cardContainer.height > 76
                }
            }
        }
    }

    // Opening Animation: opacity 0 -> 1, scale 0.97 -> 1.0, y translation +6 -> 0
    ParallelAnimation {
        id: openAnim
        NumberAnimation {
            target: cardContainer
            property: "opacity"
            from: 0.0
            to: 1.0
            duration: 150
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            target: cardContainer
            property: "scale"
            from: 0.97
            to: 1.0
            duration: 150
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            target: containerTranslate
            property: "y"
            from: 6
            to: 0
            duration: 150
            easing.type: Easing.OutCubic
        }
    }

    // Closing Animation: opacity 1 -> 0, scale 1.0 -> 0.985
    ParallelAnimation {
        id: closeAnim
        NumberAnimation {
            target: cardContainer
            property: "opacity"
            from: 1.0
            to: 0.0
            duration: 120
            easing.type: Easing.InQuad
        }
        NumberAnimation {
            target: cardContainer
            property: "scale"
            from: 1.0
            to: 0.985
            duration: 120
            easing.type: Easing.InQuad
        }
    }
}
