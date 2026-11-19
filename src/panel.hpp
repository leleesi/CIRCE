#pragma once
namespace panelui {
enum Id {
    Back = 1,
    Before,
    After,
    TryGame,
    Movie,
    OpenPair,
    Folder,
    NewPair,
    Pdf,
    History,
    Games = 12,
    Refresh,
    Report,
    TabSession = 20,
    TabMeasure,
    TabResults,
    Duration,
    NameA,
    NameB,
    ShowFps,
    Sensors,
    Record,
    Profile,
    Status,
    LiveValue,
    LiveDetail,
    SummaryA,
    SummaryB,
    ResultA,
    ResultB,
    Delta,
    Cancel,
    VideoHelp,
    TitleGame,
    TitleLive,
    TitleA,
    TitleB,
    TitleDuration,
    TitleHud,
    TitleVideo,
    TitleTest,
    TestHelp,
    TitleCompare,
    TitleFiles,
    TitleResA,
    TitleResB,
    TitleProfile,
    Wordmark,
    TitleDelta,
    ResolutionHint,
    ChooseOutput, OpenOutput, OutputPath,
    Total
};
using Callback = long long (*)(void*, unsigned, unsigned long long, long long);
bool confirm(void* owner, const wchar_t* title, const wchar_t* message, const wchar_t* accept,
             const wchar_t* dismiss, bool acceptByDefault);
void* create(void* instance, Callback callback);
void* control(Id);
bool handle(void* window, unsigned message, unsigned long long w, long long l, long long* result);
void set(Id, const wchar_t* text);
void enable(Id, bool enabled);
bool checked(Id);
void check(Id, bool value);
void position(void* game);
void displayHud(void* game, const wchar_t* text, bool fresh);
void hideHud();
void close();
} // namespace panelui
