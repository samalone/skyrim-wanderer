cmake_minimum_required(VERSION 3.24)

project(
    Wanderer
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CommonLibSSE CONFIG REQUIRED)
find_path(SIMPLEINI_INCLUDE_DIR SimpleIni.h)

set(SOURCES
    src/Main.cpp
    src/Settings.h
    src/Settings.cpp
    src/QuestTracker.h
    src/QuestTracker.cpp
    src/Overrides.h
    src/Overrides.cpp
)

add_commonlibsse_plugin(
    ${PROJECT_NAME}
    AUTHOR "wanderer"
    SOURCES ${SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE src)
target_include_directories(${PROJECT_NAME} PRIVATE ${SIMPLEINI_INCLUDE_DIR})
target_precompile_headers(${PROJECT_NAME} PRIVATE src/PCH.h)
