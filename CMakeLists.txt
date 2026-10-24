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

# the KWin decoration plugin (Plasma 6): built when KDecoration3 is installed
find_package(ECM QUIET NO_MODULE)
if(ECM_FOUND)
    list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
    include(KDEInstallDirs6 OPTIONAL)
endif()
find_package(KDecoration3 CONFIG QUIET)
find_package(KF6CoreAddons CONFIG QUIET)
if(KDecoration3_FOUND AND KF6CoreAddons_FOUND)
    add_library(org.kde.b6xfwm MODULE plugin/B6Decoration.cpp)
    target_include_directories(org.kde.b6xfwm PRIVATE plugin)
    target_link_libraries(org.kde.b6xfwm PRIVATE b6core KDecoration3::KDecoration KF6::CoreAddons)
    set_target_properties(org.kde.b6xfwm PROPERTIES PREFIX ""
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins/org.kde.kdecoration3)
    # the factory macro carries the plugin's metadata (b6decoration.json), which moc has to see
    set_property(TARGET org.kde.b6xfwm APPEND PROPERTY AUTOMOC_MACRO_NAMES "K_PLUGIN_FACTORY_WITH_JSON")
    install(TARGETS org.kde.b6xfwm DESTINATION ${KDE_INSTALL_QTPLUGINDIR}/org.kde.kdecoration3)
else()
    message(STATUS "KDecoration3 or KF6CoreAddons not found: the plugin is not built (the preview tool is)")
endif()
