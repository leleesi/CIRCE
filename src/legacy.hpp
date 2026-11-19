// Read-only access to the previous SQLite history using Windows' own SQLite library.
static void legacyHistory() {
    H lib = LoadLibraryExW(L"winsqlite3.dll", nullptr, 0x800);
    if (!lib) {
        say(words::sqliteUnavailable.get());
        return;
    }
    using Db = void*;
    auto open = (int (*)(const char*, Db*, int, const char*))GetProcAddress(lib, "sqlite3_open_v2");
    auto prepare =
        (int (*)(Db, const char*, int, Db*, const char**))GetProcAddress(lib, "sqlite3_prepare_v2");
    auto step = (int (*)(Db))GetProcAddress(lib, "sqlite3_step");
    auto column = (const W* (*)(Db, int))GetProcAddress(lib, "sqlite3_column_text16");
    auto finalize = (int (*)(Db))GetProcAddress(lib, "sqlite3_finalize");
    auto close = (int (*)(Db))GetProcAddress(lib, "sqlite3_close");
    Text<2048> path;
    W local[1400]{};
    GetEnvironmentVariableW(L"LOCALAPPDATA", local, 1400);
    path.add(local);
    path.add(L"\\Cadencia\\historico.sqlite3");
    char utf[6144];
    extern int pathUtf8(const W*, char*, int);
    Db db = nullptr, stmt = nullptr;
    Text<4096> text;
    if (open && prepare && step && column && finalize && close && pathUtf8(path.buf, utf, sizeof(utf)) &&
        open(utf, &db, 1, nullptr) == 0) {
        const char* sql = "SELECT titleA,titleB,game,seconds FROM pairs ORDER BY created DESC LIMIT 25";
        if (prepare(db, sql, -1, &stmt, nullptr) == 0) {
            while (step(stmt) == 100) {
                for (int i = 0; i < 4; ++i) {
                    const W* v = column(stmt, i);
                    if (v)
                        text.add(v);
                    if (i < 3)
                        text.add(L"   /   ");
                }
                text.add(L" s\r\n");
            }
        }
    }
    if (stmt)
        finalize(stmt);
    if (db)
        close(db);
    FreeLibrary(lib);
    MessageBoxW(panel, text.len ? text.buf : words::noHistory.get(),
                words::historyTitle.get(), 0);
}
int pathUtf8(const W* s, char* out, int size) {
    return WideCharToMultiByte(65001, 0, s, -1, out, size, nullptr, nullptr);
}
