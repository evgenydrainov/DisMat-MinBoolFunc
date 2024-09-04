cmake_minimum_required(VERSION 3.0)
project(MinBoolFunc)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_definitions($<$<CONFIG:Debug>:_DEBUG>)
add_compile_definitions($<$<CONFIG:Debug>:DEBUG>)
add_compile_definitions($<$<NOT:$<CONFIG:Debug>>:NDEBUG>)
add_compile_definitions(LUA_USE_LINUX)

include_directories(src/imgui)
include_directories(src/lua)
include_directories(/usr/include/freetype2)

set(SOURCES
	src/main_linux.cpp
	src/MinBoolFunc.cpp
	src/imgui/imgui_unity_build.cpp
	src/lua/onelua.c)

link_libraries(glfw GL freetype)

add_executable(${PROJECT_NAME} ${SOURCES})
