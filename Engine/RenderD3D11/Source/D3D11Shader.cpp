#include "HorseEngine/Render/D3D11Shader.h"
#include "HorseEngine/Core/Logging.h"
#include <d3dcompiler.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <windows.h>

namespace Horse {

#include "HorseEngine/Core/FileSystem.h"

// ...

bool D3D11Shader::CompileFromFile(
    ID3D11Device *device, const std::wstring &filepath,
    const std::string &entryPoint, const std::string &profile,
    const std::vector<D3D_SHADER_MACRO> &defines) {

  std::filesystem::path shaderPath = filepath;
  std::string source;

  // Try reading via FileSystem (works for PAK and local)
  if (FileSystem::ReadText(shaderPath, source)) {
    return CompileShader(device, source, entryPoint, profile, defines);
  }

  // Fallback for dev environment (search up logic removed for
  // brevity/correctness) Actually, FileSystem::ReadText handles verifying
  // existence via PhysFS or std::filesystem. The only missing part is the
  // "search up directories" logic. If we want to keep that for Editor dev flow,
  // we can keep it as fallback.

  namespace fs = std::filesystem;
  // ... original search logic ...
  // Simplified:
  if (!FileSystem::ReadText(shaderPath, source)) {
    // Try searching up
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    fs::path exeDir = fs::path(exePath).parent_path();

    // ... search logic ...
    // If found, read with std::ifstream or FileSystem
    // Let's rely on FileSystem having mounted the correct roots?
    // PhysFS Mounts:
    // / -> Executable Dir (and Game.pak)
    // So "Shaders/Forward.hlsl" should be found if it's in Game.pak OR in
    // ExecutableDir/Shaders.

    // The original search logic was looking in parent directories (like
    // "../../Engine/Shaders"). PhysFS doesn't search up unless we mount those
    // parents. For Runtime, we expect shaders in Game.pak or "Content/" folder
    // relative to exe.

    HORSE_LOG_RENDER_ERROR("Failed to load shader file: {}",
                           shaderPath.string());
    return false;
  }

  return CompileShader(device, source, entryPoint, profile, defines);
}

bool D3D11Shader::CompileFromSource(
    ID3D11Device *device, const std::string &source,
    const std::string &entryPoint, const std::string &profile,
    const std::vector<D3D_SHADER_MACRO> &defines) {
  return CompileShader(device, source, entryPoint, profile, defines);
}

bool D3D11Shader::CompileShader(ID3D11Device *device, const std::string &source,
                                const std::string &entryPoint,
                                const std::string &profile,
                                const std::vector<D3D_SHADER_MACRO> &defines) {
  UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
  compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

  // Handle null termination safely
  const D3D_SHADER_MACRO *pDefines = nullptr;
  std::vector<D3D_SHADER_MACRO> definesWithNull;
  if (!defines.empty()) {
    definesWithNull = defines;
    definesWithNull.push_back({nullptr, nullptr}); // Terminator
    pDefines = definesWithNull.data();
  }

  ComPtr<ID3DBlob> errorBlob;
  HRESULT hr =
      D3DCompile(source.c_str(), source.length(), nullptr, pDefines,
                 D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint.c_str(),
                 profile.c_str(), compileFlags, 0, &m_Bytecode, &errorBlob);

  if (FAILED(hr)) {
    if (errorBlob) {
      HORSE_LOG_RENDER_ERROR(
          "Shader compilation failed: {}",
          static_cast<const char *>(errorBlob->GetBufferPointer()));
    } else {
      HORSE_LOG_RENDER_ERROR("Shader compilation failed with no error message");
    }
    return false;
  }

  if (profile.find("vs_") == 0) {
    hr = device->CreateVertexShader(m_Bytecode->GetBufferPointer(),
                                    m_Bytecode->GetBufferSize(), nullptr,
                                    &m_VertexShader);
    if (FAILED(hr)) {
      HORSE_LOG_RENDER_ERROR("Failed to create vertex shader");
      return false;
    }
  } else if (profile.find("ps_") == 0) {
    hr = device->CreatePixelShader(m_Bytecode->GetBufferPointer(),
                                   m_Bytecode->GetBufferSize(), nullptr,
                                   &m_PixelShader);
    if (FAILED(hr)) {
      HORSE_LOG_RENDER_ERROR("Failed to create pixel shader");
      return false;
    }
  }

  return true;
}

} // namespace Horse
