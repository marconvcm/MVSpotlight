-- Currency Exchange Plugin for MVSpotlight
-- Converts amounts between global currencies with real-time rates, multi-source online APIs,
-- persistent caching, and offline fallbacks.

local RATES = {
    USD = 1.0,
    EUR = 0.86,
    BRL = 5.10,
    GBP = 0.74,
    JPY = 156.0,
    CAD = 1.38,
    AUD = 1.39,
    CHF = 0.81,
    CNY = 6.72,
    INR = 94.6,
    MXN = 16.9,
    KRW = 1350.0,
    SGD = 1.35,
    HKD = 7.84,
    NZD = 1.65,
    SEK = 10.5,
    NOK = 10.8,
    TRY = 33.5,
    ARS = 1508.0,
    CLP = 930.0,
    COP = 3148.0,
    PEN = 3.28,
    ZAR = 18.2,
    THB = 36.5,
    AED = 3.67,
    SAR = 3.75,
    ILS = 3.72,
    PLN = 4.02,
    BTC = 0.00001237,
    ETH = 0.0003989,
    SOL = 0.00963
}

local SYMBOLS = {
    USD = "$",
    EUR = "€",
    BRL = "R$",
    GBP = "£",
    JPY = "¥",
    CAD = "C$",
    AUD = "A$",
    CHF = "CHF",
    CNY = "¥",
    INR = "₹",
    MXN = "Mex$",
    KRW = "₩",
    SGD = "S$",
    HKD = "HK$",
    NZD = "NZ$",
    SEK = "kr",
    NOK = "kr",
    TRY = "₺",
    ARS = "ARS$",
    CLP = "CLP$",
    COP = "COL$",
    PEN = "S/",
    BTC = "₿",
    ETH = "Ξ",
    SOL = "SOL"
}

local LAST_UPDATE_TIME = 0
local CURRENT_SOURCE = "Initial Default"
local IS_UPDATING = false

local ALIASES = {
    ["$"] = "USD",
    ["us$"] = "USD",
    ["usd"] = "USD",
    ["dollar"] = "USD",
    ["dollars"] = "USD",
    ["dolar"] = "USD",
    ["dolares"] = "USD",
    ["r$"] = "BRL",
    ["brl"] = "BRL",
    ["real"] = "BRL",
    ["reais"] = "BRL",
    ["€"] = "EUR",
    ["eur"] = "EUR",
    ["euro"] = "EUR",
    ["euros"] = "EUR",
    ["£"] = "GBP",
    ["gbp"] = "GBP",
    ["pound"] = "GBP",
    ["pounds"] = "GBP",
    ["libra"] = "GBP",
    ["libras"] = "GBP",
    ["¥"] = "JPY",
    ["jpy"] = "JPY",
    ["yen"] = "JPY",
    ["cad"] = "CAD",
    ["c$"] = "CAD",
    ["aud"] = "AUD",
    ["a$"] = "AUD",
    ["chf"] = "CHF",
    ["franc"] = "CHF",
    ["francs"] = "CHF",
    ["franco"] = "CHF",
    ["cny"] = "CNY",
    ["yuan"] = "CNY",
    ["rmb"] = "CNY",
    ["inr"] = "INR",
    ["rupee"] = "INR",
    ["rupees"] = "INR",
    ["rupia"] = "INR",
    ["rupias"] = "INR",
    ["₹"] = "INR",
    ["mxn"] = "MXN",
    ["peso"] = "MXN",
    ["pesos"] = "MXN",
    ["krw"] = "KRW",
    ["won"] = "KRW",
    ["₩"] = "KRW",
    ["sgd"] = "SGD",
    ["hkd"] = "HKD",
    ["nzd"] = "NZD",
    ["sek"] = "SEK",
    ["krona"] = "SEK",
    ["nok"] = "NOK",
    ["krone"] = "NOK",
    ["try"] = "TRY",
    ["lira"] = "TRY",
    ["₺"] = "TRY",
    ["ars"] = "ARS",
    ["clp"] = "CLP",
    ["cop"] = "COP",
    ["pen"] = "PEN",
    ["sol"] = "PEN",
    ["soles"] = "PEN",
    ["btc"] = "BTC",
    ["bitcoin"] = "BTC",
    ["₿"] = "BTC",
    ["eth"] = "ETH",
    ["ethereum"] = "ETH",
    ["ether"] = "ETH",
    ["sol"] = "SOL",
    ["solana"] = "SOL"
}

