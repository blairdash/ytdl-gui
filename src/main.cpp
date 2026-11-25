// ytdl-gui - a small native Windows GUI for yt-dlp. MIT licensed.
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shellapi.h>
#include <filesystem>
#include <string>
#include <vector>
namespace fs = std::filesystem;

enum { ID_URL = 101, ID_PASTE, ID_DIR, ID_BROWSE, ID_MODE, ID_QUAL, ID_GO, ID_CANCEL, ID_BAR, ID_LOG, ID_STATUS };
enum { WM_LOGLINE = WM_APP + 1, WM_PROGRESS, WM_DONE };

static HWND hUrl, hDir, hMode, hQual, hGo, hCancel, hBar, hLog, hStatus, hMain;
static HANDLE gProc = nullptr; static volatile bool gCancel = false;

static std::wstring W(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(n, 0); MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), w.data(), n); return w;
}
static std::wstring Text(HWND h) { int n = GetWindowTextLengthW(h); std::wstring s(n + 1, 0); GetWindowTextW(h, s.data(), n + 1); s.resize(n); return s; }
static std::wstring Quote(const std::wstring& a) {
    std::wstring r = L"\""; size_t bs = 0;
    for (wchar_t c : a) {
        if (c == L'\\') { bs++; continue; }
        if (c == L'"') { r.append(bs * 2 + 1, L'\\'); r += L'"'; bs = 0; continue; }
        r.append(bs, L'\\'); bs = 0; r += c;
    }
    r.append(bs * 2, L'\\'); return r + L"\"";
}
static bool IsYouTube(std::wstring u) {
    for (auto& c : u) c = (wchar_t)towlower(c);
    for (const wchar_t* p : {L"https://www.youtube.com/", L"https://youtube.com/", L"https://m.youtube.com/", L"https://music.youtube.com/", L"https://youtu.be/"})
        if (u.rfind(p, 0) == 0) return true;
    return false;
}
static fs::path FindYtDlp() {
    wchar_t b[MAX_PATH]; GetModuleFileNameW(nullptr, b, MAX_PATH);
    fs::path local = fs::path(b).parent_path() / L"yt-dlp.exe"; if (fs::exists(local)) return local;
    if (SearchPathW(nullptr, L"yt-dlp.exe", nullptr, MAX_PATH, b, nullptr)) return b;
    return {};
}
static void AppendLog(const std::wstring& line) {
    int n = GetWindowTextLengthW(hLog); SendMessageW(hLog, EM_SETSEL, n, n);
    std::wstring l = line + L"\r\n"; SendMessageW(hLog, EM_REPLACESEL, FALSE, (LPARAM)l.c_str());
}
static void SetBusy(bool busy) {
    EnableWindow(hGo, !busy); EnableWindow(hCancel, busy); EnableWindow(hUrl, !busy); EnableWindow(hDir, !busy);
    EnableWindow(hMode, !busy); EnableWindow(hQual, !busy);
}

struct Job { std::wstring cmd; };
static DWORD WINAPI Worker(LPVOID p) {
    Job* job = (Job*)p;
    SECURITY_ATTRIBUTES sa{sizeof sa, nullptr, TRUE}; HANDLE rd = nullptr, wr = nullptr;
    CreatePipe(&rd, &wr, &sa, 0); SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0);
    STARTUPINFOW si{}; si.cb = sizeof si; si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW; si.wShowWindow = SW_HIDE;
    si.hStdOutput = wr; si.hStdError = wr; si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    PROCESS_INFORMATION pi{};
    int code = -1;
    if (CreateProcessW(nullptr, job->cmd.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        gProc = pi.hProcess; CloseHandle(wr); wr = nullptr;
        std::string buf; char tmp[4096]; DWORD got;
        while (ReadFile(rd, tmp, sizeof tmp, &got, nullptr) && got) {
            buf.append(tmp, got); size_t pos;
            while ((pos = buf.find_first_of("\r\n")) != std::string::npos) {
                std::string line = buf.substr(0, pos); buf.erase(0, pos + 1);
                if (line.empty()) continue;
                size_t pc = line.find('%');
                if (line.rfind("[download]", 0) == 0 && pc != std::string::npos && pc > 0) {
                    size_t s = pc; while (s > 0 && (isdigit((unsigned char)line[s - 1]) || line[s - 1] == '.')) s--;
                    PostMessageW(hMain, WM_PROGRESS, (WPARAM)(atof(line.substr(s, pc - s).c_str()) * 10), 0);
                    if (line.find(" ETA ") == std::string::npos && line.find("100%") == std::string::npos) continue;
                }
                PostMessageW(hMain, WM_LOGLINE, 0, (LPARAM)new std::wstring(W(line)));
            }
        }
        WaitForSingleObject(pi.hProcess, INFINITE); DWORD ec = 1; GetExitCodeProcess(pi.hProcess, &ec); code = (int)ec;
        gProc = nullptr; CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
    } else PostMessageW(hMain, WM_LOGLINE, 0, (LPARAM)new std::wstring(L"Could not start yt-dlp."));
    if (wr) CloseHandle(wr); CloseHandle(rd);
    PostMessageW(hMain, WM_DONE, gCancel ? -2 : code, 0);
    delete job; return 0;
}

