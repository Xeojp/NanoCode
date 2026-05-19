cmake_minimum_required(VERSION 3.16)
project(UltraIDE LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")

# nlohmann_json
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.2
    )
    FetchContent_MakeAvailable(json)
endif()

add_executable(UltraIDE 
    main_window.cpp
    ConfigManager.cpp
    AIClient.cpp
    AIAssistant.cpp
    AIAssistantWindow.cpp
    ContextCollector.cpp
    SecureKeyStorage.cpp
    ClipboardManager.cpp
    Scratchpad.cpp
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_tables.cpp
    imgui/backends/imgui_impl_win32.cpp
    imgui/backends/imgui_impl_dx11.cpp
)

target_include_directories(UltraIDE PRIVATE imgui imgui/backends)
target_link_libraries(UltraIDE d3d11 dxgi d3dcompiler shlwapi wincred nlohmann_json::nlohmann_json)