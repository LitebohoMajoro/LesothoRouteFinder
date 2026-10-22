#include "MainWindow.h"
#include "GraphRenderer.h"
#include "Theme.h"

#include <algorithm>
#include <climits>
#include <cwctype>
#include <string>
#include <vector>

namespace {

constexpr int ID_FIND = 104;
constexpr int ID_PLAY = 105;
constexpr int ID_NEXT = 106;
constexpr int ID_RESET = 107;
constexpr int ID_LOG = 109;
constexpr int ID_SRC_BASE = 110;    // 110..113: source chips (A..D)
constexpr int ID_DEST_BASE = 120;   // 120..123: destination chips (A..D)

constexpr int TOWN_COUNT = 4;
constexpr char kTowns[TOWN_COUNT] = {'A', 'B', 'C', 'D'};

constexpr UINT ANIMATION_INTERVAL_MS = 850;

std::wstring toWide(const std::string& text) {
    return ui::widen(text);
}

std::wstring townName(const Graph& graph, char id) {
    return toWide(graph.nameOf(id)) +
        L" (" + std::wstring(1, wchar_t(id)) + L")";
}

std::wstring pathText(
    const Graph& graph,
    const std::vector<char>& path
) {
    std::wstring text;

    for (size_t i = 0; i < path.size(); ++i) {
        if (i > 0) text += L"  →  ";
        text += toWide(graph.nameOf(path[i]));
    }

    return text;
}

std::wstring twoDigits(size_t value) {
    std::wstring text = std::to_wstring(value);
    if (text.size() < 2) text = L"0" + text;
    return text;
}

bool contains(const std::vector<char>& list, char id) {
    return std::find(list.begin(), list.end(), id) != list.end();
}

COLORREF lighten(COLORREF c, int amount) {
    return RGB(
        (std::min)(255, GetRValue(c) + amount),
        (std::min)(255, GetGValue(c) + amount),
        (std::min)(255, GetBValue(c) + amount)
    );
}

COLORREF darken(COLORREF c, int amount) {
    return RGB(
        (std::max)(0, GetRValue(c) - amount),
        (std::max)(0, GetGValue(c) - amount),
        (std::max)(0, GetBValue(c) - amount)
    );
}

// Subclass for the owner-drawn buttons: tracks mouse hover so the
// button can repaint with a hover colour, and shows a hand cursor.
LRESULT CALLBACK buttonSubclass(
    HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam
) {
    auto original = reinterpret_cast<WNDPROC>(
        GetPropW(hwnd, L"rf.orig")
    );

    switch (message) {
        case WM_MOUSEMOVE:
            if (!GetPropW(hwnd, L"rf.hover")) {
                SetPropW(hwnd, L"rf.hover", reinterpret_cast<HANDLE>(1));

                TRACKMOUSEEVENT track = {};
                track.cbSize = sizeof(track);
                track.dwFlags = TME_LEAVE;
                track.hwndTrack = hwnd;
                TrackMouseEvent(&track);

                InvalidateRect(hwnd, nullptr, FALSE);
            }
            break;

        case WM_MOUSELEAVE:
            RemovePropW(hwnd, L"rf.hover");
            InvalidateRect(hwnd, nullptr, FALSE);
            break;

        case WM_SETCURSOR:
            SetCursor(LoadCursor(nullptr, IDC_HAND));
            return TRUE;

        case WM_ERASEBKGND:
            return 1;

        case WM_NCDESTROY:
            SetWindowLongPtrW(
                hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(original)
            );
            RemovePropW(hwnd, L"rf.orig");
            RemovePropW(hwnd, L"rf.hover");
            break;
    }

    return CallWindowProcW(original, hwnd, message, wParam, lParam);
}

HWND makeButton(
    HWND parent,
    HINSTANCE instance,
    int id,
    const wchar_t* title
) {
    HWND button = CreateWindowExW(
        0, L"BUTTON", title,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        0, 0, 100, 34,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        instance,
        nullptr
    );

    WNDPROC original = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(
            button, GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(buttonSubclass)
        )
    );
    SetPropW(button, L"rf.orig", reinterpret_cast<HANDLE>(original));

    return button;
}

HFONT makeFont(int pixelHeight, int weight, const wchar_t* face) {
    return CreateFontW(
        -pixelHeight, 0, 0, 0, weight,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, face
    );
}

} // namespace

MainWindow::~MainWindow() {
    destroyFonts();
}

// ---------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------

int MainWindow::px(int value) const {
    return MulDiv(value, dpi_, 96);
}

void MainWindow::createFonts() {
    destroyFonts();

    fontTitle_   = makeFont(px(24), FW_BOLD,     L"Segoe UI");
    fontHeading_ = makeFont(px(20), FW_BOLD,     L"Segoe UI");
    fontBody_    = makeFont(px(15), FW_NORMAL,   L"Segoe UI");
    fontBold_    = makeFont(px(15), FW_SEMIBOLD, L"Segoe UI");
    fontSmall_   = makeFont(px(13), FW_NORMAL,   L"Segoe UI");
    fontLabel_   = makeFont(px(12), FW_BOLD,     L"Segoe UI");
    fontBig_     = makeFont(px(46), FW_BOLD,     L"Segoe UI");
    fontMono_    = makeFont(px(14), FW_NORMAL,   L"Consolas");

    logBrush_ = CreateSolidBrush(ui::kLogBg);
}

