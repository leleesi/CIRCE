#pragma once
// Plain handles keep the portable session reader independent of Windows headers.
namespace platform {
bool startChild(const wchar_t* path, wchar_t* args, const wchar_t* cwd, void* input, void* output,
                void* error, void** process, void** thread);
void startFlush(const wchar_t* session);
void stopTrace(const wchar_t* session, void* collector);
unsigned lastFlushError();
bool firstInstance();
unsigned preference(const wchar_t* name, unsigned fallback, unsigned maximum);
void storePreference(const wchar_t* name, unsigned value);
bool traceIdentity(wchar_t* name, unsigned capacity);

enum class OutputKind { Folder, Movie, Pdf };
enum class Choice { Selected, Cancelled, Failed };
Choice chooseOutput(void* owner, OutputKind kind, const wchar_t* directory, const wchar_t* suggested,
                    wchar_t* path, unsigned capacity);
bool defaultOutputFolder(wchar_t* path, unsigned capacity);
bool readOutputFolder(wchar_t* path, unsigned capacity);
bool storeOutputFolder(const wchar_t* path);
bool knownOutputFolder(const wchar_t* path);
} // namespace platform
