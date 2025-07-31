#include <windows.h>
#include <vector>
#include <shlwapi.h>  // PathCombine

#pragma comment(lib, "shlwapi.lib")

#include "resource.h"

// ����Դ����Ƕ��� EXE
std::vector<BYTE> LoadEmbeddedExe(int resourceId) {
    HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(resourceId), RT_RCDATA);
    if (!hRes) return {};
    HGLOBAL hResData = LoadResource(NULL, hRes);
    if (!hResData) return {};
    DWORD sz = SizeofResource(NULL, hRes);
    BYTE* data = (BYTE*)LockResource(hResData);
    return std::vector<BYTE>(data, data + sz);
}

//// ���ڴ���д����ʱ�ļ����Թ���ԱȨ������
//bool LaunchExeFromMemory(const std::vector<BYTE>& exeBytes) {
//    TCHAR tempPath[MAX_PATH];
//    GetTempPath(MAX_PATH, tempPath);
//    TCHAR tempFile[MAX_PATH];
//    GetTempFileName(tempPath, TEXT("bind"), 0, tempFile);
//
//    HANDLE h = CreateFile(tempFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
//        FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
//    if (h == INVALID_HANDLE_VALUE) return false;
//    DWORD written;
//    WriteFile(h, exeBytes.data(), (DWORD)exeBytes.size(), &written, NULL);
//    CloseHandle(h);
//
//    SHELLEXECUTEINFO sei{ sizeof(sei) };
//    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
//    sei.lpVerb = TEXT("runas");
//    sei.lpFile = tempFile;
//    sei.nShow = SW_SHOW;
//    if (!ShellExecuteEx(&sei)) return false;
//    if (sei.hProcess) CloseHandle(sei.hProcess);
//    return true;
//}

bool LaunchExeFromMemory(const std::vector<BYTE>& exeBytes) {
    TCHAR exePath[MAX_PATH] = { 0 };
    GetModuleFileName(NULL, exePath, MAX_PATH);

    // ȡĿ¼����
    PathRemoveFileSpec(exePath); // exePath ���Ŀ¼

    // ������ʱ�ļ���
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

    SHELLEXECUTEINFO sei{ sizeof(sei) };
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = TEXT("runas"); // ����ԱȨ��
    sei.lpFile = tempFile;
    sei.nShow = SW_SHOW;

    BOOL bRet = ShellExecuteEx(&sei);
    if (!bRet) return false;

    if (sei.hProcess) CloseHandle(sei.hProcess);

    return true;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    auto exeA = LoadEmbeddedExe(IDR_MAIN_EXE);
    auto exeB = LoadEmbeddedExe(IDR_SECOND_EXE);
    if (exeA.empty() || exeB.empty()) {
        MessageBox(NULL, TEXT("������Դʧ��"), TEXT("Error"), MB_ICONERROR);
        return -1;
    }

    if (!LaunchExeFromMemory(exeA)) {
        MessageBox(NULL, TEXT("���� A.exe ʧ��"), TEXT("Error"), MB_ICONERROR);
        return -2;
    }

    // �ӳ٣��������ÿ��������޸ģ�
    Sleep(2000);

    if (!LaunchExeFromMemory(exeB)) {
        MessageBox(NULL, TEXT("���� B.exe ʧ��"), TEXT("Error"), MB_ICONERROR);
        return -3;
    }

    return 0;
}
