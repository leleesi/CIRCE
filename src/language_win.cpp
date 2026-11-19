#include "language.hpp"
#include <windows.h>
#include <string>

namespace {
unsigned installedLanguage() {
    wchar_t image[32768]{};
    DWORD length = GetModuleFileNameW(nullptr, image, 32768);
    if (!length || length >= 32768)
        return 0;
    std::wstring path(image, length);
    path.resize(path.find_last_of(L"\\/") + 1);
    path += L"circe.lang";
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return 0;
    char data[5]{};
    DWORD size = 0;
    bool ok = ReadFile(file, data, sizeof(data), &size, nullptr) != FALSE;
    CloseHandle(file);
    return ok ? language::tag(data, size) : 0;
}

unsigned savedLanguage() {
    constexpr auto key = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\CIRCE";
    wchar_t value[8]{};
    DWORD length = sizeof(value);
    if (RegGetValueW(HKEY_CURRENT_USER, key, L"InstallerLanguage", RRF_RT_REG_SZ,
                     nullptr, value, &length) != ERROR_SUCCESS)
        return 0;
    if (lstrcmpW(value, L"1033") == 0)
        return static_cast<unsigned>(language::Id::English);
    if (lstrcmpW(value, L"2070") == 0)
        return static_cast<unsigned>(language::Id::Portuguese);
    return 0;
}
} // namespace

void language::initialize() {
    active = select(installedLanguage(), savedLanguage(), GetUserDefaultUILanguage());
    SetThreadUILanguage(static_cast<LANGID>(active));
}
