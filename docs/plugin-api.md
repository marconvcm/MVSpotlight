# MVSpotlight Lua Plugin API Reference

Current Plugin API Version: `1`

The MVSpotlight Lua runtime provides a secure, embedded Lua 5.4 environment. All plugin capabilities are accessed through the global `launcher` object.

---

## 1. Plugin Manifest (`manifest.json`)

Every plugin directory must contain a valid `manifest.json`:

```json
{
    "id": "org.mvspotlight.example",
    "name": "Example Plugin",
    "version": "1.0.0",
    "description": "Short description of what the plugin does",
    "author": "Developer Name",
    "entry": "plugin.lua",
    "icon": "application-x-executable",
    "minimumApiVersion": 1,
    "permissions": [
        "process.execute",
        "notifications",
        "clipboard.write"
    ]
}
```

### Manifest Fields:
| Field | Type | Required | Description |
|---|---|---|---|
| `id` | string | Yes | Unique reverse-DNS identifier (e.g. `org.example.plugin`) |
| `name` | string | Yes | Human-readable plugin name |
| `version` | string | No | Semantic version string (default: `"1.0.0"`) |
| `description` | string | No | Brief explanation of plugin functionality |
| `author` | string | No | Author or organization name |
| `entry` | string | No | Entry script file relative to directory (default: `"plugin.lua"`) |
| `icon` | string | No | Freedesktop theme icon name or path |
| `minimumApiVersion`| int | No | Minimum required API version (current: `1`) |
| `permissions` | array | No | List of security permissions requested by the plugin |

---

## 2. Permissions System

Capabilities with security implications must be explicitly declared in `permissions`. Attempting to call an API without its permission will log a denial and abort execution.

| Permission | Enables |
|---|---|
| `process.execute` | `launcher.process.run`, `launcher.process.run_async` |
| `notifications` | `launcher.notify` |
| `clipboard.read` | `launcher.get_clipboard` |
| `clipboard.write` | `launcher.copy_to_clipboard` |
| `filesystem.read` | `launcher.open_file` |
| `filesystem.write`| Write operations on disk |
| `network` | `launcher.http.get` |
| `application.launch` | `launcher.open_url`, `launcher.launch_application` |

---

## 3. Command Registration API

For single-action shortcuts or commands that do not require live query filtering.

### `launcher.register_command(table)`
Registers a quick command into the launcher search index.

#### Parameters:
- `table` (table):
  - `id` (string, optional): Unique command identifier. Defaults to `<plugin_id>.<title>`.
  - `title` (string, required): Primary title displayed in the launcher.
  - `subtitle` (string, optional): Secondary descriptive subtitle.
  - `icon` (string, optional): Freedesktop theme icon name or relative image file path.
  - `keywords` (table of strings, optional): Words that match this command.
  - `execute` (function, required): Callback invoked when the user selects and executes this command.

