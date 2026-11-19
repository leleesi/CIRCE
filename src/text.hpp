#pragma once
#include "language.hpp"
template <unsigned N> struct Text {
    wchar_t buf[N]{};
    unsigned len = 0;
    void clear() {
        len = 0;
        buf[0] = 0;
    }
    void add(const wchar_t* p) {
        while (*p && len + 1 < N)
            buf[len++] = *p++;
        buf[len] = 0;
    }
    void number(unsigned long long x) {
        wchar_t tmp[28];
        unsigned n = 0;
        do {
            tmp[n++] = L'0' + x % 10;
            x /= 10;
        } while (x);
        while (n && len + 1 < N)
            buf[len++] = tmp[--n];
        buf[len] = 0;
    }
    void decimal(double v) {
        if (!(v >= 0 && v <= 1e9)) {
            add(L"—");
            return;
        }
        auto x = (unsigned long long)(v * 10 + .5);
        number(x / 10);
        add(language::english() ? L"." : L",");
        number(x % 10);
    }
    void quote(const wchar_t* p) {
        add(L"\"");
        add(p);
        add(L"\"");
    }
};
