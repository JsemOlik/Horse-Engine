InputTest = {
    OnCreate = function(self, entity)
        Horse.LogInfo("InputTest script created for entity: " .. entity:GetName())
    end,

    OnUpdate = function(self, entity, dt)
        -- Hardcoded keys for now to test basic mapping
        -- KEY_SPACE = 0x20, KEY_W = 0x57, KEY_S = 0x53
        
        -- Testing direct key access
        if Horse.Input.IsKeyPressed(0x20) then -- Space
            Horse.LogInfo("Space key is PRESSED!")
        end

        -- Testing mapped actions/axes (requires registration in C++)
        if Horse.Input.IsActionPressed("Jump") then
            Horse.LogInfo("Action 'Jump' triggered!")
        end

        local forward = Horse.Input.GetAxisValue("Forward")
        if math.abs(forward) > 0.1 then
            Horse.LogInfo("Axis 'Forward' value: " .. forward)
        end
    end
}

return InputTest
