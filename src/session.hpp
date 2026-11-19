struct Saved {
    U version;
    W game[2048], names[2][80], paths[2][2048], framePaths[2][2048];
    double origins[2];
    Summary summary[2];
    U duration;
};
static Saved saved;

static bool writeAll(H file, const void* source, U bytes) {
    auto p = static_cast<const unsigned char*>(source);
    while (bytes) {
        U wrote = 0;
        if (!WriteFile(file, p, bytes, &wrote, nullptr) || !wrote)
            return false;
        bytes -= wrote;
        p += wrote;
    }
    return true;
}
static Text<2048> dataFile(const W* prefix, const W* extension) {
    Text<2048> path;
    joinPath(data, timedName(prefix, extension).buf, path);
    return path;
}
static bool writeFrames(const Text<2048>& path) {
    H file = CreateFileW(path.buf, 0x40000000, 0, nullptr, 1, 0, nullptr);
    if (file == bad)
        return false;
    Frame batch[512];
    U n = 0;
    bool ok = true;
    for (U i = 0; i < count && ok; ++i) {
        const auto& f = frames[i];
        if (f.end > finish || f.end - f.ms * .001 < begin - 1e-8)
            continue;
        batch[n++] = {f.end - begin, f.ms};
        if (n == 512) {
            ok = writeAll(file, batch, sizeof(batch));
            n = 0;
        }
    }
    if (ok && n)
        ok = writeAll(file, batch, n * sizeof(Frame));
    if (ok)
        ok = FlushFileBuffers(file) != 0;
    CloseHandle(file);
    if (!ok)
        DeleteFileW(path.buf);
    return ok;
}
static bool save(int index, Summary result) {
    Saved next{};
    Text<2048> destination = pairPath;
    if (index == 1)
        next = saved;
    bool updateExisting = index == 1 && !saved.summary[1].n;
    if (!updateExisting)
        destination = dataFile(words::comparisonFilename.get(), L".cad");
    next.version = 1;
    next.summary[index] = result;
    next.duration = U(finish - begin + .5);
    memcpy(next.game, gameExe, sizeof(gameExe));
    GetWindowTextW(index == 0 ? labelA : labelB, next.names[index], 80);
    if (!next.names[index][0]) {
        const W* defaultName = index ? words::after.get() : words::before.get();
        memcpy(next.names[index], defaultName, (lstrlenW(defaultName) + 1) * sizeof(W));
    }
    if (!index)
        GetWindowTextW(labelB, next.names[1], 80);
    auto path = dataFile(L"sessao-", L".frames");
    if (!writeFrames(path))
        return false;
    memcpy(next.framePaths[index], path.buf, (path.len + 1) * sizeof(W));
    next.origins[index] = begin;
    next.paths[index][0] = 0;
    if (filming)
        memcpy(next.paths[index], pending.buf, (pending.len + 1) * sizeof(W));

    // Commit the manifest last. Previous comparisons remain reopenable if a write fails.
    Text<2048> temp = destination;
    temp.add(L".tmp-");
    temp.number(GetCurrentProcessId());
    H file = CreateFileW(temp.buf, 0x40000000, 0, nullptr, 1, 0, nullptr);
    bool ok = file != bad;
    if (ok) {
        ok = writeAll(file, &next, sizeof(next)) && FlushFileBuffers(file);
        CloseHandle(file);
        if (ok)
            ok = MoveFileExW(temp.buf, destination.buf, 8 | (updateExisting ? 1 : 0)) != 0;
        if (!ok)
            DeleteFileW(temp.buf);
    }
    if (!ok) {
        DeleteFileW(path.buf);
        return false;
    }
    saved = next;
    pairPath = destination;
    for (int i = 0; i < 2; ++i) {
        stats[i] = next.summary[i];
        clip[i].clear();
        clip[i].add(next.paths[i]);
    }
    return true;
}
static bool validSummary(Summary s) {
    return s.n <= 180000 && std::isfinite(s.fps) && std::isfinite(s.p99) &&
           (s.n ? (s.fps > 0 && s.fps < 1000000 && s.p99 > 0 && s.p99 <= 60000) : (s.fps == 0 && s.p99 == 0));
}
// Documents store absolute references. Restrict the media reader to files named by CIRCE.
// A moved comparison can also resolve its media by basename beside the selected .cad.
static bool generatedFile(const W* path, const W* prefix, const W* suffix) {
    const W* p = filename(path);
    while (*prefix)
        if (*p++ != *prefix++)
            return false;
    if (*p < L'0' || *p > L'9')
        return false;
    while (*p >= L'0' && *p <= L'9')
        ++p;
    return lstrcmpiW(p, suffix) == 0;
}
static bool regularFile(const W* path) {
    U attrs = GetFileAttributesW(path);
    return attrs != 0xffffffff && !(attrs & 0x10);
}
static void resolveSidecar(W* dest, const W* pair, const W* prefix, const W* suffix) {
    if (!*dest || !generatedFile(dest, prefix, suffix)) {
        dest[0] = 0;
        return;
    }
    Text<2048> candidate;
    const W* name = filename(dest);
    if (joinPath(parentFolder(pair), name, candidate) && regularFile(candidate.buf)) {
        memcpy(dest, candidate.buf, (candidate.len + 1) * sizeof(W));
        return;
    }
    // Optional external media must be inside a directory the user explicitly selected.
    // Never probe an arbitrary path supplied by an imported comparison.
    if (!lstrcmpiW(suffix, L".mkv") && platform::knownOutputFolder(parentFolder(dest).buf) &&
        regularFile(dest))
        return;
    if (joinPath(data, name, candidate) && regularFile(candidate.buf))
        memcpy(dest, candidate.buf, (candidate.len + 1) * sizeof(W));
    else
        dest[0] = 0;
}
static bool validDocument(const Saved& s) {
    if (s.version != 1 || !s.game[0] || s.game[2047] || !s.summary[0].n ||
        (s.duration != 15 && s.duration != 30 && s.duration != 60))
        return false;
    for (int i = 0; i < 2; ++i) {
        if (s.names[i][79] || s.paths[i][2047] || s.framePaths[i][2047] || !validSummary(s.summary[i]) ||
            !std::isfinite(s.origins[i]) || s.origins[i] < 0)
            return false;
    }
    return true;
}
