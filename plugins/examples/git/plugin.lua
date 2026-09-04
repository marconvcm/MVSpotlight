-- Git Plugin for MVSpotlight

launcher.register_command({
    id = "git.copy-branch",
    title = "Copy Current Git Branch",
    subtitle = "Git · Copy the active branch name to clipboard",
    icon = "vcs-branch",
    keywords = { "git branch", "copy branch", "current branch", "git" },
    execute = function()
        local res = launcher.process.run({
            command = "git",
            arguments = { "branch", "--show-current" }
        })
        if res and res.success and res.stdout ~= "" then
            local branch = string.gsub(res.stdout, "[\r\n]+", "")
            launcher.copy_to_clipboard(branch)
            launcher.notify("Git", "Branch name copied: " .. branch, "vcs-branch")
        else
            launcher.notify("Git", "Not inside a Git repository", "dialog-warning")
        end
    end
})

launcher.register_command({
    id = "git.status",
    title = "Git Status Notification",
    subtitle = "Git · Quick summary of staged and untracked changes",
    icon = "vcs-status-modified",
    keywords = { "git status", "git diff", "git changes", "git" },
    execute = function()
        local res = launcher.process.run({
            command = "git",
            arguments = { "status", "--short" }
        })
        if res and res.success then
            local lines = {}
            for line in string.gmatch(res.stdout, "[^\r\n]+") do
                table.insert(lines, line)
                if #lines >= 5 then break end
            end
            local summary = (#lines == 0) and "Working tree clean" or table.concat(lines, "\n")
            launcher.notify("Git Status", summary, "vcs-branch")
        else
            launcher.notify("Git", "Failed to retrieve status", "dialog-warning")
        end
    end
})
