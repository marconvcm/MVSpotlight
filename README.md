# MVSpotlight: macOS Spotlight-Inspired Launcher for GNOME 50

A high-performance, keyboard-first desktop launcher and command palette for **GNOME 50 on Linux**, built with **Qt 6, Qt Quick / QML, C++, and embedded Lua 5.4**. Designed from the ground up for a pure **Wayland** environment with zero legacy X11 dependencies.

![MVSpotlight Banner](assets/icons/mvspotlight.svg)

---

## Highlights & Features

- ⚡ **Instantaneous Interaction**: Runs as a persistent background daemon; activates in perceived $<50\text{ms}$ upon pressing `Alt + Space`.
- 🪟 **Frameless Translucent Surface**: Spotlight-inspired floating panel with smooth rounded corners ($22\text{px}$), broad soft shadows, subtle 1px border, and dynamic animated height ($76\text{px} \rightarrow 620\text{px}$).
- 🤖 **Integrated AI Assistant & Multi-LLM Chat**:
  - Type `>` followed by your question (e.g. `> explain rust borrow checker`, `> write a python script for resizing images`) to invoke the AI assistant directly from the search bar.
  - Streaming responses rendered inside a sleek, full-width Markdown view with formatted code blocks, headers, bullet lists, raw markdown toggle, and one-click copy to clipboard.
  - Out-of-the-box support for **Google Gemini**, **OpenAI**, **Anthropic Claude**, **Ollama** (offline local LLMs), and **Custom OpenAI-compatible endpoints**.
- 🖥️ **Wayland Native & Multi-Monitor**: Automatically centers on the active monitor containing the mouse pointer or focused window at $26\%$ from the top of the display.
- 🎨 **Adaptive GNOME Theme**: Automatically follows GNOME's dark / light mode preference in real-time (`org.gnome.desktop.interface color-scheme`).
- 🔍 **Unified Search & Ranking Pipeline**:
  - Exact, prefix, word-prefix, substring, and fuzzy matching.
  - Configurable keystroke debouncing ($0\text{ms} - 300\text{ms}$) with instant synchronous flushing on arrow navigation or Enter.
  - Usage frecency weighting (`~/.local/state/mvspotlight/history.json`) promoting frequently and recently launched items.
  - Cancellation tokens discarding stale asynchronous results when typing quickly.
- 🚀 **Native Search Providers**:
  - **AI Assistant**: Direct AI query card and interactive Markdown response view.
  - **Applications**: Fast indexed XDG `.desktop` parser supporting names, generic names, keywords, and terminals.
  - **GNOME Settings**: Direct access to Wi-Fi, Bluetooth, Displays, Sound, Appearance, Power, Privacy, and more.
  - **Calculator**: Safe mathematical expression evaluator (`22 * 5`, `1024 / 8`, `(25 + 5) * 2`, `sqrt(144)`) with instant copy on Enter.
  - **Quick Actions**: Session management (Lock, Log Out, Suspend, Restart, Shut Down, Open Terminal, Empty Trash).
  - **Files & Workspaces**: Asynchronous search of files, folders, documents, downloads, desktop, and project workspaces (`~/Workspace`, `~/Development`, or custom paths).
- 🧩 **First-Class Lua 5.4 Plugin Runtime**:
  - Embedded Lua runtime with stable C++ API (`launcher`).
  - Manifest metadata (`manifest.json`) and lightweight permissions model (`process.execute`, `network`, `notifications`, `clipboard.write`).
  - Execution watchdog (~100ms soft timeout) and instruction hooks preventing UI freezes.
  - Automatic error isolation and recovery without crashing the launcher.
  - Live hot-reloading on save (`QFileSystemWatcher`) and developer commands (`:plugins`, `:reload`, `:logs`, `:debug`).