void MainWindow::destroyFonts() {
    HFONT* fonts[] = {
        &fontTitle_, &fontHeading_, &fontBody_, &fontBold_,
        &fontSmall_, &fontLabel_, &fontBig_, &fontMono_
    };

    for (HFONT* font : fonts) {
        if (*font) {
            DeleteObject(*font);
            *font = nullptr;
        }
    }

    if (logBrush_) {
        DeleteObject(logBrush_);
        logBrush_ = nullptr;
    }
}

bool MainWindow::create(
    HINSTANCE instance,
    int showCommand
) {
    // Render sharply on high-DPI screens instead of being bitmap-scaled.
    SetProcessDPIAware();

    HDC screen = GetDC(nullptr);
    dpi_ = GetDeviceCaps(screen, LOGPIXELSX);
    ReleaseDC(nullptr, screen);
    if (dpi_ < 96) dpi_ = 96;

    const wchar_t CLASS_NAME[] =
        L"LesothoRouteFinderWindow";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;   // we paint everything ourselves

    if (!RegisterClassW(&wc)) {
        return false;
    }

    result_ = dijkstra(graph_, source_);
    clearProgress();
    createFonts();

    const int width = (std::min)(
        px(1280), static_cast<int>(GetSystemMetrics(SM_CXSCREEN)) - 40
    );
    const int height = (std::min)(
        px(860), static_cast<int>(GetSystemMetrics(SM_CYSCREEN)) - 80
    );

    hwnd_ = CreateWindowExW(
        0, CLASS_NAME,
        L"Lesotho Smart Route Finder - Dijkstra Visualizer",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        nullptr, nullptr,
        instance, this
    );

    if (!hwnd_) return false;

    createControls(instance);
    layoutControls();

    ShowWindow(hwnd_, showCommand);
    UpdateWindow(hwnd_);

    return true;
}

void MainWindow::createControls(HINSTANCE instance) {
    for (int i = 0; i < TOWN_COUNT; ++i) {
        const std::wstring name = toWide(graph_.nameOf(kTowns[i]));

        sourceButtons_[i] =
            makeButton(hwnd_, instance, ID_SRC_BASE + i, name.c_str());
        destinationButtons_[i] =
            makeButton(hwnd_, instance, ID_DEST_BASE + i, name.c_str());
    }

    findButton_  = makeButton(hwnd_, instance, ID_FIND,  L"Find Route");
    playButton_  = makeButton(hwnd_, instance, ID_PLAY,  L"Play");
    nextButton_  = makeButton(hwnd_, instance, ID_NEXT,  L"Next Step");
    resetButton_ = makeButton(hwnd_, instance, ID_RESET, L"Reset");

    logBox_ = CreateWindowExW(
        0,
        L"EDIT", L"Ready. Choose a source and destination, then click Find Route.",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP |
        ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
        0, 0, 500, 180,
        hwnd_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_LOG)),
        instance,
        nullptr
    );

    SendMessageW(
        logBox_, WM_SETFONT,
        reinterpret_cast<WPARAM>(fontMono_), TRUE
    );

    // Dark scrollbar for the log (silently skipped on old Windows).
    using SetWindowThemeFn = HRESULT (WINAPI*)(HWND, LPCWSTR, LPCWSTR);
    if (HMODULE uxtheme = LoadLibraryW(L"uxtheme.dll")) {
        auto setTheme = reinterpret_cast<SetWindowThemeFn>(
            reinterpret_cast<void*>(
                GetProcAddress(uxtheme, "SetWindowTheme")
            )
        );
        if (setTheme) setTheme(logBox_, L"DarkMode_Explorer", nullptr);
    }
}

