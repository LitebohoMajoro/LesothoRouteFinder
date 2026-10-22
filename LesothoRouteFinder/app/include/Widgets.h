#pragma once

// NEW FILE: owner-drawn button + font helpers shared by the app shell and
// the hashing page. They reuse the palette from the Dijkstra page's
// Theme.h (dijkstra/include/Theme.h) so every page looks identical.

#include <windows.h>
#include <algorithm>

#include "Theme.h"

namespace widgets {

enum class Kind { Secondary, Primary, Success, Warning, Danger, Chip, ChipSelected };

inline COLORREF lighten(COLORREF c, int amount) {
    return RGB(
        (std::min)(255, GetRValue(c) + amount),
        (std::min)(255, GetGValue(c) + amount),
        (std::min)(255, GetBValue(c) + amount)
    );
}

inline COLORREF darken(COLORREF c, int amount) {
    return RGB(
        (std::max)(0, GetRValue(c) - amount),
        (std::max)(0, GetGValue(c) - amount),
        (std::max)(0, GetBValue(c) - amount)
    );
}

inline HFONT makeFont(int pixelHeight, int weight, const wchar_t* face) {
    return CreateFontW(
        -pixelHeight, 0, 0, 0, weight,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, face
    );
}

// Tracks mouse hover (so the button can repaint) and shows a hand cursor.
inline LRESULT CALLBACK buttonProc(
    HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam
) {
    auto original = reinterpret_cast<WNDPROC>(GetPropW(hwnd, L"rf.orig"));

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

inline HWND makeButton(
    HWND parent, HINSTANCE instance, int id, const wchar_t* title
) {
    HWND button = CreateWindowExW(
        0, L"BUTTON", title,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        0, 0, 100, 34,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        instance, nullptr
    );

    WNDPROC original = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(
            button, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(buttonProc)
        )
    );
    SetPropW(button, L"rf.orig", reinterpret_cast<HANDLE>(original));

    return button;
}

// Paints one owner-drawn button. `surface` is the colour behind it
// (used so the rounded corners blend in).
inline void paintButton(
    const DRAWITEMSTRUCT* item, Kind kind,
    HFONT font, int radius, COLORREF surface
) {
    HDC dc = item->hDC;
    RECT r = item->rcItem;

    const bool pressed = (item->itemState & ODS_SELECTED) != 0;
    const bool hover = GetPropW(item->hwndItem, L"rf.hover") != nullptr;
    const bool focused = (item->itemState & ODS_FOCUS) != 0 &&
                         (item->itemState & ODS_NOFOCUSRECT) == 0;

    wchar_t label[64] = {};
    GetWindowTextW(item->hwndItem, label, 64);

    COLORREF fill = ui::kCardAlt;
    COLORREF border = ui::kBorder;
    COLORREF text = ui::kText;

    switch (kind) {
        case Kind::Primary:
            fill = ui::kAccent; border = CLR_INVALID; text = ui::kOnAccent;
            if (hover) fill = lighten(fill, 28);
            break;
        case Kind::Success:
            fill = ui::kGreen; border = CLR_INVALID; text = ui::kOnAccent;
            if (hover) fill = lighten(fill, 24);
            break;
        case Kind::Warning:
            fill = ui::kAmber; border = CLR_INVALID; text = ui::kOnAccent;
            if (hover) fill = lighten(fill, 24);
            break;
        case Kind::ChipSelected:
            fill = RGB(10, 54, 70); border = ui::kAccent; text = ui::kAccent;
            break;
        case Kind::Chip:
            if (hover) border = ui::kTextMuted;
            break;
        case Kind::Danger:
            if (hover) { border = ui::kRed; text = ui::kRed; fill = lighten(fill, 6); }
            break;
        case Kind::Secondary:
        default:
            if (hover) { border = ui::kAccent; fill = lighten(fill, 8); }
            break;
    }

    if (pressed) fill = darken(fill, 22);

    ui::fillRect(dc, r, surface);
    ui::roundRect(dc, r, radius, fill, border, 1);

    RECT textRect = r;
    if (pressed) OffsetRect(&textRect, 0, 1);

    ui::drawText(
        dc, label, textRect, text, font,
        DT_CENTER | DT_SINGLELINE | DT_VCENTER
    );

    if (focused) {
        RECT ring = r;
        InflateRect(&ring, -3, -3);
        ui::strokeRoundRect(dc, ring, radius - 2, ui::kText, 1);
    }
}

} // namespace widgets