#include "AppWindow.h"
#include "MainWindow.h"      // Dijkstra window (dijkstra/include) - unchanged
#include "Widgets.h"
#include "Theme.h"

#include <algorithm>
#include <cwchar>
#include <string>
#include <vector>

namespace {

constexpr int ID_BACK = 301;
constexpr int ID_HOME_OPEN = 302;
constexpr int ID_HOME_CLOSED = 303;
constexpr int ID_HOME_ROUTE = 304;

constexpr wchar_t kShellClass[] = L"LesothoRouteFinderShell";
constexpr wchar_t kRouteClass[] = L"LesothoRouteFinderWindow";   // from dijkstra/src/MainWindow.cpp

BOOL CALLBACK findRouteWindow(HWND hwnd, LPARAM param) {
    wchar_t name[64] = {};
    GetClassNameW(hwnd, name, 64);

    if (wcscmp(name, kRouteClass) == 0) {
        *reinterpret_cast<HWND*>(param) = hwnd;
        return FALSE;   // found it, stop enumerating
    }
    return TRUE;
}

int xOf(LPARAM lParam) { return static_cast<short>(LOWORD(lParam)); }
int yOf(LPARAM lParam) { return static_cast<short>(HIWORD(lParam)); }

} // namespace

AppWindow::AppWindow() = default;

AppWindow::~AppWindow() {
    destroyFonts();
}

// ---------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------

int AppWindow::px(int value) const {
    return MulDiv(value, dpi_, 96);
}

int AppWindow::sidebarWidth() const { return px(224); }
int AppWindow::topBarHeight() const { return page_ == Page::Home ? 0 : px(56); }

RECT AppWindow::contentRect() const {
    RECT client;
    GetClientRect(hwnd_, &client);
    return {sidebarWidth(), topBarHeight(), client.right, client.bottom};
}

void AppWindow::createFonts() {
    destroyFonts();

    fontDisplay_ = widgets::makeFont(px(32), FW_BOLD,     L"Segoe UI");
    fontTitle_   = widgets::makeFont(px(26), FW_BOLD,     L"Segoe UI");
    fontHeading_ = widgets::makeFont(px(20), FW_BOLD,     L"Segoe UI");
    fontBody_    = widgets::makeFont(px(15), FW_NORMAL,   L"Segoe UI");
    fontBold_    = widgets::makeFont(px(15), FW_SEMIBOLD, L"Segoe UI");
    fontSmall_   = widgets::makeFont(px(13), FW_NORMAL,   L"Segoe UI");
    fontLabel_   = widgets::makeFont(px(12), FW_BOLD,     L"Segoe UI");
}

void AppWindow::destroyFonts() {
    HFONT* fonts[] = {
        &fontDisplay_, &fontTitle_, &fontHeading_, &fontBody_,
        &fontBold_, &fontSmall_, &fontLabel_
    };

    for (HFONT* font : fonts) {
        if (*font) {
            DeleteObject(*font);
            *font = nullptr;
        }
    }
}

