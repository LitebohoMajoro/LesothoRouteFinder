cmake_minimum_required(VERSION 3.16)

project(LesothoRouteFinder LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows GUI application (no console window).
add_executable(LesothoRouteFinder WIN32
    src/main.cpp
    src/MainWindow.cpp
    src/GraphRenderer.cpp
    src/Graph.cpp
    src/Dijkstra.cpp
    src/Tests.cpp
)

target_include_directories(
    LesothoRouteFinder PRIVATE include
)

target_link_libraries(LesothoRouteFinder PRIVATE gdi32 user32 comctl32)

# MinGW: bundle the C++ runtime so the .exe runs on any PC.
if(MINGW)
    target_link_options(LesothoRouteFinder PRIVATE -static)
endif()
