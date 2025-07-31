#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <shlwapi.h>  // PathCombine

#pragma comment(lib, "shlwapi.lib")

#include "resource.h"

// Load embedded EXE from resource
std::vector<BYTE> LoadEmbeddedExe(int resourceId) {
    HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(resourceId), RT_RCDATA);
    if (!hRes) return {};
    HGLOBAL hResData = LoadResource(NULL, hRes);
    if (!hResData) return {};
    DWORD sz = SizeofResource(NULL, hRes);
    BYTE* data = (BYTE*)LockResource(hResData);
    return std::vector<BYTE>(data, data + sz);
}

bool LaunchExeWithOutput(const std::wstring& exePath, const std::wstring& args, bool runAsAdmin,
    std::string& outStdout, DWORD& exitCode) {
    HANDLE hReadPipe = NULL, hWritePipe = NULL;
    SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) return false;

    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

    std::wstring commandLine = L"\"" + exePath + L"\" " + args;

    STARTUPINFO si = { sizeof(si) };
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    si.hStdInput = NULL;

    PROCESS_INFORMATION pi = {};

    BOOL success = FALSE;
    if (runAsAdmin) {
        // Running with admin privileges requires a temporary BAT or EXE launcher; output redirection is currently unsupported
        MessageBox(NULL, L"Cannot capture output when running as administrator! Please try without admin mode.", L"Limitation", MB_ICONWARNING);
        return false;
    }
    else {
        success = CreateProcessW(
            NULL, &commandLine[0], NULL, NULL, TRUE,
            CREATE_NO_WINDOW, NULL, NULL, &si, &pi
        );
    }

    CloseHandle(hWritePipe);  // Parent process no longer writes

    if (!success) {
        CloseHandle(hReadPipe);
        return false;
    }

    // Read output from child process
    char buffer[4096];
    DWORD bytesRead;
    std::ostringstream oss;

    while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = 0;
        oss << buffer;
    }

    outStdout = oss.str();

    CloseHandle(hReadPipe);
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return true;
}
bool LaunchExeFromMemory(const std::vector<BYTE>& exeBytes) {
    TCHAR exePath[MAX_PATH] = { 0 };
    GetModuleFileName(NULL, exePath, MAX_PATH);

    // Extract directory part
    PathRemoveFileSpec(exePath); // exePath becomes the directory

    // Generate temporary file name
    TCHAR tempFile[MAX_PATH] = { 0 };
    UINT uRet = GetTempFileName(exePath, TEXT("bind"), 0, tempFile);
    if (uRet == 0) return false;

    HANDLE h = CreateFile(tempFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return false;

    DWORD written = 0;
    BOOL bWrite = WriteFile(h, exeBytes.data(), (DWORD)exeBytes.size(), &written, NULL);
    CloseHandle(h);
    if (!bWrite || written != exeBytes.size()) return false;

    //SHELLEXECUTEINFO sei{ sizeof(sei) };
    //sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    //sei.lpVerb = TEXT("runas"); // Run with admin rights
    //sei.lpFile = tempFile;
    //sei.nShow = SW_SHOW;

    //BOOL bRet = ShellExecuteEx(&sei);
    //if (!bRet) return false;

    //if (sei.hProcess) CloseHandle(sei.hProcess);
    std::string output;
    DWORD retCode = 0;
    std::wstring subExePath = tempFile;
    std::wstring args = L"--help";
    if (LaunchExeWithOutput(subExePath, args, false, output, retCode)) {
        std::cout << "STDOUT:\n" << output << std::endl;
        std::cout << "Exit Code: " << retCode << std::endl;
    }
    else {
        std::cerr << "Failed to launch." << std::endl;
    }
    return true;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    auto exeA = LoadEmbeddedExe(IDR_MAIN_EXE);
    auto exeB = LoadEmbeddedExe(IDR_SECOND_EXE);
    if (exeA.empty() || exeB.empty()) {
        MessageBox(NULL, TEXT("Failed to load resources"), TEXT("Error"), MB_ICONERROR);
        return -1;
    }

    if (!LaunchExeFromMemory(exeA)) {
        MessageBox(NULL, TEXT("Failed to launch A.exe"), TEXT("Error"), MB_ICONERROR);
        return -2;
    }

    // Delay (modify here if needed)
    Sleep(2000);

    if (!LaunchExeFromMemory(exeB)) {
        MessageBox(NULL, TEXT("Failed to launch B.exe"), TEXT("Error"), MB_ICONERROR);
        return -3;
    }

    return 0;
}