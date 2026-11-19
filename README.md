<p align="center"><img src="assets/CIRCE.png" width="108" height="108" alt="CIRCE"></p>

# CIRCE

Change a graphics setting. Play the same scene twice. See what it did to your frame rate.

CIRCE is a native C++ benchmark for **Windows 11 x64**. Press **Ctrl + Home** inside your game to choose a window, record two runs and compare them. The installer, overlay, video captions and PDF report are available in English and Portuguese.

[Download for Windows](https://github.com/leleesi/CIRCE/releases/latest/download/CIRCE-Setup.exe) · [Releases](https://github.com/leleesi/CIRCE/releases) · [Português](LEIA-ME.md) · [User guide](docs/guide-en.md) · [Release notes](docs/releases/v0.0.1.md) · [Report a bug](https://github.com/leleesi/CIRCE/issues/new/choose) · [Contributing](CONTRIBUTING.md)

Created and maintained by [@leleesi](https://github.com/leleesi). Official builds are published in [leleesi/CIRCE Releases](https://github.com/leleesi/CIRCE/releases). If you distribute a modified build, please identify your fork and its maintainer.

## Install and run your first comparison

1. Open **Releases** on the GitHub repository and download **CIRCE-Setup.exe** from **v0.0.1**. Choose your language. Select the video component if you want clips, and the desktop shortcut if you want one.
2. Open CIRCE, accept the Windows administrator prompt and start the game in **borderless windowed mode**. Press **Ctrl + Home**. Check the selected game in **Session**.
3. Choose 15, 30 or 60 seconds. Select **Record Before**, play the scene, change one setting, then select **Record After** and repeat the scene.
4. Open **Results** to compare the runs. Choose where to save your PDF or side-by-side MP4. All features are free, including video and PDF export.

## What you get

| During the game | After the two runs |
| --- | --- |
| Live FPS from the selected game, measured with PresentMon | Average FPS, P99 frame time and percentage change |
| Optional system-wide CPU and RAM readings | A PDF report in the selected language |
| Optional clips of the selected window | A silent split-screen MP4 with a live FPS label on each side |

The [sample PDF report](docs/examples/comparison-en.pdf) was exported by CIRCE using two synthetic runs.

The video component downloads FFmpeg during installation. FPS measurement and PDF export work offline with the included PresentMon. The [user guide](docs/guide-en.md) covers capture profiles, output folders, diagnostics and games that use exclusive fullscreen.

## Build from source

From the repository root, build with Visual Studio 2022, the x64 C++ tools and a recent Windows SDK.

```bat
native\build.cmd
```

To build the installer from Git, run `py -3 tools\prepare_presentmon.py` to fetch and verify PresentMon, then `makensis instalador\CIRCE.nsi` with NSIS 3.11 or newer and the AMD64 Unicode stubs and plugins. For the Linux cross-build, automated tests and source layout, see [Contributing](CONTRIBUTING.md).

## License

The [MIT License](LICENSE) covers CIRCE's original code, documentation and artwork. The bundled [PresentMon](https://github.com/GameTechDev/PresentMon/releases/tag/v2.4.1) includes its [own notices](vendor/PresentMon/THIRD_PARTY.txt). The optional [FFmpeg build](https://github.com/FFmpeg-Builds/FFmpeg-Builds/releases) installs its notices with the video component.
