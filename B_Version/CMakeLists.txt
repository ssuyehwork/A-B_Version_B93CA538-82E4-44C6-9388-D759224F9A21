cmake_minimum_required(VERSION 3.16)

project(SearchTool VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent Svg)

# 源码文件路径相对于本 CMakeLists.txt 所在目录
set(SOURCES
    src/main.cpp
    src/windows/SearchAppWindow.h
    src/windows/SearchAppWindow.cpp
    src/windows/FileSearchWindow.h
    src/windows/FileSearchWindow.cpp
    src/windows/KeywordSearchWindow.h
    src/windows/KeywordSearchWindow.cpp
    src/windows/SystemTray.h
    src/windows/SystemTray.cpp
    src/windows/FramelessDialog.h
    src/windows/FramelessDialog.cpp
    src/widgets/ClickableLineEdit.h
    src/utils/StringUtils.h
    src/utils/IconHelper.h
    src/utils/SvgIcons.h
    resources/resources.qrc
)

if(WIN32)
    # 确保 rc 文件路径正确
    list(APPEND SOURCES resources/app_icon.rc)
endif()

add_executable(SearchTool ${SOURCES})

# 包含路径设置
target_include_directories(SearchTool PRIVATE
    src/windows
    src/utils
    src/widgets
    src
)

target_link_libraries(SearchTool PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::Svg
)

if(WIN32)
    target_link_libraries(SearchTool PRIVATE user32 shell32 psapi dwmapi)
    set_target_properties(SearchTool PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()