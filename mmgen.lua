local agent = {}
local mobility_model = {}
local random_waypoint_mobility, random_point_group_mobility

local vector = {}

local function table_deep_copy(tbl)
    -- actually won't work in bidirectional references,
    -- but we are not going to them
    local res = {}
    for k, v in pairs(tbl) do
        if type(v) == "table" then
            res[k] = table_deep_copy(v)
        else
            res[k] = v
        end
    end

    return res
end

-- basic vector operations

function vector.dist_to(from, to)
    local dx = from.x - to.x
    local dy = from.y - to.y
    return math.sqrt(dx * dx + dy * dy)
end

function vector.unit(v)
    local magnitude = math.sqrt(v[1] * v[1] + v[2] * v[2])
    return  {
        v[1] / magnitude,
        v[2] / magnitude,
    }
end

function vector.add(a, b)
    return {
        a[1] + b[1],
        a[2] + b[2],
    }
end

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

    local speed = 0
    if #self.path ~= 0 then
        local prev = self:get_last_pos()
        local dist = vector.dist_to(point, prev)
        if time - prev.time > 0 then
            speed = dist / (time - prev.time)
        end
    end

    table.insert(self.path, {
        x = point.x,
        y = point.y,
        time = time,
        speed = speed
    })
end

function agent:get_pos(time)
    local step = 1
    local speed = 0

    for i, point in ipairs(self.path) do
        if point.time <= time then
            step = i

            if point.speed > 0 then
                speed = point.speed
            end
        end
    end

    if not self.path[step + 1] or not self.path[step] or self.path[step].time == time then
        return self.path[step]
    end

    local dx = self.path[step + 1].x - self.path[step].x
    local dy = self.path[step + 1].y - self.path[step].y
    local dt = self.path[step + 1].time - self.path[step].time

    local ratio = (time - self.path[step].time) / dt

    return {
        x = self.path[step].x + dx * ratio,
        y = self.path[step].y + dy * ratio,
        time = time,
        speed = speed
    }
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

function agent:get_color_str()
    local col = self:get_color()
    col[1] = col[1] or 0
    col[2] = col[2] or 0
    col[3] = col[3] or 0
    return ("rgb(%d, %d, %d)"):format(table.unpack(col))
end

function agent:set_color(rgb)
    self.color = rgb
end

