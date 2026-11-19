// Paths stored in v1 comparison documents use 2048 UTF-16 units including the terminator.
static Text<2048> outputFolder;
static bool outputFolderChosen, outputDialogOpen;

static void trimFolder(Text<2048>& path) {
    while (path.len > 1 && path.buf[path.len - 1] == L'\\' && path.buf[path.len - 2] != L':')
        path.buf[--path.len] = 0;
}
static Text<2048> parentFolder(const W* path) {
    Text<2048> result;
    result.add(path);
    while (result.len && result.buf[result.len - 1] != L'\\')
        result.buf[--result.len] = 0;
    trimFolder(result);
    return result;
}
static bool joinPath(const Text<2048>& directory, const W* name, Text<2048>& result) {
    if (!directory.len) return false;
    const bool slash = directory.buf[directory.len - 1] != L'\\';
    if (directory.len + unsigned(slash) + lstrlenW(name) >= 2048) return false;
    result = directory;
    if (slash) result.add(L"\\");
    result.add(name);
    return true;
}
static Text<128> timedName(const W* prefix, const W* suffix) {
    Q ticks{};
    GetSystemTimeAsFileTime(&ticks);
    Text<128> name;
    name.add(prefix);
    name.number(ticks);
    name.add(suffix);
    return name;
}
static void loadOutputFolder() {
    W path[2048]{};
    outputFolder = data;
    outputFolderChosen = platform::readOutputFolder(path, 2048);
    if (outputFolderChosen || platform::defaultOutputFolder(path, 2048)) {
        outputFolder.clear();
        outputFolder.add(path);
        trimFolder(outputFolder);
    }
}
static bool outputFolderWritable(const Text<2048>& directory) {
    Text<2048> probe;
    if (!joinPath(directory, timedName(L".circe-write-", L".tmp").buf, probe)) return false;
    // Reserve an unused name and let Windows remove the probe when the handle closes.
    H file = CreateFileW(probe.buf, 0x40000000 | 0x10000, 0, nullptr, 1, 0x04000100, nullptr);
    if (file == bad) return false;
    CloseHandle(file);
    return true;
}
static void showOutputFolder() {
    Text<2200> label;
    label.add(words::outputLocation.get());
    label.add(outputFolder.buf);
    panelui::set(panelui::OutputPath, label.buf);
}
static bool pickOutput(platform::OutputKind kind, const W* name, Text<2048>& selected) {
    outputDialogOpen = true;
    auto choice = platform::chooseOutput(panel, kind, outputFolder.buf, name, selected.buf, 2048);
    outputDialogOpen = false;
    if (choice == platform::Choice::Failed) say(words::outputChoiceFailed.get());
    if (choice != platform::Choice::Selected) return false;
    selected.len = U(lstrlenW(selected.buf));
    return selected.len != 0;
}
static bool chooseRecordingFolder() {
    Text<2048> selected;
    if (!pickOutput(platform::OutputKind::Folder, nullptr, selected)) return false;
    trimFolder(selected);
    if (!outputFolderWritable(selected)) {
        say(words::outputFolderUnavailable.get());
        return false;
    }
    if (!platform::storeOutputFolder(selected.buf)) {
        say(words::outputPreferenceFailed.get());
        return false;
    }
    outputFolder = selected;
    outputFolderChosen = true;
    showOutputFolder();
    say(words::outputFolderSaved.get());
    return true;
}
static bool prepareClipFolder() {
    if (!outputFolderChosen) return chooseRecordingFolder();
    if (outputFolderWritable(outputFolder)) return true;
    say(words::outputFolderUnavailable.get());
    return false;
}
struct ExportTarget {
    Text<2048> path;
    bool replace = false;
};
static bool selectExport(platform::OutputKind kind, ExportTarget& target) {
    const bool video = kind == platform::OutputKind::Movie;
    const W* suffix = video ? L".mp4" : L".pdf";
    const W* prefix = video ? words::movieFilename.get() : words::comparisonFilename.get();
    Text<2048> selected;
    if (!pickOutput(kind, timedName(prefix, suffix).buf, selected)) return false;
    if (selected.len <= 4 || lstrcmpiW(selected.buf + selected.len - 4, suffix) ||
        !parentFolder(selected.buf).len) {
        say(words::outputChoiceFailed.get());
        return false;
    }
    const U attrs = GetFileAttributesW(selected.buf);
    const bool exists = attrs != 0xffffffff;
    if (exists && (attrs & (0x10 | 0x1))) {
        say(words::outputFolderUnavailable.get());
        return false;
    }
    if (exists) {
        outputDialogOpen = true;
        const bool confirmed = panelui::confirm(panel, words::replaceFile.get(), selected.buf,
                                                words::replaceButton.get(), words::cancelButton.get(), false);
        outputDialogOpen = false;
        if (!confirmed) return false;
    }
    target = {selected, exists};
    return true;
}
static H reserveExportTemp(const ExportTarget& target, const W* suffix, Text<2048>& temp) {
    if (!joinPath(parentFolder(target.path.buf), timedName(L".circe-", suffix).buf, temp) ||
        !lstrcmpiW(temp.buf, target.path.buf)) return bad;
    return CreateFileW(temp.buf, 0x40000000, 0, nullptr, 1, 0, nullptr); // CREATE_NEW
}
static bool publishExport(const Text<2048>& temp, const ExportTarget& target) {
    return MoveFileExW(temp.buf, target.path.buf, 8 | (target.replace ? 1 : 0)) != 0;
}
static void reportSaved(const W* message, const ExportTarget& target) {
    Text<2304> status;
    status.add(message);
    status.add(L" ");
    status.add(target.path.buf);
    say(status.buf);
}
