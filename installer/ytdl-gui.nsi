; ytdl-gui installer (NSIS 3). Per-user install, no admin needed.
; Downloads yt-dlp, Deno and ffmpeg from their official GitHub releases at install time (nothing is bundled).
Unicode true
!define APP "ytdl-gui"
!define VERSION "1.0.0"
!define UNINST "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP}"
!include "MUI2.nsh"
!include "LogicLib.nsh"

Name "${APP} ${VERSION}"
OutFile "..\dist\ytdl-gui-setup-${VERSION}.exe"
InstallDir "$LOCALAPPDATA\Programs\${APP}"
InstallDirRegKey HKCU "Software\${APP}" "InstallDir"
RequestExecutionLevel user
SetCompressor /SOLID lzma

!insertmacro MUI_PAGE_LICENSE "..\LICENSE"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN "$INSTDIR\ytdl-gui.exe"
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

; Download helper: Download <url> <file>. Uses curl.exe, which ships with Windows 10 1803+ and Windows 11.
!macro Download URL FILE NAME
  DetailPrint "Downloading ${NAME}..."
  nsExec::ExecToLog '"$SYSDIR\curl.exe" -L --fail --silent --show-error --retry 3 -o "${FILE}" "${URL}"'
  Pop $0
  ${If} $0 != 0
    MessageBox MB_ICONEXCLAMATION|MB_OK "Could not download ${NAME} (error $0). Check your internet connection.$\r$\nYou can run this installer again later, or put the file next to ytdl-gui.exe yourself."
    DetailPrint "FAILED: ${NAME}"
    Delete "${FILE}"
  ${EndIf}
!macroend

Section "ytdl-gui (required)" SecApp
  SectionIn RO
  SetOutPath "$INSTDIR"
  File "..\build\Release\ytdl-gui.exe"
  File "..\LICENSE"
  File "..\README.md"
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  CreateDirectory "$SMPROGRAMS\${APP}"
  CreateShortcut "$SMPROGRAMS\${APP}\${APP}.lnk" "$INSTDIR\ytdl-gui.exe"
  CreateShortcut "$SMPROGRAMS\${APP}\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
  WriteRegStr HKCU "Software\${APP}" "InstallDir" "$INSTDIR"
  WriteRegStr HKCU "${UNINST}" "DisplayName" "${APP}"
  WriteRegStr HKCU "${UNINST}" "DisplayVersion" "${VERSION}"
  WriteRegStr HKCU "${UNINST}" "Publisher" "blairdash"
  WriteRegStr HKCU "${UNINST}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
  WriteRegStr HKCU "${UNINST}" "InstallLocation" "$INSTDIR"
  WriteRegDWORD HKCU "${UNINST}" "NoModify" 1
  WriteRegDWORD HKCU "${UNINST}" "NoRepair" 1
SectionEnd

Section "yt-dlp (downloaded)" SecYtdlp
  SectionIn RO
  SetOutPath "$INSTDIR"
  !insertmacro Download "https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe" "$INSTDIR\yt-dlp.exe" "yt-dlp"
SectionEnd

Section "Deno - JavaScript runtime YouTube needs (downloaded)" SecDeno
  SetOutPath "$INSTDIR"
  !insertmacro Download "https://github.com/denoland/deno/releases/latest/download/deno-x86_64-pc-windows-msvc.zip" "$TEMP\ytdl-deno.zip" "Deno"
  IfFileExists "$TEMP\ytdl-deno.zip" 0 +3
    nsExec::ExecToLog '"$SYSDIR\tar.exe" -xf "$TEMP\ytdl-deno.zip" -C "$INSTDIR" deno.exe'
    Delete "$TEMP\ytdl-deno.zip"
SectionEnd

Section "ffmpeg - merges video+audio and makes MP3 (downloaded)" SecFfmpeg
  SetOutPath "$INSTDIR"
  !insertmacro Download "https://github.com/yt-dlp/FFmpeg-Builds/releases/latest/download/ffmpeg-master-latest-win64-gpl.zip" "$TEMP\ytdl-ffmpeg.zip" "ffmpeg"
  IfFileExists "$TEMP\ytdl-ffmpeg.zip" 0 +4
    DetailPrint "Extracting ffmpeg..."
    nsExec::ExecToLog '"$SYSDIR\tar.exe" -xf "$TEMP\ytdl-ffmpeg.zip" -C "$INSTDIR" --strip-components=2 "*/bin/ffmpeg.exe" "*/bin/ffprobe.exe"'
    Delete "$TEMP\ytdl-ffmpeg.zip"
SectionEnd

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecApp} "The ytdl-gui program."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecYtdlp} "Downloads the latest yt-dlp from github.com/yt-dlp/yt-dlp."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecDeno} "Downloads Deno from github.com/denoland/deno. Needed by current yt-dlp to read YouTube. Skip it if you already have Deno or Node on your PATH."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecFfmpeg} "Downloads ffmpeg from github.com/yt-dlp/FFmpeg-Builds. Needed for MP4 merging and MP3. Skip it if you already have ffmpeg on your PATH."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

Section "Uninstall"
  Delete "$INSTDIR\ytdl-gui.exe"
  Delete "$INSTDIR\yt-dlp.exe"
  Delete "$INSTDIR\deno.exe"
  Delete "$INSTDIR\ffmpeg.exe"
  Delete "$INSTDIR\ffprobe.exe"
  Delete "$INSTDIR\LICENSE"
  Delete "$INSTDIR\README.md"
  Delete "$INSTDIR\Uninstall.exe"
  RMDir "$INSTDIR"
  Delete "$SMPROGRAMS\${APP}\${APP}.lnk"
  Delete "$SMPROGRAMS\${APP}\Uninstall.lnk"
  RMDir "$SMPROGRAMS\${APP}"
  DeleteRegKey HKCU "${UNINST}"
  DeleteRegKey HKCU "Software\${APP}"
SectionEnd
