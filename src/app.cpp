#include "messages.hpp"
#include "win.hpp"
#include "text.hpp"
#include "metrics.hpp"
#include "csv_columns.hpp"
#include "panel.hpp"
#include "platform.hpp"
#include <cstring>
#include <cstdio>
#include <string>
#include <algorithm>
#include <atomic>
#include <cmath>
static_assert(sizeof(void*) == 8 && sizeof(W) == 2 && sizeof(Start) == 104 && sizeof(Msg) == 48 &&
              sizeof(OpenFile) == 152);
static const H bad = (H)-1;
static H panel, game, labelA, labelB, seconds, gameProcess, lastGameWindow;
static U pid = 0, stage = 0;
static bool opened = true, hotkey = false, closing = false, panelHadFocus = false, panelRequested = false;
static int side = -1;
static L freq;
static double begin, finish;
static W exe[2048], gameExe[2048];
static Text<2048> folder, data, monPath, trace, pairPath;
static Process collector;
static H pipeRead;
static U startError = 0;
static Q traceSerial = 0;
static double attachedAt;
static Frame frames[180000];
static double scratch[180000];
static U count;
static Summary stats[2];
static Text<256> notice;
static double noticeUntil;
static void resetPair();
static void syncControls();
static void cancelSession(const W*);
static double now() {
    L t;
    QueryPerformanceCounter(&t);
    return double(t) / freq;
}
static void say(const W* s) {
    panelui::set(panelui::Status, s);
    notice.clear();
    notice.add(s);
    noticeUntil = freq ? now() + 4 : 0;
}
#include "output.hpp"
#include "reader.hpp"
static void stopMonitor() {
    if (collector.proc) {
        platform::stopTrace(trace.buf, collector.proc);
        CloseHandle(collector.proc);
        CloseHandle(collector.thread);
        collector = {};
    }
    if (pipeRead) {
        CloseHandle(pipeRead);
        pipeRead = nullptr;
    }
}
static bool attach(H target) {
    U targetPid = 0;
    GetWindowThreadProcessId(target, &targetPid);
    if (!targetPid || targetPid == GetCurrentProcessId())
        return false;
    if (game == target && pid == targetPid && IsWindow(game) && collector.proc &&
        WaitForSingleObject(collector.proc, 0) == 258)
        return true;
    if (stage)
        return false;
    H gp = OpenProcess(0x101000, 0, targetPid);
    U len = 2048;
    W name[2048]{};
    if (!gp || !QueryFullProcessImageNameW(gp, 0, name, &len)) {
        U titleLength = GetWindowTextW(target, name, 2048);
        if (titleLength)
            len = titleLength;
        else {
            Text<128> fallback;
            fallback.add(words::unnamedGame.get());
            fallback.number(targetPid);
            len = fallback.len;
            memcpy(name, fallback.buf, (len + 1) * sizeof(W));
        }
    }
    if (lstrcmpiW(name, gameExe) != 0) {
        resetPair();
    }
    memcpy(gameExe, name, (len + 1) * sizeof(W));
    stopMonitor();
    if (gameProcess)
        CloseHandle(gameProcess);
    gameProcess = gp;
    game = target;
    pid = targetPid;
    resetReader();
    startError = 0;
    attachedAt = now();
    H write;
    Security sec{sizeof(sec), nullptr, 1};
    if (!CreatePipe(&pipeRead, &write, &sec, 1048576)) {
        startError = GetLastError();
        return false;
    }
    SetHandleInformation(pipeRead, 1, 0);
    Text<2048> log = data;
    log.add(L"\\presentmon.log");
    H err = CreateFileW(log.buf, 0x40000000, 3, &sec, 2, 0, nullptr);
    H input = CreateFileW(L"NUL", 0x80000000, 3, &sec, 3, 0, nullptr);

    ++traceSerial;
    W identity[256]{};
    bool stable = platform::traceIdentity(identity, 256);
    trace.clear();
    if (stable)
        trace.add(identity);
    else {
        trace.add(L"CIRCE");
        trace.number(GetCurrentProcessId());
        trace.add(L"_");
        trace.number(traceSerial);
    }
    Text<4096> cmd;
    cmd.quote(monPath.buf);
    cmd.add(L" --output_stdout --v1_metrics --qpc_time --no_track_gpu --no_track_input --no_console_stats "
            L"--terminate_on_proc_exit --process_id ");
    cmd.number(pid);
    cmd.add(L" --session_name ");
    cmd.add(trace.buf);
    if (stable)
        cmd.add(L" --stop_existing_session");
    bool ok = platform::startChild(monPath.buf, cmd.buf, folder.buf, input, write, err, &collector.proc,
                                   &collector.thread);
    if (!ok)
        startError = GetLastError();
    CloseHandle(write);
    if (err != bad)
        CloseHandle(err);
    if (input != bad)
        CloseHandle(input);
    if (ok) {
        platform::startFlush(trace.buf);
        SetTimer(panel, 1, 50, nullptr);
    } else {
        CloseHandle(pipeRead);
        pipeRead = nullptr;
    }
    return ok;
}
#include "game_list.hpp"
static void hide() {
    if (!game || !IsWindow(game)) {
        say(words::chooseOrShortcut.get());
        return;
    }
    ShowWindow(panel, hotkey ? 0 : 6);
    opened = false;
    panelHadFocus = false;
    panelRequested = false;
    if (IsIconic(game))
        ShowWindow(game, 9);
    SetForegroundWindow(game);
}
static void openPanel(bool bringOnly = false) {
    H active = GetForegroundWindow();
    U owner = 0;
    GetWindowThreadProcessId(active, &owner);
    if (!bringOnly && owner && owner != GetCurrentProcessId() && active != GetShellWindow())
        lastGameWindow = active;
    if (!bringOnly && opened && (active == panel || (panelRequested && owner == pid))) {
        hide();
        return;
    }
    if (!bringOnly && !stage && !manualTarget && active && active != panel && active != GetShellWindow()) {
        if (!attach(active))
            say(words::chooseGameWindow.get());
    }
    panelHadFocus = false;
    panelRequested = true;
    refreshGames();
    panelui::position(game);
    panelui::hideHud();
    ShowWindow(panel, 9);
    SetWindowPos(panel, (H)-1, 0, 0, 0, 0, 3);
    SetForegroundWindow(panel);
    opened = true;
    syncControls();
}
static H exportThread;
static std::atomic<U> exportCode{0};
static U cpuPct, ramMiB;
static bool cpuReady;
static H profileBox;
static U recordW = 1280, recordH = 720, recordRate = 30;
static bool hasVideo;
static bool filming, sampleRun;
static Process grab, encode;
static Text<2048> clip[2], pending;
static Text<2048> component(const W* name) {
    Text<2048> p;
    p.add(folder.buf);
    p.add(L"\\vendor\\");
    p.add(name);
    return p;
}
static void release(Process& p, bool kill = false) {
    if (!p.proc)
        return;
    if (kill && WaitForSingleObject(p.proc, 0) == 258) {
        TerminateProcess(p.proc, 1);
        WaitForSingleObject(p.proc, 1500);
    }
    CloseHandle(p.proc);
    CloseHandle(p.thread);
    p = {};
}
template <unsigned N> static bool spawn(Text<2048>& path, Text<N>& args, Start& s, Process& p, bool inherit) {
    if (inherit)
        return platform::startChild(path.buf, args.buf, data.buf, s.in, s.out, s.err, &p.proc, &p.thread);
    return CreateProcessW(path.buf, args.buf, nullptr, nullptr, 0, 0x08000000, nullptr, data.buf, &s, &p) !=
           0;
}
static bool recordVideo(bool sample) {
    Text<2048> helper = component(L"CIRCEJanela.exe"), ff = component(L"ffmpeg.exe");
    Text<8192> args;
    const auto& directory = sample ? data : outputFolder;
    Text<128> name = timedName(sample ? L"teste-" : L"clipe-", L".mkv");
    Text<2048> video;
    if (!joinPath(directory, name.buf, video)) return false;
    Text<2048> journal = video;
    journal.len -= 4;
    journal.buf[journal.len] = 0;
    journal.add(L".qpc");
    H reservation = CreateFileW(video.buf, 0x40000000, 0, nullptr, 1, 0, nullptr);
    if (reservation == bad)
        return false;
    CloseHandle(reservation);
    reservation = CreateFileW(journal.buf, 0x40000000, 0, nullptr, 1, 0, nullptr);
    if (reservation == bad) {
        DeleteFileW(video.buf);
        return false;
    }
    CloseHandle(reservation);
    pending = video;
    H read, write;
    Security sec{sizeof(sec), nullptr, 1};
    if (!CreatePipe(&read, &write, &sec, 0))
        return false;
    H nul = CreateFileW(L"NUL", 0xc0000000, 3, &sec, 3, 0, nullptr);
    Text<2048> log = data;
    log.add(L"\\video.log");
    H errors = CreateFileW(log.buf, 0x40000000, 3, &sec, 2, 0, nullptr);
    Start s{};
    s.size = sizeof(s);
    s.flags = 0x100;
    s.in = nul;
    s.err = errors != bad ? errors : nul;
    s.out = write;
    args.quote(helper.buf);
    args.add(L" ");
    args.number((Q)game);
    args.add(L" ");
    args.number(pid);
    args.add(L" ");
    args.number(recordRate);
    args.add(L" ");
    args.number((Q)finish);
    args.add(L".");
    auto frac = (U)((finish - (Q)finish) * 1000000);
    for (U d = 100000; d; d /= 10)
        args.number(frac / d % 10);
    args.add(L" ");
    args.number(recordW);
    args.add(L" ");
    args.number(recordH);
    args.add(L" ");
    args.quote(journal.buf);
    bool ok = spawn(helper, args, s, grab, true);
    CloseHandle(write);
    if (ok) {
        s.in = read;
        s.out = nul;
        args.clear();
        args.quote(ff.buf);
        args.add(
            L" -nostdin -loglevel error -y -filter_threads 1 -f rawvideo -pixel_format bgra -video_size ");
        args.number(recordW);
        args.add(L"x");
        args.number(recordH);
        args.add(L" -framerate ");
        args.number(recordRate);
        args.add(L" -i pipe:0 -c:v mpeg4 -threads 2 -q:v 4 -pix_fmt yuv420p ");
        args.quote(pending.buf);
        ok = spawn(ff, args, s, encode, true);
    }
    CloseHandle(read);
    if (nul != bad)
        CloseHandle(nul);
    if (errors != bad)
        CloseHandle(errors);
    if (!ok)
        release(grab, true);
    return ok;
}
static bool successful(Process& p) {
    U rc = 1;
    return p.proc && GetExitCodeProcess(p.proc, &rc) && rc == 0;
}
#include "session.hpp"
static void resultsText() {
    for (int i = 0; i < 2; ++i) {
        Text<256> metrics, detail;
        if (!stats[i].n)
            metrics.add(words::noRun.get());
        else {
            metrics.decimal(stats[i].fps);
            metrics.add(L" FPS   ·   P99 ");
            metrics.decimal(stats[i].p99);
            metrics.add(L" ms");
            detail.add(saved.names[i]);
            detail.add(L"\r\n");
        }
        detail.add(metrics.buf);
        panelui::set(i ? panelui::SummaryB : panelui::SummaryA, metrics.buf);
        panelui::set(i ? panelui::ResultB : panelui::ResultA, detail.buf);
    }
    Text<64> delta;
    if (stats[0].n && stats[1].n && stats[0].fps > 0) {
        double d = (stats[1].fps / stats[0].fps - 1) * 100;
        if (std::abs(d) < .05)
            d = 0;
        if (d != 0)
            delta.add(d > 0 ? L"+" : L"−");
        delta.decimal(std::abs(d));
        delta.add(L"%");
    } else
        delta.add(L"—");
    panelui::set(panelui::Delta, delta.buf);
}
static int duration() {
    L i = SendMessageW(seconds, 0x147, 0, 0);
    return i == 0 ? 15 : i == 2 ? 60 : 30;
}
static void arm(int index, bool sample) {
    if (stage || exportThread) {
        say(words::waitForTask.get());
        return;
    }
    U currentOwner = 0;
    GetWindowThreadProcessId(game, &currentOwner);
    if (!game || !IsWindow(game) || currentOwner != pid) {
        say(words::shortcutInGame.get());
        return;
    }
    if (index == 1 && (!stats[0].n || saved.duration != (U)duration())) {
        say(words::sameDuration.get());
        return;
    }
    if (!collector.proc || WaitForSingleObject(collector.proc, 0) != 258) {
        if (!attach(game)) {
            say(words::restartFailed.get());
            return;
        }
    }
    filming = panelui::checked(panelui::Record);
    if (filming && !hasVideo) {
        say(words::installVideoOrDisable.get());
        return;
    }
    if (filming && !sample) {
        if (!prepareClipFolder()) return;
        // The first destination picker can stay open while the game exits.
        if (!game || !IsWindow(game) || !GetWindowThreadProcessId(game, &currentOwner) || currentOwner != pid) {
            say(words::shortcutInGame.get());
            return;
        }
    }
    L quality = SendMessageW(profileBox, 0x147, 0, 0);
    recordW = quality > 0 ? 1920 : 1280;
    recordH = quality > 0 ? 1080 : 720;
    recordRate = quality == 2 ? 60 : 30;
    sampleRun = sample;
    pending.clear();
    frameOverflow = false;
    side = index;
    begin = now() + 3;
    finish = begin + (sample ? 3 : duration());
    stage = 1;
    hide();
}
static void discardPending() {
    if (!pending.len)
        return;
    DeleteFileW(pending.buf);
    Text<2048> journal;
    journal.add(pending.buf);
    if (journal.len >= 4) {
        journal.len -= 4;
        journal.buf[journal.len] = 0;
        journal.add(L".qpc");
        DeleteFileW(journal.buf);
    }
    pending.clear();
}
static void sampleSystem() {
    static Q prevIdle, prevKernel, prevUser, last;
    Q clock = GetTickCount64();
    if (clock - last < 500)
        return;
    last = clock;
    Q idle, kernel, user;
    if (GetSystemTimes(&idle, &kernel, &user)) {
        Q all = (kernel - prevKernel) + (user - prevUser);
        if (prevKernel && all && idle >= prevIdle && idle - prevIdle <= all) {
            cpuPct = (U)(100 * (all - (idle - prevIdle)) / all);
            cpuReady = true;
        }
        prevIdle = idle;
        prevKernel = kernel;
        prevUser = user;
    }
    struct Memory {
        U size, load;
        Q total, available, page, freePage, virt, freeVirt, extended;
    };
    Memory m{};
    m.size = sizeof(m);
    if (GlobalMemoryStatusEx(&m))
        ramMiB = (U)((m.total - m.available) >> 20);
}
static void reopen() {
    if (stage || exportThread)
        return;
    static W path[2048];
    path[0] = 0;
    OpenFile dlg{};
    dlg.size = sizeof(dlg);
    dlg.owner = panel;
    dlg.dir = data.buf;
    dlg.file = path;
    dlg.maxFile = 2048;
    dlg.flags = 0x180c;
    dlg.filter = words::comparisonFilter.get();
    dlg.title = words::openPair.get();
    if (!GetOpenFileNameW(&dlg))
        return;
    H f = CreateFileW(path, 0x80000000, 1, nullptr, 3, 0, nullptr);
    U n = 0;
    Saved value{};
    L bytes = 0;
    if (f == bad) {
        say(words::openPairFailed.get());
        return;
    }
    bool ok =
        GetFileSizeEx(f, &bytes) && bytes == sizeof(value) && ReadFile(f, &value, sizeof(value), &n, nullptr);
    CloseHandle(f);
    if (!ok || n != sizeof(value) || !validDocument(value)) {
        say(words::invalidPair.get());
        return;
    }
    for (int i = 0; i < 2; ++i) {
        resolveSidecar(value.paths[i], path, L"clipe-", L".mkv");
        resolveSidecar(value.framePaths[i], path, L"sessao-", L".frames");
    }
    if (game && lstrcmpiW(value.game, gameExe)) {
        stopMonitor();
        game = nullptr;
        pid = 0;
        manualTarget = false;
        resetReader();
        if (gameProcess) {
            CloseHandle(gameProcess);
            gameProcess = nullptr;
        }
        refreshGames();
    }
    memcpy(gameExe, value.game, sizeof(gameExe));
    saved = value;
    pairPath.clear();
    pairPath.add(path);
    for (int i = 0; i < 2; i++) {
        stats[i] = value.summary[i];
        clip[i].clear();
        clip[i].add(value.paths[i]);
    }
    SetWindowTextW(labelA, value.names[0]);
    SetWindowTextW(labelB, value.names[1]);
    SendMessageW(seconds, 0x14e, value.duration == 15 ? 0 : value.duration == 60 ? 2 : 1, 0);
    resultsText();
    say(words::pairOpened.get());
}
#include "export.hpp"
static void resetPair() {
    saved = {};
    stats[0] = {};
    stats[1] = {};
    clip[0].clear();
    clip[1].clear();
    pairPath.clear();
    resultsText();
}
#include "pdf.hpp"
#include "legacy.hpp"
#include "diagnosis.hpp"
static void syncControls() {
    using namespace panelui;
    bool target = game && IsWindow(game), idle = !stage && !exportThread;
    enable(Back, target);
    enable(Before, target && idle);
    enable(After, target && idle && stats[0].n && saved.duration == U(duration()));
    enable(TryGame, target && idle);
    enable(Report, target && idle);
    enable(Games, idle);
    enable(Refresh, idle);
    enable(Duration, idle);
    enable(NameA, idle);
    enable(NameB, idle);
    enable(Record, idle && hasVideo);
    enable(Profile, idle && hasVideo && checked(Record));
    enable(Movie, idle && hasVideo && clip[0].len && clip[1].len && saved.framePaths[0][0] &&
                      saved.framePaths[1][0]);
    enable(Pdf, idle && stats[0].n && stats[1].n);
    enable(NewPair, idle && stats[0].n);
    enable(OpenPair, idle);
    enable(ChooseOutput, idle);
    enable(Cancel, stage != 0);
    set(VideoHelp, !hasVideo         ? words::installVideoHelp.get()
                   : checked(Record) ? words::recordingEnabled.get()
                                     : words::metricsOnly.get());
}
static void cancelSession(const W* reason) {
    release(grab, true);
    release(encode, true);
    discardPending();
    stage = 0;
    filming = false;
    say(reason);
    syncControls();
}
static const W* videoError(U grabCode, U encodeCode) {
    if (grabCode == 10)
        return words::captureDenied.get();
    if (grabCode == 8)
        return words::blackFrames.get();
    if (grabCode == 3)
        return words::gameMinimized.get();
    if (encodeCode != 0)
        return words::encoderIncomplete.get();
    return words::noCaptureFrames.get();
}
static void finishSession() {
    U grabCode = 1, encodeCode = 1;
    if (grab.proc)
        GetExitCodeProcess(grab.proc, &grabCode);
    if (encode.proc)
        GetExitCodeProcess(encode.proc, &encodeCode);
    bool videoOK = !filming || (grabCode == 0 && encodeCode == 0);
    release(grab, true);
    release(encode, true);
    Summary result = summarize(frames, count, begin, finish, scratch);
    bool complete = result.n && !frameOverflow && coversSession(frames, count, begin, finish);
    bool keep = false;
    if (sampleRun) {
        Text<256> outcome;
        if (result.n) {
            outcome.add(words::testPrefix.get());
            outcome.decimal(result.fps);
            outcome.add(words::averageFpsSuffix.get());
        } else
            outcome.add(words::testNoFrames.get());
        if (filming) {
            outcome.add(L" ");
            outcome.add(videoOK ? words::testVideoOk.get() : videoError(grabCode, encodeCode));
        }
        say(outcome.buf);
    } else if (!videoOK)
        say(videoError(grabCode, encodeCode));
    else if (!complete)
        say(words::incompleteRun.get());
    else if (!save(side, result))
        say(words::saveRunFailed.get());
    else {
        keep = true;
        resultsText();
        say(side ? words::afterSaved.get()
                 : words::beforeSaved.get());
    }
    if (keep)
        pending.clear();
    else
        discardPending();
    stage = 0;
    filming = false;
    syncControls();
}
static void stepSession(double t, bool active) {
    if (!stage)
        return;
    if (stage == 2 && t >= finish)
        stage = 3;
    if (stage == 1 && t >= begin) {
        if (!active) {
            cancelSession(words::focusLostCancel.get());
            return;
        }
        if (!sampleRun && (!count || t - lastFrame > 2)) {
            cancelSession(
                words::noRecentGameFrames.get());
            return;
        }
        if (filming && !recordVideo(sampleRun)) {
            cancelSession(words::videoStartFailed.get());
            return;
        }
        stage = 2;
    }
    if (stage == 2 && !active) {
        cancelSession(words::focusLostStop.get());
        return;
    }
    if (stage == 2 && filming &&
        ((grab.proc && WaitForSingleObject(grab.proc, 0) == 0 && !successful(grab)) ||
         (encode.proc && WaitForSingleObject(encode.proc, 0) == 0 && !successful(encode)))) {
        U g = 1, e = 1;
        GetExitCodeProcess(grab.proc, &g);
        GetExitCodeProcess(encode.proc, &e);
        cancelSession(videoError(g, e == 259 ? 0 : e));
        return;
    }
    if (stage == 3) {
        bool drained = lastFrame >= finish || t > finish + 3;
        bool done =
            !filming || (WaitForSingleObject(grab.proc, 0) == 0 && WaitForSingleObject(encode.proc, 0) == 0);
        if ((drained && done) || t > finish + 10)
            finishSession();
    }
}
struct Reading {
    Text<180> detail;
    Text<32> value;
    bool fresh = false;
};
static Reading reading(double t) {
    Reading r;
    r.value.add(L"—");
    if (!game) {
        r.detail.add(words::awaitingGame.get());
        return r;
    }
    if (!collector.proc) {
        r.detail.add(words::presentMonStartFailed.get());
        r.detail.number(startError);
        return r;
    }
    if (WaitForSingleObject(collector.proc, 0) == 0) {
        U code = 0;
        GetExitCodeProcess(collector.proc, &code);
        r.detail.add(words::measurementEnded.get());
        r.detail.number(code);
        r.detail.add(words::openMeasurement.get());
        return r;
    }
    if (count) {
        double age = t - lastFrame;
        if (age >= 0 && age <= 2) {
            r.value.clear();
            r.value.number((Q)(live(frames, count, lastFrame) + .5));
            r.fresh = age <= .30;
            if (r.fresh)
                r.detail.add(words::receivingFrames.get());
            else {
                r.detail.add(words::receivedLate.get());
                r.detail.decimal(age);
                r.detail.add(L" s");
            }
        } else
            r.detail.add(words::noRecentFrames.get());
        r.detail.add(L"  ·  ");
        r.detail.add(filename(gameExe));
        return r;
    }
    if (headerSeen && (ciTime < 0 || ciMs < 0 || ciChain < 0))
        r.detail.add(words::incompatibleMetrics.get());
    else if (headerSeen && counters.invalid)
        r.detail.add(words::awaitingIntervals.get());
    else if (headerSeen)
        r.detail.add(words::awaitingWindowFrames.get());
    else if (t - attachedAt > 6) {
        r.detail.add(words::noFramesFrom.get());
        r.detail.add(filename(gameExe));
        r.detail.add(words::confirmWindow.get());
    } else
        r.detail.add(words::awaitingPresentMon.get());
    return r;
}
static void loseGame() {
    stopMonitor();
    if (gameProcess) {
        CloseHandle(gameProcess);
        gameProcess = nullptr;
    }
    game = nullptr;
    pid = 0;
    manualTarget = false;
    resetReader();
    SetTimer(panel, 1, 250, nullptr);
    if (stage)
        cancelSession(words::gameClosed.get());
    else
        say(words::windowClosed.get());
    if (!opened) {
        ShowWindow(panel, 5);
        opened = true;
    }
    refreshGames();
    syncControls();
}
static void tick() {
    double t = now();
    if (game) {
        U owner = 0;
        GetWindowThreadProcessId(game, &owner);
        bool exited = gameProcess && WaitForSingleObject(gameProcess, 0) == 0;
        if (!exited && owner != pid && !stage) {
            H next = GetForegroundWindow();
            U replacement = 0;
            GetWindowThreadProcessId(next, &replacement);
            if (replacement == pid) {
                game = next;
                owner = pid;
                refreshGames();
            }
        }
        if (owner != pid || exited)
            loseGame();
    }
    drain();
    updateDiagnostic();
    t = now();
    H foreground = GetForegroundWindow();
    U fgPid = 0;
    GetWindowThreadProcessId(foreground, &fgPid);
    bool active = game && fgPid == pid;
    if (opened && foreground == panel)
        panelHadFocus = true;
    if (active && opened && panelHadFocus) {
        ShowWindow(panel, hotkey ? 0 : 6);
        opened = false;
        panelHadFocus = false;
        panelRequested = false;
    }
    stepSession(t, active);
    if (exportThread) {
        Q progress = exportProgress.load(), clock = GetTickCount64();
        if (cancelExport || (clock >= progress && clock - progress > 30000)) {
            cancelExport = true;
            CancelSynchronousIo(exportThread);
        }
    }
    if (exportThread && WaitForSingleObject(exportThread, 0) == 0) {
        CloseHandle(exportThread);
        exportThread = nullptr;
        const U code = exportCode.load();
        if (code == ExportOk) reportSaved(words::movieSaved.get(), movieTarget);
        else say(exportMessage(code));
        syncControls();
    }
    static double nextPanel = 0, nextHud = 0;
    if (opened && t >= nextPanel) {
        auto r = reading(t);
        panelui::set(panelui::LiveValue, r.value.buf);
        panelui::set(panelui::LiveDetail, r.detail.buf);
        nextPanel = t + .24;
    }
    if (!active || opened || (!panelui::checked(panelui::ShowFps) && !stage && t >= noticeUntil)) {
        panelui::hideHud();
        return;
    }
    if (t < nextHud)
        return;
    nextHud = t + .10;
    auto r = reading(t);
    Text<512> msg;
    if (panelui::checked(panelui::ShowFps)) {
        if (r.value.buf[0] != L'—') {
            msg.add(r.value.buf);
            msg.add(L" FPS");
            if (!r.fresh) {
                msg.add(words::delayTag.get());
                msg.decimal(t - lastFrame);
                msg.add(L" s");
            }
        } else
            msg.add(r.detail.buf);
    }
    if (stage) {
        if (msg.len)
            msg.add(L"   |   ");
        if (stage == 1) {
            msg.add(words::startingIn.get());
            msg.number((U)std::ceil(begin - t));
            msg.add(L" s");
        } else if (stage == 2) {
            msg.add(sampleRun ? words::test.get() : side ? words::after.get() : words::before.get());
            msg.add(L" · ");
            msg.number((U)std::ceil(finish - t));
            msg.add(L" s");
        } else
            msg.add(words::saving.get());
    } else if (t < noticeUntil && notice.len) {
        msg.add(L"   |   ");
        msg.add(notice.buf);
    }
    if (panelui::checked(panelui::Sensors)) {
        sampleSystem();
        if (cpuReady) {
            msg.add(L"   CPU ");
            msg.number(cpuPct);
            msg.add(L"%");
        }
        if (ramMiB) {
            msg.add(L"   RAM ");
            msg.number(ramMiB);
            msg.add(L" MiB");
        }
    }
    panelui::displayHud(foreground, msg.buf, r.fresh);
}
static L callback(H h, U message, Q w, L l) {
    L result = 0;
    if (outputDialogOpen && (message == 0x8001 || message == 0x312 || message == 0x10 || message == 0x111))
        return 0;
    if (panelui::handle(h, message, w, l, &result))
        return result;
    if (message == 6 && h == panel) {
        if ((w & 65535) != 0 && !IsIconic(panel)) {
            opened = true;
            panelHadFocus = true;
        } else if (l) {
            U foregroundOwner = 0;
            GetWindowThreadProcessId((H)l, &foregroundOwner);
            if (foregroundOwner != pid && foregroundOwner != GetCurrentProcessId())
                SetWindowPos(panel, (H)-2, 0, 0, 0, 0, 0x13);
        }
    }
    if (message == 0x8001) {
        openPanel(true);
        return 0;
    }
    if (message == 0x312) {
        openPanel();
        return 0;
    }
    if (message == 0x113) {
        tick();
        return 0;
    }
    if (message == 0x111) {
        using namespace panelui;
        unsigned id = w & 65535, notification = (w >> 16) & 65535;
        if (!l && (id == 1 || id == 2))
            return 0;
        switch (id) {
        case Games:
            if (notification == 1)
                chooseGame();
            break;
        case Refresh:
            refreshGames();
            break;
        case Back:
            hide();
            break;
        case Before:
        case After:
            arm(id == After ? 1 : 0, false);
            break;
        case TryGame:
            arm(0, true);
            break;
        case Cancel:
            if (stage)
                cancelSession(words::canceled.get());
            break;
        case Report:
            runReport();
            break;
        case History:
            legacyHistory();
            break;
        case ChooseOutput:
            if (!stage && !exportThread) chooseRecordingFolder();
            break;
        case OpenOutput:
            if (L(ShellExecuteW(panel, L"open", outputFolder.buf, nullptr, nullptr, 1)) <= 32)
                say(words::openOutputFailed.get());
            break;
        case Pdf:
            exportPdf();
            break;
        case Movie:
            render();
            break;
        case OpenPair:
            reopen();
            break;
        case Folder:
            ShellExecuteW(panel, L"open", data.buf, nullptr, nullptr, 1);
            break;
        case NewPair:
            if (!stage && !exportThread) {
                resetPair();
                say(words::newPairStarted.get());
            }
            break;
        case ShowFps:
        case Sensors:
        case Record: {
            const W* key = id == ShowFps ? L"MostrarFPS" : id == Sensors ? L"Sensores" : L"Video";
            platform::storePreference(key, checked((Id)id));
            break;
        }
        case Duration:
        case Profile:
            if (notification == 1)
                platform::storePreference(id == Duration ? L"Duracao" : L"Perfil",
                                          U(SendMessageW(control((Id)id), 0x147, 0, 0)));
            break;
        default:
            return DefWindowProcW(h, message, w, l);
        }
        syncControls();
        return 0;
    }
    if (message == 0x10) {
        if (exportThread) {
            if (panelui::confirm(panel, words::exporting.get(), words::confirmStopExport.get(),
                                 words::stopButton.get(), words::continueExport.get(), true)) {
                cancelExport = true;
                CancelSynchronousIo(exportThread);
                closing = true;
            }
            return 0;
        }
        if (stage)
            cancelSession(words::canceled.get());
        UnregisterHotKey(panel, 1);
        stopMonitor();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(h, message, w, l);
}
extern "C" int wWinMain(H instance, H, W*, int) {
    language::initialize();
    if (!platform::firstInstance())
        return 0;
    U exeLen = GetModuleFileNameW(nullptr, exe, 2048);
    if (!exeLen || exeLen >= 2048 || !QueryPerformanceFrequency(&freq) || freq <= 0)
        return 1;
    folder.add(exe);
    while (folder.len && folder.buf[folder.len - 1] != L'\\')
        --folder.len;
    if (folder.len)
        --folder.len;
    folder.buf[folder.len] = 0;
    monPath = component(L"PresentMon.exe");
    W local[1500]{};
    U n = GetEnvironmentVariableW(L"LOCALAPPDATA", local, 1500);
    if (!n || n >= 1500)
        return 1;
    data.add(local);
    data.add(L"\\CIRCE");
    CreateDirectoryW(data.buf, nullptr);
    data.add(L"\\Nativo");
    CreateDirectoryW(data.buf, nullptr);
    if (GetFileAttributesW(data.buf) == 0xffffffff) {
        MessageBoxW(nullptr, words::cannotOpenData.get(), L"CIRCE", 16);
        return 1;
    }
    panel = panelui::create(instance, callback);
    if (!panel) {
        MessageBoxW(nullptr, words::cannotOpenPanel.get(), L"CIRCE", 16);
        return 1;
    }
    using namespace panelui;
    loadOutputFolder();
    showOutputFolder();
    gameList = control(Games);
    labelA = control(NameA);
    labelB = control(NameB);
    seconds = control(Duration);
    profileBox = control(Profile);
    hasVideo = regularFile(component(L"ffmpeg.exe").buf) && regularFile(component(L"CIRCEJanela.exe").buf);
    check(ShowFps, platform::preference(L"MostrarFPS", 1, 1));
    check(Sensors, platform::preference(L"Sensores", 0, 1));
    check(Record, hasVideo && platform::preference(L"Video", 0, 1));
    SendMessageW(seconds, 0x14e, platform::preference(L"Duracao", 1, 2), 0);
    SendMessageW(profileBox, 0x14e, platform::preference(L"Perfil", 0, 2), 0);
    refreshGames();
    resultsText();
    syncControls();
    hotkey = RegisterHotKey(panel, 1, 0x4002, 0x24) != 0;
    if (!hotkey)
        say(words::hotkeyTaken.get());
    if (!SetTimer(panel, 1, 250, nullptr)) {
        MessageBoxW(panel, words::timerFailed.get(), L"CIRCE", 16);
        panelui::close();
        return 1;
    }
    ShowWindow(panel, 5);
    Msg m{};
    while (GetMessageW(&m, nullptr, 0, 0) > 0) {
        if (opened && m.message == 0x100 && m.w == 27 && game) {
            if (SendMessageW(gameList, 0x157, 0, 0) || SendMessageW(seconds, 0x157, 0, 0) ||
                SendMessageW(profileBox, 0x157, 0, 0)) {
                TranslateMessage(&m);
                DispatchMessageW(&m);
            } else
                hide();
            continue;
        }
        if (opened && IsDialogMessageW(panel, &m))
            continue;
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
    if (gameProcess)
        CloseHandle(gameProcess);
    release(diagnostic);
    panelui::close();
    return 0;
}
