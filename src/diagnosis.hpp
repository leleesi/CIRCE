static Process diagnostic;
static Q diagStart, diagAccepted, diagCounterBase, diagNextWrite;
static U diagPid;
static void appendCounter(Text<8192>& report, const W* name, Q value) {
    report.add(name);
    report.number(value);
    report.add(L"\r\n");
}
static bool writeSnapshot() {
    Text<8192> report;
    report.add(words::snapshotHeading.get());
    report.add(filename(gameExe));
    report.add(L"\r\n");
    appendCounter(report, L"PID: ", pid);
    appendCounter(report, words::receivedBytes.get(), counters.bytes);
    appendCounter(report, words::lineCount.get(), counters.lines);
    appendCounter(report, words::acceptedIntervals.get(), counters.accepted);
    appendCounter(report, words::otherProcesses.get(), counters.wrongPid);
    appendCounter(report, words::invalidInterval.get(), counters.invalid);
    appendCounter(report, words::otherSurface.get(), counters.otherChain);
    appendCounter(report, words::outOfOrder.get(), counters.old);
    appendCounter(report, words::flushStatus.get(), platform::lastFlushError());
    appendCounter(report, words::win32LaunchError.get(), startError);
    U code;
    if (collector.proc && GetExitCodeProcess(collector.proc, &code))
        appendCounter(report, words::collectorStatus.get(), code);
    if (pid == diagPid && diagCounterBase == traceSerial) {
        appendCounter(report, words::observedWindow.get(), GetTickCount64() - diagStart);
        appendCounter(report, words::newIntervals.get(), counters.accepted - diagAccepted);
        if (count) {
            report.add(words::lastSample.get());
            report.decimal(live(frames, count, lastFrame));
            report.add(L"\r\n");
        }
    } else
        report.add(words::selectionChanged.get());
    report.add(words::csvHeader.get());
    for (const char* p = counters.header; *p; ++p) {
        W ch[2] = {(W)(unsigned char)*p, 0};
        report.add(ch);
    }
    auto file = data;
    file.add(L"\\leitor-fps.txt");
    auto temp = file;
    temp.add(L".tmp");
    H f = CreateFileW(temp.buf, 0x40000000, 0, nullptr, 2, 0, nullptr);
    if (f == bad)
        return false;
    bool ok = writeAll(f, report.buf, report.len * sizeof(W));
    CloseHandle(f);
    if (ok)
        ok = MoveFileExW(temp.buf, file.buf, 9) != 0;
    if (!ok)
        DeleteFileW(temp.buf);
    return ok;
}
static void updateDiagnostic() {
    if (!diagnostic.proc)
        return;
    if (WaitForSingleObject(diagnostic.proc, 0) == 0) {
        release(diagnostic);
        return;
    }
    if (GetTickCount64() >= diagNextWrite) {
        writeSnapshot();
        diagNextWrite = GetTickCount64() + 400;
    }
}
static void runReport() {
    if (!pid || !game) {
        say(words::selectBeforeReport.get());
        return;
    }
    if (stage || exportThread) {
        say(words::finishBeforeReport.get());
        return;
    }
    if (diagnostic.proc && WaitForSingleObject(diagnostic.proc, 0) == 258) {
        say(words::reportRunning.get());
        return;
    }
    release(diagnostic);
    diagStart = GetTickCount64();
    diagPid = pid;
    diagAccepted = counters.accepted;
    diagCounterBase = traceSerial;
    diagNextWrite = diagStart + 400;
    if (!writeSnapshot()) {
        say(words::snapshotWriteFailed.get());
        return;
    }
    W system[512]{};
    GetSystemDirectoryW(system, 512);
    Text<2048> ps;
    ps.add(system);
    ps.add(L"\\WindowsPowerShell\\v1.0\\powershell.exe");
    Text<2048> script = folder;
    script.add(L"\\tools\\diagnostico.ps1");
    Text<4096> args;
    args.quote(ps.buf);
    args.add(L" -NoProfile -ExecutionPolicy RemoteSigned -File ");
    args.quote(script.buf);
    args.add(L" -GamePid ");
    args.number(pid);
    args.add(language::english() ? L" -UiLanguage en" : L" -UiLanguage pt");
    Start startup{};
    startup.size = sizeof(startup);
    if (spawn(ps, args, startup, diagnostic, false)) {
        say(words::keepGameMoving.get());
        hide();
    } else
        say(words::diagnosticStartFailed.get());
}