-- Format numbers with comma separation and appropriate decimal precision
local function format_number(val, currency)
    if not val then return "0" end
    local decimals = 2
    if currency == "JPY" or currency == "KRW" or currency == "CLP" or currency == "COP" or currency == "ARS" then
        decimals = 0
    elseif val < 0.01 and val > 0 then
        decimals = 6
    end

    local formatted
    if decimals == 0 then
        formatted = string.format("%.0f", val)
    elseif decimals == 6 then
        formatted = string.format("%.6f", val):gsub("0+$", ""):gsub("%.$", "")
    else
        formatted = string.format("%.2f", val)
    end

    local int_part, dec_part = formatted:match("^(%-?%d+)(%.?.*)$")
    if int_part then
        int_part = int_part:reverse():gsub("(%d%d%d)", "%1,"):reverse():gsub("^,", "")
        return int_part .. (dec_part or "")
    end
    return formatted
end

local function format_rate(r)
    if not r then return "0" end
    if r >= 100 then
        return string.format("%.2f", r)
    elseif r >= 1 then
        return string.format("%.4f", r)
    else
        return string.format("%.4f", r)
    end
end

local function normalize_currency(c)
    if not c then return nil end
    local lower = c:lower():gsub("%s+", "")
    if ALIASES[lower] then
        return ALIASES[lower]
    end
    local upper = c:upper():gsub("%s+", "")
    if RATES[upper] then
        return upper
    end
    return nil
end

local function get_freshness_text()
    if LAST_UPDATE_TIME == 0 then
        return "Offline Fallback"
    end
    local diff = os.time() - LAST_UPDATE_TIME
    if diff < 0 then diff = 0 end
    if diff < 60 then
        return "Live (just now)"
    elseif diff < 3600 then
        return string.format("Live (%dm ago)", math.floor(diff / 60))
    elseif diff < 86400 then
        return string.format("Updated %s", os.date("%H:%M", LAST_UPDATE_TIME))
    else
        return string.format("Updated %s", os.date("%b %d", LAST_UPDATE_TIME))
    end
end

-- Serialization for persistent config caching
local function serialize_rates(rates)
    local parts = {}
    for k, v in pairs(rates) do
        table.insert(parts, string.format("%s:%.8g", k, v))
    end
    return table.concat(parts, ",")
end

local function deserialize_rates(str, target)
    if not str or #str == 0 then return 0 end
    local count = 0
    for k, v in str:gmatch("([A-Z]+):([%d%.eE%-]+)") do
        local num = tonumber(v)
        if num and num > 0 then
            target[k] = num
            count = count + 1
        end
    end
    return count
end

local function save_cache()
    pcall(function()
        launcher.set_config("rates_cache", serialize_rates(RATES))
        launcher.set_config("rates_timestamp", tostring(LAST_UPDATE_TIME))
        launcher.set_config("rates_source", CURRENT_SOURCE)
    end)
end

local function load_cache()
    pcall(function()
        local cached = launcher.get_config("rates_cache", "")
        local ts = launcher.get_config("rates_timestamp", "0")
        local src = launcher.get_config("rates_source", "")
        if cached and #cached > 0 then
            local count = deserialize_rates(cached, RATES)
            if count > 0 then
                LAST_UPDATE_TIME = tonumber(ts) or 0
                if src and #src > 0 then
                    CURRENT_SOURCE = src
                end
                launcher.log(string.format("Loaded %d cached currency rates from disk (%s)", count, get_freshness_text()))
            end
        end
    end)
end

