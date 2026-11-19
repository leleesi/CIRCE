#include "layout.hpp"
#include <windows.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <algorithm>
#include <cmath>
#include <string>
#include <array>

namespace panelui {
namespace {
constexpr COLORREF background = RGB(15, 20, 29), surface = RGB(25, 33, 46), well = RGB(13, 20, 30),
                   stroke = RGB(54, 68, 85), ink = RGB(239, 243, 249), muted = RGB(175, 188, 207),
                   mint = RGB(123, 235, 190), violet = RGB(196, 177, 255), subtle = RGB(35, 47, 63);
HWND panel, hud;
HWND controls[Total]{};
Page page = Session;
double scale = 1, hudScale = 1;
HFONT fonts[5]{}, hudFont;
HBRUSH bgBrush, cardBrush, wellBrush;
std::wstring hudCaption;
bool hudFresh, allowCancel;
RECT hudRect{};
int hudTextWidth = 0;

int px(int x) { return int(std::lround(x * scale)); }
RECT rect(Box b) { return {px(b.x), px(b.y), px(b.x + b.w), px(b.y + b.h)}; }
void fill(HDC dc, RECT r, COLORREF color) {
    SetDCBrushColor(dc, color);
    FillRect(dc, &r, (HBRUSH)GetStockObject(DC_BRUSH));
}
void roundBox(HDC dc, RECT r, COLORREF color, COLORREF border, int radius) {
    auto brush = SelectObject(dc, GetStockObject(DC_BRUSH));
    auto pen = SelectObject(dc, GetStockObject(DC_PEN));
    SetDCBrushColor(dc, color);
    SetDCPenColor(dc, border);
    RoundRect(dc, r.left, r.top, r.right, r.bottom, radius, radius);
    SelectObject(dc, pen);
    SelectObject(dc, brush);
}
const Item* describe(unsigned id) {
    for (auto& item : items)
        if (item.id == (Id)id)
            return &item;
    return nullptr;
}
int fontIndex(Kind k) {
    if (k == Section)
        return 2;
    if (k == Readout)
        return 3;
    if (k == Small)
        return 4;
    if (k == Tab || k == Primary)
        return 1;
    return 0;
}
HFONT font(int height, int weight, const wchar_t* face = L"Segoe UI") {
    return CreateFontW(-height, 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                       CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, face);
}
void arrange() {
    auto old = std::to_array(fonts);
    fonts[0] = font(px(15), FW_NORMAL);
    fonts[1] = font(px(15), FW_SEMIBOLD);
    fonts[2] = font(px(22), FW_SEMIBOLD, L"Segoe UI Variable Display");
    fonts[3] = font(px(38), FW_BOLD, L"Segoe UI Variable Display");
    fonts[4] = font(px(12), FW_SEMIBOLD);
    for (auto& item : items) {
        HWND h = controls[item.id];
        RECT r = rect(item.box);
        SendMessageW(h, WM_SETFONT, (WPARAM)fonts[fontIndex(item.kind)], FALSE);
        int height = r.bottom - r.top;
        if (item.kind == Combo) {
            SendMessageW(h, CB_SETITEMHEIGHT, -1, px(21));
            SendMessageW(h, CB_SETITEMHEIGHT, 0, px(24));
            height = px(item.id == Games ? 230 : 135);
        }
        SetWindowPos(h, nullptr, r.left, r.top, r.right - r.left, height, SWP_NOZORDER | SWP_NOACTIVATE);
        if (item.kind == Input)
            SendMessageW(h, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(px(7), px(6)));
    }
    for (auto f : old)
        if (f)
            DeleteObject(f);
    InvalidateRect(panel, nullptr, TRUE);
}
void select(Page dest) {
    page = dest;
    for (auto& item : items)
        ShowWindow(controls[item.id],
                   (item.page == Common || item.page == dest) && (item.id != Cancel || allowCancel)
                       ? SW_SHOWNA
                       : SW_HIDE);
    for (Id tab : {TabSession, TabMeasure, TabResults})
        InvalidateRect(controls[tab], nullptr, FALSE);
    InvalidateRect(panel, nullptr, TRUE);
}
void sizeAt(RECT anchor, RECT work, UINT dpi, bool centered) {
    const UINT desired = dpi ? dpi : 96;
    RECT margins{};
    auto style = (DWORD)GetWindowLongPtrW(panel, GWL_STYLE);
    AdjustWindowRectExForDpi(&margins, style, FALSE, 0, desired);
    int dw = margins.right - margins.left, dh = margins.bottom - margins.top;
    const double next = std::max(.35, std::min({desired / 96., (work.right - work.left - dw - 8.) / pageW,
                                                (work.bottom - work.top - dh - 8.) / pageH}));
    bool changed = std::abs(scale - next) > .001 || !fonts[0];
    scale = next;
    int w = px(pageW) + dw, h = px(pageH) + dh;
    int x = centered ? int((anchor.left + anchor.right - w) / 2) : int(anchor.left);
    int y = centered ? int((anchor.top + anchor.bottom - h) / 2) : int(anchor.top);
    x = std::clamp(x, int(work.left), std::max(int(work.left), int(work.right) - w));
    y = std::clamp(y, int(work.top), std::max(int(work.top), int(work.bottom) - h));
    SetWindowPos(panel, nullptr, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
    if (changed)
        arrange();
}
void drawButton(const DRAWITEMSTRUCT& d, const Item& item) {
    wchar_t caption[128]{};
    GetWindowTextW(d.hwndItem, caption, 128);
    RECT r = d.rcItem;
    const bool disabled = d.itemState & ODS_DISABLED, pressed = d.itemState & ODS_SELECTED;
    const bool active = (item.id == TabSession && page == Session) ||
                        (item.id == TabMeasure && page == Measure) ||
                        (item.id == TabResults && page == Results);
    COLORREF bg = disabled  ? surface
                  : pressed ? stroke
                            : subtle,
             fg = disabled ? RGB(140, 151, 167) : ink, line = stroke;
    bool tab = item.kind == Tab;
    if (tab) {
        bg = active ? subtle : background;
        line = bg;
        fg = active ? mint : muted;
    } else if (item.kind == Primary && !disabled) {
        bg = pressed ? RGB(103, 198, 161) : mint;
        line = bg;
        fg = well;
    } else if (item.id == After && !disabled) {
        line = RGB(123, 106, 157);
        fg = violet;
    }
    fill(d.hDC, r, (tab || item.page == Common || item.id == Cancel) ? background : surface);
    roundBox(d.hDC, r, bg, line, px(9));
    if (active) {
        RECT rail = r;
        rail.top = r.bottom - px(2);
        rail.left += px(13);
        rail.right -= px(13);
        fill(d.hDC, rail, mint);
    }
    SelectObject(d.hDC, fonts[fontIndex(item.kind)]);
    SetBkMode(d.hDC, TRANSPARENT);
    SetTextColor(d.hDC, fg);
    DrawTextW(d.hDC, caption, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    if ((d.itemState & ODS_FOCUS) && !(d.itemState & ODS_NOFOCUSRECT)) {
        InflateRect(&r, -px(4), -px(4));
        DrawFocusRect(d.hDC, &r);
    }
}
void drawCombo(const DRAWITEMSTRUCT& d) {
    RECT r = d.rcItem;
    bool selected = (d.itemState & ODS_SELECTED) && !(d.itemState & ODS_COMBOBOXEDIT);
    fill(d.hDC, r, selected ? subtle : well);
    std::wstring label;
    if (d.itemID != UINT(-1)) {
        LRESULT n = SendMessageW(d.hwndItem, CB_GETLBTEXTLEN, d.itemID, 0);
        if (n >= 0 && n < 2048) {
            label.resize(n + 1);
            SendMessageW(d.hwndItem, CB_GETLBTEXT, d.itemID, (LPARAM)label.data());
        }
    }
    r.left += px(7);
    SelectObject(d.hDC, fonts[0]);
    SetTextColor(d.hDC, selected ? mint : ink);
    SetBkMode(d.hDC, TRANSPARENT);
    DrawTextW(d.hDC, label.c_str(), -1, &r, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
    if ((d.itemState & ODS_FOCUS) && !(d.itemState & ODS_NOFOCUSRECT))
        DrawFocusRect(d.hDC, &r);
}
void paint(HWND h) {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(h, &ps);
    fill(dc, ps.rcPaint, background);
    for (auto card : cards)
        if (card.page == page)
            roundBox(dc, rect(card.box), surface, stroke, px(13));
    auto icon = (HICON)GetClassLongPtrW(h, GCLP_HICON);
    if (icon)
        DrawIconEx(dc, px(20), px(13), icon, px(34), px(34), 0, nullptr, DI_NORMAL);
    fill(dc, rect({20, 439, 684, 1}), stroke);
    fill(dc, rect({20, 456, 3, 20}), mint);
    if (page == Session) {
        fill(dc, rect({20, 279, 3, 23}), mint);
        fill(dc, rect({371, 279, 3, 23}), violet);
    }
    if (page == Results) {
        fill(dc, rect({243, 166, 1, 70}), stroke);
        fill(dc, rect({463, 166, 1, 70}), stroke);
    }
    EndPaint(h, &ps);
}

LRESULT CALLBACK hudProc(HWND h, UINT message, WPARAM w, LPARAM l) {
    if (message == WM_NCHITTEST)
        return HTTRANSPARENT;
    if (message == WM_MOUSEACTIVATE)
        return MA_NOACTIVATE;
    if (message == WM_ERASEBKGND)
        return 1;
    if (message == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(h, &ps);
        RECT r;
        GetClientRect(h, &r);
        fill(dc, r, background);
        auto bar = r;
        bar.right = std::max(2L, LONG(3 * hudScale));
        fill(dc, bar, hudFresh ? mint : violet);
        r.left += int(12 * hudScale);
        r.right -= int(8 * hudScale);
        SelectObject(dc, hudFont);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, ink);
        DrawTextW(dc, hudCaption.c_str(), -1, &r, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
        EndPaint(h, &ps);
        return 0;
    }
    return DefWindowProcW(h, message, w, l);
}
} // namespace

bool confirm(void* owner, const wchar_t* title, const wchar_t* message, const wchar_t* accept,
             const wchar_t* dismiss, bool acceptByDefault) {
    constexpr int yes = 1001, no = 1002;
    TASKDIALOG_BUTTON buttons[] = {{yes, accept}, {no, dismiss}};
    TASKDIALOGCONFIG dialog{};
    dialog.cbSize = sizeof(dialog);
    dialog.hwndParent = static_cast<HWND>(owner);
    dialog.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW;
    dialog.pszWindowTitle = title;
    dialog.pszContent = message;
    dialog.pButtons = buttons;
    dialog.cButtons = 2;
    dialog.nDefaultButton = acceptByDefault ? yes : no;
    int selected = 0;
    if (SUCCEEDED(TaskDialogIndirect(&dialog, &selected, nullptr, nullptr)))
        return selected == yes;
    UINT flags = MB_YESNO | MB_ICONQUESTION | (acceptByDefault ? MB_DEFBUTTON1 : MB_DEFBUTTON2);
    return MessageBoxExW(dialog.hwndParent, message, title, flags, static_cast<LANGID>(language::active)) == IDYES;
}

void* create(void* module, Callback callback) {
    auto inst = (HINSTANCE)module;
    bgBrush = CreateSolidBrush(background);
    wellBrush = CreateSolidBrush(well);
    cardBrush = CreateSolidBrush(surface);
    WNDCLASSW cls{};
    cls.lpszClassName = L"CIRCE";
    cls.hInstance = inst;
    cls.hbrBackground = bgBrush;
    cls.lpfnWndProc = reinterpret_cast<WNDPROC>(callback);
    cls.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    cls.hIcon = LoadIconW(inst, MAKEINTRESOURCEW(1));
    if (!RegisterClassW(&cls))
        return nullptr;
    panel = CreateWindowExW(0, cls.lpszClassName, L"CIRCE",
                            WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN, CW_USEDEFAULT,
                            CW_USEDEFAULT, pageW, pageH, nullptr, nullptr, inst, nullptr);
    if (!panel)
        return nullptr;
    BOOL dark = TRUE;
    DwmSetWindowAttribute(panel, 20, &dark, sizeof(dark));
    DwmSetWindowAttribute(panel, 35, &background, sizeof(background));
    for (auto item : items) {
        const wchar_t* type = L"STATIC";
        DWORD style = WS_CHILD, ex = 0;
        switch (item.kind) {
        case Tab:
        case Button:
        case Primary:
            type = L"BUTTON";
            style |= WS_TABSTOP | BS_OWNERDRAW;
            break;
        case Input:
            type = L"EDIT";
            style |= WS_TABSTOP | ES_AUTOHSCROLL | WS_BORDER;
            break;
        case Combo:
            type = L"COMBOBOX";
            style |= WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST | CBS_HASSTRINGS | CBS_OWNERDRAWFIXED;
            break;
        case Check:
            type = L"BUTTON";
            style |= WS_TABSTOP | BS_AUTOCHECKBOX;
            break;
        default:
            style |= SS_NOPREFIX;
            if (item.id == OutputPath) style |= SS_PATHELLIPSIS;
            break;
        }
        HWND child = CreateWindowExW(ex, type, item.caption.get(), style, 0, 0, 16, 16, panel,
                                     (HMENU)(UINT_PTR)item.id, inst, nullptr);
        if (!child)
            return nullptr;
        controls[item.id] = child;
        if (item.kind == Combo)
            SetWindowTheme(child, L"DarkMode_CFD", nullptr);
        if (item.kind == Check)
            SetWindowTheme(child, L"", L"");
        if (item.kind == Input)
            SendMessageW(child, EM_SETLIMITTEXT, 79, 0);
    }
    for (auto s : {words::seconds15.get(), words::seconds30.get(), words::seconds60.get()})
        SendMessageW(controls[Duration], CB_ADDSTRING, 0, (LPARAM)s);
    for (auto s : {L"720p · 30 FPS", L"1080p · 30 FPS", L"1080p · 60 FPS"})
        SendMessageW(controls[Profile], CB_ADDSTRING, 0, (LPARAM)s);
    SendMessageW(controls[Duration], CB_SETCURSEL, 1, 0);
    SendMessageW(controls[Profile], CB_SETCURSEL, 0, 0);
    check(ShowFps, true);
    cls.lpfnWndProc = hudProc;
    cls.lpszClassName = L"CIRCEOverlay";
    RegisterClassW(&cls);
    hud = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        cls.lpszClassName, L"CIRCE FPS", WS_POPUP, 0, 0, 100, 30, nullptr, nullptr, inst, nullptr);
    if (!hud)
        return nullptr;
    SetLayeredWindowAttributes(hud, 0, 241, LWA_ALPHA);
    SetWindowDisplayAffinity(hud, WDA_EXCLUDEFROMCAPTURE);
    select(Session);
    position(nullptr);
    return panel;
}

void* control(Id id) { return controls[id]; }
void set(Id id, const wchar_t* text) {
    if (!controls[id])
        return;
    wchar_t previous[520]{};
    GetWindowTextW(controls[id], previous, 520);
    if (lstrcmpW(previous, text))
        SetWindowTextW(controls[id], text);
}
void enable(Id id, bool enabled) {
    if (id == Cancel && allowCancel != enabled) {
        allowCancel = enabled;
        ShowWindow(controls[id], enabled && page == Session ? SW_SHOWNA : SW_HIDE);
    }
    if (controls[id] && bool(IsWindowEnabled(controls[id])) != enabled)
        EnableWindow(controls[id], enabled);
}
bool checked(Id id) { return SendMessageW(controls[id], BM_GETCHECK, 0, 0) == BST_CHECKED; }
void check(Id id, bool value) {
    SendMessageW(controls[id], BM_SETCHECK, value ? BST_CHECKED : BST_UNCHECKED, 0);
}
void position(void* target) {
    HWND game = (HWND)target;
    MONITORINFO info{};
    info.cbSize = sizeof(info);
    if (!GetMonitorInfoW(MonitorFromWindow(game ? game : panel, MONITOR_DEFAULTTONEAREST), &info))
        return;
    RECT anchor = info.rcWork;
    if (game && !IsIconic(game))
        GetWindowRect(game, &anchor);
    sizeAt(anchor, info.rcWork, GetDpiForWindow(game ? game : panel), true);
}
bool handle(void* window, unsigned message, unsigned long long w, long long l, long long* result) {
    HWND h = (HWND)window;
    *result = 0;
    if (message == WM_ERASEBKGND) {
        *result = 1;
        return true;
    }
    if (message == WM_PAINT) {
        paint(h);
        return true;
    }
    if (message == WM_MEASUREITEM) {
        auto m = (MEASUREITEMSTRUCT*)l;
        if (m->CtlType == ODT_COMBOBOX) {
            m->itemHeight = px(24);
            *result = TRUE;
            return true;
        }
    }
    if (message == WM_DRAWITEM) {
        auto d = (DRAWITEMSTRUCT*)l;
        auto item = describe(d->CtlID);
        if (!item)
            return false;
        if (item->kind == Combo)
            drawCombo(*d);
        else
            drawButton(*d, *item);
        *result = TRUE;
        return true;
    }
    if (message == WM_DPICHANGED && panel && controls[Wordmark]) {
        RECT anchor = *(RECT*)l;
        MONITORINFO info{};
        info.cbSize = sizeof(info);
        if (GetMonitorInfoW(MonitorFromRect(&anchor, MONITOR_DEFAULTTONEAREST), &info))
            sizeAt(anchor, info.rcWork, HIWORD(w), false);
        return true;
    }
    if (message == WM_CTLCOLORSTATIC || message == WM_CTLCOLOREDIT || message == WM_CTLCOLORLISTBOX ||
        message == WM_CTLCOLORBTN) {
        auto item = describe(GetDlgCtrlID((HWND)l));
        auto dc = (HDC)w;
        bool input =
            message == WM_CTLCOLOREDIT || message == WM_CTLCOLORLISTBOX || (item && item->kind == Input);
        bool back = !item || item->page == Common || item->id == TitleDuration;
        COLORREF fg = ink;
        if (item && (item->kind == Small || item->kind == Body))
            fg = muted;
        if (item && (item->id == LiveValue || item->id == TitleA || item->id == TitleResA))
            fg = mint;
        if (item && (item->id == TitleB || item->id == TitleResB))
            fg = violet;
        SetTextColor(dc, fg);
        SetBkColor(dc, input ? well : back ? background : surface);
        *result = (LRESULT)(input ? wellBrush : back ? bgBrush : cardBrush);
        return true;
    }
    if (message == WM_COMMAND && LOWORD(w) >= TabSession && LOWORD(w) <= TabResults) {
        select(LOWORD(w) == TabSession ? Session : LOWORD(w) == TabMeasure ? Measure : Results);
        return true;
    }
    return false;
}
void hideHud() {
    if (hud && IsWindowVisible(hud))
        ShowWindow(hud, SW_HIDE);
}
void displayHud(void* target, const wchar_t* text, bool fresh) {
    HWND game = (HWND)target;
    RECT client{};
    POINT origin{};
    if (!game || !GetClientRect(game, &client) || !ClientToScreen(game, &origin)) {
        hideHud();
        return;
    }
    double dpi = GetDpiForWindow(hud) / 96.;
    if (!IsWindowVisible(hud))
        dpi = GetDpiForWindow(game) / 96.;
    if (dpi <= 0)
        dpi = 1;
    bool changed = hudCaption != text || hudFresh != fresh || hudScale != dpi || !hudFont;
    if (!hudFont || hudScale != dpi) {
        if (hudFont)
            DeleteObject(hudFont);
        hudFont = font(int(16 * dpi + .5), FW_SEMIBOLD);
        hudScale = dpi;
    }
    hudFresh = fresh;
    hudCaption = text;
    if (changed || !IsWindowVisible(hud)) {
        HDC dc = GetDC(hud);
        auto old = SelectObject(dc, hudFont);
        SIZE s{};
        GetTextExtentPoint32W(dc, text, hudCaption.size(), &s);
        SelectObject(dc, old);
        ReleaseDC(hud, dc);
        hudTextWidth = s.cx + int(23 * dpi);
        SetWindowTextW(hud, text);
    }
    int pad = int(12 * dpi), height = int(32 * dpi);
    int width = std::min(hudTextWidth, int(client.right) - 2 * pad);
    if (width < 20 || client.bottom < height + pad) {
        hideHud();
        return;
    }
    RECT r{origin.x + pad, origin.y + pad, origin.x + pad + width, origin.y + pad + height};
    if (!EqualRect(&r, &hudRect) || !IsWindowVisible(hud)) {
        SetWindowPos(hud, HWND_TOPMOST, r.left, r.top, width, height, SWP_NOACTIVATE | SWP_SHOWWINDOW);
        hudRect = r;
        changed = true;
    }
    if (changed)
        InvalidateRect(hud, nullptr, FALSE);
}
void close() {
    if (hud)
        DestroyWindow(hud);
    if (panel)
        DestroyWindow(panel);
    for (auto f : fonts)
        if (f)
            DeleteObject(f);
    if (hudFont)
        DeleteObject(hudFont);
    DeleteObject(bgBrush);
    DeleteObject(wellBrush);
    DeleteObject(cardBrush);
}
} // namespace panelui
