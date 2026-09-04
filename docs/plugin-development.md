# Lua Plugin Development Guide

This guide walks you through building, testing, and debugging plugins for MVSpotlight. You can create a fully functional, production-ready launcher plugin in approximately 20–30 lines of Lua.

---

## 1. Quickstart: 20-Line Plugin

Let's build a plugin that displays your public IP address.

### 1. Create Plugin Folder
```bash
mkdir -p ~/.local/share/mvspotlight/plugins/my-ip
cd ~/.local/share/mvspotlight/plugins/my-ip
```

### 2. Create `manifest.json`
```json
{
    "id": "com.example.myip",
    "name": "My IP",
    "version": "1.0.0",
    "description": "Show public IP address and copy on enter",
    "author": "Your Name",
    "entry": "plugin.lua",
    "icon": "network-wired",
    "minimumApiVersion": 1,
    "permissions": [
        "network",
        "clipboard.write",
        "notifications"
    ]
}
```

### 3. Create `plugin.lua`
```lua
launcher.register_provider({
    id = "myip",
    search = function(query)
        if query:lower() == "ip" or query:lower() == "my ip" then
            return {
                {
                    id = "myip:check",
                    title = "Fetch Public IP Address",
                    subtitle = "Press Enter to query ifconfig.me and copy to clipboard",
                    icon = "network-wired",
                    score = 100,
                    type = "Network"
                }
            }
        end
        return {}
    end,

    execute = function()
        launcher.http.get({
            url = "https://ifconfig.me/ip",
            on_complete = function(status, body)
                if status == 200 then
                    local ip = body:gsub("%s+", "")
                    launcher.copy_to_clipboard(ip)
                    launcher.notify("Public IP", ip, "network-wired")
                else
                    launcher.notify("Error", "Could not fetch IP", "dialog-error")
                end
            end
        })
    end
})
```

### 4. Reload MVSpotlight
Type `:reload` in MVSpotlight or run `mvspotlight --reload-plugins`.
Now type `ip` $\rightarrow$ press **Enter** $\rightarrow$ your public IP is automatically copied to your clipboard with a desktop notification!

---

## 2. Plugin Locations

MVSpotlight scans the following locations:
1. **User Plugins**:
   `~/.local/share/mvspotlight/plugins/<plugin-id>/`
2. **System-wide Plugins**:
   `/usr/share/mvspotlight/plugins/<plugin-id>/`
3. **Bundled Plugins**:
   `<installation-path>/share/mvspotlight/plugins/<plugin-id>/`

Each plugin must be housed in its own subdirectory containing at least `manifest.json` and the entry script (`plugin.lua`).

---

## 3. Commands vs. Providers

### When to use `launcher.register_command`
Use commands when you want to expose fixed actions or shortcuts that trigger on specific keywords:
- "Kill Gradle"
- "Restart Bluetooth"
- "Open Project Folder"
- "Lock Screen"

```lua
launcher.register_command({
    id = "devtools.adb-restart",
    title = "Restart ADB",
    subtitle = "Kill and restart Android Debug Bridge",
    icon = "phone",
    keywords = { "adb", "android", "restart adb" },
    execute = function()
        launcher.process.run_async({
            command = "adb",
            arguments = { "kill-server" },
            on_complete = function()
                launcher.process.run({ command = "adb", arguments = { "start-server" } })
                launcher.notify("ADB", "ADB Server Restarted")
            end
        })
    end
})
```

### When to use `launcher.register_provider`
Use providers when you need to inspect the live query and return a list of items:
- Searching Docker containers (`docker ps`)
- Calculating conversions (`timestamp 1788500000`, `100 usd in eur`)
- Web search suggestions (`google <query>`, `gh <query>`)
- Git branch switcher

```lua
launcher.register_provider({
    id = "git-branches",
    search = function(query)
        -- evaluate query and return matching items
        return {
            {
                title = "main",
                subtitle = "Active branch · Press Enter to switch",
                icon = "vcs-branch",
                score = 90
            }
        }
    end,
    execute = function(item)
        -- execute action
    end
})
```

---

## 4. UI Thread Safety & Async Rules

MVSpotlight requires 60 FPS animation performance and sub-100ms latency.
- **Never perform blocking I/O on the search callback**: Do not perform synchronous network queries or long-running shell scripts inside `search(query)`.
- **Soft Timeout Watchdog**: If a search callback takes longer than ~100ms, the watchdog will abort it.
- **Async Execution**: Use `launcher.process.run_async` or `launcher.http.get` for tasks that take more than a few milliseconds.
- **Stale Searches**: The C++ search controller automatically cancels and ignores results from previous queries when the user continues typing.

---

## 5. Developer Commands & Debugging

MVSpotlight provides built-in developer commands directly in the search bar:

| Command | Action |
|---|---|
| `:plugins` | Displays all loaded plugins with version and status (`Enabled`, `Error`, `Disabled`) |
| `:reload` | Hot-reloads all Lua plugins from disk without restarting the launcher |
| `:logs` | Shows the persistent log file path (`~/.local/state/mvspotlight/launcher.log`) |
| `:debug` | Displays system info (monitors, screen coordinates, Wayland display) |
| `:theme` | Toggles between dark and light palette |

### Monitoring Logs:
```bash
tail -f ~/.local/state/mvspotlight/launcher.log
```

---

## 6. Hot Reloading During Development

MVSpotlight monitors your plugin directory with `QFileSystemWatcher`. Whenever you edit and save `plugin.lua` or `manifest.json`, the launcher reloads the script automatically!
