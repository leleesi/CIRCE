#include "platform.hpp"
#include <windows.h>
#include <evntrace.h>
#include <shobjidl.h>
#include <shlobj.h>
#include <wrl/client.h>
#include "messages.hpp"
#include <algorithm>
#include <sddl.h>
#include <cstdio>
#include <array>
#include <atomic>
#include <vector>

static HANDLE processGroup() {
    static HANDLE group = []() -> HANDLE {
        HANDLE h = CreateJobObjectW(nullptr, nullptr);
        if (!h)
            return nullptr;
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION info{};
        info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if (SetInformationJobObject(h, JobObjectExtendedLimitInformation, &info, sizeof(info)))
            return h;
        CloseHandle(h);
        return nullptr;
    }();
    return group;
}

bool platform::traceIdentity(wchar_t* name, unsigned capacity) {
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
        return false;
    DWORD size = 0, session = 0;
    GetTokenInformation(token, TokenUser, nullptr, 0, &size);
    std::vector<unsigned char> blob(size);
    bool ok = size && GetTokenInformation(token, TokenUser, blob.data(), size, &size);
    CloseHandle(token);
    wchar_t* sid = nullptr;
    if (!ok || !ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(blob.data())->User.Sid, &sid))
        return false;
    ok = ProcessIdToSessionId(GetCurrentProcessId(), &session) != FALSE;
    int len = ok ? swprintf(name, capacity, L"CIRCE.Nativo.%ls.%lu", sid, session) : -1;
    LocalFree(sid);
    return len > 0 && (unsigned)len < capacity;
}

bool platform::startChild(const wchar_t* path, wchar_t* args, const wchar_t* cwd, HANDLE input, HANDLE output,
                          HANDLE error, HANDLE* process, HANDLE* thread) {
    for (auto h : {input, output, error}) {
        if (!h || h == INVALID_HANDLE_VALUE) {
            SetLastError(ERROR_INVALID_HANDLE);
            return false;
        }
    }
    std::array<HANDLE, 3> allowed{input, output, error};
    std::sort(allowed.begin(), allowed.end(), std::less<HANDLE>{});
    const auto end = std::unique(allowed.begin(), allowed.end());
    SIZE_T size = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &size);
    if (!size)
        return false;
    std::vector<unsigned char> buffer(size);
    STARTUPINFOEXW startup{};
    startup.lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(buffer.data());
    if (!InitializeProcThreadAttributeList(startup.lpAttributeList, 1, 0, &size))
        return false;
    startup.StartupInfo.cb = sizeof(startup);
    startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    startup.StartupInfo.hStdInput = input;
    startup.StartupInfo.hStdOutput = output;
    startup.StartupInfo.hStdError = error;
    PROCESS_INFORMATION pi{};
    HANDLE group = processGroup();
    bool ok = UpdateProcThreadAttribute(startup.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                                        allowed.data(), (end - allowed.begin()) * sizeof(HANDLE), nullptr,
                                        nullptr) &&
              CreateProcessW(path, args, nullptr, nullptr, TRUE,
                             CREATE_NO_WINDOW | EXTENDED_STARTUPINFO_PRESENT | (group ? CREATE_SUSPENDED : 0),
                             nullptr, cwd, &startup.StartupInfo, &pi);
    DWORD code = ok ? 0 : GetLastError();
    DeleteProcThreadAttributeList(startup.lpAttributeList);
    if (!ok) {
        SetLastError(code);
        return false;
    }
    if (group) {
        // Nested jobs may be restricted by a host. The ordinary shutdown path still owns each handle.
        AssignProcessToJobObject(group, pi.hProcess);
        if (ResumeThread(pi.hThread) == DWORD(-1)) {
            code = GetLastError();
            TerminateProcess(pi.hProcess, 1);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            SetLastError(code);
            return false;
        }
    }
    *process = pi.hProcess;
    *thread = pi.hThread;
    return true;
}

