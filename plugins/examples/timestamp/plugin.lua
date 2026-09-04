-- Timestamp Converter Plugin for MVSpotlight

launcher.register_provider({
    id = "timestamp",
    search = function(query)
        local q = string.lower(query or "")
        local epochStr = string.match(q, "^unix%s+(%d+)")
            or string.match(q, "^epoch%s+(%d+)")
            or string.match(q, "^timestamp%s+(%d+)")
            or string.match(q, "^time%s+(%d+)")

        if epochStr then
            local epoch = tonumber(epochStr)
            if epoch and epoch > 0 then
                -- Handle milliseconds if > 10^11
                if epoch > 100000000000 then
                    epoch = math.floor(epoch / 1000)
                end

                local dateFormatted = os.date("!%Y-%m-%d %H:%M:%S UTC", epoch)
                local localFormatted = os.date("%Y-%m-%d %H:%M:%S Local", epoch)

                return {
                    {
                        id = "timestamp:" .. epochStr,
                        title = dateFormatted,
                        subtitle = string.format("Unix Epoch %s · %s · Press Enter to copy", epochStr, localFormatted),
                        icon = "preferences-system-time",
                        score = 100,
                        type = "Converter",
                        data = {
                            date = dateFormatted
                        }
                    }
                }
            end
        end

        if q == "timestamp" or q == "now" or q == "epoch" or q == "unix" then
            local now = os.time()
            local nowStr = tostring(now)
            local dateStr = os.date("!%Y-%m-%d %H:%M:%S UTC", now)

            return {
                {
                    id = "timestamp:now",
                    title = nowStr,
                    subtitle = string.format("Current Unix Epoch · %s · Press Enter to copy", dateStr),
                    icon = "preferences-system-time",
                    score = 95,
                    type = "Converter",
                    data = {
                        date = nowStr
                    }
                }
            }
        end

        return {}
    end,

    execute = function(result)
        local data = result.data or {}
        if data.date then
            launcher.copy_to_clipboard(data.date)
            launcher.notify("Timestamp Converter", "Copied to clipboard: " .. data.date, "preferences-system-time")
        end
    end
})