bool AppWindow::create(HINSTANCE instance, int showCommand) {
    // Sharp rendering on high-DPI screens (the Dijkstra page does this too).
    SetProcessDPIAware();

    HDC screen = GetDC(nullptr);
    dpi_ = GetDeviceCaps(screen, LOGPIXELSX);
    ReleaseDC(nullptr, screen);
    if (dpi_ < 96) dpi_ = 96;

    instance_ = instance;
    createFonts();

    WNDCLASSW wc = {};
    wc.lpfnWndProc = proc;
    wc.hInstance = instance;
    wc.lpszClassName = kShellClass;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;

    if (!RegisterClassW(&wc)) return false;

    RECT work = {};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);

    const int width = (std::min)(px(1440), static_cast<int>(work.right - work.left));
    const int height = (std::min)(px(900), static_cast<int>(work.bottom - work.top));

    hwnd_ = CreateWindowExW(
        0, kShellClass,
        L"Data Structures Group Assignment — Algorithm Visualiser",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        nullptr, nullptr, instance, this
    );

    if (!hwnd_) return false;

    backButton_ = widgets::makeButton(hwnd_, instance, ID_BACK, L"←  Back");
    homeOpenButton_ = widgets::makeButton(hwnd_, instance, ID_HOME_OPEN, L"Open Hashing");
    homeClosedButton_ = widgets::makeButton(hwnd_, instance, ID_HOME_CLOSED, L"Closed Hashing");
    homeRouteButton_ = widgets::makeButton(hwnd_, instance, ID_HOME_ROUTE, L"Open Route Finder");

    if (!createPages(instance)) {
        MessageBoxW(
            hwnd_, L"Could not create the Hashing or Route Finder page.",
            L"Application Error", MB_OK | MB_ICONERROR
        );
        return false;
    }

    navigate(Page::Home);

    // Start maximised: the pages are designed for a roomy window.
    ShowWindow(
        hwnd_,
        (showCommand == SW_SHOWNORMAL || showCommand == SW_SHOWDEFAULT)
            ? SW_SHOWMAXIMIZED : showCommand
    );
    UpdateWindow(hwnd_);

    return true;
}

// Creates the hashing page and the Dijkstra page, then turns the Dijkstra
// top-level window into a child of this window (no Question 2 code changes).
bool AppWindow::createPages(HINSTANCE instance) {
    if (!hashing_.create(hwnd_, instance, dpi_)) return false;

    route_ = std::make_unique<MainWindow>();
    if (!route_->create(instance, SW_HIDE)) return false;

    EnumThreadWindows(
        GetCurrentThreadId(), findRouteWindow,
        reinterpret_cast<LPARAM>(&routeHwnd_)
    );
    if (!routeHwnd_) return false;

    LONG_PTR style = GetWindowLongPtrW(routeHwnd_, GWL_STYLE);
    style &= ~static_cast<LONG_PTR>(WS_OVERLAPPEDWINDOW | WS_POPUP);
    style |= WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    SetWindowLongPtrW(routeHwnd_, GWL_STYLE, style);

    SetParent(routeHwnd_, hwnd_);

    // Tell Windows the frame changed so no title-bar area is left behind.
    SetWindowPos(
        routeHwnd_, nullptr, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE |
        SWP_FRAMECHANGED
    );
    ShowWindow(routeHwnd_, SW_HIDE);

    return true;
}

// ---------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------

AppWindow::HomeLayout AppWindow::homeLayout() const {
    const RECT c = contentRect();
    const int margin = px(32);
    const int gap = px(24);

    HomeLayout h = {};
    h.title = {c.left + margin, c.top + px(28), c.right - margin, c.top + px(130)};

    const int top = c.top + px(150);
    const int cardWidth = (c.right - c.left - 2 * margin - gap) / 2;
    const int cardHeight = std::clamp(
        static_cast<int>(c.bottom - top - margin), px(300), px(420)
    );

    h.cardHashing = {
        c.left + margin, top, c.left + margin + cardWidth, top + cardHeight
    };
    h.cardRoute = {
        h.cardHashing.right + gap, top, h.cardHashing.right + gap + cardWidth,
        top + cardHeight
    };

    return h;
}

void AppWindow::buildSidebar() {
    rows_.clear();

    const int left = px(12);
    const int right = sidebarWidth() - px(12);
    int y = px(104);

    auto section = [&](const wchar_t* text) {
        y += px(10);
        rows_.push_back({
            true, text, Page::Home, 0,
            {left + px(8), y, right, y + px(22)}
        });
        y += px(26);
    };

    auto item = [&](const wchar_t* text, Page page, COLORREF dot) {
        rows_.push_back({
            false, text, page, dot, {left, y, right, y + px(40)}
        });
        y += px(44);
    };

    section(L"MENU");
    item(L"Home", Page::Home, ui::kAccent);

    section(L"QUESTION 1  ·  HASHING");
    item(L"Open Hashing", Page::HashOpen, ui::kViolet);
    item(L"Closed Hashing", Page::HashClosed, ui::kAmber);

    section(L"QUESTION 2  ·  DIJKSTRA");
    item(L"Route Finder", Page::Route, ui::kGreen);
}

