#pragma once

// NEW FILE: shared colours and small GDI drawing helpers.
// Header-only, so no CMake / build-command changes are needed.
// To re-skin the whole app, change only the palette below.

#include <windows.h>
#include <string>

namespace ui {

// ---------- Palette (dark "neon" theme) ----------
constexpr COLORREF kWindowBg   = RGB(10, 14, 24);
constexpr COLORREF kHeaderBg   = RGB(13, 19, 33);
constexpr COLORREF kCard       = RGB(17, 24, 39);
constexpr COLORREF kCardAlt    = RGB(24, 33, 52);
constexpr COLORREF kBorder     = RGB(35, 47, 70);
constexpr COLORREF kCanvas     = RGB(12, 17, 29);
constexpr COLORREF kGrid       = RGB(26, 36, 56);
constexpr COLORREF kLogBg      = RGB(9, 13, 22);

constexpr COLORREF kText       = RGB(226, 232, 240);
constexpr COLORREF kTextDim    = RGB(148, 163, 184);
constexpr COLORREF kTextMuted  = RGB(100, 116, 139);
constexpr COLORREF kOnAccent   = RGB(8, 12, 22);   // dark text on bright fills

constexpr COLORREF kAccent     = RGB(34, 211, 238);  // cyan   - start / primary
constexpr COLORREF kGreen      = RGB(52, 211, 153);  // green  - shortest route
constexpr COLORREF kGreenGlow  = RGB(14, 72, 58);
constexpr COLORREF kAmber      = RGB(251, 191, 36);  // amber  - current / relaxed
constexpr COLORREF kViolet     = RGB(167, 139, 250); // violet - visited
constexpr COLORREF kRed        = RGB(248, 113, 113);
constexpr COLORREF kIdleNode   = RGB(51, 65, 85);
constexpr COLORREF kIdleEdge   = RGB(71, 85, 105);

// ---------- Helpers ----------
inline std::wstring widen(const std::string& text) {
    return std::wstring(text.begin(), text.end());
}

inline void fillRect(HDC dc, const RECT& rect, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(dc, &rect, brush);
    DeleteObject(brush);
}

// Rounded rectangle. Pass CLR_INVALID as border for "no border".
inline void roundRect(
    HDC dc, const RECT& r, int radius,
    COLORREF fill, COLORREF border = CLR_INVALID, int borderWidth = 1
) {
    const bool hasBorder = border != CLR_INVALID;

    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = hasBorder
        ? CreatePen(PS_INSIDEFRAME, borderWidth, border)
        : static_cast<HPEN>(GetStockObject(NULL_PEN));

    HGDIOBJ oldBrush = SelectObject(dc, brush);
    HGDIOBJ oldPen = SelectObject(dc, pen);

    const int extra = hasBorder ? 0 : 1;   // NULL_PEN excludes right/bottom
    RoundRect(
        dc, r.left, r.top, r.right + extra, r.bottom + extra,
        radius * 2, radius * 2
    );

    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(brush);
    if (hasBorder) DeleteObject(pen);
}

// Outline only (used for keyboard-focus rings).
inline void strokeRoundRect(
    HDC dc, const RECT& r, int radius, COLORREF color, int width = 1
) {
    HPEN pen = CreatePen(PS_INSIDEFRAME, width, color);
    HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));
    HGDIOBJ oldPen = SelectObject(dc, pen);

    RoundRect(
        dc, r.left, r.top, r.right, r.bottom, radius * 2, radius * 2
    );

    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(pen);
}

inline void drawText(
    HDC dc, const std::wstring& text, RECT rect,
    COLORREF color, HFONT font,
    UINT format = DT_LEFT | DT_SINGLELINE | DT_VCENTER
) {
    HGDIOBJ old = SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    DrawTextW(
        dc, text.c_str(), static_cast<int>(text.size()),
        &rect, format | DT_NOPREFIX
    );
    SelectObject(dc, old);
}

inline int textWidth(HDC dc, HFONT font, const std::wstring& text) {
    HGDIOBJ old = SelectObject(dc, font);
    SIZE size = {};
    GetTextExtentPoint32W(
        dc, text.c_str(), static_cast<int>(text.size()), &size
    );
    SelectObject(dc, old);
    return static_cast<int>(size.cx);
}

// Height needed to draw `text` word-wrapped inside `width` pixels.
inline int wrappedHeight(
    HDC dc, HFONT font, const std::wstring& text, int width
) {
    HGDIOBJ old = SelectObject(dc, font);
    RECT r = {0, 0, width, 0};
    DrawTextW(
        dc, text.c_str(), static_cast<int>(text.size()),
        &r, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX
    );
    SelectObject(dc, old);
    return static_cast<int>(r.bottom - r.top);
}

inline void fillCircle(
    HDC dc, int cx, int cy, int radius,
    COLORREF fill, COLORREF border = CLR_INVALID, int borderWidth = 1
) {
    HBRUSH brush = fill == CLR_INVALID
        ? static_cast<HBRUSH>(GetStockObject(NULL_BRUSH))
        : CreateSolidBrush(fill);
    HPEN pen = border == CLR_INVALID
        ? static_cast<HPEN>(GetStockObject(NULL_PEN))
        : CreatePen(PS_SOLID, borderWidth, border);

    HGDIOBJ oldBrush = SelectObject(dc, brush);
    HGDIOBJ oldPen = SelectObject(dc, pen);

    Ellipse(dc, cx - radius, cy - radius, cx + radius, cy + radius);

    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    if (fill != CLR_INVALID) DeleteObject(brush);
    if (border != CLR_INVALID) DeleteObject(pen);
}

} // namespace ui