static void StartDownload() {
    std::wstring url = Text(hUrl), dir = Text(hDir);
    if (!IsYouTube(url)) { MessageBoxW(hMain, L"Enter a YouTube link (https://youtube.com/... or https://youtu.be/...).", L"ytdl-gui", MB_ICONINFORMATION); return; }
    fs::path y = FindYtDlp();
    if (y.empty()) { MessageBoxW(hMain, L"yt-dlp.exe was not found.\nPut it next to ytdl-gui.exe or on your PATH.\nGet it from github.com/yt-dlp/yt-dlp/releases", L"ytdl-gui", MB_ICONWARNING); return; }
    std::error_code ec; fs::create_directories(dir, ec);
    bool audio = SendMessageW(hMode, CB_GETCURSEL, 0, 0) == 1; int q = (int)SendMessageW(hQual, CB_GETCURSEL, 0, 0);
    static const int heights[] = {0, 2160, 1440, 1080, 720, 480, 360};
    std::wstring cmd = Quote(y.wstring()) + L" --no-playlist --newline --encoding utf-8 -P " + Quote(dir) + L" -o " + Quote(L"%(title).150B [%(id)s].%(ext)s");
    if (audio) cmd += L" -x --audio-format mp3";
    else {
        int h = heights[q < 0 ? 0 : q];
        std::wstring f = h ? L"bv*[height<=" + std::to_wstring(h) + L"]+ba/b[height<=" + std::to_wstring(h) + L"]" : L"bv*+ba/b";
        cmd += L" -f " + Quote(f) + L" --merge-output-format mp4";
    }
    { wchar_t tb[MAX_PATH]; if (!SearchPathW(nullptr, L"deno.exe", nullptr, MAX_PATH, tb, nullptr) && SearchPathW(nullptr, L"node.exe", nullptr, MAX_PATH, tb, nullptr)) cmd += L" --js-runtimes node"; }
    cmd += L" " + Quote(url);
    SetWindowTextW(hLog, L""); SendMessageW(hBar, PBM_SETPOS, 0, 0); SetWindowTextW(hStatus, L"Downloading..."); gCancel = false; SetBusy(true);
    HANDLE t = CreateThread(nullptr, 0, Worker, new Job{cmd}, 0, nullptr); if (t) CloseHandle(t);
}

static void Browse() {
    BROWSEINFOW bi{}; bi.hwndOwner = hMain; bi.lpszTitle = L"Save videos to:"; bi.ulFlags = BIF_NEWDIALOGSTYLE | BIF_RETURNONLYFSDIRS;
    if (PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi)) { wchar_t p[MAX_PATH]; if (SHGetPathFromIDListW(pidl, p)) SetWindowTextW(hDir, p); CoTaskMemFree(pidl); }
}
static void Paste() {
    if (!OpenClipboard(hMain)) return;
    if (HANDLE h = GetClipboardData(CF_UNICODETEXT)) { if (auto s = (const wchar_t*)GlobalLock(h)) { std::wstring t = s; GlobalUnlock(h);
        while (!t.empty() && iswspace(t.back())) t.pop_back(); SetWindowTextW(hUrl, t.c_str()); } }
    CloseClipboard();
}