- 📦 **8 Ready-to-Use Example Plugins**:
  - **Weather**: Live global weather conditions and forecasts (`weather`, `weather tokyo`, `weather london`, `weather sao paulo`, `clima sp`).
  - **Currency Exchange**: Real-time currency conversions (`299brl to usd`, `299 brl in usd`, `$50 to eur`, `100 eur to brl`, `5000 jpy to usd`).
  - **Developer Tools**: `Kill Gradle Daemons`, `Restart ADB`, `Open Android SDK`, `Open Projects Workspace` (with configurable workspace path).
  - **Docker**: Search and manage running & stopped containers, start/stop with Enter, copy container IDs with Ctrl+Enter.
  - **Git**: Copy active Git branch, view git status.
  - **Web Search**: `g <query>`, `google <query>`, `gh <query>`, `ddg <query>`, `wiki <query>`.
  - **UUID Generator**: Typing `uuid` generates random v4 UUIDs, Enter copies to clipboard.
  - **Timestamp Converter**: Typing `unix 1788500000` converts epochs to human-readable UTC/local times.

---

## Keyboard Controls

| Shortcut | Action |
|---|---|
| `Alt + Space` | Toggle MVSpotlight launcher |
| `Ctrl + ,` | Open MVSpotlight Preferences |
| `>` or `> <prompt>` | Trigger AI Assistant mode |
| `Down Arrow` / `Tab` | Select next result |
| `Up Arrow` / `Shift + Tab` | Select previous result |
| `Enter` | Execute selected action / submit AI prompt |
| `Ctrl + Enter` | Trigger secondary action (e.g. Copy ID / Reveal in Files) |
| `Escape` | Clear query (if text present) or dismiss launcher / return from AI view |
| `:settings` / `preferences` | Open Preferences panel directly from search |
| `:plugins` | Open plugin management panel |
| `:reload` | Hot-reload all Lua plugins |
| `:theme` | Toggle light/dark palette |
| `:logs` | View persistent launcher logs |

---

## Integrated AI Assistant

MVSpotlight integrates a desktop-native AI assistant directly into your workflow without needing a web browser or separate heavy desktop app:

### Using the Assistant
1. Press `Alt + Space` to open MVSpotlight.
2. Type `>` followed by your question:
   ```
   > what is the difference between mutex and semaphore?
   ```
3. Press `Enter`. MVSpotlight streams the response into an embedded Adwaita-styled Markdown card.
4. **Action Controls**:
   - **Copy to Clipboard**: Quick copy of the entire formatted response.
   - **Rich Render / Raw Markdown**: Toggle between formatted typography and raw markdown syntax.
   - **Close / Back**: Press `Escape` or click the back icon to resume searching.

### Supported Providers
- **Google Gemini**: Uses `gemini-2.0-flash` by default (fast and responsive).
- **OpenAI**: Uses `gpt-4o-mini` or `gpt-4o`.
- **Anthropic Claude**: Uses `claude-3-5-sonnet-20241022`.
- **Ollama (Local Offline LLM)**: Connects to `http://localhost:11434` with models like `llama3.2` or `mistral` with 100% data privacy.
- **Custom OpenAI-Compatible**: Connects to any OpenAI-compatible API gateway (e.g., LocalAI, vLLM, DeepSeek).

---

## Preferences & Customization Panel

MVSpotlight includes a built-in, Wayland-native **Preferences Window** to customize your look & feel, configure AI credentials, and manage plugins.

### Opening Preferences:
- **Gear Icon**: Click the ⚙️ button in the search bar.
- **Keyboard Shortcut**: Press `Ctrl + ,` while the launcher is open.
- **Search Command**: Type `settings`, `preferences`, or `:settings` and press `Enter`.
- **CLI / D-Bus**: Run `mvspotlight --preferences` (or `-p`).
- **GNOME App Grid**: Click "MVSpotlight Preferences" in your desktop application launcher.

### Configuration Tabs:
1. 🎨 **Appearance**:
   - **Theme Mode**: Auto (follows GNOME system dark/light preference), Dark, or Light.
   - **Accent Palette**: 9 vibrant presets (macOS Blue, Emerald Green, Electric Indigo, Royal Purple, Coral Rose, Amber Orange, Cyan Teal, Ruby Crimson, Graphite) plus custom HEX input with live color indicator.
   - **Surface Opacity**: Adjustable translucent glassmorphism (65% to 100%).
   - **Corner Radius**: From sharp-modern (10px) to ultra-curved macOS style (36px).
   - **Card Width**: From compact (540px) to expansive widescreen (920px).
   - **Font Scaling**: 80% to 135% for high-DPI displays.
   - **Interactive Live Preview**: Real-time mock Spotlight card reflecting all adjustments instantly before closing.
