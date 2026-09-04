-- Weather Plugin for MVSpotlight
-- Live global weather conditions and forecasts via wttr.in

local CACHE = {}
local IN_FLIGHT = {}

local function url_encode(str)
    if not str then return "" end
    return (str:gsub("%s+", "+"):gsub("([^%w%+%-%.%_])", function(c)
        return string.format("%%%02X", string.byte(c))
    end))
end

local function get_weather_meta(desc)
    local d = (desc or ""):lower()
    if d:find("storm") or d:find("thunder") or d:find("lightning") then
        return "weather-storm", "⛈️"
    elseif d:find("snow") or d:find("blizzard") or d:find("sleet") or d:find("ice") or d:find("hail") then
        return "weather-snow", "❄️"
    elseif d:find("heavy rain") or d:find("torrential") then
        return "weather-showers", "🌧️"
    elseif d:find("rain") or d:find("shower") or d:find("drizzle") then
        return "weather-showers", "🌦️"
    elseif d:find("fog") or d:find("mist") or d:find("haze") then
        return "weather-fog", "🌫️"
    elseif d:find("overcast") or d:find("cloudy") then
        return "weather-overcast", "☁️"
    elseif d:find("partly") or d:find("few clouds") then
        return "weather-few-clouds", "⛅"
    elseif d:find("sun") or d:find("clear") then
        return "weather-clear", "☀️"
    end
    return "weather-few-clouds", "🌤️"
end

local function parse_wttr_json(body)
    if not body or #body == 0 then return nil end
    local data = {}

    data.temp_c = body:match('"temp_C":%s*"([%-%d]+)"')
    data.temp_f = body:match('"temp_F":%s*"([%-%d]+)"')
    data.feels_c = body:match('"FeelsLikeC":%s*"([%-%d]+)"')
    data.feels_f = body:match('"FeelsLikeF":%s*"([%-%d]+)"')
    data.humidity = body:match('"humidity":%s*"([%d]+)"')
    data.wind_kmh = body:match('"windspeedKmph":%s*"([%d]+)"')
    data.wind_dir = body:match('"winddir16Point":%s*"([%a]+)"')

    local desc = body:match('"weatherDesc":%s*%[%s*{%s*"value":%s*"([^"]+)"')
    if desc then
        data.desc = desc:gsub("^%s+", ""):gsub("%s+$", "")
    else
        data.desc = "Clear"
    end

    local area = body:match('"areaName":%s*%[%s*{%s*"value":%s*"([^"]+)"')
    local country = body:match('"country":%s*%[%s*{%s*"value":%s*"([^"]+)"')
    data.area = area or "Current Location"
    data.country = country or ""

    data.max_c = body:match('"maxtempC":%s*"([%-%d]+)"')
    data.min_c = body:match('"mintempC":%s*"([%-%d]+)"')
    data.max_f = body:match('"maxtempF":%s*"([%-%d]+)"')
    data.min_f = body:match('"mintempF":%s*"([%-%d]+)"')

    if data.temp_c then
        return data
    end
    return nil
end

local function fetch_weather(city)
    local key = (city or ""):lower():gsub("%s+", "")
    if IN_FLIGHT[key] then return end
    IN_FLIGHT[key] = true

    local path = #city > 0 and url_encode(city) or ""
    local url = string.format("https://wttr.in/%s?format=j1", path)

    pcall(function()
        launcher.http.get({
            url = url,
            on_complete = function(status, body)
                IN_FLIGHT[key] = nil
                if status == 200 and body then
                    local parsed = parse_wttr_json(body)
                    if parsed then
                        CACHE[key] = {
                            data = parsed,
                            query_city = city,
                            timestamp = os.time()
                        }
                        launcher.log(string.format("Cached weather for '%s' (%s, %s: %s°C)",
                            city, parsed.area, parsed.country, parsed.temp_c))
                    end
                end
            end
        })
    end)
end

-- Pre-warm local weather on startup
fetch_weather("")

