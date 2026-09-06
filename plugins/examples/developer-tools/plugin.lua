-- Developer Tools Plugin for MVSpotlight

launcher.register_command({
    id = "devtools.kill-gradle",
    title = "Kill Gradle Daemons",
    subtitle = "Developer Tools · Stop all background Gradle processes",
    icon = "utilities-terminal",
    keywords = { "gradle", "kill gradle", "stop gradle", "clean gradle" },
    execute = function()
        launcher.process.run_async({
            command = "pkill",
            arguments = { "-f", "gradle.*daemon" },
            on_complete = function(res)
                launcher.notify("Developer Tools", "Gradle daemons stopped successfully", "utilities-terminal")
            end
        })
    end
})

launcher.register_command({
    id = "devtools.restart-adb",
    title = "Restart ADB Server",
    subtitle = "Developer Tools · Kill and restart Android Debug Bridge",
    icon = "phone",
    keywords = { "adb", "restart adb", "kill adb", "android adb" },
    execute = function()
        launcher.process.run_async({
            command = "adb",
            arguments = { "kill-server" },
            on_complete = function()
                launcher.process.run_async({
                    command = "adb",
                    arguments = { "start-server" },
                    on_complete = function()
                        launcher.notify("Developer Tools", "ADB server restarted", "phone")
                    end
                })
            end
        })
    end
})

local function expand_path(p)
    if not p or p == "" then return "" end
    local home = os.getenv("HOME") or "/home"
    if p:sub(1, 2) == "~/" then
        return home .. p:sub(2)
    elseif p == "~" then
        return home
    end
    return p
end

launcher.register_command({
    id = "devtools.open-android-sdk",
    title = "Open Android SDK Directory",
    subtitle = "Developer Tools · Navigate to Android SDK folder",
    icon = "folder",
    keywords = { "android", "sdk", "android sdk", "sdk folder" },
    execute = function()
        local sdk_path = launcher.get_config("android_sdk_path", "~/Android/Sdk")
        if not sdk_path or sdk_path == "" then sdk_path = "~/Android/Sdk" end
        local full_path = expand_path(sdk_path)
        local ok = launcher.open_file(full_path)
        if not ok then
            launcher.notify("Developer Tools", "Could not open Android SDK folder: " .. full_path, "dialog-warning")
        end
    end
})

launcher.register_command({
    id = "devtools.open-workspace",
    title = "Open Projects Workspace",
    subtitle = "Developer Tools · Open development workspace folder",
    icon = "folder-saved-search",
    keywords = { "workspace", "projects", "dev", "development", "code" },
    execute = function()
        local ws_path = launcher.get_config("workspace_path", "~/Workspace")
        if not ws_path or ws_path == "" then ws_path = "~/Workspace" end
        local full_path = expand_path(ws_path)
        local ok = launcher.open_file(full_path)
        if not ok then
            launcher.notify("Developer Tools", "Could not open workspace folder: " .. full_path, "dialog-warning")
        end
    end
})
