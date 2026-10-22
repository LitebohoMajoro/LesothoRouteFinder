#pragma once

#include <windows.h>
#include <map>
#include <utility>
#include <vector>
#include "Graph.h"

// Everything the renderer needs to know to draw one frame.
struct GraphView {
    std::vector<char> route;                       // final shortest route (green)
    std::vector<char> visited;                     // towns already processed
    std::vector<std::pair<char, char>> relaxed;    // roads checked so far (amber)
    std::map<char, int> distances;                 // current distance labels
    char source = 'A';
    char current = 0;                              // town being visited (0 = none)
    char destination = 0;                          // 0 = none selected yet
    bool showDistances = false;
    double scale = 1.0;                            // DPI scale (1.0 = 96 dpi)
    HFONT nameFont = nullptr;
    HFONT smallFont = nullptr;
    HFONT idFont = nullptr;
};

class GraphRenderer {
public:
    static void draw(
        HDC hdc,
        const RECT& area,
        const Graph& graph,
        const GraphView& view
    );
};