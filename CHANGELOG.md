# Changelog

All notable changes to **MVSpotlight** will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]

---

## [1.0.0] - 2026-09-06

### Added

#### Core Desktop Architecture & Wayland Window
- **Wayland Native Window**: Floating frameless window following GNOME 50 HIG with smooth 22px rounded corners, 1px subtle border, soft drop shadows, and animated vertical expansion ($76\text{px} \rightarrow 620\text{px}$).
- **Multi-Monitor Coordinate Alignment**: Automatically centers horizontally and places at $26\%$ from display top on the active screen containing the mouse cursor or focused window.
- **Sub-50ms Background Daemon**: Persistent background service activated via `Alt + Space` with single-instance D-Bus IPC (`org.mvspotlight.Launcher`).
- **Adaptive Theme Engine**: Real-time synchronization with GNOME's system color scheme (`dark` / `light`) via `org.gnome.desktop.interface color-scheme` and freedesktop portal settings.

#### Search & Ranking Engine
- **Unified Query Pipeline**: Concurrent dispatch across synchronous and asynchronous providers with cancellation tokens that discard stale background results.
- **Configurable Debouncing**: Smooth keystroke throttling ($0\text{ms} - 300\text{ms}$, default $100\text{ms}$) with instant synchronous flushing on arrow navigation or Enter.
- **Frecency Scoring**: Intelligent hybrid frequency-recency ranking algorithm reading and persisting launch history at `~/.local/state/mvspotlight/history.json`.

#### Native Search Providers
- **Applications**: Indexed parser for XDG `.desktop` entries supporting localized names, generic descriptions, terminal launch commands, and keyword matching.
- **GNOME Settings**: Direct deep-linking into GNOME Control Center panels (Wi-Fi, Bluetooth, Displays, Sound, Appearance, Power, Privacy).
- **Calculator**: Safe mathematical expression evaluation (supports basic arithmetic, parenthesis nesting, and `sqrt`) with instant clipboard copy on Enter.
- **System Actions**: Fast session management (Lock Screen, Log Out, Suspend, Restart, Shut Down, Open Terminal, Empty Trash).
- **Files & Workspaces**: Asynchronous filesystem search targeting documents, downloads, desktop items, and developer workspaces (`~/Workspace`, `~/Development`, or custom paths).

#### Integrated AI Assistant & Multi-LLM Chat
- **Inline Trigger**: Instant activation by prefixing queries with `>` (e.g. `> explain rust borrow checker`).
- **Rich Markdown Response View**: Embedded Adwaita-styled response card with syntax-highlighted code blocks, comparison tables, and bullet lists.
- **Action Controls**: One-click "Copy Response" action, toggle between rendered typography and raw markdown syntax, and quick dismiss with `Esc`.
- **Multi-Provider Backend**:
  - **Google Gemini**: Uses `gemini-2.0-flash` by default.
  - **OpenAI**: Supports `gpt-4o` and `gpt-4o-mini`.
  - **Anthropic Claude**: Supports `claude-3-5-sonnet-20241022`.
  - **Ollama**: Connects to `http://localhost:11434` for 100% offline, privacy-first local models (`llama3.2`, `mistral`).
  - **Custom Endpoints**: Compatible with any standard OpenAI-compatible API gateway.

#### Preferences & Customization Window
- **Native Libadwaita Preferences**: Draggable headerbar, structured action rows, and 4 configuration panels:
  - **Appearance**: Dark/Light mode switcher, 9 preset Adwaita accent colors, custom HEX input, surface opacity slider, corner radius slider, and interactive live card preview.
  - **AI Assistant**: Provider selector, masked API key input with visibility toggle, custom model names, endpoint URL override, system prompt customization, temperature, and live connection testing tool.
  - **Plugins**: Per-plugin enable/disable switches, security sandbox permission inspection, dynamic schema-driven settings, and shortcut to open the plugin folder in GNOME Files.
  - **General**: Keystroke debounce slider, max search results count, clear on hide toggle, and search history stats with one-click wipe.
  - **About**: Version metadata, build specifications, and documentation links.

#### Embedded Lua 5.4 Plugin Ecosystem
- **Static Lua Runtime**: Self-contained Lua 5.4.8 static build with zero external library dependencies.
- **C++ Launcher API**: Secure Lua bindings (`launcher.register_command`, `launcher.process.run_async`, `launcher.notify`, `launcher.clipboard.set_text`, `launcher.get_config`).
- **Sandbox Permissions Model**: Per-plugin capability declarations (`process.execute`, `network`, `clipboard.write`, `notifications`) enforced by execution hooks.
- **Live Hot-Reloading**: Real-time plugin reload on file save (`QFileSystemWatcher`) and developer commands (`:plugins`, `:reload`, `:logs`, `:debug`, `:theme`).
- **8 Bundled Plugins**:
  - `Weather`: Live forecasts and condition cards via Open-Meteo API.
  - `Currency Exchange`: Real-time exchange rates with inverse rate cards.
  - `Developer Tools`: Quick actions to kill Gradle daemons, restart ADB, and open Android SDK / workspace directories.
  - `Docker`: Manage containers, view status, start/stop on Enter, copy IDs on Ctrl+Enter.
  - `Git & GitHub`: Copy current git branch and jump to repo pages.
  - `Web Search`: Direct browser search shortcuts (`g`, `google`, `gh`, `ddg`, `wiki`).
  - `UUID Generator`: Instant RFC 4122 v4 UUID generation.
  - `Timestamp Converter`: Convert unix epochs to human-readable UTC and local timestamps.

#### Packaging, CI/CD & Tooling
- **GitHub Actions Automation**: Multi-distro build matrix compiling and testing on Fedora 41 and Ubuntu 24.04.
- **Release Packaging**: Automatic generation of Fedora RPM (`.rpm`), Debian/Ubuntu DEB (`.deb`), and generic portable archive (`.tar.gz`) with SHA-256 checksums.
- **Offscreen Capture Utility**: Headless Qt tool (`tools/capture_screenshots.cpp`) rendering pixel-perfect 2x Retina PNG screenshots with transparent backgrounds for documentation.

### Changed
- Migrated codebase and D-Bus namespace to **MVSpotlight** (`org.mvspotlight.Launcher`) with legacy aliases maintained for backward compatibility.
- Expanded cross-distribution compatibility to support Qt 6.4 (Ubuntu 24.04 LTS) through Qt 6.11+ (Fedora Rawhide).

### Fixed
- Fixed result list keyboard navigation highlight wrap-around bug when cycling with Up/Down arrows.
- Fixed headless test execution abort in containerized CI environments by enforcing `QT_QPA_PLATFORM=offscreen`.
- Fixed autostart `.desktop` installation destination in CMakeLists to respect non-root packaging prefixes.

---

[Unreleased]: https://github.com/marconvcm/MVSpotlight/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/marconvcm/MVSpotlight/releases/tag/v1.0.0
