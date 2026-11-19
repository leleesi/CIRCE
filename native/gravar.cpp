// Compatibility path for machines where the WGC/D3D11 session cannot start.
#include "capture.hpp"
#include <algorithm>
#include <cstring>

int captureGdi(Capture& target) {
    HDC source = GetDC(target.window);
    HDC destination = source ? CreateCompatibleDC(source) : nullptr;
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = target.width;
    info.bmiHeader.biHeight = -static_cast<LONG>(target.height);
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    void* pixels = nullptr;
    HBITMAP bitmap =
        destination ? CreateDIBSection(destination, &info, DIB_RGB_COLORS, &pixels, nullptr, 0) : nullptr;
    int result = 5;
    if (bitmap && pixels) {
        auto previous = SelectObject(destination, bitmap);
        SetStretchBltMode(destination, COLORONCOLOR);
        double next = 0;
        int oldWidth = 0, oldHeight = 0;
        while (true) {
            double stamp = qpcSeconds();
            if (stamp >= target.end) {
                result = target.frames ? (target.hasImage ? 0 : 8) : 6;
                break;
            }
            if (stamp < next) {
                Sleep(2);
                continue;
            }
            RECT r{};
            if (!target.validWindow() || !GetClientRect(target.window, &r) || r.right <= 0 || r.bottom <= 0) {
                result = 3;
                break;
            }
            double scale = std::min(double(target.width) / r.right, double(target.height) / r.bottom);
            int w = int(scale * r.right + .5), h = int(scale * r.bottom + .5);
            if (w != oldWidth || h != oldHeight) {
                memset(pixels, 0, size_t(target.width) * target.height * 4);
                oldWidth = w;
                oldHeight = h;
            }
            if (!StretchBlt(destination, (target.width - w) / 2, (target.height - h) / 2, w, h, source, 0, 0,
                            r.right, r.bottom, SRCCOPY) ||
                !GdiFlush()) {
                result = 7;
                break;
            }
            if (!target.write(static_cast<const unsigned char*>(pixels), target.width, target.height,
                              stamp)) {
                result = 9;
                break;
            }
            if (!next || stamp - next > 1. / target.rate)
                next = stamp;
            next += 1. / target.rate;
        }
        SelectObject(destination, previous);
    }
    if (bitmap)
        DeleteObject(bitmap);
    if (destination)
        DeleteDC(destination);
    if (source)
        ReleaseDC(target.window, source);
    return result;
}
