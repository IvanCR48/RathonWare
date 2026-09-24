#include "file_unlocker.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <restartmanager.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <QFileInfo>
#include <QDir>
#include <vector>

FileUnlocker::FileUnlocker(QObject *parent)
    : QObject(parent)
{
}

FileUnlocker::~FileUnlocker()
{
}

// Strips QML "file:///" prefixes and normalizes slashes to native Windows separators ('\').
// When a user drags and drops a locked DLL or database from Windows Explorer into our QML DropArea,
// Qt passes a URL like "file:///C:/Users/name/repo/build/app.exe".
// The Win32 Restart Manager expects a raw win32 path (e.g., "C:\Users\name\repo\build\app.exe");
// passing a URI triggers ERROR_FILE_NOT_FOUND or ERROR_INVALID_NAME.
QString FileUnlocker::normalizePath(const QString& path)
{
    QString clean = path;
    if (clean.startsWith("file:///")) {
        clean = clean.mid(8);
    }
    // Convert forward slashes to Windows backslashes
    clean = QDir::toNativeSeparators(clean);
    return clean;
}

QString FileUnlocker::getProcessName(unsigned long pid)
{
    if (pid == 0) return "System Idle";
    if (pid == 4) return "System";

    QString name = "Unknown";
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);
        if (Process32FirstW(hSnapshot, &pe32)) {
            do {
                if (pe32.th32ProcessID == pid) {
                    name = QString::fromWCharArray(pe32.szExeFile);
                    break;
                }
            } while (Process32NextW(hSnapshot, &pe32));
        }
        CloseHandle(hSnapshot);
    }
    return name;
}

double FileUnlocker::getProcessMemoryMB(unsigned long pid)
{
    if (pid <= 4) return 0.0;
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!hProcess) {
        hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    }
    if (hProcess) {
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
            CloseHandle(hProcess);
            return pmc.WorkingSetSize / (1024.0 * 1024.0);
        }
        CloseHandle(hProcess);
    }
    return 0.0;
}

QString FileUnlocker::getProcessCommandLine(unsigned long pid)
{
    if (pid <= 4) return "";
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (hProcess) {
        wchar_t path[MAX_PATH] = {0};
        DWORD size = MAX_PATH;
        if (QueryFullProcessImageNameW(hProcess, 0, path, &size)) {
            CloseHandle(hProcess);
            return QString::fromWCharArray(path);
        }
        CloseHandle(hProcess);
    }
    return "";
}

// Interrogates the Windows Restart Manager API to identify all processes locking a file or directory.
// Why Restart Manager? Because the alternative (enumerating all system handles with NtQuerySystemInformation
// and calling DuplicateHandle + NtQueryObject) is notoriously hazardous: querying named pipes or hung device
// drivers hangs the calling thread forever. Restart Manager is fast, kernel-assisted, and safe.
QVariantList FileUnlocker::findLockingProcesses(const QString& filePath)
{
    QVariantList results;
    QString nativePath = normalizePath(filePath);
    if (nativePath.trimmed().isEmpty()) return results;

    DWORD dwSession = 0;
    WCHAR szSessionKey[CCH_RM_SESSION_KEY + 1] = {0};

    // Start a temporary Restart Manager session
    DWORD dwError = RmStartSession(&dwSession, 0, szSessionKey);
    if (dwError != ERROR_SUCCESS) {
        return results;
    }

    std::wstring wPath = nativePath.toStdWString();
    PCWSTR pszFile = wPath.c_str();

    // Register the target file as a monitored resource
    dwError = RmRegisterResources(dwSession, 1, &pszFile, 0, NULL, 0, NULL);
    if (dwError == ERROR_SUCCESS) {
        UINT nProcInfoNeeded = 0;
        UINT nProcInfo = 0;
        DWORD dwRebootReasons = RmRebootReasonNone;

        // First pass: determine how many processes are holding the file
        dwError = RmGetList(dwSession, &nProcInfoNeeded, &nProcInfo, NULL, &dwRebootReasons);
        if (dwError == ERROR_MORE_DATA && nProcInfoNeeded > 0) {
            // Allocate exact array for holding processes
            std::vector<RM_PROCESS_INFO> rgProcesses(nProcInfoNeeded);
            nProcInfo = nProcInfoNeeded;

            // Second pass: fill process details
            dwError = RmGetList(dwSession, &nProcInfoNeeded, &nProcInfo, rgProcesses.data(), &dwRebootReasons);
            if (dwError == ERROR_SUCCESS) {
                for (UINT i = 0; i < nProcInfo; i++) {
                    const RM_PROCESS_INFO& info = rgProcesses[i];
                    unsigned long pid = info.Process.dwProcessId;

                    QVariantMap map;
                    map["pid"] = static_cast<qlonglong>(pid);
                    map["name"] = getProcessName(pid);
                    map["appName"] = QString::fromWCharArray(info.strAppName);
                    // Flag whether this is a background Windows Service (RmService) vs user desktop app
                    map["isService"] = (info.ApplicationType == RmService);
                    map["ramMB"] = getProcessMemoryMB(pid);
                    map["cmdLine"] = getProcessCommandLine(pid);
                    results.append(map);
                }
            }
        }
    }

    // Always clean up session resources
    RmEndSession(dwSession);
    return results;
}

// 1-Click action: terminate all locking processes holding the target file.
// Used from the File Unlocker Dialog or Command Palette.
bool FileUnlocker::unlockFile(const QString& filePath)
{
    QVariantList lockers = findLockingProcesses(filePath);
    if (lockers.isEmpty()) return true;

    bool allKilled = true;
    for (const QVariant& item : lockers) {
        QVariantMap map = item.toMap();
        int pid = map["pid"].toInt();
        // Guard against touching PID <= 4
        if (pid > 4) {
            if (!killLockingProcess(pid)) {
                allKilled = false;
            }
        }
    }
    return allKilled;
}

// Forcibly kills the holding process by PID.
bool FileUnlocker::killLockingProcess(int pid)
{
    if (pid <= 4) return false;
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, static_cast<DWORD>(pid));
    if (!hProcess) return false;
    bool success = TerminateProcess(hProcess, 1);
    CloseHandle(hProcess);
    return success;
}
