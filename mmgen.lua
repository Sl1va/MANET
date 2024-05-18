local agent = {}
local mobility_model = {}
local random_waypoint_mobility, random_point_group_mobility

-- agent implementation

function agent.new(id, color)
    local instance = {
        path = {},
        id = id,
        color = color
    }
    setmetatable(instance, {
        __index = agent
    })
    return instance
end

function agent:add_point(point, time)
    if #self.path > 0 and self.path[#self.path].time >= time then
        error("Can not add point in the past!")
    end

    table.insert(self.path, {
        x = point.x,
        y = point.y,
        time = time
    })
end

function agent:get_pos(time)

end

function agent:get_last_pos()
    local path = self.path or {}
    local p = path[#path]
    return {
        x = p.x,
        y = p.y,
        time = p.time
    }
end

function agent:get_color()
    return self.color    
end

function agent:dump(filename)
    local data = ("%d %d %d\n"):format(table.unpack(self:get_color()))
    for _, point in pairs(self.path) do
        data = data .. ("%d %d %d\n"):format(point.x, point.y, point.time)
    end

    local file = io.open(filename, "w");
    file:write(data)
    file:close()
end

-- mobility model definition

function mobility_model.new()
    local instance = {
        agents = {}
    }
    setmetatable(instance, {
        __index = mobility_model
    })
    return instance
end

function mobility_model:init(config)
    for key, value in pairs(config) do
        self[key] = value
    end

    -- initialize randomly all agents, it is common part
    math.randomseed(config.seed)

    for index = 1, config.grid.agents do
        table.insert(self.agents, agent.new(index))
        self.agents[#self.agents]:add_point({
            x = math.random(config.grid.width),
            y = math.random(config.grid.height)
        }, 1)
    end
end

function mobility_model:for_each_agent(func)
    for _, agent in ipairs(self.agents) do
        func(agent)
    end
end

function mobility_model:run()
    error("Not implemented")
end

-- Random Waypoint Mobility implementation

random_waypoint_mobility = mobility_model.new()

function random_waypoint_mobility.new(config)
    local instance = mobility_model.new()
    instance:init(config)
    setmetatable(instance, {
        __index = random_waypoint_mobility
    })
    return instance
end

function random_waypoint_mobility:run()
    local tmp = 1
    self:for_each_agent(function(ag)
        ag.color = self.mobility.color
        -- provide waypoint mobility path to the agent

        local timeline = ag:get_last_pos().time

        while timeline < self.time do

            local dest = {math.random(self.grid.width), math.random(self.grid.height)}

            local dist = math.sqrt(dest[1] * dest[1] + dest[2] * dest[2])
            local speed = math.random(self.mobility.speed.min, self.mobility.speed.max)

            local time = math.floor(dist / speed)

            local last_pos = ag:get_last_pos()

            ag:add_point({
                x = dest[1],
                y = dest[2]
            }, last_pos.time + time)

            -- add pause
            local wait_time = math.random(self.mobility.pause.min, self.mobility.pause.max)
            last_pos = ag:get_last_pos()
            ag:add_point({
                x = dest[1],
                y = dest[2]
            }, last_pos.time + wait_time)

            last_pos = ag:get_last_pos()
            timeline = last_pos.time
        end
    end)
end

-- Random Group Mobility

-- random_point_group_mobility = mobility_model.new()

-- function random_point_group_mobility.new(config)
--     local instance = config
--     setmetatable(instance, {__index = random_point_group_mobility})
--     return instance
-- end

-- function random_point_group_mobility:run()

-- end

-- load config and run program

assert(arg[1] and arg[2], "Usage: mmgen CONFIG_NAME OUTDIR")

local file = io.open(arg[1], "r")
local config = load("return " .. file:read("*a"))()
file:close()

local mm
if config.mobility.model == "RWMM" then
    mm = random_waypoint_mobility.new(config)
else
    mm = random_point_group_mobility.new(config)
end

mm:run()
mm:for_each_agent(function(agent)
    agent:dump(("%s/agent_%d"):format(arg[2], agent.id))
end)

