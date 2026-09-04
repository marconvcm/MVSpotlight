-- Currency Exchange Plugin for MVSpotlight
-- Converts amounts between global currencies with real-time rates and offline fallbacks

local RATES = {
    USD = 1.0,
    EUR = 0.92,
    BRL = 5.50,
    GBP = 0.78,
    JPY = 152.5,
    CAD = 1.38,
    AUD = 1.52,
    CHF = 0.89,
    CNY = 7.24,
    INR = 83.5,
    MXN = 18.5,
    KRW = 1350.0,
    SGD = 1.35,
    HKD = 7.81,
    NZD = 1.65,
    SEK = 10.5,
    NOK = 10.8,
    TRY = 33.5,
    ARS = 950.0,
    CLP = 930.0,
    COP = 4100.0,
    PEN = 3.75,
    ZAR = 18.2,
    THB = 36.5,
    AED = 3.67,
    SAR = 3.75,
    ILS = 3.72,
    PLN = 4.02,
    BTC = 0.000016,
    ETH = 0.00038
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
    ETH = "Ξ"
}

local ALIASES = {
    ["$"] = "USD",
    ["us$"] = "USD",
    ["usd"] = "USD",
    ["dollar"] = "USD",
    ["dollars"] = "USD",
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
    ["cny"] = "CNY",
    ["yuan"] = "CNY",
    ["rmb"] = "CNY",
    ["inr"] = "INR",
    ["rupee"] = "INR",
    ["rupees"] = "INR",
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
    ["ethereum"] = "ETH"
}

-- Format numbers with comma separation and appropriate decimal precision
local function format_number(val, currency)
    if not val then return "0" end
    local decimals = 2
    if currency == "JPY" or currency == "KRW" or currency == "CLP" or currency == "COP" then
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

    -- 7. Number + Currency alone (e.g. "299brl", "299 brl"): default to USD (or EUR if from is USD)
    if not num then
        num, from_c = q:match("^(%d*%.?%d+)%s*([%a]+)$")
        if num and from_c then
            local fc_test = normalize_currency(from_c)
            if fc_test then
                to_c = (fc_test == "USD") and "EUR" or "USD"
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

-- Asynchronously fetch latest exchange rates if network permission is enabled
local function update_rates()
    pcall(function()
        launcher.http.get({
            url = "https://open.er-api.com/v6/latest/USD",
            on_complete = function(status, body)
                if status == 200 and body then
                    local updated_count = 0
                    for curr, rate in body:gmatch('"([A-Z]+)":%s*([%d%.]+)') do
                        local r = tonumber(rate)
                        if r and r > 0 then
                            RATES[curr] = r
                            updated_count = updated_count + 1
                        end
                    end
                    if updated_count > 0 then
                        launcher.log(string.format("Currency rates updated (%d currencies from open.er-api.com)", updated_count))
                    end
                end
            end
        })
    end)
end

update_rates()

-- Register Currency Search Provider
launcher.register_provider({
    id = "currency",
    name = "Currency Exchange",
    search = function(query)
        local q = query:lower():gsub("^%s+", ""):gsub("%s+$", "")
        if #q == 0 then return {} end

        -- Help item when typing currency or fx
        if q == "currency" or q == "fx" or q == "exchange" or q == "cambio" or q == "moeda" then
            return {
                {
                    id = "currency:help",
                    title = "Currency Exchange Converter",
                    subtitle = "Examples: 299brl to usd, $50 in eur, 100 eur to brl, 1000 jpy usd",
                    icon = "accessories-calculator",
                    score = 95.0,
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
        local main_subtitle = string.format("%s %s %s = %s %s %s  (1 %s = %s %s)",
            from_symbol, formatted_amount, from_curr,
            to_symbol, formatted_converted, to_curr,
            from_curr, format_rate(unit_rate), to_curr)

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
        local inverse_title = string.format("1 %s = %s %s", to_curr, format_rate(inverse_rate), from_curr)
        local inverse_subtitle = string.format("Inverse exchange rate: 1 %s = %s %s (1 %s = %s %s)",
            to_curr, format_rate(inverse_rate), from_curr,
            from_curr, format_rate(unit_rate), to_curr)

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
            launcher.notify("Currency Exchange", "Type e.g. 299brl to usd, $50 to eur, 100 eur to brl", "accessories-calculator")
            return true
        end

        return false
    end
})
