# Game Packager Guide

The **HorsePackager** is the final step in the development pipeline. it bundles your cooked assets, runtime executable, and gameplay logic into a distributable package.

## 🖥️ Using from the Editor

The Editor provides a simple interface for packaging:

1. Go to **Build -> Package Project**.
2. Select your target platform and name.
3. Choose an output directory.
4. The Editor will automatically run the Cooker and then the Packager.

## ⌨️ Command Line Interface (CLI)

The Packager can be used in your Continuous Integration (CI) systems for automated builds.

### Usage

```powershell
HorsePackager.exe <CookedAssetsDir> <OutputDir> <RuntimeExe> <GameDLL> [GameName] [IconPath]
```

### Arguments

- `<CookedAssetsDir>`: Path to the directory containing assets processed by `HorseCooker`.
- `<OutputDir>`: Where the final game bundle should be created.
- `<RuntimeExe>`: Path to the base `HorseRuntime.exe` build.
- `<GameDLL>`: Path to your compiled `HorseGame.dll` (gameplay logic).
- `[GameName]`: (Optional) The name of your executable. Defaults to `MyGame`.
- `[IconPath]`: (Optional) Path to a `.ico` file to inject into the executable.

### Example

```powershell
.\Build\Debug\Tools\HorsePackager.exe .\Cooked .\Dist .\Build\Debug\Runtime\HorseRuntime.exe .\Build\Debug\Game\HorseGame.dll "MyEpicGame" .\Assets\Icon.ico
```

## 📦 What's in the Package?

- **[GameName].exe**: The engine bootstrapper with your custom icon.
- **Game.pak**: A block-compressed archive containing all your cooked assets.
- **HorseGame.dll**: Your C++ gameplay logic.
- **Engine Shaders**: Necessary HLSL binaries for the runtime.
- **Dependencies**: All required third-party DLLs (e.g., `Qt5Core.dll` is NOT included, but `spdlog` etc. are if dynamically linked).

---

**Last Updated**: 2026-02-08
