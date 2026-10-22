#include "AppWindow.h"

#include <windows.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
    AppWindow app;

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