MainWindow::Layout MainWindow::computeLayout() const {
    RECT client;
    GetClientRect(hwnd_, &client);

    const int width = client.right;
    const int height = client.bottom;
    const int margin = px(24);
    const int gap = px(16);

    Layout layout = {};
    layout.headerHeight = px(96);
    layout.rowOneY = px(12);
    layout.rowTwoY = px(56);
    layout.chipWidth = px(78);
    layout.chipHeight = px(30);
    layout.chipGap = px(6);

    const int chipsWidth =
        TOWN_COUNT * layout.chipWidth + (TOWN_COUNT - 1) * layout.chipGap;

    layout.fromCaptionX = margin;
    layout.fromChipsX = layout.fromCaptionX + px(52);
    layout.toCaptionX = layout.fromChipsX + chipsWidth + px(28);
    layout.toChipsX = layout.toCaptionX + px(34);

    const int logHeight = std::clamp(
        height * 26 / 100, px(170), px(260)
    );

    layout.logCard = {
        margin, height - margin - logHeight,
        width - margin, height - margin
    };

    const int top = layout.headerHeight + gap;
    const int bottom = layout.logCard.top - gap;

    const int resultsWidth = std::clamp(
        width * 28 / 100, px(310), px(380)
    );

    layout.graphCard = {
        margin, top, width - margin - resultsWidth - gap, bottom
    };

    layout.resultsCard = {
        width - margin - resultsWidth, top, width - margin, bottom
    };

    layout.graphCanvas = {
        layout.graphCard.left + px(12),
        layout.graphCard.top + px(44),
        layout.graphCard.right - px(12),
        layout.graphCard.bottom - px(12)
    };

    layout.logBackground = {
        layout.logCard.left + px(14),
        layout.logCard.top + px(48),
        layout.logCard.right - px(14),
        layout.logCard.bottom - px(14)
    };

    layout.logText = {
        layout.logBackground.left + px(10),
        layout.logBackground.top + px(8),
        layout.logBackground.right - px(6),
        layout.logBackground.bottom - px(8)
    };

    return layout;
}

void MainWindow::layoutControls() {
    if (!hwnd_ || !logBox_) return;

    const Layout layout = computeLayout();
    // Source and destination chips (second header row).
    for (int i = 0; i < TOWN_COUNT; ++i) {
        const int offset = i * (layout.chipWidth + layout.chipGap);

        MoveWindow(
            sourceButtons_[i], layout.fromChipsX + offset, layout.rowTwoY,
            layout.chipWidth, layout.chipHeight, TRUE
        );
        MoveWindow(
            destinationButtons_[i], layout.toChipsX + offset, layout.rowTwoY,
            layout.chipWidth, layout.chipHeight, TRUE
        );
    }

    // Action buttons, right-aligned on the title row.
    RECT client;
    GetClientRect(hwnd_, &client);

    struct Action { HWND button; int width; };
    const Action actions[] = {
        {resetButton_, px(84)},
        {nextButton_,  px(104)},
        {playButton_,  px(92)},
        {findButton_,  px(116)}
    };

    int right = client.right - px(24);
    for (const Action& action : actions) {
        right -= action.width;
        MoveWindow(
            action.button, right, layout.rowOneY,
            action.width, px(36), TRUE
        );
        right -= px(8);
    }

    MoveWindow(
        logBox_,
        layout.logText.left, layout.logText.top,
        (std::max)(100L, layout.logText.right - layout.logText.left),
        (std::max)(40L, layout.logText.bottom - layout.logText.top),
        TRUE
    );
}

// ---------------------------------------------------------------------
// State helpers
// ---------------------------------------------------------------------

void MainWindow::clearProgress() {
    visited_.clear();
    relaxed_.clear();
    current_ = 0;
    currentStep_ = 0;

    tentative_.clear();
    for (const auto& [id, name] : graph_.towns()) {
        tentative_[id] = INT_MAX;
    }
    tentative_[source_] = 0;
}

char MainWindow::townFromName(const std::string& name) const {
    for (const auto& [id, townName] : graph_.towns()) {
        if (townName == name) return id;
    }
    return 0;
}

bool MainWindow::isFinished() const {
    return animationStarted_ && currentStep_ >= result_.steps.size();
}

// Turns one textual Dijkstra step into visual state (current town,
// distance labels, roads that were checked).
void MainWindow::applyStep(const std::string& step) {
    const std::string visit = "Visit ";
    const std::string update = "  Update ";
    const std::string equal = "  Equal-cost route to ";

    if (step.rfind(visit, 0) == 0) {
        size_t open = step.rfind(" (");

        if (open != std::string::npos && open >= visit.size()) {
            char id = townFromName(
                step.substr(visit.size(), open - visit.size())
            );

            if (id) {
                current_ = id;
                if (!contains(visited_, id)) visited_.push_back(id);
            }
        }
    }
    else if (step.rfind(update, 0) == 0) {
        size_t via = step.find(" via ");
        size_t arrow = via == std::string::npos
            ? std::string::npos : step.find(" -> ", via);
        size_t km = step.rfind(" km");

        if (via != std::string::npos &&
            arrow != std::string::npos &&
            km != std::string::npos && km > arrow + 4) {

            char to = townFromName(
                step.substr(update.size(), via - update.size())
            );
            char from = townFromName(
                step.substr(via + 5, arrow - (via + 5))
            );

            try {
                int value = std::stoi(
                    step.substr(arrow + 4, km - (arrow + 4))
                );
                if (to) tentative_[to] = value;
            } catch (...) {
            }

            if (to && from) {
                // A shorter route replaces the earlier one into `to`.
                relaxed_.erase(
                    std::remove_if(
                        relaxed_.begin(), relaxed_.end(),
                        [to](const std::pair<char, char>& e) {
                            return e.second == to;
                        }
                    ),
                    relaxed_.end()
                );
                relaxed_.push_back({from, to});
            }
        }
    }
    else if (step.rfind(equal, 0) == 0) {
        size_t via = step.find(" via ");
        size_t end = step.rfind('.');

        if (via != std::string::npos &&
            end != std::string::npos && end > via + 5) {

            char to = townFromName(
                step.substr(equal.size(), via - equal.size())
            );
            char from = townFromName(
                step.substr(via + 5, end - (via + 5))
            );

            if (to && from) relaxed_.push_back({from, to});
        }
    }
}

