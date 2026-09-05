-- GitHub Plugin for MVSpotlight
-- Enables searching & browsing repositories, listing & filtering pull requests,
-- review requests, open issues, quick navigation, and repository actions.

local USER_LOGIN = "me"
local REPOS = {}
local PRS = {}
local REVIEWS = {}
local ISSUES = {}

local LAST_UPDATE = {
    repos = 0,
    prs = 0,
    reviews = 0,
    issues = 0
}

local IN_FLIGHT = {
    repos = false,
    prs = false,
    reviews = false,
    issues = false
}

-- Micro JSON Decoder in pure Lua
local function json_decode(str)
    if not str or #str == 0 then return nil end
    local pos = 1
    local len = #str

    local function skip_whitespace()
        while pos <= len do
            local b = str:byte(pos)
            if b == 32 or b == 9 or b == 10 or b == 13 then
                pos = pos + 1
            else
                break
            end
        end
    end

    local parse_value

    local function parse_string()
        pos = pos + 1
        local start = pos
        local chunks = {}
        while pos <= len do
            local c = str:sub(pos, pos)
            if c == "\"" then
                table.insert(chunks, str:sub(start, pos - 1))
                pos = pos + 1
                return table.concat(chunks)
            elseif c == "\\" then
                table.insert(chunks, str:sub(start, pos - 1))
                pos = pos + 1
                local esc = str:sub(pos, pos)
                if esc == "n" then table.insert(chunks, "\n")
                elseif esc == "r" then table.insert(chunks, "\r")
                elseif esc == "t" then table.insert(chunks, "\t")
                elseif esc == "\"" then table.insert(chunks, "\"")
                elseif esc == "\\" then table.insert(chunks, "\\")
                elseif esc == "/" then table.insert(chunks, "/")
                elseif esc == "u" then
                    local hex = str:sub(pos + 1, pos + 4)
                    pos = pos + 4
                    local code = tonumber(hex, 16) or 63
                    if code < 128 then
                        table.insert(chunks, string.char(code))
                    else
                        table.insert(chunks, "?")
                    end
                else
                    table.insert(chunks, esc)
                end
                pos = pos + 1
                start = pos
            else
                pos = pos + 1
            end
        end
        return table.concat(chunks)
    end

    local function parse_number()
        local start = pos
        if str:sub(pos, pos) == "-" then pos = pos + 1 end
        while pos <= len and str:sub(pos, pos):match("[%d%.eE%-%+]") do
            pos = pos + 1
        end
        return tonumber(str:sub(start, pos - 1))
    end

    local function parse_array()
        pos = pos + 1
        local arr = {}
        skip_whitespace()
        if pos <= len and str:sub(pos, pos) == "]" then
            pos = pos + 1
            return arr
        end
        while pos <= len do
            local val = parse_value()
            table.insert(arr, val)
            skip_whitespace()
            local c = str:sub(pos, pos)
            if c == "]" then
                pos = pos + 1
                return arr
            elseif c == "," then
                pos = pos + 1
            else
                break
            end
        end
        return arr
    end

    local function parse_object()
        pos = pos + 1
        local obj = {}
        skip_whitespace()
        if pos <= len and str:sub(pos, pos) == "}" then
            pos = pos + 1
            return obj
        end
        while pos <= len do
            skip_whitespace()
            if str:sub(pos, pos) ~= "\"" then break end
            local key = parse_string()
            skip_whitespace()
            if str:sub(pos, pos) == ":" then pos = pos + 1 end
            local val = parse_value()
            obj[key] = val
            skip_whitespace()
            local c = str:sub(pos, pos)
            if c == "}" then
                pos = pos + 1
                return obj
            elseif c == "," then
                pos = pos + 1
            else
                break
            end
        end
        return obj
    end

    function parse_value()
        skip_whitespace()
        if pos > len then return nil end
        local c = str:sub(pos, pos)
        if c == "\"" then
            return parse_string()
        elseif c == "{" then
            return parse_object()
        elseif c == "[" then
            return parse_array()
        elseif c == "t" and str:sub(pos, pos + 3) == "true" then
            pos = pos + 4
            return true
        elseif c == "f" and str:sub(pos, pos + 4) == "false" then
            pos = pos + 5
            return false
        elseif c == "n" and str:sub(pos, pos + 3) == "null" then
            pos = pos + 4
            return nil
        else
            return parse_number()
        end
    end

    local ok, res = pcall(parse_value)
    if ok then return res else return nil end
