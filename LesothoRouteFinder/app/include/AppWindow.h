#pragma once

// NEW FILE: the single main window. It draws the sidebar, the top bar with
// the Back button and the Home menu, and hosts the two question pages:
//   - HashingPage  (Question 1)
//   - the existing Dijkstra MainWindow (Question 2), embedded unchanged.

#include <windows.h>
#include <memory>
#include <vector>

#include "HashingPage.h"

class MainWindow;   // Dijkstra window from dijkstra/include/MainWindow.h

class AppWindow {
public:
    AppWindow();
    ~AppWindow();

    bool create(HINSTANCE instance, int showCommand);
    int run();

private:
    enum class Page { Home, HashOpen, HashClosed, Route };

    struct SidebarRow {
        bool isSection;           // true = small heading, false = clickable item
        const wchar_t* text;
        Page page;
        COLORREF dot;
        RECT rect;
    };

    struct HomeLayout {
        RECT title;
        RECT cardHashing;
        RECT cardRoute;
    };

    static LRESULT CALLBACK proc(HWND, UINT, WPARAM, LPARAM);
    LRESULT handle(UINT message, WPARAM wParam, LPARAM lParam);

    int px(int value) const;
    void createFonts();
    void destroyFonts();
    bool createPages(HINSTANCE instance);
    void layout();
    void buildSidebar();
    HomeLayout homeLayout() const;
    RECT contentRect() const;
    int sidebarWidth() const;
    int topBarHeight() const;

    void navigate(Page page);
    int rowAt(POINT point) const;
    bool isActive(const SidebarRow& row) const;

    void paint(HDC dc);
    void drawSidebar(HDC dc);
    void drawTopBar(HDC dc);
    void drawHome(HDC dc);
    void drawHashPreview(HDC dc, const RECT& area);
    void drawRoutePreview(HDC dc, const RECT& area);
    void drawButton(const DRAWITEMSTRUCT* item);

    HWND hwnd_ = nullptr;
    HWND routeHwnd_ = nullptr;
    HWND backButton_ = nullptr;
    HWND homeOpenButton_ = nullptr;
    HWND homeClosedButton_ = nullptr;
    HWND homeRouteButton_ = nullptr;

    HashingPage hashing_;
    std::unique_ptr<MainWindow> route_;

    std::vector<SidebarRow> rows_;
    int hoverRow_ = -1;
    bool trackingMouse_ = false;

    HFONT fontDisplay_ = nullptr;
    HFONT fontTitle_ = nullptr;
    HFONT fontHeading_ = nullptr;
    HFONT fontBody_ = nullptr;
    HFONT fontBold_ = nullptr;
    HFONT fontSmall_ = nullptr;
    HFONT fontLabel_ = nullptr;

    HINSTANCE instance_ = nullptr;
    int dpi_ = 96;
    Page page_ = Page::Home;
};