#include <windows.h>
#include <initializer_list>
#include "idioma_ids.h"

namespace {
HINSTANCE module;

INT_PTR CALLBACK languageDialog(HWND dialog, UINT message, WPARAM wparam, LPARAM preferred) {
    if (message == WM_INITDIALOG) {
        // Both the installer and its uninstaller use icon group 103 in the main EXE.
        HINSTANCE exe = GetModuleHandleW(nullptr);
        for (int size : {ICON_BIG, ICON_SMALL}) {
            const int dimension = size == ICON_BIG ? SM_CXICON : SM_CXSMICON;
            auto icon = LoadImageW(exe, MAKEINTRESOURCEW(103), IMAGE_ICON,
                                   GetSystemMetrics(dimension), GetSystemMetrics(dimension), LR_SHARED);
            SendMessageW(dialog, WM_SETICON, size, reinterpret_cast<LPARAM>(icon));
        }
        SendMessageW(dialog, DM_SETDEFID, preferred, 0);
        SetForegroundWindow(dialog);
        SetFocus(GetDlgItem(dialog, static_cast<int>(preferred)));
        return FALSE;
    }
    if (message == WM_CLOSE) {
        EndDialog(dialog, IDCANCEL);
        return TRUE;
    }
    if (message == WM_COMMAND) {
        const WORD id = LOWORD(wparam);
        if (id == CIRCE_PORTUGUESE || id == CIRCE_ENGLISH || id == IDCANCEL) {
            EndDialog(dialog, id);
            return TRUE;
        }
    }
    return FALSE;
}

// The NSIS Unicode plugin stack stores a next pointer followed by a UTF-16 string.
// The caller supplies one argument and expects one result, so its node can be reused.
struct StackEntry { StackEntry* next; wchar_t text[1]; };
}

extern "C" BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void*) {
    if (reason == DLL_PROCESS_ATTACH)
        module = instance;
    return TRUE;
}

extern "C" __declspec(dllexport) void Choose(HWND parent, int capacity, wchar_t*, StackEntry** stack, void*) {
    if (!stack || !*stack || capacity < 1)
        return;
    wchar_t* result = (*stack)->text;
    const int preferred = lstrcmpW(result, L"preferred-2070") == 0 ? CIRCE_PORTUGUESE : CIRCE_ENGLISH;
    const INT_PTR choice = DialogBoxParamW(module, MAKEINTRESOURCEW(CIRCE_LANGUAGE_DIALOG), parent,
                                           languageDialog, preferred);
    const wchar_t* value = L"error";
    if (choice == CIRCE_PORTUGUESE) value = L"2070";
    if (choice == CIRCE_ENGLISH) value = L"1033";
    if (choice == IDCANCEL) value = L"cancel";
    lstrcpynW(result, value, capacity);
}