2. 🤖 **AI Assistant**:
   - **Provider Selection**: Gemini, OpenAI, Claude, Ollama, or Custom.
   - **API Key Field**: Masked entry with toggle visibility button.
   - **Model Selector**: Auto-populated defaults with editable custom model strings.
   - **Endpoint URL**: Custom API URL for local or self-hosted LLM endpoints.
   - **System Prompt**: Fine-tune the assistant's personality and instructions.
   - **Temperature & Max Tokens**: Adjust creativity and response length.
   - **AI Accent Color**: Custom badge and highlight color for AI responses.
   - **Test Connection**: Instant connectivity test verifying API keys and model reachability.
3. 🧩 **Plugins**:
   - **Enable / Disable Toggles**: Enable or disable any plugin on the fly with automatic provider reloading.
   - **Security Inspection**: Displays all requested sandbox permissions per plugin (`network`, `process.execute`, etc.).
   - **Schema-Driven Settings**: Interactive fields (text inputs, directory selectors, masked passwords, dropdown choices, and toggle switches) dynamically generated from `manifest.json`.
   - **Plugin Folder**: Quick button to open the plugin directory in GNOME Files.
4. ⚙️ **General**:
   - **Keystroke Debounce**: Configurable search debounce delay (0ms to 300ms, default 100ms) preventing unnecessary queries while typing.
   - **Max Results Limit**: 3 to 15 items.
   - **Clear Query on Dismiss**: Option to reset or preserve search text on close.
   - **Frecency History**: View search history statistics with a one-click "Clear Search History" button.
5. ℹ️ **About**:
   - App version, architecture details, and documentation links.

---

### Declaring Configurable Plugin Settings

Plugins can declare a `"settings"` schema array in their `manifest.json`. MVSpotlight automatically renders matching UI controls in the Preferences panel and persists values in `~/.config/mvspotlight/settings.ini`:

```json
{
    "id": "org.mvspotlight.weather",
    "name": "Weather",
    "version": "1.0.0",
    "settings": [
        {
            "key": "default_city",
            "title": "Default Location / City",
            "type": "string",
            "default": "",
            "description": "City to query when typing 'weather' alone"
        },
        {
            "key": "temperature_unit",
            "title": "Preferred Temperature Unit",
            "type": "choice",
            "choices": ["c", "f"],
            "choiceLabels": ["Celsius (°C)", "Fahrenheit (°F)"],
            "default": "c"
        },
        {
            "key": "show_forecast",
            "title": "Show High/Low Forecast Card",
            "type": "boolean",
            "default": true
        }
    ]
}
```

In your `plugin.lua`, read configuration values anytime using `launcher.get_config`:

```lua
local city = launcher.get_config("default_city", "")
local unit = launcher.get_config("temperature_unit", "c")
local forecast_enabled = launcher.get_config("show_forecast", true)
```

---

## Build Prerequisites (Fedora Linux / GNOME 50)

Install required build tools and Qt 6 packages:

```bash
sudo dnf install -y \
    cmake \
    gcc-c++ \
    qt6-qtbase-devel \
    qt6-qtdeclarative-devel \
    qt6-qtsvg-devel
```

*Note: Lua 5.4 is bundled and compiled as a self-contained static library within `3rdparty/lua-5.4.8/`, ensuring zero external Lua package dependency issues across Linux distributions.*

---

## Building from Source

```bash
# Clone the repository
git clone https://github.com/marconvcm/MVSpotlight.git
cd MVSpotlight

# Configure CMake
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# Compile (parallel build)
cmake --build build -j$(nproc)

# Run test suite
ctest --test-dir build --output-on-failure
```

---

## Installation & GNOME 50 Setup