// Re-colours buttons whose label/state changed and repaints everything.
void MainWindow::refresh() {
    if (!hwnd_) return;

    const wchar_t* playLabel = animating_
        ? L"Pause"
        : (isFinished() ? L"Replay" : L"Play");
    SetWindowTextW(playButton_, playLabel);

    for (HWND button : {findButton_, playButton_, nextButton_, resetButton_}) {
        InvalidateRect(button, nullptr, FALSE);
    }

    for (int i = 0; i < TOWN_COUNT; ++i) {
        InvalidateRect(sourceButtons_[i], nullptr, FALSE);
        InvalidateRect(destinationButtons_[i], nullptr, FALSE);
    }

    InvalidateRect(hwnd_, nullptr, FALSE);
}

// ---------------------------------------------------------------------
// Actions
// ---------------------------------------------------------------------

void MainWindow::findRoute() {
    pauseAnimation();

    result_ = dijkstra(graph_, source_);

    if (!result_.valid) {
        MessageBoxW(
            hwnd_,
            L"Invalid graph: negative road weight detected.",
            L"Dijkstra Error",
            MB_OK | MB_ICONERROR
        );
        refresh();
        return;
    }

    routes_ = allShortestPaths(
        graph_, source_, destination_, result_
    );

    // Prefer the Leribe route when TY is selected.
    if (destination_ == 'D') {
        auto preferred = std::find(
            routes_.begin(),
            routes_.end(),
            std::vector<char>({'A', 'C', 'D'})
        );

        if (preferred != routes_.end()) {
            std::iter_swap(routes_.begin(), preferred);
        }
    }

    routeCalculated_ = true;
    animationStarted_ = false;
    clearProgress();

    SetWindowTextW(
        logBox_,
        L"Route calculated successfully.\r\n"
        L"Click Play or Next Step "
        L"to watch Dijkstra's algorithm run."
    );

    refresh();
}

// Picking the town that is already the other end swaps the two.
void MainWindow::selectSource(char town) {
    if (town == destination_) destination_ = source_;
    source_ = town;
    applySelection();
}

void MainWindow::selectDestination(char town) {
    if (town == source_) source_ = destination_;
    destination_ = town;
    applySelection();
}

void MainWindow::applySelection() {
    pauseAnimation();

    // If a route is already showing, recalculate it straight away.
    if (routeCalculated_) {
        findRoute();
    } else {
        clearProgress();
        refresh();
    }
}

void MainWindow::togglePlay() {
    if (animating_) {
        pauseAnimation();
        refresh();
    } else {
        startAnimation();
    }
}

void MainWindow::startAnimation() {
    if (!routeCalculated_) {
        findRoute();
    }

    if (!routeCalculated_) return;

    if (currentStep_ >= result_.steps.size()) {
        clearProgress();
    }

    animationStarted_ = true;
    animating_ = true;

    SetTimer(hwnd_, TIMER_ID, ANIMATION_INTERVAL_MS, nullptr);

    updateLog();
    refresh();
}

void MainWindow::pauseAnimation() {
    animating_ = false;

    if (hwnd_) {
        KillTimer(hwnd_, TIMER_ID);
    }
}

// Applies one algorithm step. Returns true while more steps remain.
bool MainWindow::advanceStep() {
    if (currentStep_ >= result_.steps.size()) return false;

    applyStep(result_.steps[currentStep_]);
    ++currentStep_;

    updateLog();

    return currentStep_ < result_.steps.size();
}

void MainWindow::nextStep() {
    if (!routeCalculated_) {
        findRoute();
    }

    if (!routeCalculated_) return;

    pauseAnimation();

    animationStarted_ = true;

    if (currentStep_ < result_.steps.size()) {
        advanceStep();
    } else {
        updateLog();
    }

    refresh();
}

void MainWindow::resetRoute() {
    pauseAnimation();

    routes_.clear();
    clearProgress();

    routeCalculated_ = false;
    animationStarted_ = false;

    SetWindowTextW(
        logBox_,
        L"Ready. Choose a source and destination, then click Find Route."
    );

    refresh();
}

