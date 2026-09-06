import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property string markdownText: aiService.lastResponse
    property string promptText: aiService.currentPrompt
    property bool rawMode: false
    property bool copiedFeedback: false

    signal copyRequested()
    signal dismissRequested()

    function copyToClipboard() {
        clipboardService.setText(root.markdownText);
        notificationService.notify("MVSpotlight AI", "AI response copied to clipboard!");
        root.copiedFeedback = true;
        copyTimer.restart();
    }

    Timer {
        id: copyTimer
        interval: 2000
        onTriggered: {
            root.copiedFeedback = false;
        }
    }

    Column {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.topMargin: 6
        anchors.bottomMargin: 10
        spacing: 10

        // Top Navigation & Action Header
        Item {
            width: parent.width
            height: 36

            // Right Actions
            Row {
                id: rightActions
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                spacing: 6

                // Markdown view toggle
                AdwButton {
                    text: root.rawMode ? "Rich Render" : "Raw Markdown"
                    iconText: root.rawMode ? "🎨" : "📝"
                    styleType: "normal"
                    accentColor: configService.aiAccentColor
                    height: 28
                    onClicked: {
                        root.rawMode = !root.rawMode;
                    }
                }

                // Copy response button
                AdwButton {
                    id: copyBtn
                    text: root.copiedFeedback ? "Copied!" : "Copy Response"
                    iconText: root.copiedFeedback ? "✓" : "📋"
                    styleType: root.copiedFeedback ? "suggested" : "pill"
                    accentColor: configService.aiAccentColor
                    height: 28
                    onClicked: {
                        root.copyToClipboard();
                    }
                }

                // Web search button
                AdwButton {
                    text: "Search Web"
                    iconText: "🌐"
                    styleType: "normal"
                    accentColor: configService.aiAccentColor
                    height: 28
                    onClicked: {
                        var encoded = encodeURIComponent(root.promptText);
                        Qt.openUrlExternally("https://www.google.com/search?q=" + encoded);
                    }
                }
            }

            // Left Section (Provider Badge + Responsive Prompt Pill)
            Row {
                id: leftSection
                anchors.left: parent.left
                anchors.right: rightActions.left
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8

                // Provider badge
                Rectangle {
                    id: providerBadge
                    height: 26
                    width: providerLabel.implicitWidth + 20
                    radius: 13
                    color: configService.aiAccentColor + "22"
                    border.width: 1
                    border.color: configService.aiAccentColor + "55"
                    anchors.verticalCenter: parent.verticalCenter

                    Row {
                        anchors.centerIn: parent
                        spacing: 5
                        Text {
                            text: "✨"
                            font.pixelSize: 11
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            id: providerLabel
                            text: configService.aiProviderName + " (" + configService.aiModel + ")"
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                            color: configService.aiAccentColor
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }

                // Prompt preview pill (bounded so it never overlaps with right buttons)
                Rectangle {
                    height: 26
                    width: Math.max(50, Math.min(220, leftSection.width - providerBadge.width - 8))
                    radius: 13
                    color: themeService.isDark ? "#24242A" : "#ECECEE"
                    anchors.verticalCenter: parent.verticalCenter
                    clip: true
                    visible: width > 50

                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        anchors.right: parent.right
                        anchors.rightMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        text: "> " + root.promptText
                        font.pixelSize: 11
                        color: themeService.secondaryTextColor
                        elide: Text.ElideRight
                    }
                }
            }
        }

        // Markdown Reader Content Surface
        Rectangle {
            width: parent.width
            height: parent.height - 36 - 24 - 20
            radius: 10
            color: themeService.isDark ? "#1C1C22" : "#F7F7FA"
            border.width: 1
            border.color: themeService.isDark ? "#1FFFFFFF" : "#14000000"
            clip: true

            ScrollView {
                id: scrollView
                anchors.fill: parent
                anchors.margins: 12
                clip: true
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                TextEdit {
                    id: markdownEdit
                    width: scrollView.width - 16
                    readOnly: true
                    selectByMouse: true
                    textFormat: root.rawMode ? TextEdit.PlainText : TextEdit.RichText
                    text: root.rawMode 
                        ? root.markdownText 
                        : aiService.renderMarkdown(root.markdownText, themeService.isDark, configService.aiAccentColor)
                    font.family: root.rawMode ? "monospace" : Qt.application.font.family
                    font.pixelSize: root.rawMode ? 12 : 13
                    color: themeService.textColor
                    selectionColor: configService.aiAccentColor
                    selectedTextColor: "#FFFFFF"
                    wrapMode: TextEdit.Wrap
                    onLinkActivated: function(link) {
                        Qt.openUrlExternally(link);
                    }
                }
            }
        }

        // Bottom status & keyboard shortcuts hint bar
        Rectangle {
            width: parent.width
            height: 24
            color: "transparent"

            Row {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                spacing: 12

                Text {
                    text: {
                        var words = root.markdownText.trim().split(/\s+/).length;
                        return words > 0 ? (words + " words") : "";
                    }
                    font.pixelSize: 11
                    color: themeService.secondaryTextColor
                    opacity: 0.75
                }
            }

            Row {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                spacing: 10

                // Interactive Copy & Close chip
                Rectangle {
                    height: 22
                    width: copyHintRow.implicitWidth + 16
                    radius: 6
                    color: copyHintMouse.containsMouse 
                        ? (themeService.isDark ? "#383842" : "#D2D2DA") 
                        : (themeService.isDark ? "#28282E" : "#E2E2E6")

                    Row {
                        id: copyHintRow
                        anchors.centerIn: parent
                        spacing: 5
                        Text {
                            text: "↵"
                            font.pixelSize: 11
                            font.weight: Font.Bold
                            color: themeService.textColor
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text: "Copy & Close"
                            font.pixelSize: 11
                            color: themeService.secondaryTextColor
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    MouseArea {
                        id: copyHintMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.copyToClipboard();
                            root.dismissRequested();
                        }
                    }
                }

                // Interactive Dismiss chip
                Rectangle {
                    height: 22
                    width: dismissHintRow.implicitWidth + 16
                    radius: 6
                    color: dismissHintMouse.containsMouse 
                        ? (themeService.isDark ? "#383842" : "#D2D2DA") 
                        : (themeService.isDark ? "#28282E" : "#E2E2E6")

                    Row {
                        id: dismissHintRow
                        anchors.centerIn: parent
                        spacing: 5
                        Text {
                            text: "Esc"
                            font.pixelSize: 10
                            font.weight: Font.Bold
                            color: themeService.textColor
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text: "Dismiss"
                            font.pixelSize: 11
                            color: themeService.secondaryTextColor
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    MouseArea {
                        id: dismissHintMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.dismissRequested();
                        }
                    }
                }
            }
        }
    }
}
