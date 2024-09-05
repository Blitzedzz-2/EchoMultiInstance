#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <comdef.h>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <shellapi.h>
#include <iomanip>
#include <sstream>
#include <map>
#include <pdh.h>

#pragma comment(lib, "comsuppw.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "pdh.lib")

// Window handle for the console
HWND consoleWindow;

// Tray icon data
NOTIFYICONDATA nid;

// Structure to hold instance information
struct InstanceInfo {
    DWORD processId;
    std::wstring name;
    double cpuUsage;
    SIZE_T memoryUsage;
};

// Global variables
std::vector<InstanceInfo> instances;
PDH_HQUERY cpuQuery;
std::map<DWORD, PDH_HCOUNTER> cpuCounters;

// Function prototypes
void setConsoleColor(int color);
void printCentered(const std::wstring& text);
void printBox(const std::wstring& text);
void showLoadingAnimation();
void setupConsole();
void error(const wchar_t* error_title, const wchar_t* error_message);
void killRobloxProcesses();
std::wstring getShortcutTarget(const std::wstring& shortcutPath);
std::wstring expandEnvironmentVariables(const std::wstring& input);
std::wstring extractVersionFromPath(const std::wstring& path);
void launchRobloxInstance(const std::wstring& name);
void updateInstanceList();
void monitorResources();
void displayInstanceInfo();
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void createSystemTrayIcon(HWND hwnd);
void removeSystemTrayIcon();
void drawMenu();
void handleUserInput();

// Main function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Create a hidden window for system tray icon
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"EchoMultiInstanceClass";
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, L"EchoMultiInstanceClass", L"Echo Multi-Instance",
                               0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);

    createSystemTrayIcon(hwnd);

    // Set up console
    AllocConsole();
    FILE* dummy;
    freopen_s(&dummy, "CONIN$", "r", stdin);
    freopen_s(&dummy, "CONOUT$", "w", stdout);
    freopen_s(&dummy, "CONOUT$", "w", stderr);

    consoleWindow = GetConsoleWindow();
    setupConsole();

    // Initialize PDH query for CPU usage
    PdhOpenQuery(NULL, NULL, &cpuQuery);

    // Main program logic
    setConsoleColor(14); // Yellow
    printBox(L"✧･ﾟ: *✧･ﾟ:* 　Echo Multi-Instance　 *:･ﾟ✧*:･ﾟ✧");
    setConsoleColor(15); // White
    printCentered(L"► Created by blitzedzz ◄");
    printCentered(L"► Enhanced by Moe Ron ◄");
    printCentered(L"► (actually Reigen Arataka) ◄");
    printCentered(L"► Special thanks to the invisible hamster squad ◄");
    printCentered(L"► And to the coffee that fueled the development ◄");
    printCentered(L"► May the code be with you ◄");
    std::wcout << std::endl;

    showLoadingAnimation();
    killRobloxProcesses();

    Sleep(1000);
    HANDLE roblox_mutex = CreateMutexW(0, TRUE, L"ROBLOX_singletonMutex");

    if (roblox_mutex == 0) {
        error(L"roblox_mutex", L"Failed to start ROBLOX_singletonMutex");
    }

    if ((DWORD)GetLastError() == ERROR_ALREADY_EXISTS) {
        error(L"GetLastError", L"Mutex already exists");
    } else {
        std::thread resourceMonitorThread(monitorResources);
        resourceMonitorThread.detach();

        while (true) {
            system("cls");
            setupConsole();
            drawMenu();
            displayInstanceInfo();
            handleUserInput();
            Sleep(1000);
        }
    }

    removeSystemTrayIcon();
    return 0;
}

void setConsoleColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void printCentered(const std::wstring& text) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    int padding = (width - text.length()) / 2;
    std::wcout << std::wstring(padding, L' ') << text << std::endl;
}

void printBox(const std::wstring& text) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    int padding = (width - text.length() - 4) / 2;

    std::wcout << L"╔" << std::wstring(width - 2, L'═') << L"╗" << std::endl;
    std::wcout << L"║" << std::wstring(padding, L' ') << text << std::wstring(padding, L' ') << L"║" << std::endl;
    std::wcout << L"╚" << std::wstring(width - 2, L'═') << L"╝" << std::endl;
}

void showLoadingAnimation() {
    const wchar_t* frames[] = {L"⠋", L"⠙", L"⠹", L"⠸", L"⠼", L"⠴", L"⠦", L"⠧", L"⠇", L"⠏"};
    for (int i = 0; i < 30; ++i) {
        printCentered(L"Loading " + std::wstring(frames[i % 10]));
        Sleep(100);
        system("cls");
        setupConsole();
    }
}

