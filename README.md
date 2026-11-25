# ytdl-gui

A tiny native Windows GUI for [yt-dlp](https://github.com/yt-dlp/yt-dlp). One small `.exe`, no runtime or installer, written in plain C++ with the Win32 API (about 200 lines).

- Paste a YouTube link, pick a folder, press **Download**
- Video (MP4) or audio only (MP3), with an optional max quality (4K down to 360p)
- Live progress bar and log, with a Cancel button
- Single videos only (playlist links download just the one video)

## Install (recommended)

Download `ytdl-gui-setup-1.0.0.exe` and run it. It installs just for you (no admin) and downloads the latest **yt-dlp**, **Deno** and **ffmpeg** from their official GitHub releases during setup, so everything works right away. Untick Deno or ffmpeg on the components page if you already have them. Windows 10 1803 or newer is needed (the installer uses the built-in `curl` and `tar`). Uninstall from Windows Settings > Apps.

Update yt-dlp any time by running the installer again.

## Requirements (manual setup)

- Windows 10 or 11
- [`yt-dlp.exe`](https://github.com/yt-dlp/yt-dlp/releases/latest) next to `ytdl-gui.exe`, or anywhere on your `PATH`
- [`ffmpeg`](https://ffmpeg.org/) on your `PATH` (needed to merge video and audio, and for MP3)
- A JavaScript runtime ([Deno](https://deno.com/) or [Node.js](https://nodejs.org/)) on your `PATH`. Recent yt-dlp versions need one for YouTube. ytdl-gui passes `--js-runtimes node` automatically when only Node is found.

ytdl-gui does not bundle or download yt-dlp. Keep it current with `yt-dlp -U`.

## Build

Needs CMake and a C++17 compiler (Visual Studio / MSVC). The installer needs [NSIS 3](https://nsis.sourceforge.io/).

```
cmake -S . -B build
cmake --build build --config Release
```

The program is `build\Release\ytdl-gui.exe`. To build the installer afterwards:

```n"C:\Program Files (x86)\NSIS\makensis.exe" installer\ytdl-gui.nsi
```

The result is `dist\ytdl-gui-setup-1.0.0.exe`.

## Responsible use

Only download videos you own, that are licensed for reuse, or that you otherwise have permission to save. Downloading may violate YouTube's Terms of Service. You are responsible for how you use this tool.

## License

MIT, see [LICENSE](LICENSE). yt-dlp is released under the Unlicense and is a separate project.
