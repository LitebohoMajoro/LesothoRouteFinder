
#include "MainWindow.h"
#include <windows.h>

int WINAPI WinMain(
    HINSTANCE instance,
    HINSTANCE,
    LPSTR,
    int showCommand
) {
    MainWindow app;

    if (!app.create(instance, showCommand)) {
        MessageBoxW(
            nullptr,
            L"Failed to create the application window.",
            L"Application Error",
            MB_OK | MB_ICONERROR
        );

        return 1;
    }

    return app.run();
}