void MainWindow::updateLog() {
    std::wstring text;

    for (size_t i = 0; i < currentStep_; ++i) {
        text += twoDigits(i + 1) + L"   ";
        text += toWide(result_.steps[i]);
        text += L"\r\n";
    }

    if (currentStep_ == 0) {
        text =
            L"Press Play or Next Step to begin.\r\n";
    }

    if (currentStep_ >= result_.steps.size()) {
        text += L"\r\nAlgorithm completed successfully.\r\n";

        if (!routes_.empty()) {
            text += L"Shortest distance: ";
            text += std::to_wstring(
                result_.distances.at(destination_)
            );
            text += L" km\r\n";
        }
    }

    SetWindowTextW(logBox_, text.c_str());

    // Scroll automatically to the latest step.
    SendMessageW(logBox_, EM_SETSEL, -1, -1);
    SendMessageW(logBox_, EM_SCROLLCARET, 0, 0);
}

// ---------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------

void MainWindow::drawButton(const DRAWITEMSTRUCT* item) {
    HDC dc = item->hDC;
    RECT r = item->rcItem;

    const int id = static_cast<int>(item->CtlID);
    const bool pressed = (item->itemState & ODS_SELECTED) != 0;
    const bool hover = GetPropW(item->hwndItem, L"rf.hover") != nullptr;
    const bool focused = (item->itemState & ODS_FOCUS) != 0 &&
                         (item->itemState & ODS_NOFOCUSRECT) == 0;

    wchar_t label[64] = {};
    GetWindowTextW(item->hwndItem, label, 64);

    COLORREF fill = ui::kCardAlt;
    COLORREF border = ui::kBorder;
    COLORREF text = ui::kText;

    const bool isSource = id >= ID_SRC_BASE && id < ID_SRC_BASE + TOWN_COUNT;
    const bool isDestination =
        id >= ID_DEST_BASE && id < ID_DEST_BASE + TOWN_COUNT;

    if (isSource || isDestination) {
        const char town =
            kTowns[isSource ? id - ID_SRC_BASE : id - ID_DEST_BASE];
        const bool selected = town == (isSource ? source_ : destination_);

        if (selected) {
            fill = isSource ? RGB(10, 54, 70) : RGB(10, 58, 44);
            border = isSource ? ui::kAccent : ui::kGreen;
            text = border;
        } else if (hover) {
            border = ui::kTextMuted;
        }
    }
    else if (id == ID_FIND) {
        fill = ui::kAccent;
        border = CLR_INVALID;
        text = ui::kOnAccent;
        if (hover) fill = lighten(fill, 28);
    }
    else if (id == ID_PLAY) {
        fill = animating_ ? ui::kAmber : ui::kGreen;
        border = CLR_INVALID;
        text = ui::kOnAccent;
        if (hover) fill = lighten(fill, 24);
    }
    else {
        if (hover) {
            border = ui::kAccent;
            fill = lighten(fill, 8);
        }
    }

    if (pressed) {
        fill = darken(fill, 22);
    }

    // Paint the card colour first so the rounded corners blend in.
    ui::fillRect(dc, r, ui::kHeaderBg);
    ui::roundRect(dc, r, px(8), fill, border, 1);

    RECT textRect = r;
    if (pressed) OffsetRect(&textRect, 0, 1);

    ui::drawText(
        dc, label, textRect, text, fontBold_,
        DT_CENTER | DT_SINGLELINE | DT_VCENTER
    );

    if (focused) {
        RECT ring = r;
        InflateRect(&ring, -px(3), -px(3));
        ui::strokeRoundRect(dc, ring, px(6), ui::kText, 1);
    }
}

void MainWindow::drawHeader(HDC dc, const Layout& layout) {
    RECT client;
    GetClientRect(hwnd_, &client);
    const int width = client.right;

    RECT header = {0, 0, width, layout.headerHeight};
    ui::fillRect(dc, header, ui::kHeaderBg);

    // Accent line along the bottom edge.
    RECT line = {0, layout.headerHeight - 2, width, layout.headerHeight};
    ui::fillRect(dc, line, ui::kBorder);
    RECT lineAccent = {0, layout.headerHeight - 2, px(240), layout.headerHeight};
    ui::fillRect(dc, lineAccent, ui::kAccent);

    // Title: "LESOTHO" in white, "ROUTE FINDER" in accent colour.
    const std::wstring first = L"LESOTHO ";
    const std::wstring second = L"ROUTE FINDER";
    const int x = px(24);
    const int rowHeight = px(36);

    RECT titleRect = {x, layout.rowOneY, width, layout.rowOneY + rowHeight};
    ui::drawText(dc, first, titleRect, ui::kText, fontTitle_);

    titleRect.left = x + ui::textWidth(dc, fontTitle_, first);
    ui::drawText(dc, second, titleRect, ui::kAccent, fontTitle_);

    const int titleRight =
        titleRect.left + ui::textWidth(dc, fontTitle_, second);

    // Status chip, next to the title.
    std::wstring status = L"READY";
    COLORREF color = ui::kTextDim;

    if (animating_) {
        status = L"RUNNING";
        color = ui::kAmber;
    } else if (isFinished()) {
        status = L"COMPLETE";
        color = ui::kGreen;
    } else if (animationStarted_) {
        status = L"PAUSED";
        color = ui::kViolet;
    } else if (routeCalculated_) {
        status = L"ROUTE READY";
        color = ui::kAccent;
    }

    const int chipHeight = px(28);
    const int chipWidth =
        ui::textWidth(dc, fontLabel_, status) + px(46);

    RECT chip = {
        titleRight + px(18),
        layout.rowOneY + (rowHeight - chipHeight) / 2,
        titleRight + px(18) + chipWidth,
        layout.rowOneY + (rowHeight + chipHeight) / 2
    };

    ui::roundRect(dc, chip, chipHeight / 2, ui::kCard, color, 1);
    ui::fillCircle(
        dc, chip.left + px(18), (chip.top + chip.bottom) / 2,
        px(4), color
    );

    RECT chipText = {chip.left + px(30), chip.top, chip.right - px(10), chip.bottom};
    ui::drawText(dc, status, chipText, color, fontLabel_);

    // Captions for the source / destination chips.
    RECT from = {
        layout.fromCaptionX, layout.rowTwoY,
        layout.fromCaptionX + px(50), layout.rowTwoY + layout.chipHeight
    };
    ui::drawText(dc, L"FROM", from, ui::kAccent, fontLabel_);

    RECT to = {
        layout.toCaptionX, layout.rowTwoY,
        layout.toCaptionX + px(32), layout.rowTwoY + layout.chipHeight
    };
    ui::drawText(dc, L"TO", to, ui::kGreen, fontLabel_);
}

