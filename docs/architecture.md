# Architecture & Technical Design

## Overview

MVSpotlight is a macOS Spotlight-inspired universal command palette and desktop launcher created specifically for **GNOME 50 on Wayland**. It delivers an instant, keyboard-first desktop experience combining macOS interaction elegance with modern Linux/GNOME simplicity and Raycast-like extensibility via embedded Lua 5.4.

```
+-------------------------------------------------------------------------+
|                        GNOME 50 Wayland Session                         |
|                                                                         |
|   [Alt + Space]  --->  gsettings media-key  --->  mvspotlight --toggle  |
+-------------------------------------------------------------------------+
                                    |
                                    v (D-Bus IPC: org.mvspotlight.Launcher)
+-------------------------------------------------------------------------+
|                       MVSpotlight Core Daemon                           |
|                                                                         |
|   +-----------------------------------------------------------------+   |
|   |                      QML / Qt Quick View                        |   |
|   |   Frameless · Multi-monitor Centered (26% Top) · Translucent    |   |
|   |   Dynamic Animated Height · Soft Shadow · Dark/Light Mode       |   |
|   +-----------------------------------------------------------------+   |
|                                   ^                                     |
|                                   | QAbstractListModel                  |
|   +-----------------------------------------------------------------+   |
|   |                        SearchController                         |   |
|   |   Query Router · Unified Ranking Pipeline · Frecency Weighting  |   |
|   |   Request ID Cancellation Tokens · Action Execution             |   |
|   +-----------------------------------------------------------------+   |
|               ^                 ^                 ^                     |
|               |                 |                 |                     |
|   +-----------+--+     +--------+-----+     +-----+-----------------+   |
|   | Native Apps  |     | Math / Calc  |     | Lua Plugin Manager    |   |
|   | .desktop XDG |     | Safe Parser  |     | Embedded Lua 5.4      |   |
|   | Settings/Act |     | No shell     |     | Permissions / Sandbox |   |
|   | File Search  |     | Direct copy  |     | Async Watchdog & Rel  |   |
|   +--------------+     +--------------+     +-----------------------+   |
+-------------------------------------------------------------------------+
```

---

## 1. Wayland-Only Architecture

MVSpotlight is strictly **Wayland-native**. It avoids legacy X11 abstractions:
- **Zero X11 Dependencies**: No Xlib, XCB, `XGrabKey`, `XSetInputFocus`, or X11 window-manager hacks.
- **Wayland Window Configuration**: Frameless window flags (`Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Window`) with transparent alpha backing.
- **Active Monitor Positioning**:
  - Detects pointer position via `QCursor::pos()`.
  - Maps to the target `QScreen` in `QGuiApplication::screens()`.
  - Dynamically positions the window horizontally centered and vertically `26%` from the top of the monitor rather than mathematically centered:
    $$\text{TargetY} = \text{ScreenY} + 0.26 \times \text{ScreenHeight}$$
- **Keyboard Activation without XGrabKey**: Under GNOME Wayland, applications are intentionally forbidden from arbitrary global keystroke snooping. Activation is achieved natively through GNOME's custom shortcut system calling `mvspotlight --toggle`, which communicates with the resident daemon via D-Bus in under 5ms.

---

## 2. Background Daemon & Single-Instance D-Bus Architecture

To achieve perceived launcher appearance in `<50ms`, MVSpotlight runs as a persistent background daemon.

### D-Bus Interface Specification:
- **Service Name**: `org.mvspotlight.Launcher` (legacy alias: `org.example.Spotlight`)
- **Object Path**: `/org/mvspotlight/Launcher` (legacy alias: `/org/example/Spotlight`)
- **Interface**: `org.mvspotlight.Launcher` (legacy alias: `org.example.Spotlight`)