void AppWindow::layout() {
    if (!hwnd_ || !backButton_) return;

    const RECT c = contentRect();
    const int pageWidth = c.right - c.left;
    const int pageHeight = c.bottom - c.top;

    // Back button, in the top bar.
    const int buttonHeight = px(34);
    MoveWindow(
        backButton_, c.left + px(24), (topBarHeight() - buttonHeight) / 2,
        px(96), buttonHeight, TRUE
    );

    // Pages fill everything right of the sidebar and below the top bar.
    if (hashing_.hwnd()) {
        MoveWindow(hashing_.hwnd(), c.left, c.top, pageWidth, pageHeight, TRUE);
    }
    if (routeHwnd_) {
        MoveWindow(routeHwnd_, c.left, c.top, pageWidth, pageHeight, TRUE);
    }

    // Home buttons sit at the bottom of the two cards.
    const HomeLayout home = homeLayout();
    const int pad = px(28);
    const int bh = px(42);

    const int y1 = home.cardHashing.bottom - px(24) - bh;
    MoveWindow(homeOpenButton_, home.cardHashing.left + pad, y1, px(170), bh, TRUE);
    MoveWindow(
        homeClosedButton_, home.cardHashing.left + pad + px(170) + px(10), y1,
        px(180), bh, TRUE
    );

    const int y2 = home.cardRoute.bottom - px(24) - bh;
    MoveWindow(homeRouteButton_, home.cardRoute.left + pad, y2, px(200), bh, TRUE);

    buildSidebar();
}

// ---------------------------------------------------------------------
// Navigation
// ---------------------------------------------------------------------

bool AppWindow::isActive(const SidebarRow& row) const {
    return !row.isSection && row.page == page_;
}

