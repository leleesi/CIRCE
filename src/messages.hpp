#pragma once
#include "language.hpp"
namespace words {
struct Phrase {
    const wchar_t *pt, *en;
    constexpr Phrase(const wchar_t* shared) : pt(shared), en(shared) {}
    constexpr Phrase(const wchar_t* portuguese, const wchar_t* english) : pt(portuguese), en(english) {}
    const wchar_t* get() const { return language::active == language::Id::English ? en : pt; }
};

inline constexpr Phrase backToGame{
    L"Voltar ao jogo    Ctrl + Home",
    L"Back to game    Ctrl + Home"};
inline constexpr Phrase sessionTab{
    L"Sessão",
    L"Session"};
inline constexpr Phrase measurementTab{
    L"Medição",
    L"Measurement"};
inline constexpr Phrase resultsTab{
    L"Resultados",
    L"Results"};
inline constexpr Phrase selectGameOrShortcut{
    L"Escolhe um jogo ou prime Ctrl + Home dentro dele.",
    L"Choose a game or press Ctrl + Home in-game."};
inline constexpr Phrase gameHeading{
    L"JOGO A MEDIR",
    L"GAME TO MEASURE"};
inline constexpr Phrase refresh{
    L"Atualizar",
    L"Refresh"};
inline constexpr Phrase fpsHeading{
    L"FPS DO JOGO",
    L"GAME FPS"};
inline constexpr Phrase awaitingGame{
    L"A aguardar a seleção do jogo.",
    L"Waiting for a game to be selected."};
inline constexpr Phrase duration{
    L"Duração",
    L"Duration"};
inline constexpr Phrase cancelRun{
    L"Cancelar registo",
    L"Cancel recording"};
inline constexpr Phrase before{
    L"Antes",
    L"Before"};
inline constexpr Phrase high{
    L"Alto",
    L"High"};
inline constexpr Phrase noRun{
    L"Por registar",
    L"Not recorded yet"};
inline constexpr Phrase recordBefore{
    L"Registar Antes",
    L"Record Before"};
inline constexpr Phrase after{
    L"Depois",
    L"After"};
inline constexpr Phrase medium{
    L"Médio",
    L"Medium"};
inline constexpr Phrase recordAfter{
    L"Registar Depois",
    L"Record After"};
inline constexpr Phrase overlay{
    L"Sobreposição",
    L"Overlay"};
inline constexpr Phrase showFps{
    L"Mostrar FPS no jogo",
    L"Show FPS in-game"};
inline constexpr Phrase showSensors{
    L"Mostrar CPU e RAM do sistema",
    L"Show system CPU and RAM"};
inline constexpr Phrase borderlessHelp{
    L"Usa o modo janela sem bordas para ver o painel sobre o jogo.",
    L"Use borderless windowed mode to see the panel over the game."};
inline constexpr Phrase recording{
    L"Gravação",
    L"Recording"};
inline constexpr Phrase recordClips{
    L"Gravar clipes",
    L"Record clips"};
inline constexpr Phrase profileHeading{
    L"PERFIL DE VÍDEO",
    L"VIDEO PROFILE"};
inline constexpr Phrase testTitle{
    L"Teste no jogo",
    L"In-game test"};
inline constexpr Phrase testHelp{
    L"Experimenta uma sessão de 3 segundos ou recolhe um diagnóstico do contador.",
    L"Run a 3-second test or collect an FPS diagnostic report."};
inline constexpr Phrase testGame{
    L"Testar no jogo",
    L"Test in-game"};
inline constexpr Phrase reportButton{
    L"Relatório FPS (8 s)",
    L"FPS report (8 s)"};
inline constexpr Phrase comparisonTitle{
    L"Comparação de desempenho",
    L"Performance comparison"};
inline constexpr Phrase beforeCaps{
    L"ANTES",
    L"BEFORE"};
inline constexpr Phrase afterCaps{
    L"DEPOIS",
    L"AFTER"};
inline constexpr Phrase deltaHeading{
    L"VARIAÇÃO DE FPS",
    L"FPS CHANGE"};
inline constexpr Phrase exportMovie{
    L"Exportar MP4",
    L"Export MP4"};
inline constexpr Phrase exportPdf{
    L"Exportar PDF",
    L"Export PDF"};
inline constexpr Phrase chooseFolderButton{L"Escolher pasta…", L"Choose folder…"};
inline constexpr Phrase openOutputButton{L"Abrir destino", L"Open destination"};
inline constexpr Phrase outputLocation{L"Clipes, MP4 e PDF · ", L"Clips, MP4 and PDF · "};
inline constexpr Phrase chooseOutputFolder{L"Escolher pasta para os clipes, MP4 e PDF", L"Choose a folder for clips, MP4 and PDFs"};
inline constexpr Phrase saveMovieAs{L"Guardar vídeo MP4 como", L"Save MP4 video as"};
inline constexpr Phrase savePdfAs{L"Guardar relatório PDF como", L"Save PDF report as"};
inline constexpr Phrase mp4FileType{L"Vídeo MP4", L"MP4 video"};
inline constexpr Phrase pdfFileType{L"Documento PDF", L"PDF document"};
inline constexpr Phrase outputFolderSaved{L"Pasta guardada para os próximos clipes e exportações.", L"Folder saved for future clips and exports."};
inline constexpr Phrase outputFolderUnavailable{L"Não foi possível escrever nesta pasta. Escolhe outra em Resultados.", L"Could not write to this folder. Choose another in Results."};
inline constexpr Phrase outputChoiceFailed{L"Não foi possível selecionar o destino. Experimenta outra pasta ou um caminho mais curto.", L"Could not select the destination. Try another folder or a shorter path."};
inline constexpr Phrase outputPreferenceFailed{L"O Windows não conseguiu guardar a preferência de pasta.", L"Windows could not save your folder preference."};
inline constexpr Phrase openOutputFailed{L"Não foi possível abrir a pasta escolhida.", L"Could not open the selected folder."};
inline constexpr Phrase replaceFile{L"Substituir este ficheiro?", L"Replace this file?"};
inline constexpr Phrase replaceButton{L"Substituir", L"Replace"};
inline constexpr Phrase savedSessions{
    L"Sessões guardadas",
    L"Saved sessions"};
inline constexpr Phrase openPair{
    L"Abrir comparação",
    L"Open comparison"};
inline constexpr Phrase newPair{
    L"Nova comparação",
    L"New comparison"};
inline constexpr Phrase openFolder{
    L"Abrir pasta",
    L"Open folder"};
inline constexpr Phrase history{
    L"Histórico anterior",
    L"Previous history"};
inline constexpr Phrase unnamedGame{
    L"Jogo ",
    L"Game "};
inline constexpr Phrase chooseOrShortcut{
    L"Escolhe o jogo na lista ou usa Ctrl + Home dentro dele.",
    L"Select a game from the list or press Ctrl + Home in-game."};
inline constexpr Phrase chooseGameWindow{
    L"Escolhe a janela do jogo na lista.",
    L"Select the game window from the list."};
inline constexpr Phrase waitForTask{
    L"Aguarda a operação em curso.",
    L"Wait for the current operation to finish."};
inline constexpr Phrase shortcutInGame{
    L"No jogo, prime Ctrl + Home para o selecionar.",
    L"Press Ctrl + Home in the game to select it."};
inline constexpr Phrase sameDuration{
    L"Regista Antes com a mesma duração.",
    L"Record Before with the same duration."};
inline constexpr Phrase restartFailed{
    L"Falha ao reiniciar a medição. Abre o relatório de FPS.",
    L"Could not restart measurement. Open the FPS report."};
inline constexpr Phrase installVideoOrDisable{
    L"Instala o componente de vídeo ou desativa a gravação para medir só FPS.",
    L"Install the video component, or turn off recording to measure FPS only."};
inline constexpr Phrase comparisonFilter{
    L"Comparações CIRCE\0*.cad\0\0",
    L"CIRCE comparisons\0*.cad\0\0"};
inline constexpr Phrase openPairFailed{
    L"Não foi possível abrir a comparação.",
    L"Could not open the comparison."};
inline constexpr Phrase invalidPair{
    L"Formato de comparação inválido.",
    L"Invalid comparison file format."};
inline constexpr Phrase pairOpened{
    L"Comparação aberta.",
    L"Comparison opened."};
inline constexpr Phrase installVideoHelp{
    L"Para gravar, seleciona o componente de vídeo no instalador.",
    L"To record, select the video component in the installer."};
inline constexpr Phrase recordingEnabled{
    L"O teste e os dois registos vão incluir vídeo.",
    L"The test and both sessions will include video."};
inline constexpr Phrase metricsOnly{
    L"Os registos guardam apenas as métricas.",
    L"Only performance metrics will be saved."};
inline constexpr Phrase captureDenied{
    L"O Windows recusou a captura desta janela.",
    L"Windows denied capture of this window."};
inline constexpr Phrase blackFrames{
    L"O jogo devolveu só imagens pretas. Experimenta janela sem bordas.",
    L"The game returned only black frames. Try borderless windowed mode."};
inline constexpr Phrase gameMinimized{
    L"A janela do jogo mudou ou foi minimizada.",
    L"The game window changed or was minimized."};
inline constexpr Phrase encoderIncomplete{
    L"O FFmpeg não terminou a gravação. Consulta video.log na pasta de dados.",
    L"FFmpeg could not finish recording. Check video.log in the data folder."};
inline constexpr Phrase noCaptureFrames{
    L"O gravador não recebeu imagens utilizáveis. Consulta video.log na pasta de dados.",
    L"The recorder received no usable frames. Check video.log in the data folder."};
inline constexpr Phrase testPrefix{
    L"Teste: ",
    L"Test: "};
inline constexpr Phrase averageFpsSuffix{
    L" FPS médios.",
    L" FPS on average."};
inline constexpr Phrase testNoFrames{
    L"Teste sem fotogramas. Abre o relatório de FPS.",
    L"No frames during the test. Open the FPS report."};
inline constexpr Phrase testVideoOk{
    L"Vídeo de teste confirmado.",
    L"Test video confirmed."};
inline constexpr Phrase incompleteRun{
    L"Medição incompleta. Repete o registo com o jogo em primeiro plano.",
    L"Incomplete measurement. Repeat the session with the game in the foreground."};
inline constexpr Phrase saveRunFailed{
    L"Falha ao guardar o registo. Verifica o espaço e as permissões da pasta.",
    L"Could not save the session. Check disk space and folder permissions."};
inline constexpr Phrase afterSaved{
    L"Depois guardado. Abre Resultados para comparar.",
    L"After saved. Open Results to compare."};
inline constexpr Phrase beforeSaved{
    L"Antes guardado. Altera as definições do jogo e regista Depois.",
    L"Before saved. Change your game settings and record After."};
inline constexpr Phrase focusLostCancel{
    L"Registo cancelado. A janela do jogo perdeu o foco.",
    L"Session canceled. The game window lost focus."};
inline constexpr Phrase noRecentGameFrames{
    L"O jogo ainda não enviou fotogramas recentes. Verifica a seleção ou abre o relatório.",
    L"The game has not sent any recent frames. Check the selected game or open the report."};
inline constexpr Phrase videoStartFailed{
    L"Falha ao iniciar o vídeo. Verifica o FFmpeg e o gravador.",
    L"Could not start recording. Check FFmpeg and the recorder."};
inline constexpr Phrase focusLostStop{
    L"Registo interrompido porque o jogo perdeu o foco.",
    L"Session interrupted because the game lost focus."};
inline constexpr Phrase presentMonStartFailed{
    L"Falha ao iniciar PresentMon · erro ",
    L"Could not start PresentMon · error "};
inline constexpr Phrase measurementEnded{
    L"Medição terminada · código ",
    L"Measurement ended · code "};
inline constexpr Phrase openMeasurement{
    L". Abre Medição para diagnosticar.",
    L". Open Measurement for diagnostics."};
inline constexpr Phrase receivingFrames{
    L"A receber fotogramas",
    L"Receiving frames"};
inline constexpr Phrase receivedLate{
    L"Recebido com atraso de ",
    L"Received with a delay of "};
inline constexpr Phrase noRecentFrames{
    L"Sem fotogramas recentes",
    L"No recent frames"};
inline constexpr Phrase incompatibleMetrics{
    L"Formato de medição incompatível. Abre o relatório.",
    L"Unsupported measurement format. Open the report."};
inline constexpr Phrase awaitingIntervals{
    L"À espera de intervalos válidos do jogo.",
    L"Waiting for valid frame intervals from the game."};
inline constexpr Phrase awaitingWindowFrames{
    L"À espera de fotogramas da janela selecionada.",
    L"Waiting for frames from the selected window."};
inline constexpr Phrase noFramesFrom{
    L"Sem fotogramas de ",
    L"No frames from "};
inline constexpr Phrase confirmWindow{
    L". Confirma a janela que apresenta o jogo.",
    L". Select the window that displays the game."};
inline constexpr Phrase awaitingPresentMon{
    L"À espera dos primeiros dados do PresentMon.",
    L"Waiting for the first data from PresentMon."};
inline constexpr Phrase gameClosed{
    L"O jogo fechou. O registo foi interrompido.",
    L"The game closed. The session was interrupted."};
inline constexpr Phrase windowClosed{
    L"A janela do jogo fechou. Seleciona outra janela para continuar.",
    L"The game window closed. Select another window to continue."};
inline constexpr Phrase movieSaved{
    L"MP4 guardado no destino escolhido.",
    L"MP4 saved to the selected destination."};
inline constexpr Phrase delayTag{
    L" · atraso ",
    L" · delay "};
inline constexpr Phrase startingIn{
    L"Início em ",
    L"Starting in "};
inline constexpr Phrase test{
    L"Teste",
    L"Test"};
inline constexpr Phrase saving{
    L"A guardar…",
    L"Saving…"};
inline constexpr Phrase canceled{
    L"Registo cancelado.",
    L"Session canceled."};
inline constexpr Phrase newPairStarted{
    L"Nova comparação. Os registos anteriores estão guardados.",
    L"New comparison. Your previous sessions are saved."};
inline constexpr Phrase confirmStopExport{
    L"Queres interromper a exportação e fechar o CIRCE?",
    L"Stop the export and close CIRCE?"};
inline constexpr Phrase exporting{
    L"Exportação em curso",
    L"Export in progress"};
inline constexpr Phrase cannotOpenData{
    L"Não foi possível abrir a pasta de dados.",
    L"Could not open the data folder."};
inline constexpr Phrase cannotOpenPanel{
    L"Falha ao abrir o painel.",
    L"Could not open the panel."};
inline constexpr Phrase hotkeyTaken{
    L"Ctrl + Home está ocupado. Podes usar este painel pela barra de tarefas.",
    L"Ctrl + Home is already in use. You can open this panel from the taskbar."};
inline constexpr Phrase timerFailed{
    L"Falha ao iniciar o temporizador do painel.",
    L"Could not start the panel timer."};
inline constexpr Phrase seconds15{
    L"15 segundos",
    L"15 seconds"};
inline constexpr Phrase seconds30{
    L"30 segundos",
    L"30 seconds"};
inline constexpr Phrase seconds60{
    L"60 segundos",
    L"60 seconds"};
inline constexpr Phrase automaticGame{
    L"Automático · jogo em primeiro plano",
    L"Automatic · foreground game"};
inline constexpr Phrase finishBeforeSwitch{
    L"Conclui ou cancela o registo antes de mudar de jogo.",
    L"Finish or cancel the session before switching games."};
inline constexpr Phrase selectWithHotkey{
    L"Prime Ctrl + Home na janela do jogo que queres medir.",
    L"Press Ctrl + Home in the game window you want to measure."};
inline constexpr Phrase refreshGame{
    L"A janela mudou. Atualiza a lista e volta a escolhê-la.",
    L"The window changed. Refresh the list and select it again."};
inline constexpr Phrase measurementFailed{
    L"Falha ao iniciar a medição. Consulta o estado do jogo.",
    L"Could not start measurement. Check the game status."};
inline constexpr Phrase gameSelected{
    L"Jogo selecionado. Volta ao jogo para ver a leitura.",
    L"Game selected. Return to the game to see the reading."};
inline constexpr Phrase pdfTitle{
    L"CIRCE  |  Comparação de desempenho",
    L"CIRCE  |  Performance comparison"};
inline constexpr Phrase afterSide{
    L"Depois  ·  ",
    L"After  ·  "};
inline constexpr Phrase beforeSide{
    L"Antes  ·  ",
    L"Before  ·  "};
inline constexpr Phrase averageFps{
    L"FPS médios: ",
    L"Average FPS: "};
inline constexpr Phrase sessionDuration{
    L"Duração de cada sessão: ",
    L"Duration per session: "};
inline constexpr Phrase pdfRequiresRuns{
    L"Conclui as medições Antes e Depois para exportar o PDF.",
    L"Complete the Before and After runs to export the PDF."};
inline constexpr Phrase pdfSaved{
    L"PDF guardado no destino escolhido.",
    L"PDF saved to the selected destination."};
inline constexpr Phrase pdfWriteFailed{
    L"Falha ao guardar. Fecha o PDF se estiver aberto.",
    L"Could not save the PDF. Close it if it is open."};
inline constexpr Phrase movieFilename{
    L"Antes-Depois-",
    L"Before-After-"};
inline constexpr Phrase clockMismatch{
    L"As imagens e os tempos da sessão não permitem uma comparação sincronizada.",
    L"The frames and session timestamps cannot be synchronized."};
inline constexpr Phrase missingTiming{
    L"Faltam clipes, tempos de captura ou medições da comparação.",
    L"Clips, capture timestamps or measurements are missing."};
inline constexpr Phrase mp4EncodeFailed{
    L"Falha ao codificar o MP4. Verifica o espaço em disco e o FFmpeg.",
    L"Could not encode the MP4. Check disk space and FFmpeg."};
inline constexpr Phrase clipReadFailed{
    L"Falha ao ler os clipes de vídeo. Verifica os ficheiros da comparação.",
    L"Could not read the video clips. Check the comparison files."};
inline constexpr Phrase exportInterrupted{
    L"Exportação interrompida. Os clipes originais estão guardados.",
    L"Export interrupted. The original clips have been kept."};
inline constexpr Phrase movieWriteFailed{
    L"Falha ao guardar o MP4. Verifica o destino e o espaço disponível.",
    L"Could not save the MP4. Check the destination and available space."};
inline constexpr Phrase twoClipsRequired{
    L"Grava clipes nas duas medições para exportar o vídeo.",
    L"Record clips for both runs to export the video."};
inline constexpr Phrase creatingMovie{
    L"A sincronizar os dois clipes e a criar o MP4…",
    L"Synchronizing both clips and creating the MP4…"};
inline constexpr Phrase exportStartFailed{
    L"Falha ao iniciar a exportação.",
    L"Could not start the export."};
inline constexpr Phrase snapshotHeading{
    L"\ufeffCIRCE 0.0.1 · medição da aplicação\r\nJogo: ",
    L"\ufeffCIRCE 0.0.1 · application measurement\r\nGame: "};
inline constexpr Phrase receivedBytes{
    L"Bytes do PresentMon: ",
    L"Bytes from PresentMon: "};
inline constexpr Phrase lineCount{
    L"Linhas: ",
    L"Lines: "};
inline constexpr Phrase acceptedIntervals{
    L"Intervalos aceites: ",
    L"Accepted intervals: "};
inline constexpr Phrase otherProcesses{
    L"Outros processos: ",
    L"Other processes: "};
inline constexpr Phrase invalidInterval{
    L"Formato ou intervalo inválido: ",
    L"Invalid format or interval: "};
inline constexpr Phrase otherSurface{
    L"Outra superfície: ",
    L"Other swap chain: "};
inline constexpr Phrase outOfOrder{
    L"Fora de ordem: ",
    L"Out of order: "};
inline constexpr Phrase flushStatus{
    L"Estado da entrega ETW (0 = OK): ",
    L"ETW delivery status (0 = OK): "};
inline constexpr Phrase win32LaunchError{
    L"Erro de arranque Win32: ",
    L"Win32 startup error: "};
inline constexpr Phrase collectorStatus{
    L"Estado PresentMon (259 = ativo): ",
    L"PresentMon status (259 = active): "};
inline constexpr Phrase observedWindow{
    L"Janela observada pela aplicação (ms): ",
    L"Time observed by the application (ms): "};
inline constexpr Phrase newIntervals{
    L"Novos intervalos durante o ensaio: ",
    L"New intervals during the test: "};
inline constexpr Phrase lastSample{
    L"Última amostra (FPS): ",
    L"Latest sample (FPS): "};
inline constexpr Phrase selectionChanged{
    L"A seleção do jogo mudou durante o ensaio.\r\n",
    L"The selected game changed during the test.\r\n"};
inline constexpr Phrase csvHeader{
    L"Cabeçalho: ",
    L"Header: "};
inline constexpr Phrase selectBeforeReport{
    L"Escolhe o jogo antes do diagnóstico.",
    L"Select a game before running diagnostics."};
inline constexpr Phrase finishBeforeReport{
    L"Conclui a operação antes do diagnóstico.",
    L"Finish the current operation before running diagnostics."};
inline constexpr Phrase reportRunning{
    L"O relatório está em curso.",
    L"The report is already in progress."};
inline constexpr Phrase snapshotWriteFailed{
    L"Falha ao guardar o diagnóstico na pasta de dados.",
    L"Could not save the diagnostic data in the data folder."};
inline constexpr Phrase keepGameMoving{
    L"Diagnóstico em curso. Mantém o jogo em movimento durante 8 segundos.",
    L"Running diagnostics. Keep the game in motion for 8 seconds."};
inline constexpr Phrase diagnosticStartFailed{
    L"Falha ao iniciar o diagnóstico do Windows.",
    L"Could not start Windows diagnostics."};
inline constexpr Phrase comparisonFilename{
    L"comparacao-",
    L"comparison-"};
inline constexpr Phrase sqliteUnavailable{
    L"SQLite do Windows indisponível.",
    L"Windows SQLite is unavailable."};
inline constexpr Phrase noHistory{
    L"Sem comparações anteriores disponíveis.",
    L"No previous comparisons are available."};
inline constexpr Phrase historyTitle{
    L"Histórico anterior · últimas 25 comparações",
    L"Previous history · last 25 comparisons"};
inline constexpr Phrase cancelButton{L"Cancelar", L"Cancel"};
inline constexpr Phrase stopButton{L"Interromper e sair", L"Stop and quit"};
inline constexpr Phrase continueExport{L"Continuar a exportar", L"Keep exporting"};
}
