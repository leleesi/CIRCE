#include "messages.hpp"
static H gameList;
static bool manualTarget = false;
struct WindowChoice {
    H hwnd;
    U pid;
};
static WindowChoice choices[512];
static unsigned choiceCount;
static const W* filename(const W* path) {
    const W* file = path;
    for (const W* p = path; *p; ++p)
        if (*p == L'\\')
            file = p + 1;
    return file;
}
static I addWindow(H hwnd, L) {
    if (choiceCount >= 512 || !IsWindowVisible(hwnd) || hwnd == GetShellWindow())
        return 1;
    U owner = 0;
    GetWindowThreadProcessId(hwnd, &owner);
    if (!owner || owner == GetCurrentProcessId())
        return 1;
    W title[200]{};
    if (!GetWindowTextW(hwnd, title, 200))
        return 1;
    W path[2048]{};
    U size = 2048;
    H proc = OpenProcess(0x1000, 0, owner);
    if (proc) {
        QueryFullProcessImageNameW(proc, 0, path, &size);
        CloseHandle(proc);
    }
    Text<480> label;
    label.add(title);
    if (*path) {
        label.add(L"  ·  ");
        label.add(filename(path));
    }
    L index = SendMessageW(gameList, 0x143, 0, (L)label.buf);
    if (index > 0 && index < 512) {
        choices[index] = {hwnd, owner};
        ++choiceCount;
        if (manualTarget && hwnd == game && owner == pid)
            SendMessageW(gameList, 0x14e, index, 0);
    }
    return 1;
}
static void refreshGames() {
    SendMessageW(gameList, 0x14b, 0, 0);
    SendMessageW(gameList, 0x143, 0, (L)words::automaticGame.get());
    SendMessageW(gameList, 0x14e, 0, 0);
    choiceCount = 1;
    EnumWindows(addWindow, 0);
}
static void chooseGame() {
    if (stage) {
        say(words::finishBeforeSwitch.get());
        refreshGames();
        return;
    }
    L i = SendMessageW(gameList, 0x147, 0, 0);
    if (i < 0 || (Q)i >= choiceCount)
        return;
    if (i == 0) {
        manualTarget = false;
        if (lastGameWindow && IsWindow(lastGameWindow))
            attach(lastGameWindow);
        say(words::selectWithHotkey.get());
        return;
    }
    auto choice = choices[i];
    U owner = 0;
    GetWindowThreadProcessId(choice.hwnd, &owner);
    if (!owner || owner != choice.pid) {
        refreshGames();
        say(words::refreshGame.get());
        return;
    }
    manualTarget = true;
    if (!attach(choice.hwnd)) {
        say(words::measurementFailed.get());
        return;
    }
    say(words::gameSelected.get());
}
