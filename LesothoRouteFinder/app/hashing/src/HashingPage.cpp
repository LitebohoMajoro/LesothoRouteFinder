#pragma once

// NEW FILE: replaces hashing/include/MainWindow.h.
// The hashing visualiser is now a child "page" hosted inside the app shell
// (app/src/AppWindow.cpp) instead of being its own top-level window.

#include <windows.h>
#include <deque>
#include <string>

#include "HashSetOpen.h"
#include "HashSetClosed.h"
#include "HashTableTypes.h"

class HashingPage {
public:
    ~HashingPage();

    bool create(HWND parent, HINSTANCE instance, int dpi);
    HWND hwnd() const { return hwnd_; }

    // Shows the open- or closed-hashing table (each keeps its own state).
    void setMode(Mode mode);
    Mode mode() const { return mode_; }

    // Called by the shell when the user leaves this page.
    void onHide();

private:
    struct Layout {
        int headerHeight;
        RECT modeCard;
        RECT controlsCard;
        RECT inputBox;
        RECT capacityBox;
        int capacityCaptionX;
        RECT tableCard;
        RECT canvas;
        RECT logCard;
        RECT logBackground;
        RECT logText;
    };

    struct GridMetrics {
        int columns;
        int cellWidth;
        int rows;
        int contentHeight;
    };

    static LRESULT CALLBACK proc(HWND, UINT, WPARAM, LPARAM);
    LRESULT handle(UINT message, WPARAM wParam, LPARAM lParam);

    int px(int value) const;
    void createFonts();
    void destroyFonts();
    void createControls();
    Layout computeLayout() const;
    void layoutControls();
    GridMetrics gridMetrics(const Layout& layout) const;
    void clampScroll();
    void ensureVisible(int index);

    // Actions
    void run(int action);
    bool consume();
    void startTimer();
    void stopTimer();
    void togglePlay();
    void stepOnce();
    void cycleSpeed();
    void clearTable();
    void saveSnapshot();
    void applyCapacity();
    void syncCapacityBox();
    bool exportCurrent();
    void appendLog(const std::wstring& line);
    void refresh();

    // Painting
    void drawPage(HDC dc);
    void drawHeader(HDC dc, const Layout& layout);
    void drawModeCard(HDC dc, const Layout& layout);
    void drawControlsCard(HDC dc, const Layout& layout);
    void drawTable(HDC dc, const Layout& layout);
    void drawLogCard(HDC dc, const Layout& layout);
    void drawButton(const DRAWITEMSTRUCT* item);

    HWND hwnd_ = nullptr;
    HWND parent_ = nullptr;
    HINSTANCE instance_ = nullptr;
    HWND modeButtons_[2] = {nullptr, nullptr};
    HWND input_ = nullptr;
    HWND capacityInput_ = nullptr;
    HWND applyButton_ = nullptr;
    HWND insertButton_ = nullptr;
    HWND searchButton_ = nullptr;
    HWND removeButton_ = nullptr;
    HWND playButton_ = nullptr;
    HWND stepButton_ = nullptr;
    HWND speedButton_ = nullptr;
    HWND clearButton_ = nullptr;
    HWND saveButton_ = nullptr;
    HWND logBox_ = nullptr;

    HFONT fontTitle_ = nullptr;
    HFONT fontHeading_ = nullptr;
    HFONT fontBody_ = nullptr;
    HFONT fontBold_ = nullptr;
    HFONT fontSmall_ = nullptr;
    HFONT fontLabel_ = nullptr;
    HFONT fontMono_ = nullptr;
    HBRUSH editBrush_ = nullptr;

    int dpi_ = 96;

    Mode mode_ = Mode::Open;
    HashSetOpen open_;
    HashSetClosed closed_;

    std::deque<Event> events_;
    Event current_;
    int operationTotal_ = 0;     // events produced by the latest operation
    int operationDone_ = 0;      // events already shown
    bool timerOn_ = false;
    int speedIndex_ = 1;
    int scrollY_ = 0;

    std::wstring message_;
    std::wstring logText_;

    static constexpr UINT_PTR TIMER_ID = 1;
};