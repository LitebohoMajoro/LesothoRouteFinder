@echo off
setlocal
cd /d "%~dp0"
rem Keep unrelated runtimes (for example SWI-Prolog's older libwinpthread) out
rem of GCC's DLL search path. cc1plus must load the matching UCRT64 runtime.
set "PATH=C:\msys64\ucrt64\bin;%SystemRoot%\System32;%SystemRoot%"
if not exist "C:\msys64\ucrt64\bin\g++.exe" (
  echo MinGW UCRT64 compiler not found at C:\msys64\ucrt64\bin\g++.exe
  exit /b 1
)
set "OUTPUT=%~1"
if not defined OUTPUT set "OUTPUT=HashingVisualizer.exe"
"C:\msys64\ucrt64\bin\g++.exe" -std=c++17 -Wall -Wextra -pedantic -O2 -municode -mwindows -Iinclude src/main.cpp src/MainWindow.cpp src/LinkedList.cpp src/HashSetOpen.cpp src/HashSetClosed.cpp src/HashTableFileExporter.cpp -lcomctl32 -o "%OUTPUT%"
if errorlevel 1 exit /b 1
echo Built %OUTPUT%
