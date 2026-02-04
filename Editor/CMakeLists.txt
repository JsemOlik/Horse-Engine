# Editor - Qt5-based editor application
find_package(Qt5 COMPONENTS Widgets REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_executable(HorseEditor WIN32
    Source/main.cpp
    Source/EditorWindow.cpp
    Source/EditorWindow.h
    Source/D3D11ViewportWidget.cpp
    Source/D3D11ViewportWidget.h
    Source/Panels/HierarchyPanel.cpp
    Source/Panels/HierarchyPanel.h
    Source/Panels/InspectorPanel.cpp
    Source/Panels/InspectorPanel.h
    Source/Panels/SceneViewport.cpp
    Source/Panels/SceneViewport.h
    Source/Panels/GameViewport.cpp
    Source/Panels/GameViewport.h
    Source/Panels/ContentBrowserPanel.cpp
    Source/Panels/ContentBrowserPanel.h
    Source/Panels/ConsolePanel.cpp
    Source/Panels/ConsolePanel.h
    Source/Dialogs/ProjectSettingsDialog.cpp
    Source/Dialogs/ProjectSettingsDialog.h
)

target_include_directories(HorseEditor
    PRIVATE Source Include
)

target_link_libraries(HorseEditor
    PRIVATE
        HorseRuntime
        HorseRenderD3D11
        Qt5::Widgets
)

set_target_properties(HorseEditor PROPERTIES
    WIN32_EXECUTABLE TRUE
)
