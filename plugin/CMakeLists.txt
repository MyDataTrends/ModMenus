cmake_minimum_required(VERSION 3.21)

project(
    ModMenus
    VERSION 1.0.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

find_package(CommonLibSSE CONFIG REQUIRED)
find_package(spdlog        CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# One controller-friendly list of every mod-added menu: entries from Data/SKSE/Plugins/ModMenus/*.json, keys read from
# each mod's own config files when the list opens.
add_commonlibsse_plugin(${PROJECT_NAME}
    NAME        "Mod Menus"
    AUTHOR      "BigQuinn"
    DESCRIPTION "Open every mod menu from one controller-friendly list"
    SOURCES
        src/Main.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE src)
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog_header_only nlohmann_json::nlohmann_json)
target_precompile_headers(${PROJECT_NAME} PRIVATE src/PCH.h)