-- Robust parser handling: "299brl to usd", "299 brl in usd", "299brl usd", "R$299 to usd", "$100 to brl", etc.
local function parse_currency_query(raw)
    if not raw or #raw == 0 then return nil end
    local q = raw:lower():gsub("^%s+", ""):gsub("%s+$", "")

    -- Strip leading words
    q = q:gsub("^convert%s+", ""):gsub("^exchange%s+", ""):gsub("^calc%s+", "")

    -- Handle comma decimal separators: "299,99" -> "299.99"
    q = q:gsub("(%d+),(%d+)", "%1.%2")

    -- Replace currency symbols with standardized codes surrounded by spaces
    q = q:gsub("r%s*%$%s*", " brl ")
    q = q:gsub("us%s*%$%s*", " usd ")
    q = q:gsub("c%s*%$%s*", " cad ")
    q = q:gsub("a%s*%$%s*", " aud ")
    q = q:gsub("%$%s*", " usd ")
    q = q:gsub("€%s*", " eur ")
    q = q:gsub("£%s*", " gbp ")
    q = q:gsub("¥%s*", " jpy ")
    q = q:gsub("₹%s*", " inr ")
    q = q:gsub("₩%s*", " krw ")
    q = q:gsub("₺%s*", " try ")
    q = q:gsub("₿%s*", " btc ")

    -- Normalize internal whitespace
    q = q:gsub("%s+", " "):gsub("^%s+", ""):gsub("%s+$", "")

    local num, from_c, to_c

    -- 1. Number + From + (to/in/into/=) + To (e.g. "299brl to usd", "299 brl to usd", "299 brl in usd")
    num, from_c, to_c = q:match("^(%d*%.?%d+)%s*([%a]+)%s+to%s+([%a]+)$")
    if not num then
        num, from_c, to_c = q:match("^(%d*%.?%d+)%s*([%a]+)%s+in%s+([%a]+)$")
    end
    if not num then
        num, from_c, to_c = q:match("^(%d*%.?%d+)%s*([%a]+)%s+into%s+([%a]+)$")
    end
    if not num then
        num, from_c, to_c = q:match("^(%d*%.?%d+)%s*([%a]+)%s*=%s*([%a]+)$")
    end

    -- 2. From + Number + (to/in) + To (e.g. "brl 299 to usd", "usd 100 in eur")
    if not num then
        from_c, num, to_c = q:match("^([%a]+)%s*(%d*%.?%d+)%s+to%s+([%a]+)$")
    end
    if not num then
        from_c, num, to_c = q:match("^([%a]+)%s*(%d*%.?%d+)%s+in%s+([%a]+)$")
    end

    -- 3. Number + From + To with no connector (e.g. "299brl usd", "299 brl usd")
    if not num then
        num, from_c, to_c = q:match("^(%d*%.?%d+)%s*([%a]+)%s+([%a]+)$")
    end

    -- 4. From + Number + To with no connector (e.g. "brl 299 usd")
    if not num then
        from_c, num, to_c = q:match("^([%a]+)%s*(%d*%.?%d+)%s+([%a]+)$")
    end

    -- 5. From + (to/in) + To without number (defaults to 1, e.g. "brl to usd", "usd in eur")
    if not num then
        from_c, to_c = q:match("^([%a]+)%s+to%s+([%a]+)$")
        if not from_c then
            from_c, to_c = q:match("^([%a]+)%s+in%s+([%a]+)$")
        end
        if from_c and to_c then
            num = "1"
        end
    end

    -- 6. Two currency codes (e.g. "brl usd", "eur usd")
    if not num then
        from_c, to_c = q:match("^([%a]+)%s+([%a]+)$")
        if from_c and to_c then
            local fc_test = normalize_currency(from_c)
            local tc_test = normalize_currency(to_c)
            if fc_test and tc_test then
                num = "1"
            else
                num, from_c, to_c = nil, nil, nil
            end
        end
    end

    -- 7. Number + Currency alone (e.g. "299brl", "299 brl"): default to configured target (or USD/BRL fallback)
    if not num then
        num, from_c = q:match("^(%d*%.?%d+)%s*([%a]+)$")
        if num and from_c then
            local fc_test = normalize_currency(from_c)
            if fc_test then
                local def_target = launcher.get_config and launcher.get_config("default_target", "USD") or "USD"
                to_c = (fc_test == def_target) and ((def_target == "USD") and "BRL" or "USD") or def_target
            else
                num, from_c = nil, nil
            end
        end
    end

    -- 8. Currency + Number alone (e.g. "$50", "r$299", "usd 100"): default to configured target
    if not num then
        from_c, num = q:match("^([%a]+)%s*(%d*%.?%d+)$")
        if num and from_c then
            local fc_test = normalize_currency(from_c)
            if fc_test then
                local def_target = launcher.get_config and launcher.get_config("default_target", "USD") or "USD"
                to_c = (fc_test == def_target) and ((def_target == "USD") and "BRL" or "USD") or def_target
            else
                num, from_c = nil, nil
            end
        end
    end

    if num and from_c and to_c then
        local amount = tonumber(num)
        local fc = normalize_currency(from_c)
        local tc = normalize_currency(to_c)
        if amount and amount > 0 and fc and tc and RATES[fc] and RATES[tc] then
            return amount, fc, tc
        end
    end

    return nil