function agent:dump(filename)
    local data = ("%d %d %d\n"):format(table.unpack(self:get_color()))
    for _, point in pairs(self.path) do
        point.x = math.max(0, point.x)
        point.y = math.max(0, point.y)

        point.x = math.floor(point.x)
        point.y = math.floor(point.y)
        point.time = math.floor(point.time)

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
        if type(value) ~= "function" then
            self[key] = value
        end
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
    self:for_each_agent(function(ag)
        ag:set_color(self.mobility.color)
        -- provide waypoint mobility path to the agent

        local timeline = ag:get_last_pos().time

        while timeline < self.time do

            local dest = {math.random(self.grid.width), math.random(self.grid.height)}

            local dist = math.sqrt(dest[1] * dest[1] + dest[2] * dest[2])
            local speed = math.random(self.mobility.speed.min, self.mobility.speed.max)

            local time = dist / speed

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

        -- remove last pause
        table.remove(ag.path, #ag.path)
    end)
end

-- Random Group Mobility

random_point_group_mobility = mobility_model.new()

function random_point_group_mobility.new(config)
    local instance = mobility_model.new()
    instance:init(config)

    -- preserve config for waypoint mobility for leaders
    local rwmm = table_deep_copy(config)

    rwmm.grid.agents = config.mobility.groups
    rwmm.mobility.color = {0, 0, 0}
    instance.rwmm = rwmm

    setmetatable(instance, {
        __index = random_point_group_mobility
    })
    return instance
end

function random_point_group_mobility:for_each_leader(func)
    self:for_each_agent(function(ag)
        -- leaders are the first N agents
        if ag.id <= self.mobility.groups then
            func(ag)
        end
    end)
end

function random_point_group_mobility:for_each_nonleader(func)
    self:for_each_agent(function(ag)
        if ag.id > self.mobility.groups then
            func(ag)
        end
    end)
end

function random_point_group_mobility:for_each_follower(id, func)
    if id > self.mobility.groups then
        return
    end

    -- followers are distinguished by color
    local leader = self.agents[id]

    self:for_each_agent(function(ag)
        -- skip leaders
        if ag.id <= self.mobility.groups then
            return
        end

        if leader:get_color_str() == ag:get_color_str() then
            func(ag)
        end
    end)
end

function random_point_group_mobility:sync_movement(leader, fol)
    local mob_int = self.mobility.follower_interval
    for step = 2, self.time, mob_int do
        local lpos = leader:get_pos(step)

        local dest = {math.random(self.grid.width), math.random(self.grid.height)}

        -- unit vector
        local direction = vector.unit(dest)

        -- select distance from leader
        local dist_l = math.random(self.mobility.leader_radius)

        local fol_dest = {direction[1] * dist_l + lpos.x, direction[2] * dist_l + lpos.y}
        local fpos = fol:get_last_pos()
        local distfl = vector.dist_to(fpos, lpos)

        -- if dist to leader is more than accepted, set speed to max
        local speed
        if distfl > self.mobility.leader_radius then
            speed = self.mobility.speed.max
        else
            -- deviation also can slow down
            local prod = math.random(0, 1)
            if prod == 0 then
                prod = -1
            end

            local lspeed = lpos.speed
            local devi = math.random(self.mobility.deviation) * prod
            speed = math.max(self.mobility.speed.min, lspeed + devi)
            speed = math.min(self.mobility.speed.max, speed)
        end

        local fdir = {fol_dest[1] - fpos.x, fol_dest[2] - fpos.y}

        local fdir_unit = vector.unit(fdir)

        local fdir_speed = {fdir_unit[1] * speed * mob_int, fdir_unit[2] * speed * mob_int}

        local newx = math.max(0, math.min(fpos.x + fdir_speed[1], self.grid.width))
        local newy = math.max(0, math.min(fpos.y + fdir_speed[2], self.grid.height))
        fol:add_point({x = newx, y = newy}, step)
    end
end

function random_point_group_mobility:distribute_followers()
    local nocolor = self.grid.agents - self.mobility.groups

    while nocolor > 0 do
        self:for_each_leader(function(leader)
            local lpos = leader:get_last_pos()

            local closest

            self:for_each_nonleader(function(ag)
                if ag:get_color() ~= nil then
                    return
                end

                local dist = vector.dist_to(lpos, ag:get_last_pos())
                if not closest or dist < vector.dist_to(lpos, closest:get_last_pos()) then
                    closest = ag
                end
            end)

            if closest then
                closest:set_color(leader:get_color())
            end
        end)

        local _nocolor = 0
        self:for_each_nonleader(function(fol)
            if fol:get_color() == nil then
                _nocolor = _nocolor + 1
            end
        end)
        nocolor = _nocolor
    end
end

function random_point_group_mobility:run()
    -- leaders are obtained via random waypoint mobility model

    -- this variables are used to initialize waypoint
    -- self.color = {0, 0, 0}
    -- self.grid.agents = self.mobility.groups

    local rwmm = random_waypoint_mobility.new(self.rwmm)
    rwmm:run()

    rwmm:for_each_agent(function(ag)
        self.agents[ag.id] = ag
    end)

    -- color leaders
    self:for_each_leader(function(leader)
        leader:set_color(self.mobility.colors[leader.id])
    end)

    self:distribute_followers()

    self:for_each_leader(function(leader)
        self:for_each_follower(leader.id, function(fol)
            self:sync_movement(leader, fol)
        end)
    end)
end

-- load config and run program

assert(arg[1] and arg[2], "Usage: mmgen CONFIG_NAME OUTDIR [SEED]")

local seed = tonumber(arg[3])
if seed then
    print(("Custom seed is set to %d"):format(seed))
end

local file = io.open(arg[1], "r")
local config = load("return " .. file:read("*a"))()
file:close()

config.seed = seed or config.seed

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
