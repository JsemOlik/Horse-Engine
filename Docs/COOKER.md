# Asset Cooker Guide

The **HorseCooker** is an offline tool used to convert authoring assets (JSON, GLTF, PNG, etc.) into optimized binary formats for use in the runtime.

## 🖥️ Using from the Editor

For most users, the Asset Cooker is seamlessly integrated into the Horse Editor:

1. **Importing Assets**: Simply drag and drop your files into the **Content Browser**. The Editor will automatically generate `.meta` files and queue them for cooking.
2. **One-Click Cook**: Use the **Build -> Cook Assets** menu option to process all modified assets in your project.

## ⌨️ Command Line Interface (CLI)

For automation and build pipelines, the Cooker can be run directly from the terminal.

### Usage

```powershell
HorseCooker.exe <AssetsDir> <OutputDir> [Platform]
```

### Arguments

- `<AssetsDir>`: The path to your project's `Assets` directory.
- `<OutputDir>`: The destination for the cooked binary assets.
- `[Platform]`: (Optional) The target platform. Defaults to `Windows`.

### Examples

```powershell
# Basic cooking for windows
.\Build\Debug\Tools\HorseCooker.exe .\MyProject\Assets .\MyProject\Cooked

# Cooking for a specific platform
.\Build\Debug\Tools\HorseCooker.exe .\MyProject\Assets .\MyProject\Cooked_XB1 XboxOne
```

## 🛠️ How it Works

1. **Scanning**: The tool recursively scans the source directory for known file extensions.
2. **Identification**: It uses the `.meta` files or hashes of relative paths to identify unique assets via GUIDs.
3. **Conversion**: Each asset type is passed to a specialized "Cooker" (e.g., `TextureCooker`, `MeshCooker`).
4. **Manifest**: A `Game.manifest.json` is generated, providing a mapping from asset GUIDs to their cooked file locations.

---

**Last Updated**: 2026-02-08