void setupConsole() {
    SetConsoleTitle(L"✰ Echo Multi-Instance ✰");

    RECT rectClient, rectWindow;
    GetClientRect(consoleWindow, &rectClient);
    GetWindowRect(consoleWindow, &rectWindow);
    int posx = GetSystemMetrics(SM_CXSCREEN) / 2 - (rectWindow.right - rectWindow.left) / 2;
    int posy = GetSystemMetrics(SM_CYSCREEN) / 2 - (rectWindow.bottom - rectWindow.top) / 2;
    MoveWindow(consoleWindow, posx, posy, rectClient.right - rectClient.left, rectClient.bottom - rectClient.top, TRUE);

    CONSOLE_FONT_INFOEX cfi;
    cfi.cbSize = sizeof(cfi);
    cfi.nFont = 0;
    cfi.dwFontSize.X = 0;
    cfi.dwFontSize.Y = 16;
    cfi.FontFamily = FF_DONTCARE;
    cfi.FontWeight = FW_NORMAL;
    wcscpy_s(cfi.FaceName, L"Consolas");
    SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);

    COORD bufferSize = {80, 25};
    SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), bufferSize);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    SMALL_RECT scrollArea = {0, 0, csbi.dwSize.X - 1, csbi.dwSize.Y - 1};
    SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE), TRUE, &scrollArea);

    system("cls");
}

void error(const wchar_t* error_title, const wchar_t* error_message) {
    MessageBox(NULL, error_message, error_title, MB_ICONERROR);
    exit(-1);
}

void killRobloxProcesses() {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        error(L"CreateToolhelp32Snapshot", L"Failed to create process snapshot");
    }

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe)) {
        do {
            if (wcscmp(pe.szExeFile, L"RobloxPlayerBeta.exe") == 0) {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                if (hProcess != NULL) {
                    TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                }
            }
        } while (Process32Next(hSnapshot, &pe));
    }

    CloseHandle(hSnapshot);
}

std::wstring getShortcutTarget(const std::wstring& shortcutPath) {
    CoInitialize(NULL);
    IShellLink* psl;
    IPersistFile* ppf;

    HRESULT hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
    if (SUCCEEDED(hres)) {
        hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
        if (SUCCEEDED(hres)) {
            hres = ppf->Load(shortcutPath.c_str(), STGM_READ);
            if (SUCCEEDED(hres)) {
                WCHAR targetPath[MAX_PATH];
                psl->GetPath(targetPath, MAX_PATH, NULL, SLGP_UNCPRIORITY);
                ppf->Release();
                psl->Release();
                CoUninitialize();
                return targetPath;
            }
            ppf->Release();
        }
        psl->Release();
    }
    CoUninitialize();
    return L"";
}

std::wstring expandEnvironmentVariables(const std::wstring& input) {
    DWORD size = ExpandEnvironmentStringsW(input.c_str(), NULL, 0);
    if (size == 0) {
        return L"";
    }

    std::wstring output(size, L'\0');
    ExpandEnvironmentStringsW(input.c_str(), &output[0], size);
    output.resize(size - 1);
    return output;
}

std::wstring extractVersionFromPath(const std::wstring& path) {
    size_t startPos = path.find(L"version-");
    if (startPos == std::wstring::npos) {
        return L"";
    }

    startPos += 8;
    size_t endPos = path.find(L'\\', startPos);
    if (endPos == std::wstring::npos) {
        return L"";
    }

    return path.substr(startPos, endPos - startPos);
}

void launchRobloxInstance(const std::wstring& name) {
    std::wstring shortcutPath = expandEnvironmentVariables(L"%appdata%\\Microsoft\\Windows\\Start Menu\\Programs\\Roblox\\Roblox Player.lnk");
    std::wstring targetPath = getShortcutTarget(shortcutPath);

    if (!targetPath.empty()) {
        std::wstring version = extractVersionFromPath(targetPath);
        if (!version.empty()) {
            std::wstring robloxPath = targetPath.substr(0, targetPath.find(L"version-") + 8 + version.length()) + L"\\RobloxPlayerBeta.exe";
            ShellExecute(NULL, L"open", robloxPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
            
            // Add the new instance to our list
            updateInstanceList();
        }
        else {
            error(L"launchRobloxInstance", L"Failed to extract version from target path");
        }
    }
    else {
        error(L"launchRobloxInstance", L"Failed to get target path from shortcut");
    }
}

void updateInstanceList() {
    instances.clear();

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        error(L"CreateToolhelp32Snapshot", L"Failed to create process snapshot");
    }

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe)) {
        do {
            if (wcscmp(pe.szExeFile, L"RobloxPlayerBeta.exe") == 0) {
                InstanceInfo info;
                info.processId = pe.th32ProcessID;
                info.name = L"Roblox " + std::to_wstring(instances.size() + 1);
                info.cpuUsage = 0.0;
                info.memoryUsage = 0;

                instances.push_back(info);

                // Create CPU counter for this process
                PDH_HCOUNTER counter;
                std::wstring counterPath = L"\\Process(RobloxPlayerBeta)\\% Processor Time";
                PdhAddCounter(cpuQuery, counterPath.c_str(), 0, &counter);
                cpuCounters[info.processId] = counter;
            }
        } while (Process32Next(hSnapshot, &pe));
    }

    CloseHandle(hSnapshot);
}

