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

rem The finished application is stored here.
set "OUTDIR=C:\Users\BrainTech Holdings\Desktop\SEMESTER 7\DATA STRUCTURES\Grup Assignment\LesothoRouteFinder"
set "OUTPUT=%OUTDIR%\LesothoRouteFinder.exe"

if not exist "%OUTDIR%" mkdir "%OUTDIR%"

rem Include order matters: dijkstra\include comes first so that

rem Include order matters: Dijkstra headers first.
"C:\msys64\ucrt64\bin\g++.exe" -std=c++17 -Wall -Wextra -O2 -municode -mwindows -static ^
  -Iapp\dijkstra\include -Iapp\hashing\include -Iapp\include ^
  app\src\main.cpp app\src\AppWindow.cpp ^
  app\hashing\src\HashingPage.cpp app\hashing\src\LinkedList.cpp ^
  app\hashing\src\HashSetOpen.cpp app\hashing\src\HashSetClosed.cpp ^
  app\dijkstra\src\MainWindow.cpp app\dijkstra\src\GraphRenderer.cpp ^
  app\dijkstra\src\Graph.cpp app\dijkstra\src\Dijkstra.cpp ^
  -lgdi32 -luser32 -o "%OUTPUT%"


if errorlevel 1 (
  echo.
  echo Build FAILED. If the error says "cannot open output file", close
  echo LesothoRouteFinder.exe first and run build.bat again.
  exit /b 1
)

echo.
echo Built: %OUTPUT%