namespace {
struct TraceProperties {
    EVENT_TRACE_PROPERTIES props{};
    wchar_t logger[1024]{}, logFile[1024]{};
    TraceProperties() {
        props.Wnode.BufferSize = sizeof(*this);
        props.Wnode.Flags = WNODE_FLAG_TRACED_GUID;
        props.LoggerNameOffset = offsetof(TraceProperties, logger);
        props.LogFileNameOffset = offsetof(TraceProperties, logFile);
    }
};
HANDLE stopEvent, worker;
wchar_t traceName[256];
std::atomic<unsigned> flushStatus;
DWORD WINAPI pump(void*) {
    while (WaitForSingleObject(stopEvent, 120) == WAIT_TIMEOUT) {
        TraceProperties props;
        flushStatus = ControlTraceW(0, traceName, &props.props, EVENT_TRACE_CONTROL_FLUSH);
    }
    return 0;
}
} // namespace

void platform::startFlush(const wchar_t* session) {
    lstrcpynW(traceName, session, 256);
    flushStatus = ERROR_WMI_INSTANCE_NOT_FOUND;
    stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (stopEvent)
        worker = CreateThread(nullptr, 0, pump, nullptr, 0, nullptr);
}
unsigned platform::lastFlushError() { return flushStatus.load(); }

void platform::stopTrace(const wchar_t* session, HANDLE collector) {
    if (stopEvent)
        SetEvent(stopEvent);
    if (worker) {
        WaitForSingleObject(worker, INFINITE);
        CloseHandle(worker);
        worker = nullptr;
    }
    if (stopEvent) {
        CloseHandle(stopEvent);
        stopEvent = nullptr;
    }
    if (!collector)
        return;
    TraceProperties props;
    ControlTraceW(0, session, &props.props, EVENT_TRACE_CONTROL_STOP);
    if (WaitForSingleObject(collector, 650) == WAIT_TIMEOUT) {
        TerminateProcess(collector, 1);
        WaitForSingleObject(collector, 1500);
        // A very early switch may race the collector starting its trace.
        TraceProperties cleanup;
        ControlTraceW(0, session, &cleanup.props, EVENT_TRACE_CONTROL_STOP);
    }
}

bool platform::firstInstance() {
    // Keep the mutex handle for the lifetime of the app.
    HANDLE lock = CreateMutexW(nullptr, FALSE, L"Local\\CIRCE.Desempenho.Instance");
    if (lock && GetLastError() != ERROR_ALREADY_EXISTS)
        return true;
    if (lock)
        CloseHandle(lock);
    HWND window = FindWindowW(L"CIRCE", nullptr);
    if (window) {
        PostMessageW(window, WM_APP + 1, 0, 0);
        SetForegroundWindow(window);
    }
    return false;
}

static const wchar_t settingsKey[] = L"Software\\CIRCE\\Desempenho";
unsigned platform::preference(const wchar_t* name, unsigned fallback, unsigned maximum) {
    DWORD value = 0, size = sizeof(value);
    auto status = RegGetValueW(HKEY_CURRENT_USER, settingsKey, name, RRF_RT_REG_DWORD, nullptr, &value, &size);
    if (status == ERROR_FILE_NOT_FOUND || status == ERROR_PATH_NOT_FOUND) {
        size = sizeof(value);
        status = RegGetValueW(HKEY_CURRENT_USER, L"Software\\Cadencia\\Desempenho", name,
                              RRF_RT_REG_DWORD, nullptr, &value, &size);
    }
    return status == ERROR_SUCCESS && value <= maximum ? value : fallback;
}
void platform::storePreference(const wchar_t* name, unsigned value) {
    RegSetKeyValueW(HKEY_CURRENT_USER, settingsKey, name, REG_DWORD, &value, sizeof(value));
}

static const wchar_t recordingFolders[] = L"Software\\CIRCE\\Desempenho\\RecordingFolders";
bool platform::defaultOutputFolder(wchar_t* path, unsigned capacity) {
    PWSTR videos = nullptr;
    const HRESULT result = SHGetKnownFolderPath(FOLDERID_Videos, 0, nullptr, &videos);
    const auto length = SUCCEEDED(result) && videos ? static_cast<unsigned>(lstrlenW(videos)) : 0;
    const bool fits = length > 0 && length < capacity;
    if (fits) CopyMemory(path, videos, (length + 1) * sizeof(wchar_t));
    CoTaskMemFree(videos);
    return fits;
}
bool platform::readOutputFolder(wchar_t* path, unsigned capacity) {
    DWORD bytes = capacity * sizeof(wchar_t);
    return RegGetValueW(HKEY_CURRENT_USER, settingsKey, L"OutputFolder", RRF_RT_REG_SZ,
                        nullptr, path, &bytes) == ERROR_SUCCESS && *path;
}
bool platform::storeOutputFolder(const wchar_t* path) {
    // A comparison may refer to clips in a folder selected before the current one.
    const DWORD allowed = 1;
    return RegSetKeyValueW(HKEY_CURRENT_USER, recordingFolders, path, REG_DWORD, &allowed,
                           sizeof(allowed)) == ERROR_SUCCESS &&
           RegSetKeyValueW(HKEY_CURRENT_USER, settingsKey, L"OutputFolder", REG_SZ, path,
                           (lstrlenW(path) + 1) * sizeof(wchar_t)) == ERROR_SUCCESS;
}
bool platform::knownOutputFolder(const wchar_t* path) {
    DWORD allowed = 0, bytes = sizeof(allowed);
    return RegGetValueW(HKEY_CURRENT_USER, recordingFolders, path, RRF_RT_REG_DWORD, nullptr,
                        &allowed, &bytes) == ERROR_SUCCESS && allowed == 1;
}