int AppWindow::rowAt(POINT point) const {
    for (size_t i = 0; i < rows_.size(); ++i) {
        if (!rows_[i].isSection && PtInRect(&rows_[i].rect, point)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void AppWindow::navigate(Page next) {
    const bool wasHashing =
        page_ == Page::HashOpen || page_ == Page::HashClosed;
    const bool toHashing =
        next == Page::HashOpen || next == Page::HashClosed;

    if (wasHashing && !toHashing) hashing_.onHide();

    page_ = next;

    const bool home = next == Page::Home;

    ShowWindow(backButton_, home ? SW_HIDE : SW_SHOW);
    for (HWND button : {homeOpenButton_, homeClosedButton_, homeRouteButton_}) {
        ShowWindow(button, home ? SW_SHOW : SW_HIDE);
    }

    ShowWindow(hashing_.hwnd(), toHashing ? SW_SHOW : SW_HIDE);
    if (routeHwnd_) ShowWindow(routeHwnd_, next == Page::Route ? SW_SHOW : SW_HIDE);

    if (toHashing) {
        hashing_.setMode(next == Page::HashOpen ? Mode::Open : Mode::Closed);
    }

    layout();
    InvalidateRect(hwnd_, nullptr, FALSE);

    if (toHashing) SetFocus(hashing_.hwnd());
    else if (next == Page::Route && routeHwnd_) SetFocus(routeHwnd_);
    else if (home) SetFocus(homeOpenButton_);
}

// ---------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------

void AppWindow::drawButton(const DRAWITEMSTRUCT* item) {
    using widgets::Kind;

    switch (item->CtlID) {
        case ID_BACK:
            widgets::paintButton(item, Kind::Secondary, fontBold_, px(8), ui::kWindowBg);
            break;
        case ID_HOME_OPEN:
        case ID_HOME_ROUTE:
            widgets::paintButton(item, Kind::Primary, fontBold_, px(8), ui::kCard);
            break;
        default:
            widgets::paintButton(item, Kind::Secondary, fontBold_, px(8), ui::kCard);
            break;
    }
}

void AppWindow::drawSidebar(HDC dc) {
    RECT client;
    GetClientRect(hwnd_, &client);

    const int width = sidebarWidth();
    RECT bar = {0, 0, width, client.bottom};
    ui::fillRect(dc, bar, ui::kHeaderBg);

    RECT edge = {width - 1, 0, width, client.bottom};
    ui::fillRect(dc, edge, ui::kBorder);

    // Brand block.
    RECT logo = {px(16), px(22), px(16) + px(44), px(22) + px(44)};
    ui::roundRect(dc, logo, px(10), ui::kAccent);
    ui::drawText(
        dc, L"DS", logo, ui::kOnAccent, fontHeading_,
        DT_CENTER | DT_SINGLELINE | DT_VCENTER
    );

    RECT brand = {px(70), px(22), width - px(10), px(44)};
    ui::drawText(dc, L"DATA STRUCTURES", brand, ui::kText, fontBold_);

    RECT sub = {px(70), px(44), width - px(10), px(66)};
    ui::drawText(dc, L"Group Assignment 2026", sub, ui::kTextDim, fontSmall_);

    RECT divider = {px(16), px(86), width - px(16), px(87)};
    ui::fillRect(dc, divider, ui::kBorder);

    // Navigation rows.
    for (size_t i = 0; i < rows_.size(); ++i) {
        const SidebarRow& row = rows_[i];

        if (row.isSection) {
            ui::drawText(dc, row.text, row.rect, ui::kTextMuted, fontLabel_);
            continue;
        }

        const bool active = isActive(row);
        const bool hover = static_cast<int>(i) == hoverRow_;

        if (active || hover) {
            ui::roundRect(
                dc, row.rect, px(8),
                active ? ui::kCardAlt : RGB(20, 29, 47)
            );
        }

        if (active) {
            RECT mark = {
                row.rect.left, row.rect.top + px(9),
                row.rect.left + px(3), row.rect.bottom - px(9)
            };
            ui::fillRect(dc, mark, ui::kAccent);
        }

        ui::fillCircle(
            dc, row.rect.left + px(20), (row.rect.top + row.rect.bottom) / 2,
            px(4), row.dot
        );

        RECT label = {
            row.rect.left + px(38), row.rect.top, row.rect.right - px(8),
            row.rect.bottom
        };
        ui::drawText(
            dc, row.text, label,
            active ? ui::kText : (hover ? ui::kText : ui::kTextDim),
            active ? fontBold_ : fontBody_
        );
    }

    // Footer.
    RECT course = {px(20), client.bottom - px(70), width - px(16), client.bottom - px(50)};
    ui::drawText(dc, L"BISA4110", course, ui::kTextMuted, fontLabel_);

    RECT name = {px(20), client.bottom - px(50), width - px(16), client.bottom - px(14)};
    ui::drawText(
        dc, L"Data Structures and Algorithm Analysis", name,
        ui::kTextMuted, fontSmall_, DT_LEFT | DT_WORDBREAK
    );
}

void AppWindow::drawTopBar(HDC dc) {
    if (page_ == Page::Home) return;

    RECT client;
    GetClientRect(hwnd_, &client);

    RECT bar = {sidebarWidth(), 0, client.right, topBarHeight()};
    ui::fillRect(dc, bar, ui::kWindowBg);

    RECT line = {sidebarWidth(), topBarHeight() - 1, client.right, topBarHeight()};
    ui::fillRect(dc, line, ui::kBorder);

}

void AppWindow::drawHashPreview(HDC dc, const RECT& area) {
    const wchar_t* values[8] = {L"", L"21", L"", L"14", L"25", L"", L"38", L""};
    const int gap = px(8);
    const int areaWidth = area.right - area.left;
    const int cellWidth = (std::min)(px(64), (areaWidth - 7 * gap) / 8);
    const int cellHeight = (std::min)(px(54), static_cast<int>(area.bottom - area.top));
    const int top = area.top + (area.bottom - area.top - cellHeight) / 2;

    for (int i = 0; i < 8; ++i) {
        RECT cell = {
            area.left + i * (cellWidth + gap), top,
            area.left + i * (cellWidth + gap) + cellWidth, top + cellHeight
        };

        COLORREF fill = ui::kCardAlt;
        COLORREF border = ui::kBorder;
        if (i == 3) { fill = RGB(10, 54, 70); border = ui::kAccent; }
        else if (i == 4) { fill = RGB(58, 44, 12); border = ui::kAmber; }
        else if (values[i][0]) border = widgets::darken(ui::kViolet, 70);

        ui::roundRect(dc, cell, px(8), fill, border, i == 3 || i == 4 ? 2 : 1);

        ui::drawText(
            dc, values[i][0] ? values[i] : L"—", cell,
            values[i][0] ? ui::kText : ui::kTextMuted, fontBold_,
            DT_CENTER | DT_SINGLELINE | DT_VCENTER
        );
    }
}

void AppWindow::drawRoutePreview(HDC dc, const RECT& area) {
    const int w = area.right - area.left;
    const int h = area.bottom - area.top;
    const int r = (std::min)(px(15), h / 5);

    struct Node { wchar_t letter; int x; int y; COLORREF fill; COLORREF text; };
    const Node nodes[4] = {
        {L'A', area.left + w * 12 / 100, area.top + h / 2, ui::kAccent, ui::kOnAccent},
        {L'B', area.left + w * 48 / 100, area.top + h * 18 / 100, ui::kIdleNode, ui::kText},
        {L'C', area.left + w * 48 / 100, area.top + h * 82 / 100, ui::kGreen, ui::kOnAccent},
        {L'D', area.left + w * 84 / 100, area.top + h / 2, ui::kGreen, ui::kOnAccent}
    };

    struct Road { int from; int to; const wchar_t* km; bool route; };
    const Road roads[4] = {
        {0, 1, L"3", false}, {0, 2, L"1", true},
        {1, 3, L"2", false}, {2, 3, L"4", true}
    };

    for (const Road& road : roads) {
        const Node& a = nodes[road.from];
        const Node& b = nodes[road.to];

        HPEN pen = CreatePen(
            PS_SOLID, road.route ? px(4) : px(2),
            road.route ? ui::kGreen : ui::kIdleEdge
        );
        HGDIOBJ old = SelectObject(dc, pen);
        MoveToEx(dc, a.x, a.y, nullptr);
        LineTo(dc, b.x, b.y);
        SelectObject(dc, old);
        DeleteObject(pen);

        const int mx = (a.x + b.x) / 2;
        const int my = (a.y + b.y) / 2;
        RECT pill = {mx - px(14), my - px(10), mx + px(14), my + px(10)};
        ui::roundRect(dc, pill, px(10), ui::kCardAlt, road.route ? ui::kGreen : ui::kIdleEdge, 1);
        ui::drawText(
            dc, road.km, pill, road.route ? ui::kGreen : ui::kTextDim, fontSmall_,
            DT_CENTER | DT_SINGLELINE | DT_VCENTER
        );
    }

    for (const Node& node : nodes) {
        ui::fillCircle(dc, node.x, node.y, r, node.fill, ui::kCard, px(2));

        RECT box = {node.x - r, node.y - r, node.x + r, node.y + r};
        ui::drawText(
            dc, std::wstring(1, node.letter), box, node.text, fontBold_,
            DT_CENTER | DT_SINGLELINE | DT_VCENTER
        );
    }
}

void AppWindow::drawHome(HDC dc) {
    const HomeLayout home = homeLayout();

    // Page title.
    const std::wstring first = L"ALGORITHM ";
    const std::wstring second = L"VISUALISER";

    RECT titleRect = {home.title.left, home.title.top, home.title.right, home.title.top + px(44)};
    ui::drawText(dc, first, titleRect, ui::kText, fontDisplay_);

    titleRect.left += ui::textWidth(dc, fontDisplay_, first);
    ui::drawText(dc, second, titleRect, ui::kAccent, fontDisplay_);

    RECT subtitle = {home.title.left, home.title.top + px(48), home.title.right, home.title.top + px(72)};
    ui::drawText(
        dc,
        L"BISA4110  ·  Data Structures and Algorithm Analysis  ·  Group Assignment 2026",
        subtitle, ui::kTextDim, fontBody_
    );

    RECT lead = {home.title.left, home.title.top + px(74), home.title.right, home.title.top + px(100)};
    ui::drawText(
        dc, L"Choose a question to explore. You can switch at any time from the sidebar.",
        lead, ui::kTextMuted, fontBody_
    );

    struct CardInfo {
        RECT rect;
        const wchar_t* label;
        const wchar_t* title;
        const wchar_t* description;
        bool hashing;
    };

    const CardInfo cards[2] = {
        {home.cardHashing, L"QUESTION 1  ·  50 MARKS", L"Hashing",
         L"Insert, search and remove values step by step. Compare open hashing, "
         L"where collisions share a linked-list bucket, with closed hashing, "
         L"which uses linear probing and rehashes above a 0.5 load factor.",
         true},
        {home.cardRoute, L"QUESTION 2  ·  50 MARKS", L"Dijkstra's Algorithm",
         L"Find the shortest delivery route from Maseru (A) to every town. "
         L"Watch the priority queue visit towns, relax roads and update "
         L"distances one step at a time.",
         false}
    };

    for (const CardInfo& card : cards) {
        const RECT& c = card.rect;
        ui::roundRect(dc, c, px(14), ui::kCard, ui::kBorder, 1);

        const int pad = px(28);
        const int innerWidth = c.right - c.left - 2 * pad;

        RECT label = {c.left + pad, c.top + px(24), c.right - pad, c.top + px(44)};
        ui::drawText(dc, card.label, label, ui::kAccent, fontLabel_);

        RECT title = {c.left + pad, c.top + px(48), c.right - pad, c.top + px(88)};
        ui::drawText(dc, card.title, title, ui::kText, fontTitle_);

        const int descTop = c.top + px(98);
        const int descHeight = ui::wrappedHeight(dc, fontBody_, card.description, innerWidth);

        RECT desc = {c.left + pad, descTop, c.right - pad, descTop + descHeight};
        ui::drawText(
            dc, card.description, desc, ui::kTextDim, fontBody_,
            DT_LEFT | DT_WORDBREAK
        );

        const int buttonsTop = c.bottom - px(24) - px(42);
        RECT preview = {
            c.left + pad, descTop + descHeight + px(20),
            c.right - pad, buttonsTop - px(20)
        };

        if (preview.bottom - preview.top >= px(70)) {
            if (card.hashing) drawHashPreview(dc, preview);
            else drawRoutePreview(dc, preview);
        }
    }
}

void AppWindow::paint(HDC dc) {
    RECT client;
    GetClientRect(hwnd_, &client);
    ui::fillRect(dc, client, ui::kWindowBg);

    drawSidebar(dc);
    drawTopBar(dc);

    if (page_ == Page::Home) drawHome(dc);
}

// ---------------------------------------------------------------------
// Message loop and window procedure
// ---------------------------------------------------------------------

int AppWindow::run() {
    MSG message = {};

    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        // Tab / Shift+Tab / Space / Enter inside the active page.
        HWND active = nullptr;
        if (page_ == Page::Route) active = routeHwnd_;
        else if (page_ == Page::HashOpen || page_ == Page::HashClosed) {
            active = hashing_.hwnd();
        }

        if (active && IsDialogMessageW(active, &message)) continue;

        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return static_cast<int>(message.wParam);
}

LRESULT CALLBACK AppWindow::proc(
    HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam
) {
    AppWindow* self = nullptr;

    if (message == WM_NCCREATE) {
        self = static_cast<AppWindow*>(
            reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams
        );
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->hwnd_ = hwnd;
    } else {
        self = reinterpret_cast<AppWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (self) return self->handle(message, wParam, lParam);

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT AppWindow::handle(UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_GETMINMAXINFO: {
            auto* info = reinterpret_cast<MINMAXINFO*>(lParam);
            info->ptMinTrackSize.x = px(1290);
            info->ptMinTrackSize.y = px(830);
            return 0;
        }

        case WM_SIZE:
            layout();
            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;

        case WM_COMMAND: {
            if (HIWORD(wParam) != BN_CLICKED) return 0;

            switch (LOWORD(wParam)) {
                case ID_BACK:         navigate(Page::Home);       break;
                case ID_HOME_OPEN:    navigate(Page::HashOpen);   break;
                case ID_HOME_CLOSED:  navigate(Page::HashClosed); break;
                case ID_HOME_ROUTE:   navigate(Page::Route);      break;
                default: break;
            }
            return 0;
        }

        case WM_DRAWITEM: {
            auto* item = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
            if (item->CtlType == ODT_BUTTON) {
                drawButton(item);
                return TRUE;
            }
            break;
        }

        case WM_MOUSEMOVE: {
            POINT point = {xOf(lParam), yOf(lParam)};
            const int row = rowAt(point);

            if (!trackingMouse_) {
                TRACKMOUSEEVENT track = {};
                track.cbSize = sizeof(track);
                track.dwFlags = TME_LEAVE;
                track.hwndTrack = hwnd_;
                TrackMouseEvent(&track);
                trackingMouse_ = true;
            }

            if (row != hoverRow_) {
                hoverRow_ = row;
                RECT bar = {0, 0, sidebarWidth(), 4000};
                InvalidateRect(hwnd_, &bar, FALSE);
            }
            return 0;
        }

        case WM_MOUSELEAVE: {
            trackingMouse_ = false;
            if (hoverRow_ != -1) {
                hoverRow_ = -1;
                RECT bar = {0, 0, sidebarWidth(), 4000};
                InvalidateRect(hwnd_, &bar, FALSE);
            }
            return 0;
        }

        case WM_SETCURSOR:
            if (reinterpret_cast<HWND>(wParam) == hwnd_ &&
                LOWORD(lParam) == HTCLIENT) {
                POINT point;
                GetCursorPos(&point);
                ScreenToClient(hwnd_, &point);

                if (rowAt(point) >= 0) {
                    SetCursor(LoadCursor(nullptr, IDC_HAND));
                    return TRUE;
                }
            }
            break;

        case WM_LBUTTONDOWN: {
            POINT point = {xOf(lParam), yOf(lParam)};
            const int row = rowAt(point);

            if (row >= 0) navigate(rows_[static_cast<size_t>(row)].page);
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC dc = BeginPaint(hwnd_, &ps);

            RECT client;
            GetClientRect(hwnd_, &client);

            if (client.right > 0 && client.bottom > 0) {
                HDC mem = CreateCompatibleDC(dc);
                HBITMAP bitmap = CreateCompatibleBitmap(dc, client.right, client.bottom);
                HGDIOBJ oldBitmap = SelectObject(mem, bitmap);

                paint(mem);

                BitBlt(dc, 0, 0, client.right, client.bottom, mem, 0, 0, SRCCOPY);

                SelectObject(mem, oldBitmap);
                DeleteObject(bitmap);
                DeleteDC(mem);
            }

            EndPaint(hwnd_, &ps);
            return 0;
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcW(hwnd_, message, wParam, lParam);
}