void monitorResources() {
    while (true) {
        updateInstanceList();

        // Collect CPU usage data
        PdhCollectQueryData(cpuQuery);

        for (auto& instance : instances) {
            // Get CPU usage
            PDH_FMT_COUNTERVALUE counterValue;
            PdhGetFormattedCounterValue(cpuCounters[instance.processId], PDH_FMT_DOUBLE, NULL, &counterValue);
            instance.cpuUsage = counterValue.doubleValue;

            // Get memory usage
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, instance.processId);
            if (hProcess != NULL) {
                PROCESS_MEMORY_COUNTERS pmc;
                if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
                    instance.memoryUsage = pmc.WorkingSetSize;
                }
                CloseHandle(hProcess);
            }
        }

        Sleep(1000); // Update every second
    }
}

void displayInstanceInfo() {
    setConsoleColor(11); // Light Cyan
    std::wcout << L"┌───────────────────────────────────────────────────────────────┐" << std::endl;
    std::wcout << L"│                     Active Roblox Instances                   │" << std::endl;
    std::wcout << L"├───────────┬────────────┬──────────────┬──────────────────────┤" << std::endl;
    std::wcout << L"│ Instance  │ Process ID │   CPU Usage  │     Memory Usage     │" << std::endl;
    std::wcout << L"├───────────┼────────────┼──────────────┼──────────────────────┤" << std::endl;

    for (const auto& instance : instances) {
        std::wstringstream ss;
        ss << L"│ " << std::setw(9) << std::left << instance.name
           << L"│ " << std::setw(10) << std::right << instance.processId
           << L"│ " << std::setw(11) << std::fixed << std::setprecision(2) << instance.cpuUsage << L"%"
           << L"│ " << std::setw(17) << std::fixed << std::setprecision(2) << (instance.memoryUsage / 1048576.0) << L" MB    │";
        
        std::wcout << ss.str() << std::endl;
    }

    std::wcout << L"└───────────┴────────────┴──────────────┴──────────────────────┘" << std::endl;
    setConsoleColor(15); // White
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_USER + 1:
            if (lParam == WM_LBUTTONUP || lParam == WM_RBUTTONUP) {
                POINT pt;
                GetCursorPos(&pt);
                HMENU hMenu = CreatePopupMenu();
                AppendMenu(hMenu, MF_STRING, 1, L"Show Console");
                AppendMenu(hMenu, MF_STRING, 2, L"Exit");
                SetForegroundWindow(hwnd);
                WORD cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, NULL);
                DestroyMenu(hMenu);
                if (cmd == 1) {
                    ShowWindow(consoleWindow, SW_SHOW);
                } else if (cmd == 2) {
                    PostQuitMessage(0);
                }
            }
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void createSystemTrayIcon(HWND hwnd) {
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_USER + 1;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcscpy_s(nid.szTip, L"EchoMulti-Instance");
    Shell_NotifyIcon(NIM_ADD, &nid);
}

void removeSystemTrayIcon() {
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

void drawMenu() {
    setConsoleColor(10); // Green
    printBox(L"✧･ﾟ: *✧･ﾟ:* 　Echo Multi-Instance　 *:･ﾟ✧*:･ﾟ✧");
    setConsoleColor(15); // White
    std::wcout << std::endl;
    printCentered(L"╔═══════════════════ Main Menu ═══════════════════╗");
    printCentered(L"║                                                 ║");
    printCentered(L"║  [1] Launch New Roblox Instance                 ║");
    printCentered(L"║  [2] Refresh Instance List                      ║");
    printCentered(L"║  [3] Terminate Specific Instance                ║");
    printCentered(L"║  [4] Terminate All Instances                    ║");
    printCentered(L"║  [5] Minimize to System Tray                    ║");
    printCentered(L"║  [6] Exit                                       ║");
    printCentered(L"║                                                 ║");
    printCentered(L"╚═════════════════════════════════════════════════╝");
    std::wcout << std::endl;
}

void handleUserInput() {
    std::wcout << L"Enter your choice: ";
    int choice;
    std::wcin >> choice;

    switch (choice) {
        case 1:
            launchRobloxInstance(L"New Instance");
            break;
        case 2:
            updateInstanceList();
            break;
        case 3:
            {
                int index;
                std::wcout << L"Enter the index of the instance to terminate: ";
                std::wcin >> index;
                if (index > 0 && index <= instances.size()) {
                    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, instances[index-1].processId);
                    if (hProcess != NULL) {
                        TerminateProcess(hProcess, 0);
                        CloseHandle(hProcess);
                        updateInstanceList();
                    }
                } else {
                    std::wcout << L"Invalid index!" << std::endl;
                    Sleep(1000);
                }
            }
            break;
        case 4:
            killRobloxProcesses();
            updateInstanceList();
            break;
        case 5:
            ShowWindow(consoleWindow, SW_HIDE);
            break;
        case 6:
            removeSystemTrayIcon();
            exit(0);
        default:
            std::wcout << L"Invalid choice!" << std::endl;
            Sleep(1000);
    }
}
