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

QVariantList FileUnlocker::findLockingProcesses(const QString& filePath)
{
    QVariantList results;
    QString nativePath = normalizePath(filePath);
    if (nativePath.trimmed().isEmpty()) return results;

    DWORD dwSession = 0;
    WCHAR szSessionKey[CCH_RM_SESSION_KEY + 1] = {0};

    DWORD dwError = RmStartSession(&dwSession, 0, szSessionKey);
    if (dwError != ERROR_SUCCESS) {
        return results;
    }

    std::wstring wPath = nativePath.toStdWString();
    PCWSTR pszFile = wPath.c_str();

    dwError = RmRegisterResources(dwSession, 1, &pszFile, 0, NULL, 0, NULL);
    if (dwError == ERROR_SUCCESS) {
        UINT nProcInfoNeeded = 0;
        UINT nProcInfo = 0;
        DWORD dwRebootReasons = RmRebootReasonNone;

        dwError = RmGetList(dwSession, &nProcInfoNeeded, &nProcInfo, NULL, &dwRebootReasons);
        if (dwError == ERROR_MORE_DATA && nProcInfoNeeded > 0) {
            std::vector<RM_PROCESS_INFO> rgProcesses(nProcInfoNeeded);
            nProcInfo = nProcInfoNeeded;

            dwError = RmGetList(dwSession, &nProcInfoNeeded, &nProcInfo, rgProcesses.data(), &dwRebootReasons);
            if (dwError == ERROR_SUCCESS) {
                for (UINT i = 0; i < nProcInfo; i++) {
                    const RM_PROCESS_INFO& info = rgProcesses[i];
                    unsigned long pid = info.Process.dwProcessId;

                    QVariantMap map;
                    map["pid"] = static_cast<qlonglong>(pid);
                    map["name"] = getProcessName(pid);
                    map["appName"] = QString::fromWCharArray(info.strAppName);
                    map["isService"] = (info.ApplicationType == RmService);
                    map["ramMB"] = getProcessMemoryMB(pid);
                    map["cmdLine"] = getProcessCommandLine(pid);
                    results.append(map);
                }
            }
        }
    }

    RmEndSession(dwSession);
    return results;
}

bool FileUnlocker::unlockFile(const QString& filePath)
{
    QVariantList lockers = findLockingProcesses(filePath);
    if (lockers.isEmpty()) return true;

    bool allKilled = true;
    for (const QVariant& item : lockers) {
        QVariantMap map = item.toMap();
        int pid = map["pid"].toInt();
        if (pid > 4) {
            if (!killLockingProcess(pid)) {
                allKilled = false;
            }
        }
    }
    return allKilled;
}

bool FileUnlocker::killLockingProcess(int pid)
{
    if (pid <= 4) return false;
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, static_cast<DWORD>(pid));
    if (!hProcess) return false;
    bool success = TerminateProcess(hProcess, 1);
    CloseHandle(hProcess);
    return success;
}
