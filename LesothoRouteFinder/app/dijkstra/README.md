# Lesotho Smart Route Finder

A native Win32 desktop app that visualizes Dijkstra's shortest-path algorithm across a small Lesotho road network. It supports route selection, guided animation, execution logs, and built-in algorithm checks.

## Build

From this directory, configure and build with CMake on Windows:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

The executable is produced in the build directory. The existing `LesothoRouteFinder.exe` and `LesothoRouteFinderGUI.exe` files are retained here as previously built copies.