void MainWindow::drawGraphCard(HDC dc, const Layout& layout) {
    const RECT& card = layout.graphCard;
    ui::roundRect(dc, card, px(12), ui::kCard, ui::kBorder, 1);

    RECT title = {
        card.left + px(18), card.top + px(10),
        card.left + px(220), card.top + px(38)
    };
    ui::drawText(dc, L"ROAD NETWORK", title, ui::kAccent, fontLabel_);

    // Legend, right-aligned. Skipped when the card is too narrow.
    struct Item { const wchar_t* name; COLORREF color; };
    const Item items[] = {
        {L"Start", ui::kAccent},
        {L"Current", ui::kAmber},
        {L"Visited", ui::kViolet},
        {L"Shortest route", ui::kGreen},
        {L"Unvisited", ui::kIdleNode}
    };

    const int dot = px(10);
    const int dotGap = px(6);
    const int itemGap = px(18);

    int total = 0;
    for (const Item& item : items) {
        total += dot + dotGap +
            ui::textWidth(dc, fontSmall_, item.name) + itemGap;
    }
    total -= itemGap;

    int x = card.right - px(18) - total;

    if (x > card.left + px(190)) {
        const int midY = card.top + px(24);

        for (const Item& item : items) {
            ui::fillCircle(
                dc, x + dot / 2, midY, dot / 2, item.color
            );

            const int textW = ui::textWidth(dc, fontSmall_, item.name);
            RECT label = {
                x + dot + dotGap, midY - px(10),
                x + dot + dotGap + textW + px(4), midY + px(10)
            };
            ui::drawText(dc, item.name, label, ui::kTextDim, fontSmall_);

            x += dot + dotGap + textW + itemGap;
        }
    }

    // Build the picture for the renderer.
    GraphView view;
    view.scale = dpi_ / 96.0;
    view.source = source_;
    view.nameFont = fontBold_;
    view.smallFont = fontSmall_;
    view.idFont = fontHeading_;

    const bool showRoute =
        routeCalculated_ && !routes_.empty() &&
        (!animationStarted_ || isFinished());

    if (showRoute) view.route = routes_.front();

    view.visited = visited_;
    view.relaxed = relaxed_;
    view.current = isFinished() ? 0 : current_;
    view.destination = routeCalculated_ ? destination_ : 0;

    if (animationStarted_) {
        view.distances = tentative_;
        view.showDistances = true;
    } else if (routeCalculated_) {
        view.distances = result_.distances;
        view.showDistances = true;
    }

    GraphRenderer::draw(dc, layout.graphCanvas, graph_, view);
}

