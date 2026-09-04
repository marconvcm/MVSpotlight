-- Web Search Plugin for MVSpotlight

local function url_encode(str)
    if str then
        str = string.gsub(str, "\n", "\r\n")
        str = string.gsub(str, "([^%w %-%_%.%~])", function(c)
            return string.format("%%%02X", string.byte(c))
        end)
        str = string.gsub(str, " ", "+")
    end
    return str
end

launcher.register_provider({
    id = "websearch",
    search = function(query)
        local q = query or ""
        if q == "" then return {} end

        local results = {}

        -- Google search: "g <query>" or "google <query>"
        local gQuery = string.match(q, "^google%s+(.+)") or string.match(q, "^g%s+(.+)")
        if gQuery then
            table.insert(results, {
                id = "web:google:" .. gQuery,
                title = "Search Google for \"" .. gQuery .. "\"",
                subtitle = "Web Search · Open in default browser",
                icon = "applications-internet",
                score = 98,
                type = "Web Search",
                data = {
                    url = "https://www.google.com/search?q=" .. url_encode(gQuery)
                }
            })
        end

        -- GitHub search: "gh <query>" or "github <query>"
        local ghQuery = string.match(q, "^github%s+(.+)") or string.match(q, "^gh%s+(.+)")
        if ghQuery then
            table.insert(results, {
                id = "web:github:" .. ghQuery,
                title = "Search GitHub for \"" .. ghQuery .. "\"",
                subtitle = "GitHub Code & Repositories · Open in browser",
                icon = "vcs-merge",
                score = 98,
                type = "GitHub",
                data = {
                    url = "https://github.com/search?q=" .. url_encode(ghQuery)
                }
            })
        end

        -- DuckDuckGo: "ddg <query>"
        local ddgQuery = string.match(q, "^ddg%s+(.+)")
        if ddgQuery then
            table.insert(results, {
                id = "web:ddg:" .. ddgQuery,
                title = "Search DuckDuckGo for \"" .. ddgQuery .. "\"",
                subtitle = "Private Web Search · Open in browser",
                icon = "applications-internet",
                score = 98,
                type = "Web Search",
                data = {
                    url = "https://duckduckgo.com/?q=" .. url_encode(ddgQuery)
                }
            })
        end

        -- Wikipedia: "wiki <query>"
        local wikiQuery = string.match(q, "^wiki%s+(.+)")
        if wikiQuery then
            table.insert(results, {
                id = "web:wiki:" .. wikiQuery,
                title = "Search Wikipedia for \"" .. wikiQuery .. "\"",
                subtitle = "Encyclopedia Search · Open in browser",
                icon = "accessories-dictionary",
                score = 98,
                type = "Reference",
                data = {
                    url = "https://en.wikipedia.org/wiki/Special:Search?search=" .. url_encode(wikiQuery)
                }
            })
        end

        return results
    end,

    execute = function(result)
        local data = result.data or {}
        if data.url then
            launcher.open_url(data.url)
        end
    end
})
