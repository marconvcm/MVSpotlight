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
    src/providers/AiProvider.cpp
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
    src/services/ConfigService.cpp
    src/services/AiService.cpp
    src/services/MarkdownRenderer.cpp
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
install(FILES data/mvspotlight.desktop data/mvspotlight-preferences.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES data/autostart/mvspotlight.desktop DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/xdg/autostart)
install(FILES assets/icons/mvspotlight.svg assets/icons/spotlight-qt.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
install(DIRECTORY plugins/examples/ DESTINATION ${CMAKE_INSTALL_DATADIR}/mvspotlight/plugins)
install(FILES data/systemd/mvspotlight.service DESTINATION lib/systemd/user)
install(FILES LICENSE README.md CHANGELOG.md DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/mvspotlight)

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
    src/providers/AiProvider.cpp
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
    src/services/ConfigService.cpp
    src/services/AiService.cpp
    src/services/MarkdownRenderer.cpp
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
set_tests_properties(mvspotlight_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# ------------------------------------------------------------------------------
# Screenshot Capture Tool (Headless offscreen virtual capture)
# ------------------------------------------------------------------------------
set(CAPTURE_SOURCES
    tools/capture_screenshots.cpp
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
    src/providers/AiProvider.cpp
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
    src/services/ConfigService.cpp
    src/services/AiService.cpp
    src/services/MarkdownRenderer.cpp
    src/services/IconImageProvider.cpp
    src/dbus/SpotlightAdaptor.cpp
    qml/qml.qrc
    assets/assets.qrc
)

add_executable(mvspotlight_capture ${CAPTURE_SOURCES})
target_include_directories(mvspotlight_capture PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/providers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lua
    ${CMAKE_CURRENT_SOURCE_DIR}/src/services
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dbus
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/lua-5.4.8/src
)
target_link_libraries(mvspotlight_capture PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2
    Qt6::DBus Qt6::Network Qt6::Concurrent Qt6::Svg lua54 m dl
)

# ------------------------------------------------------------------------------
# CPack Packaging Configuration (RPM, TGZ)
# ------------------------------------------------------------------------------
set(CPACK_PACKAGE_NAME "mvspotlight")
set(CPACK_PACKAGE_VENDOR "MVSpotlight")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "macOS Spotlight-inspired launcher for GNOME 50 on Wayland")
set(CPACK_PACKAGE_DESCRIPTION "A polished desktop launcher and search application for GNOME 50 on Linux, \
inspired by the visual refinement of macOS Spotlight, built with Qt 6, Qt Quick, Wayland-native \
architecture, and an extensible Lua plugin system.")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/marconvcm/MVSpotlight")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_CONTACT "MVSpotlight Contributors <marconvm@users.noreply.github.com>")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_RELOCATABLE FALSE)

# Default generators
set(CPACK_GENERATOR "RPM;TGZ")

# RPM Generator Settings
set(CPACK_RPM_PACKAGE_NAME "mvspotlight")
set(CPACK_RPM_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_RPM_PACKAGE_RELEASE "1.fc44")
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_GROUP "Applications/System")
set(CPACK_RPM_PACKAGE_URL "https://github.com/marconvcm/MVSpotlight")
set(CPACK_RPM_PACKAGE_AUTOREQ "YES")
set(CPACK_RPM_PACKAGE_AUTOPROV "YES")
set(CPACK_RPM_PACKAGE_RELOCATABLE FALSE)
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
set(CPACK_RPM_PACKAGE_DESCRIPTION "${CPACK_PACKAGE_DESCRIPTION}")
set(CPACK_RPM_PACKAGE_SUGGESTS "gh")

# Exclude standard filesystem directories owned by base Fedora packages
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
    /usr/bin
    /usr/lib
    /usr/lib/systemd
    /usr/lib/systemd/user
    /usr/share
    /usr/share/applications
    /usr/share/icons
    /usr/share/icons/hicolor
    /usr/share/icons/hicolor/scalable
    /usr/share/icons/hicolor/scalable/apps
    /etc
    /etc/xdg
    /etc/xdg/autostart
)

# Post install and post uninstall scripts for icon cache updates
set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/packaging/rpm/post_install.sh")
set(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/packaging/rpm/post_uninstall.sh")

# Debian Generator Settings
set(CPACK_DEBIAN_PACKAGE_NAME "mvspotlight")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Marcos Vinicius <marconvcm@gmail.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/marconvcm/MVSpotlight")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt6core6, libqt6gui6, libqt6qml6, libqt6quick6, libqt6quickcontrols2-6, libqt6svg6, libqt6dbus6, qml6-module-qtquick, qml6-module-qtquick-controls2, qml6-module-qtquick-layouts, hicolor-icon-theme")
set(CPACK_DEBIAN_PACKAGE_SUGGESTS "gh")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    "${CMAKE_CURRENT_SOURCE_DIR}/packaging/debian/postinst;${CMAKE_CURRENT_SOURCE_DIR}/packaging/debian/postrm"
)

include(CPack)