void MainWindow::drawResults(HDC dc, const Layout& layout) {
    const RECT& card = layout.resultsCard;
    ui::roundRect(dc, card, px(12), ui::kCard, ui::kBorder, 1);

    const int left = card.left + px(20);
    const int right = card.right - px(20);
    const int bottomLimit = card.bottom - px(12);
    int y = card.top + px(14);

    auto caption = [&](const wchar_t* text, COLORREF color) {
        RECT r = {left, y, right, y + px(18)};
        ui::drawText(dc, text, r, color, fontLabel_);
        y += px(22);
    };

    caption(L"SHORTEST PATH RESULTS", ui::kAccent);
    y += px(6);

    // -- Empty state ----------------------------------------------------
    if (!routeCalculated_) {
        RECT message = {left, y, right, bottomLimit};
        ui::drawText(
            dc,
            L"Choose a source and a destination, then click Find Route "
            L"to see the shortest path between them.\n\n"
            L"Use Next Step to follow Dijkstra's algorithm one move "
            L"at a time, or Play to watch it run.",
            message, ui::kTextDim, fontBody_, DT_LEFT | DT_WORDBREAK
        );
        return;
    }

    // -- Destination ----------------------------------------------------
    caption(L"FROM  \u2192  TO", ui::kTextDim);
    {
        RECT r = {left, y, right, y + px(26)};
        ui::drawText(
            dc,
            townName(graph_, source_) + L"  \u2192  " +
                townName(graph_, destination_),
            r, ui::kText, fontBold_
        );
        y += px(34);
    }

    if (routes_.empty()) {
        RECT r = {left, y, right, y + px(26)};
        ui::drawText(
            dc, L"Destination unreachable", r, ui::kRed, fontHeading_
        );
        return;
    }

    // -- Distance -------------------------------------------------------
    caption(L"SHORTEST DISTANCE", ui::kTextDim);
    {
        const std::wstring number =
            std::to_wstring(result_.distances.at(destination_));
        const int numberWidth = ui::textWidth(dc, fontBig_, number);

        RECT r = {left, y, left + numberWidth + px(4), y + px(54)};
        ui::drawText(dc, number, r, ui::kGreen, fontBig_);

        RECT unit = {left + numberWidth + px(8), y + px(14), right, y + px(54)};
        ui::drawText(
            dc, L"km", unit, ui::kTextDim, fontHeading_,
            DT_LEFT | DT_SINGLELINE | DT_VCENTER
        );
        y += px(60);
    }

    // -- Route ----------------------------------------------------------
    caption(L"SHORTEST ROUTE", ui::kTextDim);
    {
        const std::wstring text = pathText(graph_, routes_.front());
        const int h = ui::wrappedHeight(dc, fontBold_, text, right - left);

        RECT r = {left, y, right, y + h};
        ui::drawText(dc, text, r, ui::kText, fontBold_, DT_LEFT | DT_WORDBREAK);
        y += h + px(12);
    }

    if (routes_.size() > 1 && y + px(50) < bottomLimit) {
        caption(L"ALTERNATIVE ROUTE (SAME DISTANCE)", ui::kAmber);

        const std::wstring text = pathText(graph_, routes_[1]);
        const int h = ui::wrappedHeight(dc, fontBody_, text, right - left);

        RECT r = {left, y, right, y + h};
        ui::drawText(dc, text, r, ui::kTextDim, fontBody_, DT_LEFT | DT_WORDBREAK);
        y += h + px(12);
    }

    // -- Distance table (only if there is room) -------------------------
    const std::map<char, int>& distances =
        animationStarted_ ? tentative_ : result_.distances;

    int rows = 0;
    int maxDistance = 1;
    for (const auto& [id, name] : graph_.towns()) {
        if (id == source_) continue;
        ++rows;

        auto d = distances.find(id);
        if (d != distances.end() && d->second != INT_MAX) {
            maxDistance = (std::max)(maxDistance, d->second);
        }
    }

    const int rowHeight = px(34);
    if (rows > 0 && y + px(26) + rows * rowHeight <= bottomLimit) {
        std::wstring heading = L"DISTANCES FROM " + toWide(graph_.nameOf(source_));
        for (wchar_t& ch : heading) ch = static_cast<wchar_t>(std::towupper(ch));

        caption(heading.c_str(), ui::kTextDim);

        for (const auto& [id, name] : graph_.towns()) {
            if (id == source_) continue;

            auto d = distances.find(id);
            const bool known =
                d != distances.end() && d->second != INT_MAX;

            RECT nameRect = {left, y, right, y + px(18)};
            ui::drawText(
                dc, toWide(name), nameRect,
                id == destination_ ? ui::kText : ui::kTextDim,
                id == destination_ ? fontBold_ : fontBody_
            );

            ui::drawText(
                dc,
                known ? std::to_wstring(d->second) + L" km" : L"∞",
                nameRect,
                known ? ui::kText : ui::kTextMuted,
                fontBold_, DT_RIGHT | DT_SINGLELINE | DT_VCENTER
            );

            RECT track = {left, y + px(21), right, y + px(27)};
            ui::roundRect(dc, track, px(3), ui::kCardAlt);

            if (known) {
                RECT bar = track;
                bar.right = track.left + (std::max)(
                    px(6),
                    static_cast<int>(
                        (track.right - track.left) *
                        static_cast<double>(d->second) / maxDistance
                    )
                );
                ui::roundRect(
                    dc, bar, px(3),
                    id == destination_ ? ui::kGreen : ui::kAccent
                );
            }

            y += rowHeight;
        }
    }
}