static HWND Mk(const wchar_t* cls, const wchar_t* txt, DWORD style, int x, int y, int w, int h, int id, DWORD ex = 0) {
    HWND c = CreateWindowExW(ex, cls, txt, WS_CHILD | WS_VISIBLE | style, x, y, w, h, hMain, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
    SendMessageW(c, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE); return c;
}
static LRESULT CALLBACK Proc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_CREATE: {
        hMain = h;
        Mk(L"STATIC", L"YouTube link:", 0, 12, 14, 90, 18, 0);
        hUrl = Mk(L"EDIT", L"", WS_TABSTOP | ES_AUTOHSCROLL, 105, 10, 400, 24, ID_URL, WS_EX_CLIENTEDGE);
        Mk(L"BUTTON", L"Paste", WS_TABSTOP, 515, 10, 65, 24, ID_PASTE);
        Mk(L"STATIC", L"Save to:", 0, 12, 48, 90, 18, 0);
        wchar_t* dl = nullptr; std::wstring def = L"."; if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Downloads, 0, nullptr, &dl))) { def = (fs::path(dl) / L"YouTube").wstring(); CoTaskMemFree(dl); }
        hDir = Mk(L"EDIT", def.c_str(), WS_TABSTOP | ES_AUTOHSCROLL, 105, 44, 400, 24, ID_DIR, WS_EX_CLIENTEDGE);
        Mk(L"BUTTON", L"Browse...", WS_TABSTOP, 515, 44, 65, 24, ID_BROWSE);
        Mk(L"STATIC", L"Format:", 0, 12, 82, 90, 18, 0);
        hMode = Mk(L"COMBOBOX", L"", WS_TABSTOP | CBS_DROPDOWNLIST, 105, 78, 140, 120, ID_MODE);
        SendMessageW(hMode, CB_ADDSTRING, 0, (LPARAM)L"Video (MP4)"); SendMessageW(hMode, CB_ADDSTRING, 0, (LPARAM)L"Audio only (MP3)"); SendMessageW(hMode, CB_SETCURSEL, 0, 0);
        Mk(L"STATIC", L"Max quality:", 0, 270, 82, 80, 18, 0);
        hQual = Mk(L"COMBOBOX", L"", WS_TABSTOP | CBS_DROPDOWNLIST, 355, 78, 150, 160, ID_QUAL);
        for (auto s : {L"Best", L"2160p (4K)", L"1440p", L"1080p", L"720p", L"480p", L"360p"}) SendMessageW(hQual, CB_ADDSTRING, 0, (LPARAM)s);
        SendMessageW(hQual, CB_SETCURSEL, 0, 0);
        hGo = Mk(L"BUTTON", L"Download", WS_TABSTOP | BS_DEFPUSHBUTTON, 12, 116, 100, 30, ID_GO);
        hCancel = Mk(L"BUTTON", L"Cancel", WS_TABSTOP, 120, 116, 80, 30, ID_CANCEL); EnableWindow(hCancel, FALSE);
        hBar = Mk(PROGRESS_CLASSW, L"", PBS_SMOOTH, 12, 156, 568, 18, ID_BAR); SendMessageW(hBar, PBM_SETRANGE32, 0, 1000);
        hStatus = Mk(L"STATIC", FindYtDlp().empty() ? L"yt-dlp.exe not found - put it next to this app." : L"Ready.", 0, 12, 180, 568, 18, ID_STATUS);
        hLog = Mk(L"EDIT", L"", WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL, 12, 204, 568, 190, ID_LOG, WS_EX_CLIENTEDGE);
        return 0; }
    case WM_COMMAND:
        switch (LOWORD(w)) {
        case ID_GO: StartDownload(); break;
        case ID_PASTE: Paste(); break;
        case ID_BROWSE: Browse(); break;
        case ID_CANCEL: gCancel = true; if (gProc) TerminateProcess(gProc, 1); break;
        }
        return 0;
    case WM_LOGLINE: { std::wstring* s = (std::wstring*)l; AppendLog(*s); delete s; return 0; }
    case WM_PROGRESS: SendMessageW(hBar, PBM_SETPOS, w, 0); SetWindowTextW(hStatus, (L"Downloading... " + std::to_wstring((int)(w / 10)) + L"%").c_str()); return 0;
    case WM_DONE:
        SetBusy(false);
        if ((int)w == 0) { SendMessageW(hBar, PBM_SETPOS, 1000, 0); SetWindowTextW(hStatus, L"Done."); }
        else if ((int)w == -2) SetWindowTextW(hStatus, L"Cancelled.");
        else SetWindowTextW(hStatus, L"Failed - see the log below.");
        return 0;
    case WM_DESTROY: if (gProc) TerminateProcess(gProc, 1); PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(h, m, w, l);
}
int WINAPI wWinMain(HINSTANCE hi, HINSTANCE, PWSTR, int show) {
    CoInitialize(nullptr); INITCOMMONCONTROLSEX ic{sizeof ic, ICC_PROGRESS_CLASS}; InitCommonControlsEx(&ic);
    WNDCLASSW wc{}; wc.lpfnWndProc = Proc; wc.hInstance = hi; wc.hCursor = LoadCursorW(nullptr, IDC_ARROW); wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1); wc.lpszClassName = L"YtdlGui";
    RegisterClassW(&wc);
    HWND w = CreateWindowExW(0, L"YtdlGui", L"ytdl-gui", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 606, 445, nullptr, nullptr, hi, nullptr);
    ShowWindow(w, show);
    MSG m; while (GetMessageW(&m, nullptr, 0, 0)) { if (!IsDialogMessageW(w, &m)) { TranslateMessage(&m); DispatchMessageW(&m); } }
    return 0;
}
