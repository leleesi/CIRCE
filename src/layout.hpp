#pragma once
#include "panel.hpp"
#include "messages.hpp"
namespace panelui {
enum Page { Common,Session,Measure,Results };
enum Kind { Label,Section,Small,Readout,Body,Input,Combo,Button,Primary,Check,Tab };
struct Box { int x,y,w,h; };
struct Item { Id id;Page page;Kind kind;Box box;words::Phrase caption; };
inline constexpr int pageW=724,pageH=500;
inline constexpr Item items[]={
 {Wordmark,Common,Section,{62,14,240,30},L"CIRCE"},
 {Back,Common,Button,{487,12,217,34},words::backToGame},
 {TabSession,Common,Tab,{20,58,99,34},words::sessionTab},
 {TabMeasure,Common,Tab,{125,58,120,34},words::measurementTab},
 {TabResults,Common,Tab,{251,58,107,34},words::resultsTab},
 {Status,Common,Body,{33,447,662,44},words::selectGameOrShortcut},

 {TitleGame,Session,Small,{36,120,372,18},words::gameHeading},
 {Games,Session,Combo,{36,148,350,30},L""},
 {Refresh,Session,Button,{397,147,82,31},words::refresh},
 {TitleLive,Session,Small,{518,120,164,18},words::fpsHeading},
 {LiveValue,Session,Readout,{518,140,162,45},L"—"},
 {LiveDetail,Session,Body,{36,188,646,24},words::awaitingGame},
 {TitleDuration,Session,Label,{20,230,65,25},words::duration},
 {Duration,Session,Combo,{91,228,123,26},L""},
 {Cancel,Session,Button,{503,226,201,30},words::cancelRun},
 {TitleA,Session,Section,{37,277,292,27},words::before},
 {NameA,Session,Input,{37,313,300,29},words::high},
 {SummaryA,Session,Body,{37,348,300,25},words::noRun},
 {Before,Session,Primary,{37,383,300,35},words::recordBefore},
 {TitleB,Session,Section,{388,277,292,27},words::after},
 {NameB,Session,Input,{388,313,299,29},words::medium},
 {SummaryB,Session,Body,{388,348,299,25},words::noRun},
 {After,Session,Button,{388,383,299,35},words::recordAfter},

 {TitleHud,Measure,Section,{37,120,288,28},words::overlay},
 {ShowFps,Measure,Check,{37,163,292,26},words::showFps},
 {Sensors,Measure,Check,{37,199,292,26},words::showSensors},
 {ResolutionHint,Measure,Body,{37,244,290,44},words::borderlessHelp},
 {TitleVideo,Measure,Section,{388,120,276,28},words::recording},
 {Record,Measure,Check,{388,160,287,26},words::recordClips},
 {TitleProfile,Measure,Small,{388,198,287,18},words::profileHeading},
 {Profile,Measure,Combo,{388,222,288,26},L""},
 {VideoHelp,Measure,Body,{388,259,289,41},L""},
 {TitleTest,Measure,Section,{38,326,250,28},words::testTitle},
 {TestHelp,Measure,Body,{38,362,310,50},words::testHelp},
 {TryGame,Measure,Primary,{389,324,287,36},words::testGame},
 {Report,Measure,Button,{389,373,287,34},words::reportButton},

 {TitleCompare,Results,Section,{37,119,470,30},words::comparisonTitle},
 {TitleResA,Results,Small,{38,164,184,19},words::beforeCaps},
 {TitleResB,Results,Small,{257,164,184,19},words::afterCaps},
 {TitleDelta,Results,Small,{481,164,180,19},words::deltaHeading},
 {ResultA,Results,Body,{38,192,194,52},words::noRun},
 {ResultB,Results,Body,{257,192,194,52},words::noRun},
 {Delta,Results,Section,{481,186,182,48},L"—"},
 {Movie,Results,Primary,{37,247,151,33},words::exportMovie},
 {Pdf,Results,Button,{199,247,143,33},words::exportPdf},
 {ChooseOutput,Results,Button,{353,247,183,33},words::chooseFolderButton},
 {OpenOutput,Results,Button,{547,247,134,33},words::openOutputButton},
 {OutputPath,Results,Small,{38,283,641,20},L""},
 {TitleFiles,Results,Section,{38,331,298,29},words::savedSessions},
 {OpenPair,Results,Button,{37,380,160,34},words::openPair},
 {NewPair,Results,Button,{208,380,152,34},words::newPair},
 {Folder,Results,Button,{371,380,151,34},words::openFolder},
 {History,Results,Button,{533,380,146,34},words::history}
};
struct Card { Page page;Box box; };
inline constexpr Card cards[]={
 {Session,{20,105,684,116}},{Session,{20,266,335,165}},{Session,{371,266,333,165}},
 {Measure,{20,105,335,202}},{Measure,{371,105,333,202}},{Measure,{20,312,684,119}},
 {Results,{20,105,684,201}},{Results,{20,315,684,116}}
};
}