static HRESULT showOutputDialog(HWND owner, platform::OutputKind kind, const wchar_t* directory,
                                const wchar_t* suggested, wchar_t* path, unsigned capacity) {
    using Microsoft::WRL::ComPtr;
    using platform::OutputKind;
    const bool folder = kind == OutputKind::Folder;
    const bool movie = kind == OutputKind::Movie;
    ComPtr<IFileDialog> dialog;
    HRESULT hr = CoCreateInstance(folder ? CLSID_FileOpenDialog : CLSID_FileSaveDialog, nullptr,
                                  CLSCTX_INPROC_SERVER, IID_PPV_ARGS(dialog.GetAddressOf()));
    if (FAILED(hr)) return hr;
    FILEOPENDIALOGOPTIONS options{};
    if (FAILED(hr = dialog->GetOptions(&options))) return hr;
    options |= FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_NOCHANGEDIR | FOS_DONTADDTORECENT;
    if (folder) {
        options |= FOS_PICKFOLDERS;
    } else {
        // Only the completed temporary file is published. Confirm replacements in the application.
        options = (options & ~FOS_OVERWRITEPROMPT) | FOS_STRICTFILETYPES | FOS_NOREADONLYRETURN |
                  FOS_NOTESTFILECREATE;
    }
    const auto& title = folder ? words::chooseOutputFolder : (movie ? words::saveMovieAs : words::savePdfAs);
    if (FAILED(hr = dialog->SetOptions(options)) || FAILED(hr = dialog->SetTitle(title.get())))
        return hr;
    ComPtr<IShellItem> initial;
    if (SUCCEEDED(SHCreateItemFromParsingName(directory, nullptr, IID_PPV_ARGS(initial.GetAddressOf()))))
        dialog->SetFolder(initial.Get());
    if (!folder) {
        const COMDLG_FILTERSPEC format{(movie ? words::mp4FileType : words::pdfFileType).get(),
                                       movie ? L"*.mp4" : L"*.pdf"};
        if (FAILED(hr = dialog->SetFileTypes(1, &format)) ||
            FAILED(hr = dialog->SetDefaultExtension(movie ? L"mp4" : L"pdf")) ||
            FAILED(hr = dialog->SetFileName(suggested)))
            return hr;
    }
    if (FAILED(hr = dialog->Show(owner))) return hr;
    ComPtr<IShellItem> chosen;
    PWSTR name = nullptr;
    if (FAILED(hr = dialog->GetResult(chosen.GetAddressOf())) ||
        FAILED(hr = chosen->GetDisplayName(SIGDN_FILESYSPATH, &name))) return hr;
    const auto length = static_cast<unsigned>(lstrlenW(name));
    if (length && length < capacity)
        CopyMemory(path, name, (length + 1) * sizeof(wchar_t));
    else
        hr = HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
    CoTaskMemFree(name);
    return hr;
}
platform::Choice platform::chooseOutput(void* owner, OutputKind kind, const wchar_t* directory,
                                        const wchar_t* suggested, wchar_t* path, unsigned capacity) {
    HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(init)) return Choice::Failed;
    HRESULT result = showOutputDialog(static_cast<HWND>(owner), kind, directory, suggested, path, capacity);
    CoUninitialize();
    if (result == HRESULT_FROM_WIN32(ERROR_CANCELLED)) return Choice::Cancelled;
    return SUCCEEDED(result) ? Choice::Selected : Choice::Failed;
}
