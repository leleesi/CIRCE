// Synchronise both recordings on their actual capture times and overlap only.
struct Decode {
    Process process{};
    H pipe = nullptr;
    double clocks[4000];
    int n = 0, index = -1;
    unsigned char pixels[640 * 360 * 3];
    Frame telemetry[180000];
    int events = 0, eventIndex = 0;
};
static Decode decode[2];
static unsigned char mosaic[1280 * 360 * 3];
static Saved job;
static ExportTarget movieTarget;
enum ExportStatus : U {
    ExportOk,
    ExportMedia,
    ExportDecoder,
    ExportEncoder,
    ExportClock,
    ExportCancelled,
    ExportPublish
};
static std::atomic<bool> cancelExport{false};
static std::atomic<Q> exportProgress{0};

static bool readBlock(H file, void* into, U n) {
    auto dest = (unsigned char*)into;
    while (n) {
        U got = 0;
        if (cancelExport.load() || !ReadFile(file, dest, n, &got, nullptr) || !got)
            return false;
        dest += got;
        n -= got;
    }
    return true;
}
static bool load(const W* path, void* out, U capacity, U stride, int& count) {
    H f = CreateFileW(path, 0x80000000, 1, nullptr, 3, 0, nullptr);
    if (f == bad)
        return false;
    L size = 0;
    bool ok = GetFileSizeEx(f, &size) && size > 0 && size <= capacity && size % stride == 0 &&
              readBlock(f, out, (U)size);
    CloseHandle(f);
    if (ok)
        count = int(size / stride);
    return ok;
}
static U decoder(int i) {
    auto& d = decode[i];
    d.index = -1;
    d.eventIndex = 0;
    Text<2048> stamps;
    stamps.add(job.paths[i]);
    if (stamps.len < 4)
        return ExportMedia;
    stamps.len -= 4;
    stamps.buf[stamps.len] = 0;
    stamps.add(L".qpc");
    if (!load(stamps.buf, d.clocks, sizeof(d.clocks), 8, d.n) ||
        !load(job.framePaths[i], d.telemetry, sizeof(d.telemetry), sizeof(Frame), d.events))
        return ExportMedia;
    for (int k = 0; k < d.n; ++k) {
        d.clocks[k] -= job.origins[i];
        if (!(d.clocks[k] >= 0 && d.clocks[k] <= job.duration + .01) || (k && d.clocks[k] <= d.clocks[k - 1]))
            return ExportClock;
    }
    for (int k = 0; k < d.events; ++k) {
        auto f = d.telemetry[k];
        if (!(f.end >= 0 && f.end <= job.duration + .01 && f.ms > 0 && f.ms <= 60000) ||
            (k && f.end <= d.telemetry[k - 1].end))
            return ExportClock;
    }
    Security sec{sizeof(sec), nullptr, 1};
    H write;
    if (!CreatePipe(&d.pipe, &write, &sec, 0))
        return ExportDecoder;
    H nul = CreateFileW(L"NUL", 0xc0000000, 3, &sec, 3, 0, nullptr);
    Start s{};
    s.in = s.err = nul;
    s.out = write;
    auto ff = component(L"ffmpeg.exe");
    Text<8192> args;
    args.quote(ff.buf);
    args.add(
        L" -v error -nostdin -filter_threads 1 -threads 2 -protocol_whitelist file,pipe -f matroska -i ");
    args.quote(job.paths[i]);
    args.add(L" -an -vf scale=640:360 -fps_mode passthrough -threads 1 -f rawvideo -pix_fmt rgb24 pipe:1");
    bool ok = spawn(ff, args, s, d.process, true);
    CloseHandle(write);
    if (nul != bad)
        CloseHandle(nul);
    return ok ? ExportOk : ExportDecoder;
}
static const char* glyph(wchar_t c) {
    static const char* digits[] = {"111101101101111", "010110010010111", "111001111100111", "111001111001111",
                                   "101101111001001", "111100111001111", "111100111101111", "111001001001001",
                                   "111101111101111", "111101111001111"};
    if (c >= '0' && c <= '9')
        return digits[c - '0'];
    switch (c) {
    case 'A':
        return "010101111101101";
    case 'B':
        return "110101110101110";
    case 'R':
        return "110101110101101";
    case 'N':
        return "101111111111101";
    case 'T':
        return "111010010010010";
    case 'E':
        return "111100110100111";
    case 'S':
        return "111100111001111";
    case 'D':
        return "110101101101110";
    case 'P':
        return "110101110100100";
    case 'O':
        return "111101101101111";
    case 'I':
        return "111010010010111";
    case 'F':
        return "111100110100100";
    case '-':
        return "000000111000000";
    default:
        return "000000000000000";
    }
}
static void videoLabel(int side, double fps) {
    Text<36> legend;
    legend.add((side ? words::afterCaps : words::beforeCaps).get());
    legend.add(L"  ");
    if (fps > 0 && fps < 100000)
        legend.number((unsigned)(fps + .5));
    else
        legend.add(L"-");
    legend.add(L" FPS");
    int length = (int)legend.len, left = side * 640 + 9, top = 9;
    const unsigned rgb[2][3] = {{123, 235, 190}, {196, 177, 255}};
    for (int y = 0; y < 22; ++y)
        for (int x = 0; x < length * 8 + 16; ++x) {
            auto dest = mosaic + ((top + y) * 1280 + left + x) * 3;
            int dx = x - 8, dy = y - 6;
            bool lit = dx >= 0 && dx / 8 < length && dx % 8 < 6 && dy >= 0 && dy < 10 &&
                       glyph(legend.buf[dx / 8])[(dy / 2) * 3 + dx % 8 / 2] == '1';
            for (int k = 0; k < 3; ++k)
                dest[k] = x < 3 ? rgb[side][k] : lit ? 242 : (dest[k] / 5 + 13);
        }
}
static bool compose(double t) {
    for (int i = 0; i < 2; ++i) {
        auto& d = decode[i];
        while (d.index + 1 < d.n && d.clocks[d.index + 1] <= t + .00001) {
            if (!readBlock(d.pipe, d.pixels, sizeof(d.pixels)))
                return false;
            ++d.index;
        }
        if (d.index < 0)
            return false;
        for (int y = 0; y < 360; ++y)
            memcpy(mosaic + (y * 1280 + i * 640) * 3, d.pixels + y * 640 * 3, 640 * 3);
        while (d.eventIndex < d.events && d.telemetry[d.eventIndex].end <= t)
            ++d.eventIndex;
        videoLabel(i, live(d.telemetry, d.eventIndex, t));
    }
    return true;
}
static bool startWriter(const Text<2048>& dest, Process& writer, H& input) {
    H read;
    Security sec{sizeof(sec), nullptr, 1};
    if (!CreatePipe(&read, &input, &sec, 0))
        return false;
    H nul = CreateFileW(L"NUL", 0xc0000000, 3, &sec, 3, 0, nullptr);
    Start s{};
    s.in = read;
    s.out = s.err = nul;
    auto ff = component(L"ffmpeg.exe");
    Text<8192> args;
    args.quote(ff.buf);
    args.add(L" -nostdin -y -v error -filter_threads 1 -f rawvideo -pixel_format rgb24 -video_size 1280x360 "
             L"-framerate 30 -i pipe:0 -c:v mpeg4 -threads 2 -q:v 4 -pix_fmt yuv420p -movflags +faststart ");
    args.quote(dest.buf);
    bool ok = spawn(ff, args, s, writer, true);
    CloseHandle(read);
    if (nul != bad)
        CloseHandle(nul);
    return ok;
}
static U exportComparison(const ExportTarget& target) {
    Process writer{};
    H input = nullptr;
    Text<2048> temp;
    H reservation = reserveExportTemp(target, L".part.mp4", temp);
    if (reservation == bad) return ExportPublish;
    CloseHandle(reservation);
    U state = decoder(0);
    if (state == ExportOk)
        state = decoder(1);
    if (state == ExportOk && !startWriter(temp, writer, input))
        state = ExportEncoder;
    if (state == ExportOk) {
        double first = std::max(decode[0].clocks[0], decode[1].clocks[0]);
        double last = std::min(decode[0].clocks[decode[0].n - 1], decode[1].clocks[decode[1].n - 1]);
        if (last <= first)
            state = ExportClock;
        for (int i = 0; state == ExportOk && !cancelExport && first + i / 30. <= last; ++i) {
            if (!compose(first + i / 30.))
                state = ExportDecoder;
            else if (!writeAll(input, mosaic, sizeof(mosaic)))
                state = ExportEncoder;
            exportProgress = GetTickCount64();
        }
    }
    if (input)
        CloseHandle(input);
    for (auto& d : decode) {
        if (d.pipe) {
            CloseHandle(d.pipe);
            d.pipe = nullptr;
        }
        release(d.process, true);
    }
    if (writer.proc) {
        if (state == ExportOk && !cancelExport &&
            (WaitForSingleObject(writer.proc, 15000) != 0 || !successful(writer)))
            state = ExportEncoder;
        release(writer, true);
    }
    if (cancelExport)
        state = ExportCancelled;
    if (state == ExportOk && !publishExport(temp, target))
        state = ExportPublish;
    if (state != ExportOk)
        DeleteFileW(temp.buf);
    return state;
}
static U exportJob(void*) { return exportCode = exportComparison(movieTarget); }
static const W* exportMessage(U code) {
    switch (code) {
    case ExportOk:
        return words::movieSaved.get();
    case ExportClock:
        return words::clockMismatch.get();
    case ExportMedia:
        return words::missingTiming.get();
    case ExportEncoder:
        return words::mp4EncodeFailed.get();
    case ExportDecoder:
        return words::clipReadFailed.get();
    case ExportCancelled:
        return words::exportInterrupted.get();
    default:
        return words::movieWriteFailed.get();
    }
}
static void render() {
    if (stage || exportThread)
        return;
    if (!clip[0].len || !clip[1].len) {
        say(words::twoClipsRequired.get());
        return;
    }
    job = saved;
    if (!selectExport(platform::OutputKind::Movie, movieTarget)) return;
    cancelExport = false;
    exportProgress = GetTickCount64();
    exportThread = CreateThread(nullptr, 0, exportJob, nullptr, 0, nullptr);
    say(exportThread ? words::creatingMovie.get() : words::exportStartFailed.get());
}