### One-Step Installation Script:
```bash
./scripts/install.sh
```
This script:
1. Compiles the release binary (`mvspotlight`) if needed.
2. Installs `mvspotlight` to `~/.local/bin/mvspotlight` and creates a `spotlight-qt` backward-compatibility symlink.
3. Installs high-resolution SVG icons to `~/.local/share/icons/hicolor/scalable/apps/`.
4. Installs desktop entry to `~/.local/share/applications/mvspotlight.desktop`.
5. Installs autostart configuration to `~/.config/autostart/mvspotlight.desktop`.
6. Installs example plugins to `~/.local/share/mvspotlight/plugins/`.
7. Automatically configures the GNOME 50 `Alt + Space` custom shortcut using `gsettings`.
8. Configures and starts the persistent `systemd --user` unit (`mvspotlight.service`).

### Uninstallation:
```bash
./scripts/uninstall.sh
```

---

## CLI & D-Bus IPC

When `mvspotlight` is already running in the background, secondary invocations communicate directly with the daemon via D-Bus (`org.mvspotlight.Launcher`):

```bash
# Toggle launcher visibility (bound to Alt+Space)
mvspotlight --toggle

# Show launcher with pre-filled search query
mvspotlight --search "firefox"

# Show or hide explicitly
mvspotlight --show
mvspotlight --hide

# Open Preferences Window
mvspotlight --preferences

# Hot-reload all Lua plugins
mvspotlight --reload-plugins

# List installed plugins from CLI
mvspotlight --list-plugins

# Launch background daemon directly
mvspotlight --daemon
```

---

## Lua Plugin Development

Developing plugins is simple and requires only 20–30 lines of Lua.

Example: **Developer Tools** command (`~/.local/share/mvspotlight/plugins/my-plugin/plugin.lua`):

```lua
launcher.register_command({
    id = "devtools.kill-gradle",
    title = "Kill Gradle Daemons",
    subtitle = "Stop all background Gradle processes",
    icon = "utilities-terminal",
    keywords = { "gradle", "kill gradle" },
    execute = function()
        launcher.process.run_async({
            command = "pkill",
            arguments = { "-f", "gradle.*daemon" },
            on_complete = function(res)
                launcher.notify("Developer Tools", "Gradle daemons stopped", "utilities-terminal")
            end
        })
    end
})
```

For complete API documentation and guides, see:
- 📖 [Architecture & Technical Design](docs/architecture.md)
- 🔌 [Lua Plugin API Reference](docs/plugin-api.md)
- 🛠️ [Lua Plugin Development Tutorial](docs/plugin-development.md)

---

## Project Structure

```
.
├── CMakeLists.txt              # CMake build configuration (mvspotlight)
├── 3rdparty/                   # Embedded Lua 5.4.8 runtime
├── src/
│   ├── main.cpp                # Application entry, CLI parser & Wayland window coordinator
│   ├── core/                   # SearchController, SearchResult, SearchResultModel
│   ├── providers/              # Applications, Settings, Calculator, Actions, Files, AiProvider, DevTools
│   ├── lua/                    # LuaEngine, LuaPluginManager, LuaApi, LuaPermissions
│   ├── services/               # ConfigService, AiService, MarkdownRenderer, Theme, Process, History
│   └── dbus/                   # D-Bus org.mvspotlight.Launcher & legacy org.example.Spotlight adaptors
├── qml/                        # Qt Quick QML components
│   ├── Main.qml                # Spotlight frameless floating window & animations
│   ├── PreferencesWindow.qml   # Preferences & customization window
│   └── components/             # SearchBar, ResultList, ResultItem, AiResponseView, Adw Controls
├── plugins/examples/           # Example Lua plugins (Weather, Currency, DevTools, Docker, Git, Web, UUID, Time)
├── assets/                     # Application SVG icons and resources
├── data/                       # .desktop and systemd service files
├── scripts/                    # Shortcut setup, install, and uninstall scripts
├── tests/                      # QtTest automated unit test suite
└── docs/                       # Technical documentation and guides
```

---

## License

MIT License. Designed with care for the modern Linux & GNOME desktop.
