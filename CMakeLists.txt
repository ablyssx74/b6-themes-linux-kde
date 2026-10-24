cmake_minimum_required(VERSION 3.16)
project(b6-themes-kde VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui)

# the frame drawing: Qt only
add_library(b6core STATIC src/XfwmTheme.cpp src/FrameRenderer.cpp)
target_include_directories(b6core PUBLIC src)
target_link_libraries(b6core PUBLIC Qt6::Core Qt6::Gui)
set_target_properties(b6core PROPERTIES POSITION_INDEPENDENT_CODE ON)

# draws frames to a PNG, for looking at a theme without a window manager
add_executable(b6-preview tools/preview.cpp)
target_link_libraries(b6-preview PRIVATE b6core)
