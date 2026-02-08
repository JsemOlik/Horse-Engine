# Lua Scripting Reference

This document covers everything you can do within Lua scripts in the Horse Engine.

## Script Lifecycle

Every Lua script should follow this structure:

```lua
local MyScript = {}

-- Called when the script component is first created
function MyScript:OnCreate(entity)
    Horse.LogInfo("Script created for: " .. entity:GetName())
end

-- Called every frame in Play Mode
function MyScript:OnUpdate(entity, deltaTime)
    -- Your logic here
end

return MyScript
```

## Global Namespace: `Horse`

### Logging

Use these to output messages to the Editor Console:

- `Horse.LogInfo(message)`
- `Horse.LogWarn(message)`
- `Horse.LogError(message)`

### Input

Access keyboard and mouse state:

#### Keyboard

- `Horse.Input.IsKeyPressed(keyCode)`: Returns boolean. Use `Horse.KEY_W`, `Horse.KEY_SPACE`, etc.
- `Horse.Input.IsActionPressed(actionName)`: Checks if any key mapped to an action (e.g., "Jump") is pressed.
- `Horse.Input.GetAxisValue(axisName)`: Returns a float (usually -1.0 to 1.0) for an axis (e.g., "Forward").

#### Mouse

- `Horse.Input.IsMouseButtonPressed(buttonCode)`: Returns boolean. Use `Horse.KEY_LBUTTON`, `Horse.KEY_RBUTTON`.
- `Horse.Input.GetMousePosition()`: Returns a table with `x` and `y`.

## Entity API

The `entity` object passed to `OnCreate` and `OnUpdate` has these methods:

- `entity:GetName()`: Returns the entity's tag name.
- `entity:GetUUID()`: Returns the unique identifier string.
- `entity:GetTransform()`: Returns the `TransformComponent`.

## Components

### TransformComponent

Access and modify an entity's position, rotation, and scale:

- `transform.Position`: Table with `x`, `y`, `z`.
- `transform.Rotation`: Table with `x`, `y`, `z` (Euler angles).
- `transform.Scale`: Table with `x`, `y`, `z`.

Example of moving an entity:

```lua
function MyScript:OnUpdate(entity, deltaTime)
    local transform = entity:GetTransform()
    local pos = transform.Position
    pos.x = pos.x + 1.0 * deltaTime
    transform.Position = pos
end
```

## Key Codes Reference

Standard key codes are prefixed with `Horse.KEY_`. Examples:

- `Horse.KEY_W`, `Horse.KEY_A`, `Horse.KEY_S`, `Horse.KEY_D`
- `Horse.KEY_SPACE`, `Horse.KEY_ESCAPE`, `Horse.KEY_SHIFT`, `Horse.KEY_CONTROL`
- `Horse.KEY_0` through `Horse.KEY_9`
- `Horse.KEY_LBUTTON`, `Horse.KEY_RBUTTON`, `Horse.KEY_MBUTTON`
