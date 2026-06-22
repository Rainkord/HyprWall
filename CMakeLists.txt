cmake_minimum_required(VERSION 3.20)
project(HyprWall VERSION 0.4.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Gui Core)

find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)

set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/MonitorDetector.cpp
    src/WallpaperApplier.cpp
    src/ConfigManager.cpp
    src/ServiceManager.cpp
    src/MonitorBar.cpp
    src/GalleryDelegate.cpp
)

set(HEADERS
    src/MainWindow.h
    src/MonitorDetector.h
    src/WallpaperApplier.h
    src/ConfigManager.h
    src/ServiceManager.h
    src/Types.h
    src/Strings.h
    src/ToggleSwitch.h
    src/MonitorBar.h
    src/GalleryDelegate.h
    src/GalleryConstants.h
)

add_executable(hyprwall ${SOURCES} ${HEADERS})

target_link_libraries(hyprwall
    Qt6::Widgets
    Qt6::Gui
    Qt6::Core
    ${WAYLAND_CLIENT_LIBRARIES}
)

target_include_directories(hyprwall PRIVATE
    ${WAYLAND_CLIENT_INCLUDE_DIRS}
    src/
)

include(GNUInstallDirs)
install(TARGETS hyprwall DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES assets/hyprwall.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES systemd/hyprwall.service DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/systemd/user)
install(FILES src/hyprwall.qss DESTINATION ${CMAKE_INSTALL_DATADIR}/hyprwall)
