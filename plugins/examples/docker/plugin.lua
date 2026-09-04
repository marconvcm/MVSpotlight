-- Docker Containers Plugin for MVSpotlight

local plugin = {}

launcher.register_provider({
    id = "docker",
    search = function(query)
        local q = string.lower(query or "")
        -- Trigger on 'docker' or 'd ' or direct container name
        local isDockerQuery = string.find(q, "^docker") or string.find(q, "^d ")

        local res = launcher.process.run({
            command = "docker",
            arguments = { "ps", "-a", "--format", "{{.ID}}\t{{.Names}}\t{{.Status}}\t{{.Image}}" },
            timeout = 1500
        })

        if not res or not res.success then
            if isDockerQuery then
                return {
                    {
                        id = "docker:daemon-down",
                        title = "Docker Daemon Unavailable",
                        subtitle = "Ensure the docker service is active (systemctl start docker)",
                        icon = "dialog-warning",
                        score = 80,
                        data = {}
                    }
                }
            end
            return {}
        end

        local results = {}
        local filter = q
        if isDockerQuery then
            filter = string.gsub(filter, "^docker%s*", "")
            filter = string.gsub(filter, "^d%s*", "")
        end

        for line in string.gmatch(res.stdout, "[^\r\n]+") do
            local id, name, status, image = string.match(line, "([^\t]+)\t([^\t]+)\t([^\t]+)\t([^\t]+)")
            if id and name then
                local lowerName = string.lower(name)
                local lowerImage = string.lower(image)

                local isMatch = (filter == "")
                    or string.find(lowerName, filter, 1, true)
                    or string.find(lowerImage, filter, 1, true)
                    or isDockerQuery

                if isMatch then
                    local isRunning = string.find(status, "^Up") ~= nil
                    local actionVerb = isRunning and "Stop" or "Start"
                    local iconName = isRunning and "media-playback-start" or "media-playback-pause"

                    table.insert(results, {
                        id = "docker:" .. id,
                        title = name,
                        subtitle = string.format("Docker (%s) · %s · Press Enter to %s", image, status, actionVerb),
                        icon = iconName,
                        score = isDockerQuery and 90 or 75,
                        type = "Docker",
                        secondaryAction = id,
                        secondaryActionLabel = "Copy Container ID",
                        data = {
                            containerId = id,
                            name = name,
                            running = isRunning
                        }
                    })
                end
            end
        end

        return results
    end,

    execute = function(result)
        local data = result.data or {}
        local cid = data.containerId
        local name = data.name or cid
        if not cid then return end

        local action = data.running and "stop" or "start"
        launcher.process.run_async({
            command = "docker",
            arguments = { action, cid },
            on_complete = function(res)
                if res.success then
                    launcher.notify("Docker", string.format("Container %s %sed", name, action), "application-x-docker-compose")
                else
                    launcher.notify("Docker Error", string.format("Failed to %s %s", action, name), "dialog-error")
                end
            end
        })
    end
})
