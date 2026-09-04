-- UUID Generator Plugin for MVSpotlight

local function generate_uuid_v4()
    local template = "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx"
    math.randomseed(os.time() + math.floor(os.clock() * 1000000))
    return string.gsub(template, "[xy]", function(c)
        local v = (c == "x") and math.random(0, 15) or math.random(8, 11)
        return string.format("%x", v)
    end)
end

launcher.register_provider({
    id = "uuid",
    search = function(query)
        local q = string.lower(query or "")
        if q == "uuid" or q == "guid" or q == "uuid v4" or q == "generate uuid" then
            local newUuid = generate_uuid_v4()
            return {
                {
                    id = "uuid:" .. newUuid,
                    title = newUuid,
                    subtitle = "Random UUID v4 · Press Enter to copy",
                    icon = "accessories-calculator",
                    score = 100,
                    type = "Generator",
                    data = {
                        uuid = newUuid
                    }
                }
            }
        end
        return {}
    end,

    execute = function(result)
        local data = result.data or {}
        if data.uuid then
            launcher.copy_to_clipboard(data.uuid)
            launcher.notify("UUID Generator", "Copied to clipboard: " .. data.uuid, "accessories-calculator")
        end
    end
})
