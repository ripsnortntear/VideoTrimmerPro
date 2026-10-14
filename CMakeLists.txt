cmake_minimum_required(VERSION 3.22)
project(VideoTrimmer LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Widgets
    Multimedia
    MultimediaWidgets
    Network
)

set(APP_ICON_RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/app.rc")

add_executable(VideoTrimmer WIN32
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/RangeSlider.h
    src/RangeSlider.cpp
    src/ThumbnailPreview.h   # <-- Add this line
    src/TrimEngine.h
    src/TrimEngine.cpp
    src/FFmpegManager.h
    src/FFmpegManager.cpp
    src/UpdateChecker.h
    src/UpdateChecker.cpp
    ${APP_ICON_RESOURCE}
)

target_link_libraries(VideoTrimmer PRIVATE
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    Qt6::Network
)

if(MSVC)
    target_compile_options(VideoTrimmer PRIVATE /W4 /permissive- /utf-8)
endif()