void MainWindow::drawLogCard(HDC dc, const Layout& layout) {
    const RECT& card = layout.logCard;
    ui::roundRect(dc, card, px(12), ui::kCard, ui::kBorder, 1);

    RECT title = {
        card.left + px(18), card.top + px(10),
        card.left + px(400), card.top + px(30)
    };
    ui::drawText(
        dc,
        L"ALGORITHM EXECUTION LOG",
        title, ui::kAccent, fontLabel_
    );

    if (animationStarted_) {
        RECT step = {
            card.right - px(220), card.top + px(10),
            card.right - px(18), card.top + px(30)
        };
        ui::drawText(
            dc,
            L"Step " + std::to_wstring(currentStep_) + L" / " +
                std::to_wstring(result_.steps.size()),
            step, ui::kTextDim, fontSmall_,
            DT_RIGHT | DT_SINGLELINE | DT_VCENTER
        );

        // Progress bar.
        RECT track = {
            card.left + px(18), card.top + px(34),
            card.right - px(18), card.top + px(38)
        };
        ui::roundRect(dc, track, px(2), ui::kCardAlt);

        if (!result_.steps.empty() && currentStep_ > 0) {
            RECT fill = track;
            fill.right = track.left + static_cast<int>(
                (track.right - track.left) *
                static_cast<double>(currentStep_) / result_.steps.size()
            );
            ui::roundRect(
                dc, fill, px(2),
                isFinished() ? ui::kGreen : ui::kAccent
            );
        }
    }

    ui::roundRect(dc, layout.logBackground, px(8), ui::kLogBg, ui::kBorder, 1);
}

void MainWindow::drawDashboard(HDC dc) {
    RECT client;
    GetClientRect(hwnd_, &client);
    ui::fillRect(dc, client, ui::kWindowBg);

    const Layout layout = computeLayout();

    drawHeader(dc, layout);
    drawGraphCard(dc, layout);
    drawResults(dc, layout);
    drawLogCard(dc, layout);
}

int MainWindow::run() {
    MSG message = {};

    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        // Lets Tab / Shift+Tab / Space work across the buttons.
        if (hwnd_ && IsDialogMessageW(hwnd_, &message)) {
            continue;
        }

        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return static_cast<int>(message.wParam);
}

LRESULT CALLBACK MainWindow::WindowProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
) {
    if (message == WM_NCCREATE) {
        auto* create =
            reinterpret_cast<CREATESTRUCTW*>(lParam);

        auto* self = static_cast<MainWindow*>(
            create->lpCreateParams
        );

        SetWindowLongPtrW(
            hwnd,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(self)
        );

        return TRUE;
    }

    auto* self = reinterpret_cast<MainWindow*>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA)
    );

    if (self) {
        return self->handleMessage(
            hwnd, message, wParam, lParam
        );
    }

    return DefWindowProcW(
        hwnd, message, wParam, lParam
    );
}

LRESULT MainWindow::handleMessage(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
) {
    switch (message) {
        case WM_GETMINMAXINFO: {
            auto* info =
                reinterpret_cast<MINMAXINFO*>(lParam);

            info->ptMinTrackSize.x = px(1050);
            info->ptMinTrackSize.y = px(760);
            return 0;
        }

        case WM_COMMAND: {
            if (HIWORD(wParam) != BN_CLICKED) {
                return 0;
            }

            const int id = LOWORD(wParam);

            if (id >= ID_SRC_BASE && id < ID_SRC_BASE + TOWN_COUNT) {
                selectSource(kTowns[id - ID_SRC_BASE]);
            } else if (id >= ID_DEST_BASE && id < ID_DEST_BASE + TOWN_COUNT) {
                selectDestination(kTowns[id - ID_DEST_BASE]);
            } else {
                switch (id) {
                    case ID_FIND:  findRoute();   break;
                    case ID_PLAY:  togglePlay();  break;
                    case ID_NEXT:  nextStep();    break;
                    case ID_RESET: resetRoute();  break;
                    default: break;
                }
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

        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT: {
            if (reinterpret_cast<HWND>(lParam) == logBox_) {
                HDC controlDc = reinterpret_cast<HDC>(wParam);
                SetTextColor(controlDc, ui::kText);
                SetBkColor(controlDc, ui::kLogBg);
                return reinterpret_cast<LRESULT>(logBrush_);
            }
            break;
        }

        case WM_TIMER:
            if (wParam == TIMER_ID) {
                if (!advanceStep()) {
                    pauseAnimation();
                }
                refresh();
                return 0;
            }
            break;

        case WM_SIZE:
            layoutControls();
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC dc = BeginPaint(hwnd, &ps);

            RECT client;
            GetClientRect(hwnd, &client);

            // Draw into an off-screen bitmap first: no flicker.
            if (client.right > 0 && client.bottom > 0) {
                HDC mem = CreateCompatibleDC(dc);
                HBITMAP bitmap = CreateCompatibleBitmap(
                    dc, client.right, client.bottom
                );
                HGDIOBJ oldBitmap = SelectObject(mem, bitmap);

                drawDashboard(mem);

                BitBlt(
                    dc, 0, 0, client.right, client.bottom,
                    mem, 0, 0, SRCCOPY
                );

                SelectObject(mem, oldBitmap);
                DeleteObject(bitmap);
                DeleteDC(mem);
            }

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_DESTROY:
            pauseAnimation();
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcW(
        hwnd, message, wParam, lParam
    );
}