#### Methods:
| Method | Arguments | Description |
|---|---|---|
| `Show()` | *None* | Positions and animates launcher onto active monitor |
| `Hide()` | *None* | Hides launcher and clears query |
| `Toggle()` | *None* | Toggles launcher visibility |
| `Search(query)` | `string query` | Displays launcher pre-filled with search query |
| `ReloadPlugins()` | *None* | Hot-reloads all Lua plugins |
| `ListPlugins()` | *None* $\rightarrow$ `string[]` | Returns list of loaded plugins and states |

#### Signals:
- `Shown()`: Emitted when launcher appears.
- `Hidden()`: Emitted when launcher is dismissed.

When invoking `mvspotlight` CLI (e.g. `mvspotlight --toggle`), the process checks if `org.mvspotlight.Launcher` is registered. If present, it executes the D-Bus call and terminates immediately with exit code 0.

---

## 3. Search & Ranking Pipeline

Every keystroke produces a monotonically increasing `quint64 requestId`. Search results pass through an integrated ranking and deduplication pipeline.

### Scoring Factors:
1. **Exact Match** (Score: $100.0$)
2. **Prefix Match** (Score: $90.0 - 95.0$)
3. **Word-Boundary Match** (Score: $80.0 - 89.0$)
4. **Substring Match** (Score: $65.0 - 79.0$)
5. **Fuzzy Match** (Score: $45.0 - 64.0$)
6. **Frecency Boost**:
   Computed by `UsageHistory` from `~/.local/state/mvspotlight/history.json`:
   $$\text{Score}_{\text{final}} = \text{Score}_{\text{base}} + \text{FrecencyBoost}$$
   Where $\text{FrecencyBoost} = \min(10, 3 \times \log_2(1 + \text{launchCount})) \times \text{recencyFactor} \times 2.0$.

### Asynchronous Request Cancellation:
As the user types (`d` $\rightarrow$ `do` $\rightarrow$ `doc` $\rightarrow$ `dock` $\rightarrow$ `docker`), previous asynchronous searches (such as filesystem scans) compare their capture ID against `m_currentRequestId`. If stale, results are discarded before touching the UI model.

---

## 4. Embedded Lua 5.4 Runtime & Plugin System

Lua plugins provide extensibility without recompilation:
- **Embedded C Lua 5.4**: Bundled and compiled directly into the binary with zero external shared library dependencies.
- **Sandboxed Execution**: Dangerous C standard library calls (`os.execute`, `os.remove`, `os.rename`, `os.exit`) are stripped from global Lua tables.
- **Safe Process Execution**: All external commands are mediated by `launcher.process.run` using C++ `QProcess` with argument lists (preventing shell injection).
- **Lightweight Permissions Layer**: Plugins declare required permissions in `manifest.json` (`process.execute`, `filesystem.read`, `notifications`, `clipboard.write`, etc.). Unauthorized API calls are rejected and logged.
- **Watchdog Protection**: Soft execution timeout (~100ms) with `lua_sethook` instruction count hooks to prevent scripts from freezing the UI thread.
- **Crash Isolation & Recovery**: All Lua calls execute inside protected environments (`lua_pcall` with stack tracebacks). Faulty plugins are isolated; if a plugin encounters $\ge 3$ consecutive fatal errors, it is automatically disabled for the session.
- **Hot Reloading**: `QFileSystemWatcher` observes user and system plugin folders to reinitialize plugins upon file save.

---

## 5. UI Presentation & Surface Design

- **Frameless Window**: Zero native window borders or titlebars.
- **Translucent Charcoal & Off-White**: Follows GNOME `org.gnome.desktop.interface color-scheme` in real-time.
- **Dynamic Animated Height**:
  $$\text{Height} = \begin{cases} 76\text{px} & \text{query empty} \\ 180\text{px} & \text{no results} \\ \min(620\text{px}, 76 + 12 + \min(7, N) \times 62) & N \text{ results} \end{cases}$$
- **Multi-layer Shadow**: Outward drop-shadow rendered outside the main card surface within the transparent window frame.
- **Typography**: Uses system font family at high DPI with demibold titles and muted subtitles.
