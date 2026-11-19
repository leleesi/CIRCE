// The fixture tests exercise this same streaming decoder used for the process pipe.
static char line[8192];
static unsigned used;
static bool overflowLine, headerSeen, frameOverflow;
static int ciPid = -1, ciTime = -1, ciMs = -1, ciChain = -1;
struct ReaderReport {
    Q bytes{}, lines{}, wrongPid{}, invalid{}, otherChain{}, old{}, accepted{};
    char header[2048]{};
};
static ReaderReport counters;
struct SurfaceRate {
    Q id;
    unsigned events;
};
static SurfaceRate surfaces[16];
static Q mainChain;
static double lastFrame, surfaceEpoch;

static void resetReader() {
    counters = {};
    headerSeen = overflowLine = frameOverflow = false;
    count = used = 0;
    ciPid = ciTime = ciMs = ciChain = -1;
    mainChain = 0;
    lastFrame = surfaceEpoch = 0;
    for (auto& item : surfaces)
        item = {};
}
static bool numberField(const char* s, unsigned base, Q& value) {
    value = 0;
    if (base == 16 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        s += 2;
    if (!*s)
        return false;
    for (; *s; ++s) {
        unsigned d;
        if (*s >= '0' && *s <= '9')
            d = *s - '0';
        else if (*s >= 'a' && *s <= 'f')
            d = 10 + *s - 'a';
        else if (*s >= 'A' && *s <= 'F')
            d = 10 + *s - 'A';
        else
            return false;
        if (d >= base || value > (~Q(0) - d) / base)
            return false;
        value = value * base + d;
    }
    return true;
}
static bool intervalField(const char* s, double& value) {
    if (*s < '0' || *s > '9')
        return false;
    value = 0;
    while (*s >= '0' && *s <= '9') {
        value = value * 10 + (*s++ - '0');
        if (value > 60000)
            return false;
    }
    if (*s == '.') {
        ++s;
        double weight = .1;
        while (*s >= '0' && *s <= '9') {
            value += (*s++ - '0') * weight;
            weight *= .1;
        }
    }
    return !*s && value > 0 && value <= 60000;
}
static void observeSurface(Q id, double stamp) {
    if (stamp - surfaceEpoch >= .25) {
        unsigned best = 0, incumbent = 0;
        Q preferred = mainChain;
        for (auto item : surfaces) {
            if (item.id == mainChain)
                incumbent = item.events;
            if (item.events > best) {
                best = item.events;
                preferred = item.id;
            }
        }
        if (stage < 2 && best >= 4 && preferred != mainChain && best > incumbent * 2) {
            mainChain = preferred;
            count = 0;
            lastFrame = 0;
        }
        for (auto& item : surfaces)
            item = {};
        surfaceEpoch = stamp;
    }
    for (auto& item : surfaces)
        if (item.id == id || !item.id) {
            item.id = id;
            ++item.events;
            break;
        }
}
static void row() {
    ++counters.lines;
    char* fields[100];
    int n = 1;
    bool quoted = false;
    fields[0] = line;
    // Save the unsplit header once. Avoid the old per-event copy and header scan.
    const bool maybeHeader =
        !headerSeen || line[0] == 'A' || line[0] == '"' || (unsigned char)line[0] == 0xef;
    char savedHeader[2048];
    bool headerCandidate = maybeHeader && strstr(line, "rocess") != nullptr;
    if (headerCandidate) {
        strncpy(savedHeader, line, sizeof(savedHeader) - 1);
        savedHeader[sizeof(savedHeader) - 1] = 0;
    }
    for (char* p = line; *p; ++p) {
        if (*p == '"')
            quoted = !quoted;
        else if (*p == ',' && !quoted) {
            if (n == 100) {
                ++counters.invalid;
                return;
            }
            *p = 0;
            fields[n++] = p + 1;
        }
    }
    if (quoted) {
        ++counters.invalid;
        return;
    }
    for (int i = 0; i < n; ++i) {
        char*& p = fields[i];
        while (*p == ' ' || *p == '\t')
            ++p;
        auto end = p + strlen(p);
        while (end > p && (end[-1] == ' ' || end[-1] == '\t'))
            --end;
        if (end - p >= 2 && *p == '"' && end[-1] == '"') {
            ++p;
            --end;
        }
        *end = 0;
    }
    if (headerCandidate || !headerSeen) {
        for (int i = 0; i < n; ++i)
            if (columnIs(fields[i], "processid")) {
                headerSeen = true;
                ciPid = i;
                ciTime = ciMs = ciChain = -1;
                if (headerCandidate)
                    strcpy(counters.header, savedHeader);
                for (int j = 0; j < n; ++j) {
                    if (columnIs(fields[j], "qpctime") || columnIs(fields[j], "timeinqpc"))
                        ciTime = j;
                    if (columnIs(fields[j], "msbetweenpresents"))
                        ciMs = j;
                    if (columnIs(fields[j], "swapchainaddress"))
                        ciChain = j;
                }
                return;
            }
    }
    if (ciTime < 0 || ciMs < 0 || ciPid < 0 || ciChain < 0 || ciPid >= n || ciMs >= n || ciTime >= n ||
        ciChain >= n) {
        ++counters.invalid;
        return;
    }
    Q owner = 0, stamp = 0, chain = 0;
    double ms = 0;
    if (!numberField(fields[ciPid], 10, owner)) {
        ++counters.invalid;
        return;
    }
    if (owner != pid) {
        ++counters.wrongPid;
        return;
    }
    if (!numberField(fields[ciTime], 10, stamp) || !stamp || !numberField(fields[ciChain], 16, chain) ||
        !intervalField(fields[ciMs], ms) || freq <= 0) {
        ++counters.invalid;
        return;
    }
    if (!chain)
        chain = ~Q(0);
    double t = double(stamp) / freq;
    observeSurface(chain, t);
    if (chain != mainChain) {
        if (mainChain && (stage >= 2 || t - lastFrame < .5)) {
            ++counters.otherChain;
            return;
        }
        mainChain = chain;
        count = 0;
    }
    if (t <= lastFrame) {
        ++counters.old;
        return;
    }
    lastFrame = t;
    ++counters.accepted;
    constexpr unsigned capacity = sizeof(frames) / sizeof(*frames);
    if (count == capacity) {
        if (stage >= 2)
            frameOverflow = true;
        memmove(frames, frames + capacity / 2, (capacity / 2) * sizeof(Frame));
        count = capacity / 2;
    }
    frames[count++] = {t, ms};
}
static void consumeBytes(const char* bytes, U n) {
    counters.bytes += n;
    for (U j = 0; j < n; ++j) {
        char c = bytes[j];
        if (!c || c == '\r')
            continue;
        if (c == '\n') {
            if (overflowLine)
                ++counters.invalid;
            else if (used) {
                line[used] = 0;
                row();
            }
            used = 0;
            overflowLine = false;
        } else if (used + 1 < sizeof(line))
            line[used++] = c;
        else
            overflowLine = true;
    }
}
static void drain() {
    if (!pipeRead)
        return;
    static char bytes[32768];
    U avail = 0, got = 0;
    for (int i = 0; i < 16 && PeekNamedPipe(pipeRead, nullptr, 0, nullptr, &avail, nullptr) && avail; ++i) {
        if (!ReadFile(pipeRead, bytes, avail < sizeof(bytes) ? avail : sizeof(bytes), &got, nullptr) || !got)
            break;
        consumeBytes(bytes, got);
    }
}
