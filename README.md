# MVSpotlight: macOS Spotlight-Inspired Launcher for GNOME 50

A high-performance, keyboard-first desktop launcher and command palette for **GNOME 50 on Linux**, built with **Qt 6, Qt Quick / QML, C++, and embedded Lua 5.4**. Designed from the ground up for a pure **Wayland** environment with zero legacy X11 dependencies.

![MVSpotlight Banner](assets/icons/mvspotlight.svg)

---

## Highlights & Features

- ⚡ **Instantaneous Interaction**: Runs as a persistent background daemon; activates in perceived $<50\text{ms}$ upon pressing `Alt + Space`.
- 🪟 **Frameless Translucent Surface**: Spotlight-inspired floating panel with smooth rounded corners ($22\text{px}$), broad soft shadows, subtle 1px border, and dynamic animated height ($76\text{px} \rightarrow 620\text{px}$).
- 🖥️ **Wayland Native & Multi-Monitor**: Automatically centers on the active monitor containing the mouse pointer or focused window at $26\%$ from the top of the display.
- 🎨 **Adaptive GNOME Theme**: Automatically follows GNOME's dark / light mode preference in real-time (`org.gnome.desktop.interface color-scheme`).
- 🔍 **Unified Search & Ranking Pipeline**:
  - Exact, prefix, word-prefix, substring, and fuzzy matching.
  - Usage frecency weighting (`~/.local/state/mvspotlight/history.json`) promoting frequently and recently launched items.
  - Cancellation tokens discarding stale asynchronous results when typing quickly.
- 🚀 **Native Search Providers**:
  - **Applications**: Fast indexed XDG `.desktop` parser supporting names, generic names, keywords, and terminals.
  - **GNOME Settings**: Direct access to Wi-Fi, Bluetooth, Displays, Sound, Appearance, Power, Privacy, and more.
  - **Calculator**: Safe mathematical expression evaluator (`22 * 5`, `1024 / 8`, `(25 + 5) * 2`, `sqrt(144)`) with instant copy on Enter.
  - **Quick Actions**: Session management (Lock, Log Out, Suspend, Restart, Shut Down, Open Terminal, Empty Trash).
  - **Files**: Asynchronous background search of documents, downloads, desktop, and project folders.
- 🧩 **First-Class Lua 5.4 Plugin Runtime**:
  - Embedded Lua runtime with stable C++ API (`launcher`).
  - Manifest metadata (`manifest.json`) and lightweight permissions model (`process.execute`, `network`, `notifications`, `clipboard.write`).
  - Execution watchdog (~100ms soft timeout) and instruction hooks preventing UI freezes.
  - Automatic error isolation and recovery without crashing the launcher.
  - Live hot-reloading on save (`QFileSystemWatcher`) and developer commands (`:plugins`, `:reload`, `:logs`, `:debug`).
- 📦 **7 Ready-to-Use Example Plugins**:
  - **Currency Exchange**: Real-time currency conversions (`299brl to usd`, `299 brl in usd`, `$50 to eur`, `100 eur to brl`, `5000 jpy to usd`).
  - **Developer Tools**: `Kill Gradle Daemons`, `Restart ADB`, `Open Android SDK`, `Open Projects Workspace`.
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
| `Down Arrow` / `Tab` | Select next result |
| `Up Arrow` / `Shift + Tab` | Select previous result |
| `Enter` | Execute selected action |
| `Ctrl + Enter` | Trigger secondary action (e.g. Copy ID / Reveal) |
| `Escape` | Clear query (if text present) or dismiss launcher |
| `:plugins` | List loaded Lua plugins and states |
| `:reload` | Hot-reload all Lua plugins |
| `:theme` | Toggle light/dark palette |
| `:logs` | View persistent launcher logs |

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
git clone https://github.com/marconvm/MVSpotlight.git
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
│   ├── providers/              # Applications, Settings, Calculator, Actions, Files, DevTools
│   ├── lua/                    # LuaEngine, LuaPluginManager, LuaApi, LuaPermissions
│   ├── services/               # Desktop entries, usage frecency, theme, processes, icons
│   └── dbus/                   # D-Bus org.mvspotlight.Launcher & legacy org.example.Spotlight adaptors
├── qml/                        # Qt Quick QML components
│   ├── Main.qml                # Spotlight frameless floating window & animations
│   └── components/             # SearchBar, ResultList, ResultItem, ResultIcon
├── plugins/examples/           # Example Lua plugins (DevTools, Docker, Git, Web, UUID, Time)
├── assets/                     # Application SVG icons and resources
├── data/                       # .desktop and systemd service files
├── scripts/                    # Shortcut setup, install, and uninstall scripts
├── tests/                      # QtTest automated unit test suite
└── docs/                       # Technical documentation and guides
```

---

## License

MIT License. Designed with care for the modern Linux & GNOME desktop.