local function extract_city(query)
    local q = query:lower():gsub("^%s+", ""):gsub("%s+$", "")

    -- Exact keywords
    if q == "weather" or q == "clima" or q == "tempo" or q == "forecast" then
        return ""
    end

    -- "weather in <city>", "weather for <city>", "weather <city>"
    local city = q:match("^weather%s+in%s+(.+)$")
        or q:match("^weather%s+for%s+(.+)$")
        or q:match("^weather%s+(.+)$")
        or q:match("^clima%s+em%s+(.+)$")
        or q:match("^clima%s+(.+)$")
        or q:match("^tempo%s+em%s+(.+)$")
        or q:match("^tempo%s+(.+)$")
        or q:match("^forecast%s+in%s+(.+)$")
        or q:match("^forecast%s+(.+)$")

    -- "<city> weather"
    if not city then
        city = q:match("^(.+)%s+weather$")
            or q:match("^(.+)%s+clima$")
            or q:match("^(.+)%s+tempo$")
    end

    if city then
        city = city:gsub("^%s+", ""):gsub("%s+$", "")
        if #city > 0 then
            return city
        end
    end

    return nil
end

launcher.register_provider({
    id = "weather",
    name = "Weather",
    search = function(query)
        local city = extract_city(query)
        if city == nil then return {} end

        local key = city:lower():gsub("%s+", "")
        local cached = CACHE[key]

        -- If not cached, initiate background fetch
        if not cached or (os.time() - cached.timestamp > 900) then
            fetch_weather(city)
        end

        local display_city = #city > 0 and (city:gsub("^%l", string.upper)) or "Your Location"

        if cached and cached.data then
            local w = cached.data
            local icon, emoji = get_weather_meta(w.desc)
            local location = w.area
            if w.country and #w.country > 0 and w.country ~= w.area then
                location = location .. ", " .. w.country
            end

            local title = string.format("%s %s: %s°C (%s°F) • %s", emoji, location, w.temp_c, w.temp_f, w.desc)
            local subtitle = string.format("Feels like %s°C · Humidity %s%% · Wind %s km/h %s · Enter to open wttr.in",
                w.feels_c or w.temp_c, w.humidity or "N/A", w.wind_kmh or "0", w.wind_dir or "")

            local copy_text = string.format("%s: %s°C (%s°F), %s, Humidity %s%%, Wind %s km/h",
                location, w.temp_c, w.temp_f, w.desc, w.humidity or "", w.wind_kmh or "")

            local web_target = #city > 0 and url_encode(city) or url_encode(w.area or "")
            local web_url = "https://wttr.in/" .. web_target

            local results = {
                {
                    id = "weather:current:" .. key,
                    title = title,
                    subtitle = subtitle,
                    icon = icon,
                    score = 115.0,
                    type = "Weather",
                    provider = "Weather",
                    action = "open:" .. web_url,
                    secondaryActionLabel = "Copy Weather Summary",
                    secondaryAction = "copy:" .. copy_text
                }
            }

            -- Forecast card if max/min temps are available
            if w.max_c and w.min_c then
                local f_title = string.format("📅 Today's Forecast: High %s°C / Low %s°C (%s°F / %s°F)",
                    w.max_c, w.min_c, w.max_f or "", w.min_f or "")
                local f_subtitle = string.format("%s · Press Enter to view 3-day forecast on wttr.in", location)

                table.insert(results, {
                    id = "weather:forecast:" .. key,
                    title = f_title,
                    subtitle = f_subtitle,
                    icon = icon,
                    score = 105.0,
                    type = "Forecast",
                    provider = "Weather",
                    action = "open:" .. web_url,
                    secondaryActionLabel = "Copy Forecast",
                    secondaryAction = "copy:" .. f_title
                })
            end

            return results
        else
            -- Loading / Initial fetch card
            local web_target = #city > 0 and url_encode(city) or ""
            local web_url = "https://wttr.in/" .. web_target

            return {
                {
                    id = "weather:loading:" .. key,
                    title = string.format("🌤️ Weather for %s", display_city),
                    subtitle = "Fetching live report from wttr.in... Press Enter to open in browser",
                    icon = "weather-few-clouds",
                    score = 110.0,
                    type = "Weather",
                    provider = "Weather",
                    action = "open:" .. web_url,
                    secondaryActionLabel = "Copy City Name",
                    secondaryAction = "copy:" .. display_city
                }
            }
        end
    end,

    execute = function(result, action)
        local act = action or result.action or ""

        if act:sub(1, 5) == "open:" then
            local url = act:sub(6)
            launcher.open_url(url)
            return true
        end

        if act:sub(1, 5) == "copy:" then
            local text = act:sub(6)
            launcher.copy_to_clipboard(text)
            launcher.notify("Weather", string.format("Copied: %s", text), "weather-few-clouds")
            return true
        end

        return false
    end
})
