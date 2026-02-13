# 📜 Lua Scripting Reference

Welcome to the Horse Engine scripting guide. We use **LuaJIT** to provide a fast, flexible, and easy-to-use scripting environment for your gameplay logic.

---

## 🚀 Script Structure

Every script should be a module that returns a table containing lifecycle functions.

```lua
local PlayerMovement = {}

-- Called once when the script component is initialized
function PlayerMovement:OnCreate(entity)
    self.speed = 5.0
    Horse.LogInfo("Entity " .. entity:GetName() .. " is ready!")
end

-- Called every frame during Play Mode
-- @param deltaTime: time since last frame in seconds
function PlayerMovement:OnUpdate(entity, deltaTime)
    local transform = entity:GetTransform()
    local pos = transform.Position

    if Horse.Input.IsKeyPressed(Horse.KEY_W) then
        pos.z = pos.z + self.speed * deltaTime
    end

    transform.Position = pos
end

return PlayerMovement
```

---

## 🛠️ Global Namespace: `Horse`

The `Horse` namespace is your gateway to engine functionality.

### 📝 Logging

- `Horse.LogInfo(msg)`: Output an informative message to the console.
- `Horse.LogWarn(msg)`: Output a warning.
- `Horse.LogError(msg)`: Output an error (includes stack trace).

### ⌨️ Input

Access real-time input status for keyboard and mouse.

| Method                      | Description                                  |
| :-------------------------- | :------------------------------------------- |
| `IsKeyPressed(keyCode)`     | Returns `true` if the key is currently held. |
| `IsActionPressed(name)`     | Checks for an action binding (e.g., "Jump"). |
| `GetAxisValue(name)`        | Returns a float (-1.0 to 1.0) for an axis.   |
| `IsMouseButtonPressed(btn)` | Checks for `Horse.KEY_LBUTTON`, `RBUTTON`.   |
| `GetMousePosition()`        | Returns a table `{x, y}`.                    |

---

## 🧊 Entity API

The `entity` object represents a game object in the scene.

- **`entity:GetName()`**: Returns the string name (tag).
- **`entity:GetUUID()`**: Returns the stable GUID string.
- **`entity:GetTransform()`**: Returns the `TransformComponent` (guaranteed).

---

## 🔧 Components

### TransformComponent

Manages position, rotation, and scale in 3D space.

- **`.Position`**: Table `{x, y, z}`.
- **`.Rotation`**: Table `{x, y, z}` (Euler angles in degrees).
- **`.Scale`**: Table `{x, y, z}`.

> [!TIP]
> Always re-assign the table to actually apply changes:
> `local p = t.Position; p.x = 10; t.Position = p`

---

## ⌨️ Key Codes Reference

Common keys are available under `Horse.KEY_`:

- `W`, `A`, `S`, `D`, `SPACE`, `SHIFT`, `CONTROL`, `ESCAPE`
- `0` through `9`
- `LBUTTON`, `RBUTTON`, `MBUTTON`

---

**Last Updated**: 2026-02-08
