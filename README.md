# ytdl-gui

A tiny native Windows GUI for [yt-dlp](https://github.com/yt-dlp/yt-dlp). One small `.exe`, no runtime or installer, written in plain C++ with the Win32 API (about 200 lines).

- Paste a YouTube link, pick a folder, press **Download**
- Video (MP4) or audio only (MP3), with an optional max quality (4K down to 360p)
- Live progress bar and log, with a Cancel button
- Single videos only (playlist links download just the one video)

## Requirements

- Windows 10 or 11
- [`yt-dlp.exe`](https://github.com/yt-dlp/yt-dlp/releases/latest) next to `ytdl-gui.exe`, or anywhere on your `PATH`
- [`ffmpeg`](https://ffmpeg.org/) on your `PATH` (needed to merge video and audio, and for MP3)
- A JavaScript runtime ([Deno](https://deno.com/) or [Node.js](https://nodejs.org/)) on your `PATH`. Recent yt-dlp versions need one for YouTube. ytdl-gui passes `--js-runtimes node` automatically when only Node is found.

ytdl-gui does not bundle or download yt-dlp. Keep it current with `yt-dlp -U`.

## Build

Needs CMake and a C++17 compiler (Visual Studio / MSVC).

```
cmake -S . -B build
cmake --build build --config Release
```

The program is `build\Release\ytdl-gui.exe`.

## Responsible use

Only download videos you own, that are licensed for reuse, or that you otherwise have permission to save. Downloading may violate YouTube's Terms of Service. You are responsible for how you use this tool.

## License

MIT, see [LICENSE](LICENSE). yt-dlp is released under the Unlicense and is a separate project.
