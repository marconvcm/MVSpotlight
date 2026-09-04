cmake_minimum_required(VERSION 3.20)
project(mvspotlight VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Qt 6 dependencies (Wayland-native, no X11)
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
    DBus
    Network
    Concurrent
    Svg
    Test
)

# Embedded Lua 5.4 runtime
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/lua-5.4.8/src")
    file(GLOB LUA_SOURCES "3rdparty/lua-5.4.8/src/*.c")
    list(REMOVE_ITEM LUA_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/lua-5.4.8/src/lua.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/lua-5.4.8/src/luac.c"
    )
    add_library(lua54 STATIC ${LUA_SOURCES})
    target_compile_definitions(lua54 PRIVATE LUA_USE_LINUX)
    target_link_libraries(lua54 PRIVATE m dl)
    target_include_directories(lua54 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/lua-5.4.8/src")
else()
    find_package(Lua 5.4 REQUIRED)
endif()

set(PROJECT_SOURCES
    src/main.cpp
    src/core/SearchResult.cpp
    src/core/SearchResultModel.cpp
    src/core/SearchController.cpp
    src/providers/SearchProvider.cpp
    src/providers/ApplicationProvider.cpp
    src/providers/FileProvider.cpp
    src/providers/SettingsProvider.cpp
    src/providers/CalculatorProvider.cpp
    src/providers/ActionProvider.cpp
    src/providers/DeveloperCommandProvider.cpp
    src/lua/LuaPermissions.cpp
    src/lua/LuaEngine.cpp
    src/lua/LuaPlugin.cpp
    src/lua/LuaPluginManager.cpp
    src/lua/LuaSearchProvider.cpp
    src/lua/LuaApi.cpp
    src/services/DesktopEntryService.cpp
    src/services/UsageHistory.cpp
    src/services/SystemActions.cpp
    src/services/ProcessService.cpp
    src/services/ClipboardService.cpp
    src/services/NotificationService.cpp
    src/services/ThemeService.cpp
    src/services/IconImageProvider.cpp
    src/dbus/SpotlightAdaptor.cpp
    qml/qml.qrc
    assets/assets.qrc
)

add_executable(mvspotlight ${PROJECT_SOURCES})

target_include_directories(mvspotlight PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/providers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lua
    ${CMAKE_CURRENT_SOURCE_DIR}/src/services
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dbus
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/lua-5.4.8/src
)

target_link_libraries(mvspotlight PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::DBus
    Qt6::Network
    Qt6::Concurrent
    Qt6::Svg
    lua54
    m
    dl
)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(mvspotlight PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

# Installation rules
include(GNUInstallDirs)
install(TARGETS mvspotlight DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES data/mvspotlight.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES data/autostart/mvspotlight.desktop DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/xdg/autostart)
install(FILES assets/icons/mvspotlight.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
install(DIRECTORY plugins/examples/ DESTINATION ${CMAKE_INSTALL_DATADIR}/mvspotlight/plugins)

# Testing
enable_testing()
add_executable(mvspotlight_tests
    tests/test_spotlight.cpp
    src/core/SearchResult.cpp
    src/core/SearchResultModel.cpp
    src/core/SearchController.cpp
    src/providers/SearchProvider.cpp
    src/providers/ApplicationProvider.cpp
    src/providers/FileProvider.cpp
    src/providers/SettingsProvider.cpp
    src/providers/CalculatorProvider.cpp
    src/providers/ActionProvider.cpp
    src/providers/DeveloperCommandProvider.cpp
    src/lua/LuaPermissions.cpp
    src/lua/LuaEngine.cpp
    src/lua/LuaPlugin.cpp
    src/lua/LuaPluginManager.cpp
    src/lua/LuaSearchProvider.cpp
    src/lua/LuaApi.cpp
    src/services/DesktopEntryService.cpp
    src/services/UsageHistory.cpp
    src/services/SystemActions.cpp
    src/services/ProcessService.cpp
    src/services/ClipboardService.cpp
    src/services/NotificationService.cpp
    src/services/ThemeService.cpp
    src/services/IconImageProvider.cpp
    src/dbus/SpotlightAdaptor.cpp
)

target_include_directories(mvspotlight_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/providers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lua
    ${CMAKE_CURRENT_SOURCE_DIR}/src/services
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dbus
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/lua-5.4.8/src
)

target_link_libraries(mvspotlight_tests PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::DBus
    Qt6::Network
    Qt6::Concurrent
    Qt6::Svg
    Qt6::Test
    lua54
    m
    dl
)

add_test(NAME mvspotlight_tests COMMAND mvspotlight_tests)