#### Example:
```lua
launcher.register_command({
    id = "devtools.kill-gradle",
    title = "Kill Gradle Daemons",
    subtitle = "Stop all background Gradle processes",
    icon = "utilities-terminal",
    keywords = { "gradle", "kill gradle", "stop gradle" },
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

---

## 4. Search Provider API

For dynamic plugins that evaluate user queries and return structured results in real time.

### `launcher.register_provider(table)`
Registers a live query search provider.

#### Parameters:
- `table` (table):
  - `id` (string, optional): Provider identifier. Defaults to plugin ID.
  - `search` (function, required): `function(query)` $\rightarrow$ returns table of results.
  - `execute` (function, optional): `function(result)` invoked when an item is selected.

#### Result Object Structure:
| Key | Type | Description |
|---|---|---|
| `id` | string | Unique result ID |
| `title` | string | Main title displayed in large semibold text |
| `subtitle` | string | Descriptive secondary text |
| `icon` | string | Icon name from theme or local file |
| `score` | number | Suggested score (0–100) |
| `type` | string | Badge label (e.g. "Docker", "Generator", "Web") |
| `data` | table | Arbitrary payload passed to `execute(result)` |
| `secondaryAction` | string | Payload for Ctrl+Enter action |
| `secondaryActionLabel` | string | Label describing the secondary action |

#### Example:
```lua
launcher.register_provider({
    id = "uuid",
    search = function(query)
        if string.lower(query) == "uuid" then
            local id = "c5394859-8476-4cf9-0c9c-987856006b3a"
            return {
                {
                    id = "uuid:" .. id,
                    title = id,
                    subtitle = "Random UUID v4 · Press Enter to copy",
                    icon = "accessories-calculator",
                    score = 100,
                    type = "Generator",
                    data = { value = id }
                }
            }
        end
        return {}
    end,

    execute = function(result)
        launcher.copy_to_clipboard(result.data.value)
        launcher.notify("UUID Generator", "Copied UUID to clipboard")
    end
})
```

---

## 5. System Interaction APIs

### `launcher.notify(title, body, [icon])`
Sends a desktop notification using GNOME's native notification system via D-Bus.
- Requires permission: `notifications`
- Example: `launcher.notify("MVSpotlight", "Task finished", "dialog-information")`

### `launcher.copy_to_clipboard(text)`
Copies the string `text` to the system clipboard and selection clipboard.
- Requires permission: `clipboard.write`
- Example: `launcher.copy_to_clipboard("result data")`

### `launcher.get_clipboard()`
Retrieves current text content from the system clipboard.
- Requires permission: `clipboard.read`
- Returns: `string`

### `launcher.open_url(url)`
Opens a URL in the user's default web browser.
- Requires permission: `application.launch` or `network`
- Example: `launcher.open_url("https://github.com")`

### `launcher.open_file(filePath)`
Opens a local file or folder in the default file manager or editor via `xdg-open` / `gio`.
- Requires permission: `filesystem.read`
- Example: `launcher.open_file("/home/user/Documents")`

### `launcher.launch_application(command)`
Executes an application detached from the launcher process.
- Requires permission: `application.launch`
- Example: `launcher.launch_application("firefox")`

---

## 6. Configuration & Persistence

Configuration values are safely namespaced per plugin in `~/.config/mvspotlight/plugins.ini`. Plugins cannot overwrite or access other plugins' configuration.

### `launcher.get_config(key, [defaultValue])`
Retrieves a persistent setting value.
- Returns string, number, boolean, or default value.
- Example: `local socket = launcher.get_config("socket", "/var/run/docker.sock")`

### `launcher.set_config(key, value)`
Stores a persistent setting value.
- Example: `launcher.set_config("socket", "/run/user/1000/podman/podman.sock")`

---

## 7. Process Execution APIs

MVSpotlight enforces secure process execution via `QProcess` without shell interpolation.

### `launcher.process.run(options)`
Synchronously executes an external binary with a soft timeout.
- Requires permission: `process.execute`
- Options table:
  - `command` (string, required): Program binary name or path.
  - `arguments` (table of strings, optional): Command-line argument list.
  - `timeout` (number, optional): Timeout in milliseconds (default: 5000).
  - `working_dir` (string, optional): Working directory for process.
- Returns table:
  - `success` (boolean): `true` if exit code was 0.
  - `exit_code` (integer): Process exit status code.
  - `stdout` (string): Captured stdout output.
  - `stderr` (string): Captured stderr output.
  - `error` (string, optional): Error message if execution failed.

### `launcher.process.run_async(options)`
Asynchronously executes an external binary without blocking the UI thread.
- Requires permission: `process.execute`
- Options table:
  - `command` (string, required)
  - `arguments` (table of strings, optional)
  - `timeout` (number, optional)
  - `working_dir` (string, optional)
  - `on_complete` (function, required): `function(result)` callback.

#### Example:
```lua
launcher.process.run_async({
    command = "git",
    arguments = { "status", "--short" },
    on_complete = function(res)
        if res.success then
            launcher.notify("Git", res.stdout)
        end
    end
})
```

---

## 8. Network APIs

### `launcher.http.get(options)`
Asynchronously performs an HTTP GET request without blocking the UI thread.
- Requires permission: `network`
- Options table:
  - `url` (string, required): Request URL.
  - `on_complete` (function, required): `function(statusCode, responseBody)` callback.

#### Example:
```lua
launcher.http.get({
    url = "https://api.github.com/zen",
    on_complete = function(status, body)
        if status == 200 then
            launcher.notify("GitHub Zen", body)
        end
    end
})
```

---

## 9. Logging & Diagnostics

### `launcher.log(message)`
Writes a timestamped line to the launcher's persistent log file at:
`~/.local/state/mvspotlight/launcher.log`
