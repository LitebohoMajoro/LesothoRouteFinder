#include "GraphRenderer.h"
#include "Theme.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <map>
#include <string>

namespace {

// The road network is drawn at SS x resolution and shrunk with a
// high-quality filter, which gives smooth (anti-aliased) lines and circles.
constexpr int SS = 2;

struct Pt {
    double x;
    double y;
};

enum class EdgeState { Idle, Relaxed, Route };

struct EdgeDraw {
    Pt from;
    Pt to;
    int weight;
    EdgeState state;
};

struct Pill {
    RECT rect;          // in screen coordinates
    std::wstring text;
    COLORREF color;
};

bool contains(const std::vector<char>& list, char id) {
    return std::find(list.begin(), list.end(), id) != list.end();
}

bool onRoute(char from, char to, const std::vector<char>& path) {
    for (size_t i = 1; i < path.size(); ++i) {
        if (path[i - 1] == from && path[i] == to) return true;
    }
    return false;
}

bool wasRelaxed(const GraphView& view, char from, char to) {
    return std::find(
        view.relaxed.begin(), view.relaxed.end(),
        std::make_pair(from, to)
    ) != view.relaxed.end();
}

void drawLine(HDC dc, Pt a, Pt b, int width, COLORREF color) {
    LOGBRUSH brush = {};
    brush.lbStyle = BS_SOLID;
    brush.lbColor = color;

    HPEN pen = ExtCreatePen(
        PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_ROUND | PS_JOIN_ROUND,
        width, &brush, 0, nullptr
    );

    HGDIOBJ old = SelectObject(dc, pen);
    MoveToEx(dc, std::lround(a.x), std::lround(a.y), nullptr);
    LineTo(dc, std::lround(b.x), std::lround(b.y));
    SelectObject(dc, old);
    DeleteObject(pen);
}

// Filled triangle whose tip touches `tip`, pointing along (ux, uy).
void drawArrowHead(
    HDC dc, Pt tip, double ux, double uy,
    double length, double halfWidth, COLORREF color
) {
    Pt base = {tip.x - ux * length, tip.y - uy * length};

    POINT points[3] = {
        {std::lround(tip.x), std::lround(tip.y)},
        {std::lround(base.x - uy * halfWidth),
         std::lround(base.y + ux * halfWidth)},
        {std::lround(base.x + uy * halfWidth),
         std::lround(base.y - ux * halfWidth)}
    };

    HBRUSH brush = CreateSolidBrush(color);
    HGDIOBJ oldBrush = SelectObject(dc, brush);
    HGDIOBJ oldPen = SelectObject(dc, GetStockObject(NULL_PEN));
    Polygon(dc, points, 3);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(brush);
}

COLORREF edgeColor(EdgeState state) {
    switch (state) {
        case EdgeState::Route:   return ui::kGreen;
        case EdgeState::Relaxed: return ui::kAmber;
        default:                 return ui::kIdleEdge;
    }
}

} // namespace

