#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <comdef.h>
#include <string>

#pragma comment(lib, "comsuppw.lib")
#pragma comment(lib, "Ole32.lib")

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
    output.resize(size - 1); // Remove the extra null character added by ExpandEnvironmentStringsW
    return output;
}

std::wstring extractVersionFromPath(const std::wstring& path) {
    size_t startPos = path.find(L"version-");
    if (startPos == std::wstring::npos) {
        return L"";
    }

    startPos += 8; // Move past "version-"
    size_t endPos = path.find(L'\\', startPos);
    if (endPos == std::wstring::npos) {
        return L"";
    }

    return path.substr(startPos, endPos - startPos);
}

void restartRobloxProcess() {
    std::wstring shortcutPath = expandEnvironmentVariables(L"%appdata%\\Microsoft\\Windows\\Start Menu\\Programs\\Roblox\\Roblox Player.lnk");
    std::wstring targetPath = getShortcutTarget(shortcutPath);

    if (!targetPath.empty()) {
        std::wstring version = extractVersionFromPath(targetPath);
        if (!version.empty()) {
            std::wstring robloxPath = targetPath.substr(0, targetPath.find(L"version-") + 8 + version.length()) + L"\\RobloxPlayerBeta.exe";
            ShellExecute(NULL, L"open", robloxPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
        }
        else {
            error(L"restartRobloxProcess", L"Failed to extract version from target path");
        }
    }
    else {
        error(L"restartRobloxProcess", L"Failed to get target path from shortcut");
    }
}

int main() {
    SetConsoleTitle(L"Echo multi instance [Loading] | made by blitzedzz");
    system("cmd.exe /c mode con: cols=173 lines=9001");
    system("curl https://raw.githubusercontent.com/Blitzedzz-2/EchoMultiInstance/main/storage/ascii");
    std::wcout << L"\nLoading\n";
    killRobloxProcesses();

    Sleep(3000);
    HANDLE roblox_mutex = CreateMutexW(0, TRUE, L"ROBLOX_singletonMutex");

    if (roblox_mutex == 0) {
        error(L"roblox_mutex", L"Failed to start ROBLOX_singletonMutex");
    }

    if ((DWORD)GetLastError() == ERROR_ALREADY_EXISTS) {
        error(L"GetLastError", L"Already exists");
    }
    else {
        system("cls");
        system("curl https://raw.githubusercontent.com/Blitzedzz-2/EchoMultiInstance/main/storage/ascii");

        std::wcout << L"\nStarted!\nMake sure to keep this open!\n";

        SetConsoleTitle(L"Echo multi instance [Started] | made by blitzedzz");
        std::wstring shortcutPath = expandEnvironmentVariables(L"%appdata%\\Microsoft\\Windows\\Start Menu\\Programs\\Roblox\\Roblox Player.lnk");
        std::wstring targetPath = getShortcutTarget(shortcutPath);
        std::wstring version = extractVersionFromPath(targetPath);

        if (!version.empty()) {
            std::wcout << L"Version: " << version << L"\n";
        }
        else {
            std::wcout << L"Version: Not found (May have bugs)\n";
        }

        restartRobloxProcess();

        while (true) {
            Sleep(1000);
        }
    }
}
