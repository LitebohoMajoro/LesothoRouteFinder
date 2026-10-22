#pragma once

#include <windows.h>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "Graph.h"
#include "Dijkstra.h"

class MainWindow {
public:
    ~MainWindow();

    bool create(HINSTANCE instance, int showCommand);
    int run();

private:
    // Pixel rectangles of every region, recomputed from the window size.
    struct Layout {
        int headerHeight;
        int rowOneY;          // title row: title, status chip, action buttons
        int rowTwoY;          // selector row: source and destination chips
        int chipWidth;
        int chipHeight;
        int chipGap;
        int fromCaptionX;
        int fromChipsX;
        int toCaptionX;
        int toChipsX;
        RECT graphCard;
        RECT graphCanvas;
        RECT resultsCard;
        RECT logCard;
        RECT logBackground;
        RECT logText;
    };

    static LRESULT CALLBACK WindowProc(
        HWND hwnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam
    );

    LRESULT handleMessage(
        HWND hwnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam
    );

    // Setup
    int px(int value) const;
    void createFonts();
    void destroyFonts();
    void createControls(HINSTANCE instance);
    Layout computeLayout() const;
    void layoutControls();

    // Actions
    void findRoute();
    void selectSource(char town);
    void selectDestination(char town);
    void applySelection();
    void togglePlay();
    void startAnimation();
    void pauseAnimation();
    void nextStep();
    bool advanceStep();
    void resetRoute();

    // State helpers
    void clearProgress();
    void applyStep(const std::string& step);
    char townFromName(const std::string& name) const;
    bool isFinished() const;
    void updateLog();
    void refresh();

    // Painting
    void drawDashboard(HDC dc);
    void drawHeader(HDC dc, const Layout& layout);
    void drawGraphCard(HDC dc, const Layout& layout);
    void drawResults(HDC dc, const Layout& layout);
    void drawLogCard(HDC dc, const Layout& layout);
    void drawButton(const DRAWITEMSTRUCT* item);

    HWND hwnd_ = nullptr;
    HWND sourceButtons_[4] = {nullptr, nullptr, nullptr, nullptr};
    HWND destinationButtons_[4] = {nullptr, nullptr, nullptr, nullptr};
    HWND findButton_ = nullptr;
    HWND playButton_ = nullptr;
    HWND nextButton_ = nullptr;
    HWND resetButton_ = nullptr;
    HWND logBox_ = nullptr;

    HFONT fontTitle_ = nullptr;
    HFONT fontHeading_ = nullptr;
    HFONT fontBody_ = nullptr;
    HFONT fontBold_ = nullptr;
    HFONT fontSmall_ = nullptr;
    HFONT fontLabel_ = nullptr;
    HFONT fontBig_ = nullptr;
    HFONT fontMono_ = nullptr;
    HBRUSH logBrush_ = nullptr;

    int dpi_ = 96;

    Graph graph_ = createAssignmentGraph();
    ShortestPaths result_;

    std::vector<std::vector<char>> routes_;

    // Animation progress
    std::vector<char> visited_;
    std::vector<std::pair<char, char>> relaxed_;
    std::map<char, int> tentative_;
    char current_ = 0;

    char source_ = 'A';
    char destination_ = 'D';

    bool routeCalculated_ = false;
    bool animating_ = false;
    bool animationStarted_ = false;

    size_t currentStep_ = 0;

    static constexpr UINT_PTR TIMER_ID = 1;
}; 