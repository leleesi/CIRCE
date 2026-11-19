# CIRCE v.0.0.1

CIRCE measures game performance on Windows 11 x64. Press **Ctrl + Home** in a game to open its native panel. PresentMon 2.4.1 and the capture helper are included. Internet access is needed for the optional FFmpeg video component. Windows requests elevation when CIRCE starts.

## Your first comparison

1. Close the previous version using the X on its panel. Open `CIRCE-Setup.exe`. The first window displays the green C and the **Português** and **English** buttons. Select your language. Choose the components and whether to create a desktop shortcut. Run the same installer again to switch languages. New video captions, PDF reports and diagnostics use your choice. Existing recordings and custom profile names work in both languages.
2. Start the game in **borderless windowed mode** and press **Ctrl + Home**. Automatic selection uses the foreground game window. In **Session**, you can also select the window by its title and executable. The game's exclusive fullscreen mode may affect whether an external overlay can appear.
3. Choose a duration of 15, 30 or 60 seconds and name your configuration. In **Measurement**, choose whether to display FPS, system CPU and RAM, and whether to record clips. CIRCE saves these preferences.
4. Click **Record Before**, return to the game and let the three-second countdown finish. Change the game settings and click **Record After** with the same duration.
5. Open **Results** to compare average FPS, P99 frame time and percentage change. An MP4 requires two sessions recorded with video. A PDF requires two completed measurements.

Before the first clip recording, select a folder. You can change it later under **Results → Choose folder…**. **Open destination** opens that folder. CIRCE remembers the choice for your Windows user. **Export MP4** and **Export PDF** open **Save as** in that folder. You can choose a different name or location for each file. If you cancel, you can export again later.

Before and After clips stay alongside their `.qpc` timestamps. If you change folders between recordings, CIRCE can find both clips when you reopen the comparison. Reconnect external drives before opening clips saved there.

Each new Before session starts a comparison. Repeating an already completed After saves a new version of the pair. Comparisons are stored under `%LOCALAPPDATA%\CIRCE\Nativo`. Open a saved `.cad` with **Open comparison**. When moving it, keep its `.frames`, `.qpc` and `.mkv` files beside it. The legacy SQLite history opens read-only. To uninstall, use **Settings → Apps → Installed apps → CIRCE**. Uninstalling keeps comparisons and clips.

## Recording and exports

Every feature in CIRCE is free. To record clips, select the video component in the installer and enable **Measurement → Record clips**. You can export a PDF even without the video component.

When upgrading from Cadência, the installer recognizes the legacy installation. It moves comparisons from `%LOCALAPPDATA%\Cadencia\Nativo` to `%LOCALAPPDATA%\CIRCE\Nativo` if the destination is free. If both locations already exist, each keeps its files. **Open comparison** can open the older files directly.

## Measurement and video

Live FPS uses the most recent 100 ms of presentation intervals from the selected process. CIRCE marks samples delayed by 300 ms or more and reports a lack of recent frames after two seconds. Bringing the panel to the foreground can change a game's frame rate. Session averages use complete intervals recorded during the session. A saved measurement must cover at least 90% of the session. P99 is the 99th percentile of frame intervals, in milliseconds. CPU and RAM readings are system-wide.

CIRCE uses Windows Graphics Capture and Direct3D 11. If capture fails to start, the recorder may use GDI for the same window. Some games, protected content or drivers can prevent capture. Software MPEG-4 encoding can affect performance. Use the same 720p30, 1080p30 or 1080p60 profile for both sessions. The side-by-side MP4 is silent, 1280 × 360 at 30 FPS. It uses the time range common to both clips and shows each side's measured FPS.

**Test in-game** runs a three-second temporary session. **FPS report (8 s)** compares the program's counters with an independent PresentMon capture, then opens `fps-diagnostics.txt` in Notepad. The report includes the executable, PID, accepted or rejected readings and any available recorder log. Raw logs from external tools retain their original wording.

## Build and test

Use `native\build.cmd` to build on Windows with Visual Studio 2022, the x64 C++ tools and a recent Windows SDK. The Linux cross-build and the full test suite are documented in [Contributing](../CONTRIBUTING.md). The Releases page offers the Windows installer and a separate project ZIP with the pinned PresentMon executable for offline builds.

The tests cover frame data, two generated clips, English and Portuguese PDF and video exports, language selection, output folders and the compiled installer. An in-game ETW, capture and overlay test needs a Windows PC and GPU.
