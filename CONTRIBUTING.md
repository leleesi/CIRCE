# Contributing to CIRCE

Contributions and issue reports are welcome in English or Portuguese. Podes abrir uma issue ou enviar uma alteração em português.

## Work on the project

| Directory | Contents |
| --- | --- |
| `src/` | In-game panel, frame metrics, sessions, localization and exports |
| `native/` | Windows window capture, recording and the Visual Studio build |
| `instalador/` | NSIS installer, language chooser and uninstaller |
| `tools/` | Packaging, dependency checks, FPS diagnostics and test entry points |
| `tests/` | Reference frame data, native tests and installer scenarios |
| `assets/` · `vendor/` | Original CIRCE artwork and pinned PresentMon notices and checksums |

## Build and test

On Windows, install Visual Studio 2022 with **Desktop development with C++** and a recent Windows SDK. From the repository root run `native\build.cmd`. The output is in `bin\`. For an installer, run `py -3 tools\prepare_presentmon.py` once, then `makensis instalador\CIRCE.nsi` using NSIS 3.11 or newer with its AMD64 Unicode stubs and plugins.

The dependency helper fetches the official PresentMon 2.4.1 x64 file and checks its SHA-256 against `vendor/PresentMon/SHA256SUMS`. Use `--source PATH` for an offline build with a local copy. The standalone Windows installer includes the verified file.

On x86-64 Linux, the portable suite uses Python 3, G++ with C++20, FFmpeg with ffprobe and Poppler's pdftotext. Install PowerShell 7 to include the installer and diagnostics checks.

```sh
sh tools/testar.sh --portable
```

The full suite also needs x86-64 MinGW-w64 with WinRT capture headers v14 or newer, MinGW binutils, NSIS with AMD64 Unicode support, 7-Zip and Python 3. It rebuilds the Windows executables, generates the installer and inspects its contents and startup behavior.

```sh
sh tools/testar.sh
```

To package without running the suite, use `sh tools/package.sh`. `CXX_WIN`, `WINDRES` and `SEVENZIP` can override tool paths. For hands-on checks on Windows, install the build, open a game in borderless mode and follow the [first comparison](docs/guide-en.md) in each affected language.

## Report a bug or propose a change

Open an issue with the CIRCE version, Windows version, game, display mode, selected language, steps and expected result. For an FPS issue, **FPS report (8 s)** in the game panel generates a diagnostic report. Share only the relevant portion and remove any personal paths.

For a pull request, describe the behavior you changed and the test you ran. If your change adds visible text, update both languages and check the affected screen or export.
