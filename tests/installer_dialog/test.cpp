// Exercise the installer's own Win32 modal callback. Win32 calls (including clicks)
// are substituted here. The resource templates and images are checked in the actual EXE.
#include "../../instalador/idioma.cpp"
#include <cassert>
#include <cwchar>
#include <cstring>
#include <string>

static int action, defaultButton, finished, iconCount;
static HWND focused;
static bool foreground;
static HINSTANCE const plugin = reinterpret_cast<HINSTANCE>(1);
static HINSTANCE const executable = reinterpret_cast<HINSTANCE>(2);
HINSTANCE GetModuleHandleW(const wchar_t*) { return executable; }
int GetSystemMetrics(int metric) { return metric == SM_CXICON ? 32 : 16; }
void* LoadImageW(HINSTANCE instance,const wchar_t* resource,unsigned type,int w,int h,unsigned) {
    assert(instance==executable && resource==MAKEINTRESOURCEW(103) && type==IMAGE_ICON && w==h);
    return reinterpret_cast<void*>(5);
}
INT_PTR SendMessageW(HWND,UINT msg,WPARAM value,LPARAM image) {
    if (msg==WM_SETICON) { assert(image==5); ++iconCount; }
    if (msg==DM_SETDEFID) defaultButton=static_cast<int>(value);
    return 0;
}
HWND GetDlgItem(HWND,int id) { return reinterpret_cast<HWND>(std::uintptr_t(id)); }
HWND SetFocus(HWND id) { focused=id; return id; }
BOOL SetForegroundWindow(HWND) { foreground=true; return TRUE; }
BOOL EndDialog(HWND,INT_PTR result) { finished=static_cast<int>(result); return TRUE; }
int lstrcmpW(const wchar_t* a,const wchar_t* b) { return std::wcscmp(a,b); }
wchar_t* lstrcpynW(wchar_t* dst,const wchar_t* src,int capacity) {
    std::wcsncpy(dst,src,capacity-1);dst[capacity-1]=0;return dst;
}
INT_PTR DialogBoxParamW(HINSTANCE mod,const wchar_t* resource,HWND,
                        INT_PTR (*callback)(HWND,UINT,WPARAM,LPARAM),LPARAM preferred) {
    assert(mod==plugin && resource==MAKEINTRESOURCEW(CIRCE_LANGUAGE_DIALOG));
    if (action==-1) return -1;
    auto h=reinterpret_cast<HWND>(3);
    assert(callback(h,WM_INITDIALOG,0,preferred)==FALSE);
    assert(defaultButton==preferred && focused==GetDlgItem(h,defaultButton) && foreground && iconCount==2);
    assert(callback(h,WM_COMMAND,999,0)==FALSE && finished==0);
    if (!action) callback(h,WM_CLOSE,0,0);
    else callback(h,WM_COMMAND,action,0);
    return finished;
}
int main() {
    DllMain(plugin,DLL_PROCESS_ATTACH,nullptr);
    // Allocate the variable-size NSIS stack entry in the same layout as the plugin ABI.
    struct TestEntry { StackEntry* next; wchar_t text[128]; } entry{};
    static_assert(offsetof(TestEntry,text)==offsetof(StackEntry,text));
    for (auto initial : {L"preferred-1033",L"preferred-2070"}) {
        for (int click : {CIRCE_PORTUGUESE,CIRCE_ENGLISH,IDCANCEL,0,-1}) {
            action=click;defaultButton=finished=iconCount=0;focused=nullptr;foreground=false;
            std::wcscpy(entry.text,initial);
            auto top=reinterpret_cast<StackEntry*>(&entry);
            Choose(nullptr,128,nullptr,&top,nullptr);
            const wchar_t* expected=click==CIRCE_PORTUGUESE ? L"2070" : click==CIRCE_ENGLISH ? L"1033"
                                    : click==-1 ? L"error" : L"cancel";
            assert(top==reinterpret_cast<StackEntry*>(&entry) && std::wcscmp(entry.text,expected)==0);
            if (click!=-1) assert(defaultButton==(std::wcscmp(initial,L"preferred-2070")==0 ? CIRCE_PORTUGUESE : CIRCE_ENGLISH));
        }
    }
}
