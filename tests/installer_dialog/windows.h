#pragma once
#include <cstddef>
#include <cstdint>
using HWND=void*; using HINSTANCE=void*; using UINT=unsigned; using DWORD=unsigned;
using WPARAM=std::uintptr_t; using LPARAM=std::intptr_t; using INT_PTR=std::intptr_t;
using WORD=unsigned short; using BOOL=int;
#define CALLBACK
#define WINAPI
#define __declspec(x)
#define TRUE 1
#define FALSE 0
#define WM_INITDIALOG 0x110
#define WM_COMMAND 0x111
#define WM_SETICON 0x80
#define WM_CLOSE 0x10
#define DM_SETDEFID 0x401
#define ICON_BIG 1
#define ICON_SMALL 0
#define SM_CXICON 11
#define SM_CXSMICON 49
#define IMAGE_ICON 1
#define LR_SHARED 0x8000
#define IDCANCEL 2
#define DLL_PROCESS_ATTACH 1
#define LOWORD(x) static_cast<WORD>(x)
#define MAKEINTRESOURCEW(x) reinterpret_cast<const wchar_t*>(static_cast<std::uintptr_t>(x))
HINSTANCE GetModuleHandleW(const wchar_t*);
int GetSystemMetrics(int);
void* LoadImageW(HINSTANCE,const wchar_t*,unsigned,int,int,unsigned);
INT_PTR SendMessageW(HWND,UINT,WPARAM,LPARAM);
HWND GetDlgItem(HWND,int);
HWND SetFocus(HWND);
BOOL SetForegroundWindow(HWND);
BOOL EndDialog(HWND,INT_PTR);
int lstrcmpW(const wchar_t*,const wchar_t*);
wchar_t* lstrcpynW(wchar_t*,const wchar_t*,int);
INT_PTR DialogBoxParamW(HINSTANCE,const wchar_t*,HWND,INT_PTR (*)(HWND,UINT,WPARAM,LPARAM),LPARAM);
