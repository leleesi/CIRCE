#pragma once
#include <windows.h>
#include <wrl/client.h>
#include <cstdio>
#include <vector>

using Microsoft::WRL::ComPtr;
struct Capture {
    HWND window{};
    DWORD pid{};
    unsigned rate{}, width{}, height{};
    double end{};
    FILE* journal{};
    unsigned frames{};
    bool hasImage{};
    bool validWindow() const {
        DWORD owner = 0;
        return GetWindowThreadProcessId(window, &owner) && owner == pid && !IsIconic(window);
    }
    bool write(const unsigned char* pixels, unsigned w, unsigned h, double stamp);
};
inline double qpcSeconds() {
    static const double frequency = [] {
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        return double(f.QuadPart);
    }();
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return t.QuadPart / frequency;
}
inline void check(HRESULT hr) {
    if (FAILED(hr))
        throw hr;
}
int captureGdi(Capture& session);
