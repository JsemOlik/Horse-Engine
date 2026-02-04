# RenderD3D11 - Direct3D 11 Renderer Module
add_library(HorseRenderD3D11 STATIC
    Source/D3D11Renderer.cpp
    Source/D3D11Shader.cpp
)

target_include_directories(HorseRenderD3D11
    PUBLIC Include
    PRIVATE Source
)

target_link_libraries(HorseRenderD3D11
    PUBLIC
        HorseRuntime
    PRIVATE
        d3d11.lib
        dxgi.lib
        d3dcompiler.lib
        dxguid.lib
)
