@echo off
setlocal
cd /d "%~dp0.."
rem Match build.bat: keep other applications' runtime DLLs out of GCC's path.
set "PATH=C:\msys64\ucrt64\bin;%SystemRoot%\System32;%SystemRoot%"
if not exist "C:\msys64\ucrt64\bin\g++.exe" (
  echo MinGW UCRT64 compiler not found at C:\msys64\ucrt64\bin\g++.exe
  exit /b 1
)
"C:\msys64\ucrt64\bin\g++.exe" -std=c++17 -Wall -Wextra -pedantic -O2 -Iinclude tests\hash_structures_test.cpp src\LinkedList.cpp src\HashSetOpen.cpp src\HashSetClosed.cpp src\HashTableFileExporter.cpp -o tests\hash_structures_test.exe
if errorlevel 1 exit /b 1
tests\hash_structures_test.exe
if errorlevel 1 exit /b 1
echo Hash structure regression tests passed.