end

-- Multi-source asynchronous rate fetching:
-- 1. AwesomeAPI: Real-time commercial bid/ask rates for BRL, USD, EUR, GBP, JPY, CAD, CHF, AUD, CNY, ARS, MXN, CLP, COP, PEN, INR, BTC, ETH, SOL
-- 2. Open ER-API: Global coverage for 166 world currencies
local function update_rates(is_manual, on_done)
    if IS_UPDATING then
        if on_done then on_done(false, "Update already in progress") end
        return
    end
    IS_UPDATING = true

    local awesome_url = "https://economia.awesomeapi.com.br/last/USD-BRL,USD-EUR,USD-GBP,USD-JPY,USD-CAD,USD-CHF,USD-AUD,USD-CNY,USD-ARS,USD-MXN,USD-CLP,USD-COP,USD-PEN,USD-INR,BTC-USD,ETH-USD,SOL-USD"

    local ok, err = pcall(function()
        launcher.http.get({
            url = awesome_url,
            on_complete = function(status, body)
                launcher.log(string.format("AwesomeAPI response: status=%s, body_len=%d", tostring(status), body and #body or 0))
                local awesome_count = 0
                if status == 200 and body then
                    for pair, bid in body:gmatch([["([A-Z]+)":{[^}]*"bid":"([%d%.]+)"]]) do
                        local b = tonumber(bid)
                        if b and b > 0 then
                            if pair:sub(1, 3) == "USD" then
                                local target = pair:sub(4)
                                RATES[target] = b
                                awesome_count = awesome_count + 1
                            elseif pair:sub(4, 6) == "USD" or pair:sub(5, 7) == "USD" then
                                local source = pair:gsub("USD$", "")
                                RATES[source] = 1.0 / b
                                awesome_count = awesome_count + 1
                            end
                        end
                    end
                end

                -- Chain to Open ER-API for worldwide coverage
                local er_ok, er_err = pcall(function()
                    launcher.http.get({
                        url = "https://open.er-api.com/v6/latest/USD",
                        on_complete = function(er_status, er_body)
                            launcher.log(string.format("ER-API response: status=%s, body_len=%d", tostring(er_status), er_body and #er_body or 0))
                            IS_UPDATING = false
                            local er_count = 0
                            if er_status == 200 and er_body then
                                for curr, rate in er_body:gmatch('"([A-Z]+)":%s*([%d%.]+)') do
                                    local r = tonumber(rate)
                                    if r and r > 0 then
                                        -- Keep AwesomeAPI real-time quotes for major pairs, fill all other 150+ currencies
                                        if curr ~= "BRL" and curr ~= "BTC" and curr ~= "ETH" and curr ~= "SOL" then
                                            RATES[curr] = r
                                        end
                                        er_count = er_count + 1
                                    end
                                end
                            end

                            if awesome_count > 0 or er_count > 0 then
                                LAST_UPDATE_TIME = os.time()
                                CURRENT_SOURCE = awesome_count > 0 and "AwesomeAPI + ER-API" or "open.er-api.com"
                                save_cache()
                                local brl_val = RATES["BRL"] or 0
                                local eur_brl = (RATES["BRL"] or 0) / (RATES["EUR"] or 1)
                                launcher.log(string.format("Currency rates updated (%d real-time, %d global). 1 USD = R$ %.4f",
                                    awesome_count, er_count, brl_val))

                                if on_done then
                                    on_done(true, string.format("1 USD = R$ %.4f  •  1 EUR = R$ %.4f", brl_val, eur_brl))
                                end
                            else
                                launcher.log("Failed to update currency rates from network")
                                if on_done then
                                    on_done(false, "Could not fetch exchange rates from network")
                                end
                            end
                        end
                    })
                end)

                if not er_ok then
                    IS_UPDATING = false
                    if awesome_count > 0 then
                        LAST_UPDATE_TIME = os.time()
                        CURRENT_SOURCE = "AwesomeAPI"
                        save_cache()
                        if on_done then
                            on_done(true, string.format("1 USD = R$ %.4f", RATES["BRL"] or 0))
                        end
                    elseif on_done then
                        on_done(false, "Failed to connect to ER-API: " .. tostring(er_err))
                    end
                end
            end
        })
    end)

    if not ok then
        IS_UPDATING = false
        if on_done then
            on_done(false, "Network error: " .. tostring(err))
        end
    end
end

-- Initialize persistent cache and fetch fresh rates
load_cache()
update_rates(false)

-- Register Currency Search Provider
launcher.register_provider({
    id = "currency",
    name = "Currency Exchange",
    search = function(query)
        local q = query:lower():gsub("^%s+", ""):gsub("%s+$", "")
        if #q == 0 then return {} end

        local freshness = get_freshness_text()

        -- Auto-refresh in background if cached rates are older than 30 minutes
        if os.time() - LAST_UPDATE_TIME > 1800 and not IS_UPDATING then
            update_rates(false)
        end

        -- Help / Status / Update action commands
        if q == "update rates" or q == "refresh rates" or q == ":update-rates" or q == "rates update" or
           q == "rates" or q == "currency" or q == "exchange" or q == "cambio" or q == "moeda" or q == "fx" then

            local brl = format_rate(RATES["BRL"] or 5.10)
            local eur_brl = format_rate((RATES["BRL"] or 5.10) / (RATES["EUR"] or 0.86))
            local gbp_brl = format_rate((RATES["BRL"] or 5.10) / (RATES["GBP"] or 0.74))
            local btc_usd = format_number(1.0 / (RATES["BTC"] or 0.00001237), "USD")

            return {
                {
                    id = "currency:force_update",
                    title = "Update Exchange Rates",
                    subtitle = string.format("Status: %s (%s) • 1 USD = R$ %s • Press Enter to refresh now", freshness, CURRENT_SOURCE, brl),
                    icon = "view-refresh",
                    score = 150.0,
                    type = "Currency Action",
                    provider = "Currency Exchange",
                    action = "force_update"
                },
                {
                    id = "currency:summary:brl",
                    title = string.format("USD = R$ %s  |  EUR = R$ %s  |  GBP = R$ %s", brl, eur_brl, gbp_brl),
                    subtitle = string.format("Live Market Rates • %s (%s) • Press Enter to copy USD/BRL", freshness, CURRENT_SOURCE),
                    icon = "accessories-calculator",
                    score = 140.0,
                    type = "Exchange Rate",
                    provider = "Currency Exchange",
                    action = "copy:" .. brl,
                    secondaryActionLabel = "Copy All Rates",
                    secondaryAction = string.format("copy_full:USD/BRL: %s, EUR/BRL: %s, GBP/BRL: %s", brl, eur_brl, gbp_brl)
                },
                {
                    id = "currency:summary:crypto",
                    title = string.format("Bitcoin = $ %s USD", btc_usd),
                    subtitle = string.format("Live Crypto Rates • %s (%s)", freshness, CURRENT_SOURCE),
                    icon = "accessories-calculator",
                    score = 130.0,
                    type = "Cryptocurrency",
                    provider = "Currency Exchange",
                    action = "copy:" .. btc_usd
                },
                {
                    id = "currency:help",
                    title = "Currency Converter Help",
                    subtitle = "Examples: 299brl to usd, $50 in eur, 100 eur to brl, 1000 jpy usd, 50 btc usd",
                    icon = "help-browser",
                    score = 100.0,
                    type = "Currency",
                    provider = "Currency Exchange",
                    action = "help"
                }
            }
        end

        local amount, from_curr, to_curr = parse_currency_query(query)
        if not amount then return {} end

        local rate_from = RATES[from_curr]
        local rate_to = RATES[to_curr]
        local unit_rate = rate_to / rate_from
        local converted = amount * unit_rate

        local formatted_amount = format_number(amount, from_curr)
        local formatted_converted = format_number(converted, to_curr)
        local from_symbol = SYMBOLS[from_curr] or from_curr
        local to_symbol = SYMBOLS[to_curr] or to_curr

        local main_title = string.format("%s %s %s", to_symbol, formatted_converted, to_curr)
        local main_subtitle = string.format("%s %s %s = %s %s %s  (1 %s = %s %s) • %s",
            from_symbol, formatted_amount, from_curr,
            to_symbol, formatted_converted, to_curr,
            from_curr, format_rate(unit_rate), to_curr,
            freshness)

        local copy_value = formatted_converted
        local full_value = string.format("%s %s = %s %s", formatted_amount, from_curr, formatted_converted, to_curr)

        local results = {
            {
                id = string.format("currency:%s:%s", from_curr, to_curr),
                title = main_title,
                subtitle = main_subtitle,
                icon = "accessories-calculator",
                score = 120.0,
                type = "Currency",
                provider = "Currency Exchange",
                action = "copy:" .. copy_value,
                secondaryActionLabel = "Copy Full Text",
                secondaryAction = "copy_full:" .. full_value
            }
        }

        -- Add Inverse Rate result
        local inverse_rate = rate_from / rate_to
        local inverse_title = string.format("1 %s = %s %s", from_curr, format_rate(unit_rate), to_curr)
        local inverse_subtitle = string.format("Inverse: 1 %s = %s %s • %s • %s",
            to_curr, format_rate(inverse_rate), from_curr,
            CURRENT_SOURCE, freshness)

        table.insert(results, {
            id = string.format("currency:rate:%s:%s", from_curr, to_curr),
            title = inverse_title,
            subtitle = inverse_subtitle,
            icon = "accessories-calculator",
            score = 110.0,
            type = "Exchange Rate",
            provider = "Currency Exchange",
            action = "copy:" .. format_rate(unit_rate),
            secondaryActionLabel = "Copy Inverse Rate",
            secondaryAction = "copy:" .. format_rate(inverse_rate)
        })

        return results
    end,

    execute = function(result, action)
        local act = action or result.action or ""

        if act == "force_update" then
            launcher.notify("Currency Exchange", "Fetching latest exchange rates from AwesomeAPI & ER-API...", "view-refresh")
            update_rates(true, function(success, msg)
                if success then
                    launcher.notify("Currency Exchange", "Rates updated!\n" .. msg, "accessories-calculator")
                else
                    launcher.notify("Currency Exchange", "Failed to update rates: " .. msg, "dialog-warning")
                end
            end)
            return true
        end

        if act:sub(1, 5) == "copy:" then
            local val = act:sub(6)
            launcher.copy_to_clipboard(val)
            launcher.notify("Currency Exchange", string.format("Copied %s to clipboard", val), "accessories-calculator")
            return true
        end

        if act:sub(1, 10) == "copy_full:" then
            local val = act:sub(11)
            launcher.copy_to_clipboard(val)
            launcher.notify("Currency Exchange", string.format("Copied: %s", val), "accessories-calculator")
            return true
        end

        if act == "help" then
            launcher.notify("Currency Exchange", "Type e.g. 299brl to usd, $50 to eur, 100 eur to brl, or 'update rates'", "accessories-calculator")
            return true
        end

        return false
    end
})
