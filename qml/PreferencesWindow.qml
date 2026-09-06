import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import "components"

Window {
    id: prefWindow
    title: "Preferences"
    width: 900
    height: 620
    minimumWidth: 780
    minimumHeight: 520
    color: "transparent"
    flags: Qt.Window | Qt.FramelessWindowHint

    function openPreferences() {
        if (Screen.width > 0 && Screen.height > 0) {
            prefWindow.x = (Screen.width - prefWindow.width) / 2;
            prefWindow.y = (Screen.height - prefWindow.height) / 2;
        }
        prefWindow.show();
        prefWindow.raise();
        prefWindow.requestActivate();
    }

    property int currentTab: 0 // 0: Appearance, 1: AI Assistant, 2: Plugins, 3: General, 4: About
    property var pluginList: pluginManager ? pluginManager.getPluginList() : []
    property int selectedPluginIndex: 0
    property bool showApiKey: false
    property bool aiTesting: false
    property bool aiTestSuccess: false
    property string aiTestMessage: ""

    function reloadPluginData() {
        if (pluginManager) {
            pluginList = pluginManager.getPluginList();
            if (selectedPluginIndex >= pluginList.length) {
                selectedPluginIndex = Math.max(0, pluginList.length - 1);
            }
        }
    }

    Connections {
        target: pluginManager
        function onPluginsReloaded() {
            prefWindow.reloadPluginData();
        }
    }

    Connections {
        target: aiService
        function onTestConnectionFinished(success, message) {
            prefWindow.aiTesting = false;
            prefWindow.aiTestSuccess = success;
            prefWindow.aiTestMessage = message;
        }
    }

    Component.onCompleted: {
        reloadPluginData();
    }

    // Outer Adwaita Window Container
    Rectangle {
        id: windowFrame
        anchors.fill: parent
        radius: 12
        color: themeService.isDark ? "#242424" : "#FAFAFA"
        border.width: 1
        border.color: themeService.isDark ? "#17FFFFFF" : "#1F000000"
        clip: true

        // Libadwaita HeaderBar (Height: 48px)
        Rectangle {
            id: headerBar
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 48
            color: themeService.isDark ? "#2E2E2E" : "#EBEBEB"

            // Bottom border separator
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                height: 1
                color: themeService.isDark ? "#0FFFFFFF" : "#14000000"
            }

            // Window drag area (native Wayland move)
            MouseArea {
                anchors.fill: parent
                onPressed: prefWindow.startSystemMove()
            }

            // Left: App Brand & Icon
            Row {
                anchors.left: parent.left
                anchors.leftMargin: 16
                anchors.verticalCenter: parent.verticalCenter
                spacing: 10

                Image {
                    width: 26
                    height: 26
                    source: "qrc:/assets/icons/mvspotlight.svg"
                    sourceSize.width: 52
                    sourceSize.height: 52
                    fillMode: Image.PreserveAspectFit
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    text: "MVSpotlight"
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    color: themeService.textColor
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            // Center: Window Title
            Text {
                anchors.centerIn: parent
                text: "Preferences"
                font.pixelSize: 14
                font.weight: Font.DemiBold
                color: themeService.textColor
            }

            // Right: Adwaita Close Button (circular)
            Rectangle {
                id: closeBtn
                anchors.right: parent.right
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                width: 26
                height: 26
                radius: 13
                color: closeMouse.pressed 
                    ? (themeService.isDark ? "#2EFFFFFF" : "#24000000")
                    : (closeMouse.containsMouse 
                        ? (themeService.isDark ? "#1AFFFFFF" : "#12000000")
                        : "transparent")

                Behavior on color {
                    ColorAnimation { duration: 100 }
                }

                Text {
                    anchors.centerIn: parent
                    text: "✕"
                    font.pixelSize: 11
                    font.bold: true
                    color: themeService.secondaryTextColor
                }

                MouseArea {
                    id: closeMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: prefWindow.close()
                }
            }
        }

        // Main Body: Sidebar + Content Area
        Row {
            anchors.top: headerBar.bottom
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right

            // Adwaita Navigation Sidebar (Width: 220px)
            Rectangle {
                id: sidebar
                width: 220
                height: parent.height
                color: themeService.isDark ? "#242424" : "#F2F2F2"

                // Right border separator
                Rectangle {
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    width: 1
                    color: themeService.isDark ? "#0FFFFFFF" : "#14000000"
                }

                Column {
                    anchors.top: parent.top
                    anchors.topMargin: 12
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    spacing: 4

                    // Sidebar Navigation Items
                    Repeater {
                        model: [
                            { name: "Appearance",   icon: "🎨" },
                            { name: "AI Assistant", icon: "✨" },
                            { name: "Plugins",      icon: "🧩" },
                            { name: "General",      icon: "⚙️" },
                            { name: "About",        icon: "ℹ️" }
                        ]

                        Rectangle {
                            id: navItem
                            width: parent.width
                            height: 40
                            radius: 8

                            property bool isSelected: prefWindow.currentTab === index

                            // Adwaita sidebar selection: neutral translucent pill (NOT solid green!)
                            color: isSelected 
                                ? (themeService.isDark ? "#383838" : "#E2E2E2") 
                                : (navMouse.containsMouse 
                                    ? (themeService.isDark ? "#12FFFFFF" : "#0A000000") 
                                    : "transparent")

                            Behavior on color {
                                ColorAnimation { duration: 120 }
                            }

                            // Active accent indicator pill on the left edge
                            Rectangle {
                                visible: navItem.isSelected
                                anchors.left: parent.left
                                anchors.leftMargin: 2
                                anchors.verticalCenter: parent.verticalCenter
                                width: 3
                                height: 18
                                radius: 1.5
                                color: themeService.accentColor
                            }

                            Row {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left
                                anchors.leftMargin: 14
                                spacing: 10

                                Text {
                                    text: modelData.icon
                                    font.pixelSize: 15
                                    anchors.verticalCenter: parent.verticalCenter
                                }

                                Text {
                                    text: modelData.name
                                    font.pixelSize: 13
                                    font.weight: navItem.isSelected ? Font.DemiBold : Font.Normal
                                    color: navItem.isSelected 
                                        ? (themeService.isDark ? "#FFFFFF" : "#000000") 
                                        : (themeService.isDark ? "#D0D0D0" : "#404040")
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }

                            MouseArea {
                                id: navMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    prefWindow.currentTab = index;
                                    if (index === 2) {
                                        prefWindow.reloadPluginData();
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Main Content Area Stack
            Item {
                id: contentContainer
                width: parent.width - sidebar.width
                height: parent.height

                // ==========================================
                // TAB 0: Appearance
                // ==========================================
                ScrollView {
                    id: appearanceTab
                    anchors.fill: parent
                    clip: true
                    contentWidth: width
                    visible: prefWindow.currentTab === 0
                    ScrollBar.vertical.policy: ScrollBar.AsNeeded

                    Column {
                        width: Math.min(650, appearanceTab.width - 48)
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: parent.top
                        anchors.topMargin: 24
                        anchors.bottomMargin: 28
                        spacing: 22

                        // Group 1: Style
                        AdwPreferencesGroup {
                            title: "Style & Theme"
                            description: "Select whether MVSpotlight follows GNOME's system color scheme or uses an explicit theme."

                            Rectangle {
                                width: parent.width
                                height: 56
                                color: "transparent"

                                AdwSegmentedControl {
                                    anchors.centerIn: parent
                                    width: Math.min(parent.width - 24, 380)
                                    currentValue: configService.themeMode
                                    options: [
                                        { label: "System", value: "auto",  icon: "🌓" },
                                        { label: "Dark",   value: "dark",  icon: "🌙" },
                                        { label: "Light",  value: "light", icon: "☀️" }
                                    ]
                                    onSelected: function(val, idx) {
                                        configService.themeMode = val;
                                    }
                                }
                            }
                        }

                        // Group 2: Accent Color
                        AdwPreferencesGroup {
                            title: "Accent Color"
                            description: "Used for focus rings, selection highlights, active switches, and sliders."

                            // Official GNOME 8 Accents + Custom
                            Rectangle {
                                width: parent.width
                                height: 60
                                color: "transparent"

                                RowLayout {
                                    anchors.centerIn: parent
                                    spacing: 12

                                    Repeater {
                                        model: [
                                            { name: "Blue",     color: "#3584E4" }, // Adwaita Blue
                                            { name: "Teal",     color: "#2190A4" }, // Adwaita Teal
                                            { name: "Green",    color: "#3A944C" }, // Adwaita Green
                                            { name: "Yellow",   color: "#E5A50A" }, // Adwaita Yellow
                                            { name: "Orange",   color: "#E66100" }, // Adwaita Orange
                                            { name: "Red",      color: "#E01B24" }, // Adwaita Red
                                            { name: "Purple",   color: "#9141AC" }, // Adwaita Purple
                                            { name: "Slate",    color: "#77767B" }, // Adwaita Slate
                                            { name: "Emerald",  color: "#10B981" }  // Modern Emerald
                                        ]

                                        Rectangle {
                                            id: swatchItem
                                            Layout.preferredWidth: 32
                                            Layout.preferredHeight: 32
                                            radius: 16
                                            color: modelData.color

                                            property bool isSelected: (configService.accentColor.toUpperCase() === modelData.color.toUpperCase())

                                            border.width: isSelected ? 3 : 0
                                            border.color: "#FFFFFF"

                                            scale: isSelected ? 1.15 : (swatchMouse.containsMouse ? 1.08 : 1.0)
                                            Behavior on scale { NumberAnimation { duration: 100 } }

                                            Text {
                                                anchors.centerIn: parent
                                                text: "✓"
                                                color: "#FFFFFF"
                                                font.pixelSize: 13
                                                font.bold: true
                                                visible: swatchItem.isSelected
                                            }

                                            MouseArea {
                                                id: swatchMouse
                                                anchors.fill: parent
                                                hoverEnabled: true
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: {
                                                    configService.accentColor = modelData.color;
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            // Custom Hex Color Row
                            AdwActionRow {
                                title: "Custom Accent Color"
                                subtitle: "RGB code in hexadecimal format (#RRGGBB)"
                                showDivider: false

                                Rectangle {
                                    Layout.preferredWidth: 26
                                    Layout.preferredHeight: 26
                                    Layout.alignment: Qt.AlignVCenter
                                    radius: 13
                                    color: configService.accentColor
                                    border.width: 1
                                    border.color: "#2E000000"
                                }

                                AdwTextField {
                                    preferredWidth: 110
                                    text: configService.accentColor
                                    onEditingFinished: {
                                        if (text.length >= 4 && text.charAt(0) === '#') {
                                            configService.accentColor = text;
                                        }
                                    }
                                }
                            }
                        }

                        // Group 3: Window Geometry & Glassmorphism
                        AdwPreferencesGroup {
                            title: "Window Geometry & Transparency"

                            AdwActionRow {
                                title: "Surface Opacity"
                                subtitle: "Translucent glass effect applied to launcher card background"
                                AdwSlider {
                                    from: 65
                                    to: 100
                                    stepSize: 1
                                    unit: "%"
                                    value: Math.round(configService.surfaceOpacity * 100)
                                    onMoved: {
                                        configService.surfaceOpacity = value / 100.0;
                                    }
                                }
                            }

                            AdwActionRow {
                                title: "Corner Radius"
                                subtitle: "Border curvature of search card"
                                AdwSlider {
                                    from: 10
                                    to: 36
                                    stepSize: 1
                                    unit: "px"
                                    value: configService.cornerRadius
                                    onMoved: {
                                        configService.cornerRadius = Math.round(value);
                                    }
                                }
                            }

                            AdwActionRow {
                                title: "Launcher Card Width"
                                subtitle: "Horizontal search window width on screen"
                                AdwSlider {
                                    from: 540
                                    to: 920
                                    stepSize: 10
                                    unit: "px"
                                    value: configService.cardWidth
                                    onMoved: {
                                        configService.cardWidth = Math.round(value);
                                    }
                                }
                            }

                            AdwActionRow {
                                title: "Typography Scale"
                                subtitle: "Scale text for display resolution and accessibility"
                                showDivider: false
                                AdwSlider {
                                    from: 80
                                    to: 135
                                    stepSize: 5
                                    unit: "%"
                                    value: Math.round(configService.fontSizeScale * 100)
                                    onMoved: {
                                        configService.fontSizeScale = value / 100.0;
                                    }
                                }
                            }
                        }

                        // Group 4: Live Preview
                        AdwPreferencesGroup {
                            title: "Live Launcher Preview"
                            description: "Real-time preview of the Spotlight launcher reflecting changes instantly."

                            Rectangle {
                                width: parent.width
                                height: 110
                                color: "transparent"

                                Rectangle {
                                    anchors.centerIn: parent
                                    width: Math.min(parent.width - 32, 480)
                                    height: 68
                                    radius: Math.round(configService.cornerRadius * 0.7)
                                    color: themeService.backgroundColor
                                    border.width: 1
                                    border.color: themeService.borderColor

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 16
                                        anchors.rightMargin: 16
                                        spacing: 12

                                        Text {
                                            text: "🔍"
                                            font.pixelSize: 16
                                            Layout.alignment: Qt.AlignVCenter
                                        }

                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            Layout.alignment: Qt.AlignVCenter
                                            spacing: 2

                                            Text {
                                                text: "weather tokyo"
                                                font.pixelSize: 13
                                                font.bold: true
                                                color: themeService.textColor
                                            }

                                            Text {
                                                text: "Tokyo: 22°C • Clear"
                                                font.pixelSize: 11
                                                color: themeService.secondaryTextColor
                                            }
                                        }

                                        Rectangle {
                                            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                                            height: 24
                                            width: 64
                                            radius: 12
                                            color: themeService.selectionColor

                                            Text {
                                                anchors.centerIn: parent
                                                text: "Return ↵"
                                                font.pixelSize: 9
                                                font.bold: true
                                                color: themeService.accentColor
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // Reset to Defaults Button
                        Row {
                            anchors.right: parent.right
                            spacing: 10

                            AdwButton {
                                text: "Reset Look & Feel to Defaults"
                                iconText: "↺"
                                styleType: "normal"
                                onClicked: {
                                    configService.themeMode = "auto";
                                    configService.accentColor = "#3584E4";
                                    configService.surfaceOpacity = 0.85;
                                    configService.cornerRadius = 22;
                                    configService.cardWidth = 680;
                                    configService.fontSizeScale = 1.0;
                                }
                            }
                        }
                    }
                }

                // ==========================================
                // TAB 1: AI Assistant
                // ==========================================
                ScrollView {
                    id: aiTab
                    anchors.fill: parent
                    clip: true
                    contentWidth: width
                    visible: prefWindow.currentTab === 1
                    ScrollBar.vertical.policy: ScrollBar.AsNeeded

                    Column {
                        width: Math.min(650, aiTab.width - 48)
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: parent.top
                        anchors.topMargin: 24
                        anchors.bottomMargin: 28
                        spacing: 22

                        // Hero Banner / Instructions
                        Rectangle {
                            width: parent.width
                            height: 76
                            radius: 12
                            color: themeService.isDark ? "#1F1630" : "#F7F0FF"
                            border.width: 1
                            border.color: configService.aiAccentColor + "33"

                            Row {
                                anchors.fill: parent
                                anchors.margins: 14
                                spacing: 14

                                Rectangle {
                                    width: 44
                                    height: 44
                                    radius: 22
                                    color: configService.aiAccentColor + "22"
                                    border.width: 1
                                    border.color: configService.aiAccentColor + "55"
                                    anchors.verticalCenter: parent.verticalCenter

                                    Text {
                                        anchors.centerIn: parent
                                        text: "✨"
                                        font.pixelSize: 22
                                    }
                                }

                                Column {
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: parent.width - 64
                                    spacing: 3

                                    Text {
                                        text: "Direct AI Activation via '>'"
                                        font.pixelSize: 13
                                        font.weight: Font.DemiBold
                                        color: themeService.textColor
                                    }

                                    Text {
                                        text: "Type '>' in the search bar (e.g. '> explain quantum computing') to switch MVSpotlight into AI mode and prompt your configured model."
                                        font.pixelSize: 11
                                        color: themeService.secondaryTextColor
                                        wrapMode: Text.WordWrap
                                        width: parent.width
                                    }
                                }
                            }
                        }

                        // Group 1: Provider & Model
                        AdwPreferencesGroup {
                            title: "AI Provider & Model"
                            description: "Select which AI engine powers responses and customize the model identifier."

                            AdwActionRow {
                                title: "AI Provider"
                                subtitle: "LLM service backend"

                                AdwComboBox {
                                    id: providerCombo
                                    preferredWidth: 220
                                    model: [
                                        "Google Gemini",
                                        "OpenAI (ChatGPT)",
                                        "Anthropic Claude",
                                        "Ollama (Local)",
                                        "Custom (OpenAI-compatible)"
                                    ]
                                    currentIndex: {
                                        var p = configService.aiProvider;
                                        if (p === "gemini") return 0;
                                        if (p === "openai") return 1;
                                        if (p === "claude") return 2;
                                        if (p === "ollama") return 3;
                                        if (p === "custom") return 4;
                                        return 0;
                                    }
                                    onActivated: function(idx) {
                                        var pids = ["gemini", "openai", "claude", "ollama", "custom"];
                                        if (idx >= 0 && idx < pids.length) {
                                            configService.aiProvider = pids[idx];
                                            if (pids[idx] === "ollama" && configService.aiEndpoint.length === 0) {
                                                configService.aiEndpoint = "http://localhost:11434";
                                            }
                                        }
                                    }
                                }
                            }

                            AdwActionRow {
                                title: "Model Name"
                                subtitle: "Model identifier (e.g. " + (configService.aiProvider === "gemini" ? "gemini-2.0-flash, gemini-1.5-pro" : (configService.aiProvider === "openai" ? "gpt-4o-mini, gpt-4o" : (configService.aiProvider === "claude" ? "claude-3-5-sonnet-20241022" : "llama3.2, mistral"))) + ")"
                                showDivider: false

                                AdwTextField {
                                    preferredWidth: 240
                                    text: configService.aiModel
                                    placeholderText: "e.g. " + (configService.aiProvider === "gemini" ? "gemini-2.0-flash" : "gpt-4o-mini")
                                    onEditingFinished: {
                                        if (text.trim().length > 0) {
                                            configService.aiModel = text.trim();
                                        }
                                    }
                                }
                            }
                        }

                        // Group 2: Credentials & Endpoint
                        AdwPreferencesGroup {
                            title: "Authentication & Endpoint"
                            description: "API key and connection endpoints. Keys are stored locally in your GNOME user session configuration."

                            AdwActionRow {
                                title: "API Key"
                                subtitle: configService.aiProvider === "ollama" ? "Not required for local Ollama instances" : "Secret API authentication key"

                                AdwTextField {
                                    id: apiKeyField
                                    preferredWidth: 200
                                    echoMode: prefWindow.showApiKey ? TextInput.Normal : TextInput.Password
                                    text: configService.aiApiKey
                                    placeholderText: configService.aiProvider === "ollama" ? "(None needed)" : "Enter API key..."
                                    onEditingFinished: {
                                        configService.aiApiKey = text.trim();
                                    }
                                }

                                AdwButton {
                                    text: prefWindow.showApiKey ? "Hide" : "Show"
                                    iconText: prefWindow.showApiKey ? "👁️‍🗨️" : "👁️"
                                    styleType: "normal"
                                    onClicked: {
                                        prefWindow.showApiKey = !prefWindow.showApiKey;
                                    }
                                }
                            }

                            AdwActionRow {
                                visible: configService.aiProvider === "ollama" || configService.aiProvider === "custom"
                                title: "Endpoint URL"
                                subtitle: configService.aiProvider === "ollama" ? "Base URL of Ollama daemon" : "OpenAI-compatible server endpoint"

                                AdwTextField {
                                    preferredWidth: 250
                                    text: configService.aiEndpoint
                                    placeholderText: configService.aiProvider === "ollama" ? "http://localhost:11434" : "https://api.deepseek.com/v1"
                                    onEditingFinished: {
                                        configService.aiEndpoint = text.trim();
                                    }
                                }
                            }

                            // Connection Test Row
                            AdwActionRow {
                                title: "Connection Diagnostic"
                                subtitle: prefWindow.aiTestMessage.length > 0 
                                    ? prefWindow.aiTestMessage 
                                    : "Verify network connectivity and API authentication"

                                AdwButton {
                                    text: prefWindow.aiTesting ? "Testing..." : "⚡ Test Connection"
                                    enabled: !prefWindow.aiTesting
                                    styleType: prefWindow.aiTestSuccess ? "suggested" : "normal"
                                    onClicked: {
                                        prefWindow.aiTesting = true;
                                        prefWindow.aiTestMessage = "Testing connection...";
                                        aiService.testConnection(configService.aiProvider, configService.aiApiKey,
                                                                 configService.aiModel, configService.aiEndpoint);
                                    }
                                }
                            }

                            // Provider Documentation Link
                            AdwActionRow {
                                title: "Provider Portal"
                                subtitle: "Obtain an API key or view API setup docs"
                                showDivider: false

                                AdwButton {
                                    text: {
                                        var p = configService.aiProvider;
                                        if (p === "gemini") return "Google AI Studio ↗";
                                        if (p === "openai") return "OpenAI Platform ↗";
                                        if (p === "claude") return "Anthropic Console ↗";
                                        if (p === "ollama") return "Ollama Setup ↗";
                                        return "API Portal ↗";
                                    }
                                    iconText: "🔗"
                                    styleType: "normal"
                                    onClicked: {
                                        var p = configService.aiProvider;
                                        if (p === "gemini") Qt.openUrlExternally("https://aistudio.google.com/app/apikey");
                                        else if (p === "openai") Qt.openUrlExternally("https://platform.openai.com/api-keys");
                                        else if (p === "claude") Qt.openUrlExternally("https://console.anthropic.com/settings/keys");
                                        else if (p === "ollama") Qt.openUrlExternally("https://ollama.com");
                                        else Qt.openUrlExternally("https://github.com/marconvm/MVSpotlight");
                                    }
                                }
                            }
                        }

                        // Group 3: AI Mode Appearance & Spotlight Color
                        AdwPreferencesGroup {
                            title: "AI Mode Glow & Visual Identity"
                            description: "When you type '>', MVSpotlight dynamically shifts its border, ambient shadow glow, and badge to this color."

                            // Preset Palette
                            Rectangle {
                                width: parent.width
                                height: 60
                                color: "transparent"

                                RowLayout {
                                    anchors.centerIn: parent
                                    spacing: 12

                                    Repeater {
                                        model: [
                                            { name: "Purple",    color: "#8A2BE2" }, // Violet / Purple
                                            { name: "Magenta",   color: "#9B59B6" }, // Amethyst
                                            { name: "Cyan",      color: "#00D2FF" }, // Electric Cyan
                                            { name: "Neon Rose", color: "#FF007F" }, // Cyber Neon
                                            { name: "Emerald",   color: "#10B981" }, // AI Green
                                            { name: "Amber",     color: "#F59E0B" }  // Warm Solar
                                        ]

                                        Rectangle {
                                            id: aiColorSwatch
                                            Layout.preferredWidth: 32
                                            Layout.preferredHeight: 32
                                            radius: 16
                                            color: modelData.color

                                            property bool isSelected: (configService.aiAccentColor.toUpperCase() === modelData.color.toUpperCase())

                                            border.width: isSelected ? 3 : 0
                                            border.color: "#FFFFFF"

                                            scale: isSelected ? 1.15 : (aiSwatchMouse.containsMouse ? 1.08 : 1.0)
                                            Behavior on scale { NumberAnimation { duration: 100 } }

                                            Text {
                                                anchors.centerIn: parent
                                                text: "✓"
                                                color: "#FFFFFF"
                                                font.pixelSize: 13
                                                font.bold: true
                                                visible: aiColorSwatch.isSelected
                                            }

                                            MouseArea {
                                                id: aiSwatchMouse
                                                anchors.fill: parent
                                                hoverEnabled: true
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: {
                                                    configService.aiAccentColor = modelData.color;
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            // Custom AI Color Row
                            AdwActionRow {
                                title: "Custom AI Glow Color"
                                subtitle: "RGB code in hexadecimal format (#RRGGBB)"
                                showDivider: false

                                Rectangle {
                                    Layout.preferredWidth: 26
                                    Layout.preferredHeight: 26
                                    Layout.alignment: Qt.AlignVCenter
                                    radius: 13
                                    color: configService.aiAccentColor
                                    border.width: 1
                                    border.color: "#2E000000"
                                }

                                AdwTextField {
                                    preferredWidth: 110
                                    text: configService.aiAccentColor
                                    onEditingFinished: {
                                        if (text.length >= 4 && text.charAt(0) === '#') {
                                            configService.aiAccentColor = text;
                                        }
                                    }
                                }
                            }
                        }

                        // Group 4: Prompting & Tuning
                        AdwPreferencesGroup {
                            title: "Tuning & System Persona"
                            description: "Configure how the AI behaves and responds to your queries."

                            AdwActionRow {
                                title: "Creativity / Temperature"
                                subtitle: configService.aiTemperature < 0.4 ? "Precise & deterministic" : (configService.aiTemperature > 0.8 ? "Creative & expressive" : "Balanced")
                                AdwSlider {
                                    from: 0.0
                                    to: 1.5
                                    stepSize: 0.05
                                    value: configService.aiTemperature
                                    onMoved: {
                                        configService.aiTemperature = value;
                                    }
                                }
                            }

                            AdwActionRow {
                                title: "Max Response Tokens"
                                subtitle: "Maximum length of generated completion"
                                AdwSlider {
                                    from: 256
                                    to: 4096
                                    stepSize: 128
                                    unit: "tok"
                                    value: configService.aiMaxTokens
                                    onMoved: {
                                        configService.aiMaxTokens = Math.round(value);
                                    }
                                }
                            }

                            AdwActionRow {
                                title: "System Prompt"
                                subtitle: "Guiding instructions prepended to every AI query"
                                showDivider: false

                                AdwTextField {
                                    preferredWidth: 280
                                    text: configService.aiSystemPrompt
                                    placeholderText: "Instructions for AI..."
                                    onEditingFinished: {
                                        configService.aiSystemPrompt = text.trim();
                                    }
                                }
                            }
                        }

                        // Reset AI to Defaults
                        Rectangle {
                            width: parent.width
                            height: 48
                            color: "transparent"

                            AdwButton {
                                text: "Reset AI Settings to Defaults"
                                iconText: "↺"
                                styleType: "normal"
                                onClicked: {
                                    configService.aiProvider = "gemini";
                                    configService.aiModel = "gemini-2.0-flash";
                                    configService.aiEndpoint = "";
                                    configService.aiSystemPrompt = "You are an intelligent desktop assistant. Give concise, direct, and helpful answers.";
                                    configService.aiTemperature = 0.7;
                                    configService.aiAccentColor = "#8A2BE2";
                                    configService.aiMaxTokens = 1024;
                                    prefWindow.aiTestMessage = "";
                                }
                            }
                        }
                    }
                }

                // ==========================================
                // TAB 2: Plugins
                // ==========================================
                Item {
                    id: pluginsTab
                    anchors.fill: parent
                    anchors.margins: 20
                    visible: prefWindow.currentTab === 2

                    Row {
                        anchors.fill: parent
                        spacing: 16

                        // Left: Plugin Master List Card
                        Rectangle {
                            width: 270
                            height: parent.height
                            radius: 12
                            color: themeService.isDark ? "#303030" : "#FFFFFF"
                            border.width: 1
                            border.color: themeService.isDark ? "#14FFFFFF" : "#14000000"
                            clip: true

                            Column {
                                anchors.fill: parent

                                // Header
                                Rectangle {
                                    width: parent.width
                                    height: 40
                                    color: themeService.isDark ? "#08FFFFFF" : "#05000000"

                                    Rectangle {
                                        anchors.bottom: parent.bottom
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        height: 1
                                        color: themeService.isDark ? "#0FFFFFFF" : "#0F000000"
                                    }

                                    Row {
                                        anchors.fill: parent
                                        anchors.leftMargin: 12
                                        anchors.rightMargin: 12

                                        Text {
                                            text: "INSTALLED PLUGINS (" + prefWindow.pluginList.length + ")"
                                            font.pixelSize: 11
                                            font.weight: Font.Bold
                                            color: themeService.isDark ? "#9A9996" : "#5E5C64"
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                    }
                                }

                                // List items
                                ScrollView {
                                    width: parent.width
                                    height: parent.height - 40 - 52
                                    clip: true
                                    ScrollBar.vertical.policy: ScrollBar.AsNeeded

                                    ListView {
                                        id: pluginListView
                                        width: parent.width
                                        model: prefWindow.pluginList

                                        delegate: Rectangle {
                                            width: pluginListView.width
                                            height: 52

                                            property bool isSelected: (index === prefWindow.selectedPluginIndex)

                                            color: isSelected 
                                                ? (themeService.isDark ? "#383838" : "#EAEAEA")
                                                : (pMouse.containsMouse 
                                                    ? (themeService.isDark ? "#0DFFFFFF" : "#08000000") 
                                                    : "transparent")

                                            Rectangle {
                                                anchors.bottom: parent.bottom
                                                anchors.left: parent.left
                                                anchors.right: parent.right
                                                anchors.leftMargin: 46
                                                height: 1
                                                color: themeService.isDark ? "#0AFFFFFF" : "#0A000000"
                                            }

                                            RowLayout {
                                                anchors.fill: parent
                                                anchors.leftMargin: 10
                                                anchors.rightMargin: 10
                                                spacing: 10

                                                Rectangle {
                                                    Layout.preferredWidth: 30
                                                    Layout.preferredHeight: 30
                                                    Layout.alignment: Qt.AlignVCenter
                                                    radius: 7
                                                    color: modelData.enabled ? themeService.accentColor : (themeService.isDark ? "#424248" : "#D0D0D6")

                                                    Text {
                                                        anchors.centerIn: parent
                                                        text: "🧩"
                                                        font.pixelSize: 14
                                                    }
                                                }

                                                ColumnLayout {
                                                    Layout.fillWidth: true
                                                    Layout.alignment: Qt.AlignVCenter
                                                    spacing: 1

                                                    Text {
                                                        Layout.fillWidth: true
                                                        text: modelData.name || modelData.id
                                                        font.pixelSize: 12
                                                        font.weight: isSelected ? Font.DemiBold : Font.Normal
                                                        color: themeService.textColor
                                                        elide: Text.ElideRight
                                                    }

                                                    Text {
                                                        text: "v" + (modelData.version || "1.0.0")
                                                        font.pixelSize: 10
                                                        color: themeService.isDark ? "#9A9996" : "#77767B"
                                                    }
                                                }

                                                AdwSwitch {
                                                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                                                    checked: modelData.enabled
                                                    onToggled: function(c) {
                                                        pluginManager.setPluginEnabled(modelData.id, c);
                                                        prefWindow.reloadPluginData();
                                                    }
                                                }
                                            }

                                            MouseArea {
                                                id: pMouse
                                                anchors.fill: parent
                                                hoverEnabled: true
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: {
                                                    prefWindow.selectedPluginIndex = index;
                                                }
                                            }
                                        }
                                    }
                                }

                                // Bottom Footer Button
                                Rectangle {
                                    width: parent.width
                                    height: 52
                                    color: themeService.isDark ? "#05FFFFFF" : "#05000000"

                                    Rectangle {
                                        anchors.top: parent.top
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        height: 1
                                        color: themeService.isDark ? "#0FFFFFFF" : "#0F000000"
                                    }

                                    AdwButton {
                                        anchors.centerIn: parent
                                        text: "Reload All Plugins"
                                        iconText: "🔄"
                                        styleType: "normal"
                                        onClicked: {
                                            pluginManager.reloadAll();
                                            prefWindow.reloadPluginData();
                                        }
                                    }
                                }
                            }
                        }

                        // Right: Selected Plugin Details & Settings
                        ScrollView {
                            width: parent.width - 270 - 16
                            height: parent.height
                            clip: true
                            contentWidth: width
                            ScrollBar.vertical.policy: ScrollBar.AsNeeded

                            Column {
                                id: pluginDetailsColumn
                                width: parent.width - 12
                                spacing: 18

                                property var currentPlugin: (prefWindow.pluginList.length > prefWindow.selectedPluginIndex) 
                                    ? prefWindow.pluginList[prefWindow.selectedPluginIndex] 
                                    : null

                                visible: currentPlugin !== null

                                // Group 1: Header Info
                                AdwPreferencesGroup {
                                    title: "Plugin Details"

                                    Rectangle {
                                        width: parent.width
                                        implicitHeight: infoRow.implicitHeight + 24
                                        color: "transparent"

                                        RowLayout {
                                            id: infoRow
                                            anchors.fill: parent
                                            anchors.margins: 14
                                            spacing: 12

                                            Rectangle {
                                                Layout.preferredWidth: 44
                                                Layout.preferredHeight: 44
                                                Layout.alignment: Qt.AlignVCenter
                                                radius: 10
                                                color: themeService.accentColor

                                                Text {
                                                    anchors.centerIn: parent
                                                    text: "🧩"
                                                    font.pixelSize: 22
                                                }
                                            }

                                            ColumnLayout {
                                                Layout.fillWidth: true
                                                Layout.alignment: Qt.AlignVCenter
                                                spacing: 3

                                                Text {
                                                    Layout.fillWidth: true
                                                    text: pluginDetailsColumn.currentPlugin ? pluginDetailsColumn.currentPlugin.name : ""
                                                    font.pixelSize: 15
                                                    font.bold: true
                                                    color: themeService.textColor
                                                }

                                                Text {
                                                    Layout.fillWidth: true
                                                    text: (pluginDetailsColumn.currentPlugin ? pluginDetailsColumn.currentPlugin.id : "") +
                                                          " · v" + (pluginDetailsColumn.currentPlugin ? pluginDetailsColumn.currentPlugin.version : "1.0.0") +
                                                          " by " + (pluginDetailsColumn.currentPlugin ? pluginDetailsColumn.currentPlugin.author : "Unknown")
                                                    font.pixelSize: 11
                                                    color: themeService.isDark ? "#9A9996" : "#77767B"
                                                }

                                                Text {
                                                    Layout.fillWidth: true
                                                    text: pluginDetailsColumn.currentPlugin ? pluginDetailsColumn.currentPlugin.description : ""
                                                    font.pixelSize: 11
                                                    color: themeService.textColor
                                                    wrapMode: Text.WordWrap
                                                }
                                            }
                                        }

                                        Rectangle {
                                            anchors.bottom: parent.bottom
                                            anchors.left: parent.left
                                            anchors.right: parent.right
                                            height: 1
                                            color: themeService.isDark ? "#0FFFFFFF" : "#0F000000"
                                        }
                                    }

                                    AdwActionRow {
                                        title: "Plugin Activation"
                                        subtitle: "Enable or disable this plugin provider"
                                        AdwSwitch {
                                            checked: pluginDetailsColumn.currentPlugin ? pluginDetailsColumn.currentPlugin.enabled : false
                                            onToggled: function(c) {
                                                if (pluginDetailsColumn.currentPlugin) {
                                                    pluginManager.setPluginEnabled(pluginDetailsColumn.currentPlugin.id, c);
                                                    prefWindow.reloadPluginData();
                                                }
                                            }
                                        }
                                    }

                                    AdwActionRow {
                                        title: "Plugin Source Folder"
                                        subtitle: "Open directory in default file manager"
                                        showDivider: false

                                        AdwButton {
                                            text: "Open Folder ↗"
                                            iconText: "📂"
                                            styleType: "normal"
                                            onClicked: {
                                                if (pluginDetailsColumn.currentPlugin && pluginDetailsColumn.currentPlugin.directory) {
                                                    Qt.openUrlExternally("file://" + pluginDetailsColumn.currentPlugin.directory);
                                                }
                                            }
                                        }
                                    }
                                }

                                // Group 2: Permissions
                                AdwPreferencesGroup {
                                    title: "Sandbox Permissions"
                                    description: "System capabilities requested by this plugin."

                                    Rectangle {
                                        width: parent.width
                                        implicitHeight: permFlow.implicitHeight + 24
                                        color: "transparent"

                                        Flow {
                                            id: permFlow
                                            anchors.fill: parent
                                            anchors.margins: 12
                                            spacing: 6

                                            Repeater {
                                                model: (pluginDetailsColumn.currentPlugin && pluginDetailsColumn.currentPlugin.permissions) 
                                                    ? pluginDetailsColumn.currentPlugin.permissions : []

                                                Rectangle {
                                                    height: 24
                                                    width: permText.width + 16
                                                    radius: 12
                                                    color: themeService.isDark ? "#0FFFFFFF" : "#0A000000"
                                                    border.color: themeService.isDark ? "#14FFFFFF" : "#14000000"
                                                    border.width: 1

                                                    Text {
                                                        id: permText
                                                        anchors.centerIn: parent
                                                        text: modelData
                                                        font.pixelSize: 10
                                                        font.weight: Font.Medium
                                                        color: themeService.accentColor
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }

                                // Group 3: Configurable Settings
                                AdwPreferencesGroup {
                                    title: "Configuration"
                                    description: "Customize settings declared in the plugin manifest."

                                    Rectangle {
                                        visible: !pluginDetailsColumn.currentPlugin || 
                                                 !pluginDetailsColumn.currentPlugin.settingsSchema || 
                                                 pluginDetailsColumn.currentPlugin.settingsSchema.length === 0
                                        width: parent.width
                                        height: 48
                                        color: "transparent"

                                        Text {
                                            anchors.centerIn: parent
                                            text: "No configurable options declared in manifest.json for this plugin."
                                            font.pixelSize: 11
                                            color: themeService.isDark ? "#9A9996" : "#77767B"
                                        }
                                    }

                                    Repeater {
                                        model: (pluginDetailsColumn.currentPlugin && pluginDetailsColumn.currentPlugin.settingsSchema) 
                                            ? pluginDetailsColumn.currentPlugin.settingsSchema : []

                                        AdwActionRow {
                                            property string pluginId: pluginDetailsColumn.currentPlugin ? pluginDetailsColumn.currentPlugin.id : ""
                                            property string settingKey: modelData.key
                                            property var defaultVal: modelData.default !== undefined ? modelData.default : ""
                                            showDivider: index < (pluginDetailsColumn.currentPlugin.settingsSchema.length - 1)

                                            title: modelData.title || modelData.key
                                            subtitle: modelData.description || ""

                                            AdwTextField {
                                                id: settingInput
                                                visible: modelData.type !== "choice" && modelData.type !== "boolean"
                                                echoMode: modelData.type === "password" ? TextInput.Password : TextInput.Normal
                                                placeholderText: modelData.default !== undefined ? String(modelData.default) : ""
                                                text: String(configService.getPluginSetting(pluginId, settingKey, defaultVal))
                                                preferredWidth: (modelData.type === "path" || modelData.type === "folder") ? 180 : 200
                                                onEditingFinished: {
                                                    configService.setPluginSetting(pluginId, settingKey, text);
                                                }
                                                Connections {
                                                    target: pluginDetailsColumn
                                                    function onCurrentPluginChanged() {
                                                        settingInput.text = String(configService.getPluginSetting(pluginId, settingKey, defaultVal));
                                                    }
                                                }
                                            }

                                            AdwButton {
                                                visible: modelData.type === "path" || modelData.type === "folder"
                                                text: ""
                                                iconText: "📂"
                                                styleType: "normal"
                                                onClicked: {
                                                    var cur = settingInput.text || defaultVal || "";
                                                    var selected = configService.chooseDirectory("Select " + (modelData.title || "Folder"), cur);
                                                    if (selected && selected.length > 0) {
                                                        settingInput.text = selected;
                                                        configService.setPluginSetting(pluginId, settingKey, selected);
                                                    }
                                                }
                                            }

                                            AdwComboBox {
                                                visible: modelData.type === "choice"
                                                preferredWidth: 180
                                                model: modelData.choiceLabels || modelData.choices || []
                                                currentIndex: {
                                                    var cur = String(configService.getPluginSetting(pluginId, settingKey, defaultVal));
                                                    var list = modelData.choices || [];
                                                    for (var i = 0; i < list.length; ++i) {
                                                        if (String(list[i]) === cur) return i;
                                                    }
                                                    return 0;
                                                }
                                                onActivated: function(idx) {
                                                    var choices = modelData.choices || [];
                                                    if (idx >= 0 && idx < choices.length) {
                                                        configService.setPluginSetting(pluginId, settingKey, choices[idx]);
                                                    }
                                                }
                                            }

                                            AdwSwitch {
                                                visible: modelData.type === "boolean"
                                                checked: Boolean(configService.getPluginSetting(pluginId, settingKey, defaultVal))
                                                onToggled: function(c) {
                                                    configService.setPluginSetting(pluginId, settingKey, c);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // ==========================================
                // TAB 3: General
                // ==========================================
                ScrollView {
                    id: generalTab
                    anchors.fill: parent
                    clip: true
                    contentWidth: width
                    visible: prefWindow.currentTab === 3
                    ScrollBar.vertical.policy: ScrollBar.AsNeeded

                    Column {
                        width: Math.min(650, generalTab.width - 48)
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: parent.top
                        anchors.topMargin: 24
                        anchors.bottomMargin: 28
                        spacing: 22

                        // Group 1: Global Shortcut
                        AdwPreferencesGroup {
                            title: "Keyboard Shortcuts"
                            description: "Global shortcut bound to the Wayland system daemon."

                            AdwActionRow {
                                title: "Global Activation Shortcut"
                                subtitle: "Toggle launcher window from anywhere"
                                showDivider: false

                                Rectangle {
                                    Layout.preferredHeight: 26
                                    Layout.preferredWidth: 88
                                    Layout.alignment: Qt.AlignVCenter
                                    radius: 6
                                    color: themeService.isDark ? "#14FFFFFF" : "#0F000000"
                                    border.width: 1
                                    border.color: themeService.isDark ? "#1FFFFFFF" : "#1A000000"

                                    Text {
                                        anchors.centerIn: parent
                                        text: "Alt + Space"
                                        font.pixelSize: 11
                                        font.bold: true
                                    }
                                }

                                AdwButton {
                                    text: "GNOME Shortcuts ↗"
                                    iconText: "⌨"
                                    styleType: "normal"
                                    onClicked: {
                                        Qt.openUrlExternally("gnome-control-center:keyboard");
                                    }
                                }
                            }
                        }

                        // Group 2: Search Behavior
                        AdwPreferencesGroup {
                            title: "Search Behavior"

                            AdwActionRow {
                                title: "Maximum Search Results"
                                subtitle: "Number of search results visible without scrolling"
                                AdwSlider {
                                    from: 3
                                    to: 15
                                    stepSize: 1
                                    value: configService.maxResults
                                    onMoved: {
                                        configService.maxResults = Math.round(value);
                                    }
                                }
                            }

                            AdwActionRow {
                                title: "Typing Search Debounce"
                                subtitle: configService.searchDebounceMs === 0 ? "Instant (no delay)" : (configService.searchDebounceMs + " ms delay while typing")
                                AdwSlider {
                                    from: 0
                                    to: 300
                                    stepSize: 25
                                    value: configService.searchDebounceMs
                                    onMoved: {
                                        configService.searchDebounceMs = Math.round(value);
                                    }
                                }
                            }

                            AdwActionRow {
                                title: "Clear Query on Dismiss"
                                subtitle: "Automatically reset search bar text when launcher closes"
                                showDivider: false
                                AdwSwitch {
                                    checked: configService.clearOnHide
                                    onToggled: function(c) {
                                        configService.clearOnHide = c;
                                    }
                                }
                            }
                        }

                        // Group 3: History & Privacy
                        AdwPreferencesGroup {
                            title: "Search History & Privacy"

                            AdwActionRow {
                                title: "Frecency Ranking"
                                subtitle: "Boost frequently and recently launched applications and actions"
                                AdwSwitch {
                                    checked: configService.frecencyEnabled
                                    onToggled: function(c) {
                                        configService.frecencyEnabled = c;
                                    }
                                }
                            }

                            AdwActionRow {
                                title: "Search History Entries"
                                subtitle: usageHistory.entryCount() + " launches recorded in local database"
                                showDivider: false

                                AdwButton {
                                    text: "Clear Search History"
                                    iconText: "🗑"
                                    styleType: "destructive"
                                    onClicked: {
                                        usageHistory.clear();
                                    }
                                }
                            }
                        }
                    }
                }

                // ==========================================
                // TAB 4: About
                // ==========================================
                ScrollView {
                    id: aboutTab
                    anchors.fill: parent
                    clip: true
                    contentWidth: width
                    visible: prefWindow.currentTab === 4
                    ScrollBar.vertical.policy: ScrollBar.AsNeeded

                    Column {
                        width: Math.min(600, aboutTab.width - 48)
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: parent.top
                        anchors.topMargin: 24
                        anchors.bottomMargin: 28
                        spacing: 22

                        // Centered App Header
                        Rectangle {
                            width: parent.width
                            height: 140
                            color: "transparent"

                            Column {
                                anchors.centerIn: parent
                                spacing: 8

                                Image {
                                    width: 72
                                    height: 72
                                    source: "qrc:/assets/icons/mvspotlight.svg"
                                    sourceSize.width: 144
                                    sourceSize.height: 144
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    fillMode: Image.PreserveAspectFit
                                }

                                Text {
                                    text: "MVSpotlight"
                                    font.pixelSize: 20
                                    font.bold: true
                                    color: themeService.textColor
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }

                                Text {
                                    text: "Version 1.0.0 (Native Wayland Edition)"
                                    font.pixelSize: 12
                                    color: themeService.isDark ? "#9A9996" : "#77767B"
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }
                            }
                        }

                        // About Description
                        Text {
                            width: Math.min(520, parent.width)
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "A fast, modular, macOS Spotlight-inspired desktop launcher designed natively for GNOME 50 on Linux. Built with Qt 6, Qt Quick, pure Wayland protocols, and an extensible embedded Lua 5.4 sandboxed plugin ecosystem."
                            font.pixelSize: 12
                            color: themeService.textColor
                            wrapMode: Text.WordWrap
                            horizontalAlignment: Text.AlignHCenter
                        }

                        // Project Links Group
                        AdwPreferencesGroup {
                            title: "Project & Information"

                            AdwActionRow {
                                title: "Source Code Repository"
                                subtitle: "Browse GitHub repository, releases, and issue tracker"
                                AdwButton {
                                    text: "GitHub ↗"
                                    iconText: "🐙"
                                    styleType: "normal"
                                    onClicked: Qt.openUrlExternally("https://github.com/marconvm/MVSpotlight")
                                }
                            }

                            AdwActionRow {
                                title: "Documentation & Plugin API"
                                subtitle: "Read user guides and plugin creation tutorials"
                                AdwButton {
                                    text: "Open Guide ↗"
                                    iconText: "📖"
                                    styleType: "normal"
                                    onClicked: Qt.openUrlExternally("file:///usr/share/doc/mvspotlight/README.md")
                                }
                            }

                            AdwActionRow {
                                title: "License"
                                subtitle: "Free and Open Source Software"
                                Text {
                                    text: "MIT License"
                                    font.pixelSize: 12
                                    font.weight: Font.Medium
                                    color: themeService.isDark ? "#9A9996" : "#77767B"
                                    Layout.alignment: Qt.AlignVCenter
                                }
                            }

                            AdwActionRow {
                                title: "Platform & Desktop Environment"
                                subtitle: "Native Wayland compositor interface"
                                showDivider: false
                                Text {
                                    text: "GNOME 50 (Wayland)"
                                    font.pixelSize: 12
                                    font.weight: Font.Medium
                                    color: themeService.isDark ? "#9A9996" : "#77767B"
                                    Layout.alignment: Qt.AlignVCenter
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