end

-- Relative time formatting (e.g. "2h ago", "3d ago")
local function parse_iso_time(iso_str)
    if not iso_str then return nil end
    local y, m, d, h, min, s = iso_str:match("(%d+)-(%d+)-(%d+)T(%d+):(%d+):(%d+)")
    if not y then return nil end
    return os.time({
        year = tonumber(y),
        month = tonumber(m),
        day = tonumber(d),
        hour = tonumber(h),
        min = tonumber(min),
        sec = tonumber(s)
    })
end

local function format_time_ago(iso_str)
    local t = parse_iso_time(iso_str)
    if not t then return iso_str or "" end
    local diff = os.time() - t
    if diff < 60 then
        return "just now"
    elseif diff < 3600 then
        return string.format("%dm ago", math.floor(diff / 60))
    elseif diff < 86400 then
        return string.format("%dh ago", math.floor(diff / 3600))
    elseif diff < 86400 * 30 then
        return string.format("%dd ago", math.floor(diff / 86400))
    else
        return string.format("%dmo ago", math.floor(diff / (86400 * 30)))
    end
end

local function url_encode(str)
    if not str then return "" end
    return (str:gsub("%s+", "+"):gsub("([^%w%+%-%.%_])", function(c)
        return string.format("%%%02X", string.byte(c))
    end))
end

-- Disk Persistence
local function load_cache()
    pcall(function()
        local saved_user = launcher.get_config("user_login", "")
        if saved_user and #saved_user > 0 then USER_LOGIN = saved_user end

        local repos_raw = launcher.get_config("cache_repos", "")
        if #repos_raw > 0 then
            local data = json_decode(repos_raw)
            if data and type(data) == "table" then REPOS = data end
        end

        local prs_raw = launcher.get_config("cache_prs", "")
        if #prs_raw > 0 then
            local data = json_decode(prs_raw)
            if data and type(data) == "table" then PRS = data end
        end

        local revs_raw = launcher.get_config("cache_reviews", "")
        if #revs_raw > 0 then
            local data = json_decode(revs_raw)
            if data and type(data) == "table" then REVIEWS = data end
        end

        local issues_raw = launcher.get_config("cache_issues", "")
        if #issues_raw > 0 then
            local data = json_decode(issues_raw)
            if data and type(data) == "table" then ISSUES = data end
        end
    end)
end

-- Asynchronous Background Sync
local function fetch_user()
    pcall(function()
        local custom_user = launcher.get_config("user_login", "")
        if custom_user and #custom_user > 0 then
            USER_LOGIN = custom_user
            return
        end

        launcher.process.run_async({
            command = "gh",
            args = { "api", "user", "-q", ".login" },
            timeout_ms = 4000,
            on_complete = function(res)
                if res.success and res.exit_code == 0 and res.stdout then
                    local user = res.stdout:gsub("%s+", "")
                    if #user > 0 then
                        USER_LOGIN = user
                        launcher.set_config("user_login", user)
                    end
                end
            end
        })
    end)
end

