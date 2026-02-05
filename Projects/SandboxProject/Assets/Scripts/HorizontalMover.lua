-- HorizontalMover.lua

local script = {}

function script:OnCreate(entity)
    Horse.LogInfo("HorizontalMover: OnCreate for " .. entity:GetName())
    self.speed = 2.0
    self.timer = 0.0
end

function script:OnUpdate(entity, deltaTime)
    self.timer = self.timer + deltaTime
    
    local transform = entity:GetTransform()
    local pos = transform.Position
    
    -- Move left and right
    pos.x = math.sin(self.timer * self.speed) * 5.0
    
    transform.Position = pos
end

return script