void GraphRenderer::draw(
    HDC hdc,
    const RECT& area,
    const Graph& graph,
    const GraphView& view
) {
    const int width = area.right - area.left;
    const int height = area.bottom - area.top;

    if (width < 120 || height < 120) return;

    const double s = view.scale;
    const double radius = 21.0 * s;
    const double labelBlock = 44.0 * s;   // two text lines + gap
    const double pad = (std::min)(
        radius + labelBlock + 4.0 * s, height * 0.32
    );

    HFONT nameFont = view.nameFont
        ? view.nameFont
        : static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    HFONT smallFont = view.smallFont ? view.smallFont : nameFont;
    HFONT idFont = view.idFont ? view.idFont : nameFont;

    // Town positions, relative to the canvas (1x coordinates).
    std::map<char, Pt> positions = {
        {'A', {width * 0.10, height * 0.5}},
        {'B', {width * 0.50, pad}},
        {'C', {width * 0.50, height - pad}},
        {'D', {width * 0.90, height * 0.5}}
    };

    // ---- Collect edges ----
    std::vector<EdgeDraw> edges;

    for (const auto& [from, name] : graph.towns()) {
        auto a = positions.find(from);
        if (a == positions.end()) continue;

        for (const Edge& edge : graph.roadsFrom(from)) {
            auto b = positions.find(edge.to);
            if (b == positions.end()) continue;

            EdgeState state = EdgeState::Idle;
            if (onRoute(from, edge.to, view.route)) {
                state = EdgeState::Route;
            } else if (wasRelaxed(view, from, edge.to)) {
                state = EdgeState::Relaxed;
            }

            edges.push_back({a->second, b->second, edge.weight, state});
        }
    }

    // Draw idle edges first, route edges last (so they sit on top).
    std::stable_sort(
        edges.begin(), edges.end(),
        [](const EdgeDraw& l, const EdgeDraw& r) {
            return static_cast<int>(l.state) < static_cast<int>(r.state);
        }
    );

    // ---- Supersampled drawing surface ----
    HDC mem = CreateCompatibleDC(hdc);
    HBITMAP bitmap = CreateCompatibleBitmap(hdc, width * SS, height * SS);

    if (!mem || !bitmap) {
        if (bitmap) DeleteObject(bitmap);
        if (mem) DeleteDC(mem);
        return;
    }

    HGDIOBJ oldBitmap = SelectObject(mem, bitmap);

    RECT full = {0, 0, width * SS, height * SS};
    ui::fillRect(mem, full, ui::kCanvas);

    // Subtle dot grid.
    {
        HBRUSH dot = CreateSolidBrush(ui::kGrid);
        const int step = (std::max)(12, static_cast<int>(28 * s)) * SS;

        for (int y = step / 2; y < full.bottom; y += step) {
            for (int x = step / 2; x < full.right; x += step) {
                RECT d = {x, y, x + SS * 2, y + SS * 2};
                FillRect(mem, &d, dot);
            }
        }
        DeleteObject(dot);
    }

    std::vector<Pill> pills;

    // Roads.
    for (const EdgeDraw& e : edges) {
        double dx = e.to.x - e.from.x;
        double dy = e.to.y - e.from.y;
        double length = std::sqrt(dx * dx + dy * dy);
        if (length < 1.0) continue;

        double ux = dx / length;
        double uy = dy / length;

        Pt start = {e.from.x + ux * (radius + 3 * s),
                    e.from.y + uy * (radius + 3 * s)};
        Pt tip   = {e.to.x - ux * (radius + 5 * s),
                    e.to.y - uy * (radius + 5 * s)};

        const double arrowLength = 13.0 * s;
        Pt base = {tip.x - ux * arrowLength, tip.y - uy * arrowLength};

        COLORREF color = edgeColor(e.state);
        double thickness = e.state == EdgeState::Idle ? 2.5
                         : e.state == EdgeState::Relaxed ? 3.5 : 5.0;

        auto scaled = [](Pt p) { return Pt{p.x * SS, p.y * SS}; };

        if (e.state == EdgeState::Route) {
            // Soft "glow" under the shortest route.
            drawLine(
                mem, scaled(start), scaled(base),
                static_cast<int>(13 * s * SS), ui::kGreenGlow
            );
        }

        drawLine(
            mem, scaled(start), scaled(base),
            static_cast<int>(thickness * s * SS), color
        );

        drawArrowHead(
            mem, scaled(tip), ux, uy,
            arrowLength * SS, 6.0 * s * SS, color
        );

        // Distance pill at the middle of the road.
        std::wstring label = std::to_wstring(e.weight) + L" km";
        int textW = ui::textWidth(hdc, smallFont, label);
        double pillW = textW + 18 * s;
        double pillH = 22 * s;

        double mx = (e.from.x + e.to.x) / 2.0;
        double my = (e.from.y + e.to.y) / 2.0;

        RECT local = {
            static_cast<LONG>(mx - pillW / 2),
            static_cast<LONG>(my - pillH / 2),
            static_cast<LONG>(mx + pillW / 2),
            static_cast<LONG>(my + pillH / 2)
        };

        RECT big = {
            local.left * SS, local.top * SS,
            local.right * SS, local.bottom * SS
        };

        ui::roundRect(
            mem, big, (big.bottom - big.top) / 2,
            ui::kCardAlt, color, static_cast<int>(2 * SS)
        );

        pills.push_back({
            {area.left + local.left, area.top + local.top,
             area.left + local.right, area.top + local.bottom},
            label, color
        });
    }

    // Towns.
    struct TownDraw {
        char id;
        Pt pos;
        COLORREF fill;
        COLORREF letter;
    };
    std::vector<TownDraw> towns;

    for (const auto& [id, name] : graph.towns()) {
        auto position = positions.find(id);
        if (position == positions.end()) continue;

        const bool isCurrent = id == view.current;
        const bool isRoute = contains(view.route, id);
        const bool isVisited = contains(view.visited, id);

        COLORREF fill = ui::kIdleNode;
        COLORREF letter = ui::kText;

        if (isCurrent)              { fill = ui::kAmber;  letter = ui::kOnAccent; }
        else if (isRoute)           { fill = ui::kGreen;  letter = ui::kOnAccent; }
        else if (id == view.source) { fill = ui::kAccent; letter = ui::kOnAccent; }
        else if (isVisited)         { fill = ui::kViolet; letter = ui::kOnAccent; }

        int cx = static_cast<int>(position->second.x * SS);
        int cy = static_cast<int>(position->second.y * SS);
        int r = static_cast<int>(radius * SS);

        if (isCurrent) {
            ui::fillCircle(
                mem, cx, cy, r + static_cast<int>(7 * s * SS),
                CLR_INVALID, ui::kAmber, static_cast<int>(3 * s * SS)
            );
        } else if (id == view.destination) {
            ui::fillCircle(
                mem, cx, cy, r + static_cast<int>(6 * s * SS),
                CLR_INVALID, ui::kGreen, static_cast<int>(2 * s * SS)
            );
        }

        ui::fillCircle(
            mem, cx, cy, r, fill, ui::kCanvas,
            static_cast<int>(3 * s * SS)
        );

        towns.push_back({id, position->second, fill, letter});
    }

    // ---- Shrink the smooth layer onto the real surface ----
    SetStretchBltMode(hdc, HALFTONE);
    SetBrushOrgEx(hdc, 0, 0, nullptr);
    StretchBlt(
        hdc, area.left, area.top, width, height,
        mem, 0, 0, width * SS, height * SS, SRCCOPY
    );

    SelectObject(mem, oldBitmap);
    DeleteObject(bitmap);
    DeleteDC(mem);

    // ---- Text on top (drawn at 1x so it stays crisp) ----
    for (const Pill& pill : pills) {
        ui::drawText(
            hdc, pill.text, pill.rect, pill.color, smallFont,
            DT_CENTER | DT_SINGLELINE | DT_VCENTER
        );
    }

    const double lineHeight = 20.0 * s;
    const double centerY = height / 2.0;

    for (const TownDraw& town : towns) {
        int cx = area.left + static_cast<int>(town.pos.x);
        int cy = area.top + static_cast<int>(town.pos.y);
        int r = static_cast<int>(radius);

        // Letter inside the circle.
        RECT letterRect = {cx - r, cy - r, cx + r, cy + r};
        ui::drawText(
            hdc, std::wstring(1, static_cast<wchar_t>(town.id)),
            letterRect, town.letter, idFont,
            DT_CENTER | DT_SINGLELINE | DT_VCENTER
        );

        // Name + distance, above the node if it is in the upper half.
        const bool above = town.pos.y < centerY - 1;
        int nameY, distY;

        if (above) {
            distY = cy - r - static_cast<int>(8 * s + lineHeight);
            nameY = distY - static_cast<int>(lineHeight);
        } else {
            nameY = cy + r + static_cast<int>(8 * s);
            distY = nameY + static_cast<int>(lineHeight);
        }

        int half = static_cast<int>(90 * s);

        RECT nameRect = {
            cx - half, nameY, cx + half, nameY + static_cast<int>(lineHeight)
        };
        ui::drawText(
            hdc, ui::widen(graph.nameOf(town.id)), nameRect,
            ui::kText, nameFont, DT_CENTER | DT_SINGLELINE | DT_VCENTER
        );

        std::wstring info;
        COLORREF infoColor = ui::kTextDim;

        if (view.showDistances) {
            auto d = view.distances.find(town.id);

            if (d == view.distances.end() || d->second == INT_MAX) {
                info = L"∞";
                infoColor = ui::kTextMuted;
            } else {
                info = std::to_wstring(d->second) + L" km";

                if (town.id == view.current)        infoColor = ui::kAmber;
                else if (contains(view.route, town.id)) infoColor = ui::kGreen;
                else                                infoColor = ui::kText;
            }
        } else if (town.id == view.source) {
            info = L"start";
            infoColor = ui::kAccent;
        }

        if (!info.empty()) {
            RECT infoRect = {
                cx - half, distY, cx + half,
                distY + static_cast<int>(lineHeight)
            };
            ui::drawText(
                hdc, info, infoRect, infoColor, smallFont,
                DT_CENTER | DT_SINGLELINE | DT_VCENTER
            );
        }
    }
}