local function fetch_repos(force)
    if IN_FLIGHT.repos then return end
    local ttl = tonumber(launcher.get_config("cache_duration", 600)) or 600
    if not force and (os.time() - LAST_UPDATE.repos < ttl) then return end
    IN_FLIGHT.repos = true

    pcall(function()
        launcher.process.run_async({
            command = "gh",
            args = { "repo", "list", "--limit", "40", "--json", "nameWithOwner,description,url,updatedAt,isPrivate,stargazerCount" },
            timeout_ms = 8000,
            on_complete = function(res)
                IN_FLIGHT.repos = false
                if res.success and res.exit_code == 0 and res.stdout then
                    local data = json_decode(res.stdout)
                    if data and type(data) == "table" then
                        REPOS = data
                        LAST_UPDATE.repos = os.time()
                        launcher.set_config("cache_repos", res.stdout)
                        launcher.log(string.format("GitHub plugin: updated %d repositories", #data))
                    end
                end
            end
        })
    end)
end

local function fetch_prs(force)
    if IN_FLIGHT.prs then return end
    if not force and (os.time() - LAST_UPDATE.prs < 180) then return end
    IN_FLIGHT.prs = true

    pcall(function()
        launcher.process.run_async({
            command = "gh",
            args = { "search", "prs", "--author", "@me", "--state", "open", "--limit", "30", "--json", "title,number,repository,url,updatedAt,state" },
            timeout_ms = 8000,
            on_complete = function(res)
                IN_FLIGHT.prs = false
                if res.success and res.exit_code == 0 and res.stdout then
                    local data = json_decode(res.stdout)
                    if data and type(data) == "table" then
                        PRS = data
                        LAST_UPDATE.prs = os.time()
                        launcher.set_config("cache_prs", res.stdout)
                        launcher.log(string.format("GitHub plugin: updated %d open PRs", #data))
                    end
                end
            end
        })
    end)
end

local function fetch_reviews(force)
    if IN_FLIGHT.reviews then return end
    if not force and (os.time() - LAST_UPDATE.reviews < 180) then return end
    IN_FLIGHT.reviews = true

    pcall(function()
        launcher.process.run_async({
            command = "gh",
            args = { "search", "prs", "--review-requested", "@me", "--state", "open", "--limit", "15", "--json", "title,number,repository,url,updatedAt" },
            timeout_ms = 8000,
            on_complete = function(res)
                IN_FLIGHT.reviews = false
                if res.success and res.exit_code == 0 and res.stdout then
                    local data = json_decode(res.stdout)
                    if data and type(data) == "table" then
                        REVIEWS = data
                        LAST_UPDATE.reviews = os.time()
                        launcher.set_config("cache_reviews", res.stdout)
                    end
                end
            end
        })
    end)
end

local function fetch_issues(force)
    if IN_FLIGHT.issues then return end
    if not force and (os.time() - LAST_UPDATE.issues < 300) then return end
    IN_FLIGHT.issues = true

    pcall(function()
        launcher.process.run_async({
            command = "gh",
            args = { "search", "issues", "--author", "@me", "--state", "open", "--limit", "20", "--json", "title,number,repository,url,updatedAt" },
            timeout_ms = 8000,
            on_complete = function(res)
                IN_FLIGHT.issues = false
                if res.success and res.exit_code == 0 and res.stdout then
                    local data = json_decode(res.stdout)
                    if data and type(data) == "table" then
                        ISSUES = data
                        LAST_UPDATE.issues = os.time()
                        launcher.set_config("cache_issues", res.stdout)
                    end
                end
            end
        })
    end)
end

local function fetch_all(force)
    fetch_user()
    fetch_repos(force)
    fetch_prs(force)
    fetch_reviews(force)
    fetch_issues(force)
end

-- Initialize cache and trigger background sync
load_cache()
fetch_all(false)

-- Register Provider
launcher.register_provider({
    id = "github",
    name = "GitHub",
    search = function(query)
        local raw = query:gsub("^%s+", ""):gsub("%s+$", "")
        local q = raw:lower()
        if #q == 0 then return {} end

        -- 1. Direct Owner/Repo Match (e.g. "facebook/react", "torvalds/linux", "marconvcm/hitkill")
        local direct_owner, direct_repo = raw:match("^([%w%.%_%-]+)/([%w%.%_%-]+)$")
        if direct_owner and direct_repo and direct_owner:lower() ~= "http:" and direct_owner:lower() ~= "https:" then
            local full_repo = direct_owner .. "/" .. direct_repo
            local base_url = "https://github.com/" .. full_repo
            return {
                {
                    id = "gh:repo:" .. full_repo,
                    title = full_repo,
                    subtitle = "Open repository on GitHub • Press Enter",
                    icon = "folder-remote",
                    score = 150.0,
                    type = "Repository",
                    provider = "GitHub",
                    action = "open_url:" .. base_url,
                    secondaryActionLabel = "Copy Clone Command",
                    secondaryAction = "copy_clone:" .. base_url
                },
                {
                    id = "gh:repo:prs:" .. full_repo,
                    title = "Pull Requests: " .. full_repo,
                    subtitle = "Browse pull requests on GitHub",
                    icon = "vcs-merge-request",
                    score = 140.0,
                    type = "Pull Requests",
                    provider = "GitHub",
                    action = "open_url:" .. base_url .. "/pulls"
                },
                {
                    id = "gh:repo:issues:" .. full_repo,
                    title = "Issues: " .. full_repo,
                    subtitle = "Browse issues on GitHub",
                    icon = "dialog-warning",
                    score = 135.0,
                    type = "Issues",
                    provider = "GitHub",
                    action = "open_url:" .. base_url .. "/issues"
                },
                {
                    id = "gh:repo:releases:" .. full_repo,
                    title = "Releases: " .. full_repo,
                    subtitle = "View tags and releases",
                    icon = "applications-development",
                    score = 130.0,
                    type = "Releases",
                    provider = "GitHub",
                    action = "open_url:" .. base_url .. "/releases"
                }
            }
        end

        -- 2. Check for "gh" or "github" prefix
        local sub = nil
        if q == "gh" or q == "github" then
            sub = ""
        elseif q:sub(1, 3) == "gh " then
            sub = raw:sub(4):gsub("^%s+", "")
        elseif q:sub(1, 7) == "github " then
            sub = raw:sub(8):gsub("^%s+", "")
        end

        if not sub then return {} end

        local sub_lower = sub:lower():gsub("^%s+", ""):gsub("%s+$", "")

        -- Case A: "gh" or "github" alone -> Dashboard Hub
        if #sub_lower == 0 then
            -- Lazy refresh if data is old
            fetch_all(false)

            local pr_count = #PRS
            local repo_count = #REPOS
            local review_count = #REVIEWS
            local issue_count = #ISSUES

            local hub_items = {
                {
                    id = "gh:hub:prs",
                    title = string.format("Pull Requests (%d open)", pr_count),
                    subtitle = "View your open pull requests • Type 'gh pr' to filter or press Enter to open",
                    icon = "vcs-merge-request",
                    score = 160.0,
                    type = "GitHub Dashboard",
                    provider = "GitHub",
                    action = "open_url:https://github.com/pulls"
                },
                {
                    id = "gh:hub:repos",
                    title = string.format("My Repositories (%d)", repo_count),
                    subtitle = "Browse and search repositories • Type 'gh repo' or press Enter to open",
                    icon = "folder-remote",
                    score = 150.0,
                    type = "GitHub Dashboard",
                    provider = "GitHub",
                    action = "open_url:https://github.com/" .. USER_LOGIN .. "?tab=repositories"
                }
            }

            if review_count > 0 then
                table.insert(hub_items, {
                    id = "gh:hub:reviews",
                    title = string.format("Review Requests (%d)", review_count),
                    subtitle = "Pull requests awaiting your review • Type 'gh review'",
                    icon = "emblem-favorite",
                    score = 145.0,
                    type = "GitHub Dashboard",
                    provider = "GitHub",
                    action = "open_url:https://github.com/pulls/review-requested"
                })
            end

            if issue_count > 0 then
                table.insert(hub_items, {
                    id = "gh:hub:issues",
                    title = string.format("My Issues (%d open)", issue_count),
                    subtitle = "Browse open issues created by you • Type 'gh issue'",
                    icon = "dialog-warning",
                    score = 140.0,
                    type = "GitHub Dashboard",
                    provider = "GitHub",
                    action = "open_url:https://github.com/issues"
                })
            end

            table.insert(hub_items, {
                id = "gh:hub:new",
                title = "New Repository",
                subtitle = "Create a new repository on GitHub • github.com/new",
                icon = "applications-development",
                score = 135.0,
                type = "Action",
                provider = "GitHub",
                action = "open_url:https://github.com/new"
            })

            table.insert(hub_items, {
                id = "gh:hub:gist",
                title = "New Gist",
                subtitle = "Create a code snippet or note • gist.github.com",
                icon = "text-x-generic",
                score = 130.0,
                type = "Action",
                provider = "GitHub",
                action = "open_url:https://gist.github.com"
            })

            table.insert(hub_items, {
                id = "gh:hub:profile",
                title = string.format("Profile (@%s)", USER_LOGIN),
                subtitle = "Open your GitHub profile page",
                icon = "applications-development",
                score = 125.0,
                type = "Navigation",
                provider = "GitHub",
                action = "open_url:https://github.com/" .. USER_LOGIN
            })

            table.insert(hub_items, {
                id = "gh:hub:trending",
                title = "Trending Repositories",
                subtitle = "See what the GitHub community is most excited about today",
                icon = "folder-remote",
                score = 120.0,
                type = "Navigation",
                provider = "GitHub",
                action = "open_url:https://github.com/trending"
            })

            table.insert(hub_items, {
                id = "gh:hub:refresh",
                title = "Refresh GitHub Data",
                subtitle = "Re-sync cached repositories, pull requests, and issues from gh CLI",
                icon = "view-refresh",
                score = 110.0,
                type = "Action",
                provider = "GitHub",
                action = "force_refresh"
            })

            return hub_items
        end

        -- Case B: Pull Requests ("gh pr", "gh prs", "gh pull", "gh pr <filter>")
        local is_pr_query = false
        local pr_term = ""
        if sub_lower == "pr" or sub_lower == "prs" or sub_lower == "pull" or sub_lower == "pulls" or sub_lower == "pr list" then
            is_pr_query = true
        elseif sub_lower:sub(1, 3) == "pr " then
            is_pr_query = true
            pr_term = sub_lower:sub(4):gsub("^%s+", "")
        elseif sub_lower:sub(1, 4) == "prs " then
            is_pr_query = true
            pr_term = sub_lower:sub(5):gsub("^%s+", "")
        elseif sub_lower:sub(1, 5) == "pull " or sub_lower:sub(1, 6) == "pulls " then
            is_pr_query = true
            pr_term = sub_lower:gsub("^%a+%s+", "")
        end

        if is_pr_query then
            fetch_prs(false)
            local results = {}

            for _, pr in ipairs(PRS) do
                local repo_name = (pr.repository and pr.repository.nameWithOwner) or ""
                local title = pr.title or ""
                local num_str = tostring(pr.number or "")
                local match = true

                if #pr_term > 0 then
                    local lower_title = title:lower()
                    local lower_repo = repo_name:lower()
                    if not lower_title:find(pr_term, 1, true) and
                       not lower_repo:find(pr_term, 1, true) and
                       not num_str:find(pr_term, 1, true) then
                        match = false
                    end
                end

                if match then
                    local item_title = string.format("%s #%s: %s", repo_name, num_str, title)
                    local time_str = format_time_ago(pr.updatedAt)
                    local subtitle = string.format("Updated %s • Press Enter to open PR in browser", time_str)

                    table.insert(results, {
                        id = "gh:pr:" .. (pr.url or num_str),
                        title = item_title,
                        subtitle = subtitle,
                        icon = "vcs-merge-request",
                        score = 150.0 - #results,
                        type = "Pull Request",
                        provider = "GitHub",
                        action = "open_url:" .. pr.url,
                        secondaryActionLabel = "Copy PR URL",
                        secondaryAction = "copy:" .. pr.url
                    })

                    if #results >= 15 then break end
                end
            end

            if #results == 0 then
                table.insert(results, {
                    id = "gh:pr:none",
                    title = #pr_term > 0 and ("No PRs matching '" .. pr_term .. "'") or "No Open Pull Requests",
                    subtitle = "Press Enter to open your pull requests on GitHub",
                    icon = "vcs-merge-request",
                    score = 120.0,
                    type = "Pull Request",
                    provider = "GitHub",
                    action = "open_url:https://github.com/pulls"
                })
            end

            return results
        end

        -- Case C: Review Requests ("gh review", "gh reviews")
        if sub_lower == "review" or sub_lower == "reviews" or sub_lower == "pr review" then
            fetch_reviews(false)
            local results = {}

            for _, pr in ipairs(REVIEWS) do
                local repo_name = (pr.repository and pr.repository.nameWithOwner) or ""
                local title = pr.title or ""
                local num_str = tostring(pr.number or "")
                local item_title = string.format("[Review] %s #%s: %s", repo_name, num_str, title)
                local time_str = format_time_ago(pr.updatedAt)

                table.insert(results, {
                    id = "gh:review:" .. (pr.url or num_str),
                    title = item_title,
                    subtitle = string.format("Updated %s • Review requested", time_str),
                    icon = "emblem-favorite",
                    score = 150.0 - #results,
                    type = "Review Request",
                    provider = "GitHub",
                    action = "open_url:" .. pr.url,
                    secondaryActionLabel = "Copy URL",
                    secondaryAction = "copy:" .. pr.url
                })
            end

            if #results == 0 then
                table.insert(results, {
                    id = "gh:review:none",
                    title = "No Pending Review Requests",
                    subtitle = "All caught up! Press Enter to open review requests page",
                    icon = "emblem-favorite",
                    score = 120.0,
                    type = "Review Request",
                    provider = "GitHub",
                    action = "open_url:https://github.com/pulls/review-requested"
                })
            end

            return results
        end

        -- Case D: Repositories ("gh repo", "gh repos", "gh repo <name>")
        local is_repo_query = false
        local repo_term = ""
        if sub_lower == "repo" or sub_lower == "repos" or sub_lower == "my repos" or sub_lower == "repositories" then
            is_repo_query = true
        elseif sub_lower:sub(1, 5) == "repo " then
            is_repo_query = true
            repo_term = sub_lower:sub(6):gsub("^%s+", "")
        elseif sub_lower:sub(1, 6) == "repos " then
            is_repo_query = true
            repo_term = sub_lower:sub(7):gsub("^%s+", "")
        end

        if is_repo_query then
            fetch_repos(false)
            local results = {}

            for _, repo in ipairs(REPOS) do
                local name = repo.nameWithOwner or repo.name or ""
                local desc = repo.description or ""
                local match = true

                if #repo_term > 0 then
                    local lower_name = name:lower()
                    local lower_desc = desc:lower()
                    if not lower_name:find(repo_term, 1, true) and
                       not lower_desc:find(repo_term, 1, true) then
                        match = false
                    end
                end

                if match then
                    local is_priv = repo.isPrivate and "🔒 " or ""
                    local stars = (repo.stargazerCount and repo.stargazerCount > 0) and string.format(" • ⭐ %d", repo.stargazerCount) or ""
                    local desc_prefix = #desc > 0 and (desc .. " • ") or ""
                    local time_str = format_time_ago(repo.updatedAt)
                    local subtitle = string.format("%s%sUpdated %s%s", is_priv, desc_prefix, time_str, stars)

                    table.insert(results, {
                        id = "gh:repo:" .. name,
                        title = name,
                        subtitle = subtitle,
                        icon = "folder-remote",
                        score = 140.0 - #results,
                        type = "Repository",
                        provider = "GitHub",
                        action = "open_url:" .. repo.url,
                        secondaryActionLabel = "Copy Clone Command",
                        secondaryAction = "copy_clone:" .. repo.url
                    })

                    if #results >= 15 then break end
                end
            end

            if #results == 0 and #repo_term > 0 then
                table.insert(results, {
                    id = "gh:repo:search_web",
                    title = "Search GitHub for '" .. repo_term .. "'",
                    subtitle = "Search repositories across all of GitHub",
                    icon = "internet-web-browser",
                    score = 120.0,
                    type = "Web Search",
                    provider = "GitHub",
                    action = "open_url:https://github.com/search?q=" .. url_encode(repo_term) .. "&type=repositories"
                })
            end

            return results
        end

        -- Case E: Issues ("gh issue", "gh issues", "gh issue <filter>")
        local is_issue_query = false
        local issue_term = ""
        if sub_lower == "issue" or sub_lower == "issues" or sub_lower == "my issues" then
            is_issue_query = true
        elseif sub_lower:sub(1, 6) == "issue " then
            is_issue_query = true
            issue_term = sub_lower:sub(7):gsub("^%s+", "")
        elseif sub_lower:sub(1, 7) == "issues " then
            is_issue_query = true
            issue_term = sub_lower:sub(8):gsub("^%s+", "")
        end

        if is_issue_query then
            fetch_issues(false)
            local results = {}

            for _, issue in ipairs(ISSUES) do
                local repo_name = (issue.repository and issue.repository.nameWithOwner) or ""
                local title = issue.title or ""
                local num_str = tostring(issue.number or "")
                local match = true

                if #issue_term > 0 then
                    local lower_title = title:lower()
                    local lower_repo = repo_name:lower()
                    if not lower_title:find(issue_term, 1, true) and
                       not lower_repo:find(issue_term, 1, true) and
                       not num_str:find(issue_term, 1, true) then
                        match = false
                    end
                end

                if match then
                    local item_title = string.format("%s #%s: %s", repo_name, num_str, title)
                    local time_str = format_time_ago(issue.updatedAt)

                    table.insert(results, {
                        id = "gh:issue:" .. (issue.url or num_str),
                        title = item_title,
                        subtitle = string.format("Updated %s • Press Enter to open issue", time_str),
                        icon = "dialog-warning",
                        score = 140.0 - #results,
                        type = "Issue",
                        provider = "GitHub",
                        action = "open_url:" .. issue.url,
                        secondaryActionLabel = "Copy URL",
                        secondaryAction = "copy:" .. issue.url
                    })

                    if #results >= 15 then break end
                end
            end

            if #results == 0 then
                table.insert(results, {
                    id = "gh:issue:none",
                    title = #issue_term > 0 and ("No issues matching '" .. issue_term .. "'") or "No Open Issues",
                    subtitle = "Press Enter to open your issues on GitHub",
                    icon = "dialog-warning",
                    score = 120.0,
                    type = "Issue",
                    provider = "GitHub",
                    action = "open_url:https://github.com/issues"
                })
            end

            return results
        end

        -- Case F: Global GitHub Search ("gh search <term>", "gh s <term>")
        if sub_lower:sub(1, 7) == "search " or sub_lower:sub(1, 2) == "s " then
            local term = sub:gsub("^%a+%s+", "")
            local encoded = url_encode(term)
            return {
                {
                    id = "gh:search:all",
                    title = "Search GitHub: " .. term,
                    subtitle = "Search everything on GitHub • github.com/search",
                    icon = "internet-web-browser",
                    score = 150.0,
                    type = "GitHub Search",
                    provider = "GitHub",
                    action = "open_url:https://github.com/search?q=" .. encoded
                },
                {
                    id = "gh:search:repos",
                    title = "Search Repositories: " .. term,
                    subtitle = "Find open source repositories",
                    icon = "folder-remote",
                    score = 140.0,
                    type = "GitHub Search",
                    provider = "GitHub",
                    action = "open_url:https://github.com/search?q=" .. encoded .. "&type=repositories"
                },
                {
                    id = "gh:search:code",
                    title = "Search Code: " .. term,
                    subtitle = "Find code snippets and functions",
                    icon = "applications-development",
                    score = 135.0,
                    type = "GitHub Search",
                    provider = "GitHub",
                    action = "open_url:https://github.com/search?q=" .. encoded .. "&type=code"
                },
                {
                    id = "gh:search:prs",
                    title = "Search Pull Requests: " .. term,
                    subtitle = "Find pull requests matching keyword",
                    icon = "vcs-merge-request",
                    score = 130.0,
                    type = "GitHub Search",
                    provider = "GitHub",
                    action = "open_url:https://github.com/search?q=" .. encoded .. "&type=pullrequests"
                }
            }
        end

        -- Case G: Shortcuts ("gh new", "gh gist", "gh profile", "gh trending", "gh notif")
        if sub_lower == "new" or sub_lower == "new repo" or sub_lower == "create" then
            return {
                {
                    id = "gh:new:repo",
                    title = "Create New Repository",
                    subtitle = "github.com/new",
                    icon = "applications-development",
                    score = 150.0,
                    type = "Action",
                    provider = "GitHub",
                    action = "open_url:https://github.com/new"
                },
                {
                    id = "gh:new:gist",
                    title = "Create New Gist",
                    subtitle = "gist.github.com",
                    icon = "text-x-generic",
                    score = 140.0,
                    type = "Action",
                    provider = "GitHub",
                    action = "open_url:https://gist.github.com"
                }
            }
        end

        if sub_lower == "gist" or sub_lower == "gists" then
            return {
                {
                    id = "gh:gists",
                    title = "GitHub Gists",
                    subtitle = "Open gist.github.com",
                    icon = "text-x-generic",
                    score = 150.0,
                    type = "Action",
                    provider = "GitHub",
                    action = "open_url:https://gist.github.com"
                }
            }
        end

        if sub_lower == "profile" or sub_lower == "me" then
            return {
                {
                    id = "gh:profile",
                    title = "My Profile (@" .. USER_LOGIN .. ")",
                    subtitle = "Open https://github.com/" .. USER_LOGIN,
                    icon = "applications-development",
                    score = 150.0,
                    type = "Navigation",
                    provider = "GitHub",
                    action = "open_url:https://github.com/" .. USER_LOGIN
                }
            }
        end

        if sub_lower == "trending" then
            return {
                {
                    id = "gh:trending",
                    title = "Trending Repositories",
                    subtitle = "Explore trending repositories on GitHub",
                    icon = "folder-remote",
                    score = 150.0,
                    type = "Navigation",
                    provider = "GitHub",
                    action = "open_url:https://github.com/trending"
                }
            }
        end

        if sub_lower == "notif" or sub_lower == "notifications" then
            return {
                {
                    id = "gh:notifications",
                    title = "GitHub Notifications",
                    subtitle = "Open https://github.com/notifications",
                    icon = "dialog-information",
                    score = 150.0,
                    type = "Navigation",
                    provider = "GitHub",
                    action = "open_url:https://github.com/notifications"
                }
            }
        end

        -- Case H: Arbitrary filter across Repos, PRs, and Issues (e.g. "gh hitkill", "gh tulka", "gh bug")
        fetch_all(false)
        local results = {}
        local term = sub_lower

        -- 1. Search Repos
        for _, repo in ipairs(REPOS) do
            local name = repo.nameWithOwner or repo.name or ""
            local desc = repo.description or ""
            if name:lower():find(term, 1, true) or desc:lower():find(term, 1, true) then
                local is_priv = repo.isPrivate and "🔒 " or ""
                local stars = (repo.stargazerCount and repo.stargazerCount > 0) and string.format(" • ⭐ %d", repo.stargazerCount) or ""
                local desc_prefix = #desc > 0 and (desc .. " • ") or ""
                local time_str = format_time_ago(repo.updatedAt)

                table.insert(results, {
                    id = "gh:repo:" .. name,
                    title = name,
                    subtitle = string.format("%s%sUpdated %s%s", is_priv, desc_prefix, time_str, stars),
                    icon = "folder-remote",
                    score = 140.0 - #results,
                    type = "Repository",
                    provider = "GitHub",
                    action = "open_url:" .. repo.url,
                    secondaryActionLabel = "Copy Clone Command",
                    secondaryAction = "copy_clone:" .. repo.url
                })
                if #results >= 5 then break end
            end
        end

        -- 2. Search PRs
        for _, pr in ipairs(PRS) do
            local repo_name = (pr.repository and pr.repository.nameWithOwner) or ""
            local title = pr.title or ""
            local num_str = tostring(pr.number or "")
            if title:lower():find(term, 1, true) or repo_name:lower():find(term, 1, true) or num_str:find(term, 1, true) then
                local time_str = format_time_ago(pr.updatedAt)
                table.insert(results, {
                    id = "gh:pr:" .. (pr.url or num_str),
                    title = string.format("%s #%s: %s", repo_name, num_str, title),
                    subtitle = string.format("Updated %s • Pull Request", time_str),
                    icon = "vcs-merge-request",
                    score = 130.0 - #results,
                    type = "Pull Request",
                    provider = "GitHub",
                    action = "open_url:" .. pr.url,
                    secondaryActionLabel = "Copy PR URL",
                    secondaryAction = "copy:" .. pr.url
                })
                if #results >= 10 then break end
            end
        end

        -- 3. Search Issues
        for _, issue in ipairs(ISSUES) do
            local repo_name = (issue.repository and issue.repository.nameWithOwner) or ""
            local title = issue.title or ""
            local num_str = tostring(issue.number or "")
            if title:lower():find(term, 1, true) or repo_name:lower():find(term, 1, true) or num_str:find(term, 1, true) then
                local time_str = format_time_ago(issue.updatedAt)
                table.insert(results, {
                    id = "gh:issue:" .. (issue.url or num_str),
                    title = string.format("%s #%s: %s", repo_name, num_str, title),
                    subtitle = string.format("Updated %s • Issue", time_str),
                    icon = "dialog-warning",
                    score = 120.0 - #results,
                    type = "Issue",
                    provider = "GitHub",
                    action = "open_url:" .. issue.url,
                    secondaryActionLabel = "Copy URL",
                    secondaryAction = "copy:" .. issue.url
                })
                if #results >= 15 then break end
            end
        end

        -- Always append Web Search option
        table.insert(results, {
            id = "gh:web:search",
            title = "Search GitHub for '" .. sub .. "'",
            subtitle = "Search repositories, code, and issues across all of GitHub",
            icon = "internet-web-browser",
            score = 90.0,
            type = "Web Search",
            provider = "GitHub",
            action = "open_url:https://github.com/search?q=" .. url_encode(sub)
        })

        return results
    end,

    execute = function(result, action)
        local act = action or result.action or ""

        if act:sub(1, 9) == "open_url:" then
            local url = act:sub(10)
            launcher.open_url(url)
            return true
        end

        if act:sub(1, 11) == "copy_clone:" then
            local url = act:sub(12)
            local cmd = "git clone " .. url .. ".git"
            launcher.copy_to_clipboard(cmd)
            launcher.notify("GitHub", "Copied clone command to clipboard:\n" .. cmd, "folder-remote")
            return true
        end

        if act:sub(1, 5) == "copy:" then
            local val = act:sub(6)
            launcher.copy_to_clipboard(val)
            launcher.notify("GitHub", "Copied to clipboard:\n" .. val, "vcs-merge-request")
            return true
        end

        if act == "force_refresh" then
            fetch_all(true)
            launcher.notify("GitHub", "Syncing repositories and pull requests from GitHub...", "view-refresh")
            return true
        end

        return false
    end
})
