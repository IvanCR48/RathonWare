#include "process_manager.h"
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <processthreadsapi.h>
#include <QDebug>
#include <QQueue>
#include <vector>

// Internal definitions for process command line reading (PEB access)
typedef NTSTATUS(NTAPI* pfnNtQueryInformationProcess)(
    HANDLE ProcessHandle,
    DWORD ProcessInformationClass,
    PVOID ProcessInformation,
    ULONG ProcessInformationLength,
    PULONG ReturnLength
);

typedef NTSTATUS(NTAPI* pfnNtSuspendProcess)(HANDLE ProcessHandle);
typedef NTSTATUS(NTAPI* pfnNtResumeProcess)(HANDLE ProcessHandle);

static pfnNtSuspendProcess NtSuspendProcess = nullptr;
static pfnNtResumeProcess NtResumeProcess = nullptr;

struct PROCESS_BASIC_INFORMATION {
    NTSTATUS ExitStatus;
    PVOID PebBaseAddress;
    ULONG_PTR AffinityMask;
    LONG BasePriority;
    ULONG_PTR UniqueProcessId;
    ULONG_PTR InheritedFromUniqueProcessId;
};

struct UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
};

struct RTL_USER_PROCESS_PARAMETERS {
    BYTE Reserved1[16];
    PVOID Reserved2[10];
    UNICODE_STRING ImagePathName;
    UNICODE_STRING CommandLine;
};

struct PEB {
    BYTE Reserved1[2];
    BYTE BeingDebugged;
    BYTE Reserved2[1];
    PVOID Reserved3[2];
    PVOID Ldr;
    RTL_USER_PROCESS_PARAMETERS* ProcessParameters;
};

static ULONGLONG SubtractFileTime(const FILETIME& ftA, const FILETIME& ftB) {
    ULARGE_INTEGER a, b;
    a.LowPart = ftA.dwLowDateTime;
    a.HighPart = ftA.dwHighDateTime;
    b.LowPart = ftB.dwLowDateTime;
    b.HighPart = ftB.dwHighDateTime;
    return a.QuadPart - b.QuadPart;
}

static QString QueryProcessUsername(HANDLE hProcess) {
    HANDLE hToken = NULL;
    if (OpenProcessToken(hProcess, TOKEN_QUERY, &hToken)) {
        DWORD size = 0;
        GetTokenInformation(hToken, TokenUser, NULL, 0, &size);
        if (size > 0) {
            std::vector<BYTE> buffer(size);
            if (GetTokenInformation(hToken, TokenUser, buffer.data(), size, &size)) {
                TOKEN_USER* pTokenUser = reinterpret_cast<TOKEN_USER*>(buffer.data());
                SID_NAME_USE sidType;
                wchar_t name[256];
                wchar_t domain[256];
                DWORD nameSize = 256;
                DWORD domainSize = 256;
                if (LookupAccountSidW(NULL, pTokenUser->User.Sid, name, &nameSize, domain, &domainSize, &sidType)) {
                    CloseHandle(hToken);
                    return QString::fromWCharArray(name);
                }
            }
        }
        CloseHandle(hToken);
    }
    return "SYSTEM";
}

static QString QueryCommandLine(HANDLE hProcess) {
    static pfnNtQueryInformationProcess NtQueryInformationProcess = nullptr;
    if (!NtQueryInformationProcess) {
        HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");
        if (hNtDll) {
            NtQueryInformationProcess = (pfnNtQueryInformationProcess)GetProcAddress(hNtDll, "NtQueryInformationProcess");
        }
    }

    if (!NtQueryInformationProcess) return "N/A";

    PROCESS_BASIC_INFORMATION pbi;
    ULONG returnLength = 0;
    NTSTATUS status = NtQueryInformationProcess(hProcess, 0, &pbi, sizeof(pbi), &returnLength);
    if (status == 0 && pbi.PebBaseAddress != nullptr) {
        PEB peb;
        SIZE_T read = 0;
        if (ReadProcessMemory(hProcess, pbi.PebBaseAddress, &peb, sizeof(peb), &read)) {
            RTL_USER_PROCESS_PARAMETERS params;
            if (ReadProcessMemory(hProcess, peb.ProcessParameters, &params, sizeof(params), &read)) {
                std::vector<wchar_t> cmdBuffer(params.CommandLine.Length / sizeof(wchar_t) + 1, 0);
                if (ReadProcessMemory(hProcess, params.CommandLine.Buffer, cmdBuffer.data(), params.CommandLine.Length, &read)) {
                    return QString::fromWCharArray(cmdBuffer.data());
                }
            }
        }
    }
    
    // Fallback to Executable Image Path
    wchar_t path[MAX_PATH] = {0};
    DWORD size = MAX_PATH;
    if (QueryFullProcessImageNameW(hProcess, 0, path, &size)) {
        return QString::fromWCharArray(path);
    }
    
    return "N/A";
}

static QString GetPriorityString(HANDLE hProcess) {
    DWORD pc = GetPriorityClass(hProcess);
    switch (pc) {
    case IDLE_PRIORITY_CLASS: return "Idle";
    case BELOW_NORMAL_PRIORITY_CLASS: return "Below Normal";
    case NORMAL_PRIORITY_CLASS: return "Normal";
    case ABOVE_NORMAL_PRIORITY_CLASS: return "Above Normal";
    case HIGH_PRIORITY_CLASS: return "High";
    case REALTIME_PRIORITY_CLASS: return "Realtime";
    default: return "Normal";
    }
}

ProcessManager::ProcessManager()
{
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    m_numCores = sysInfo.dwNumberOfProcessors;
    if (m_numCores < 1) m_numCores = 1;

    // Load ntdll exports for Process Suspend & Resume
    HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");
    if (hNtDll) {
        NtSuspendProcess = (pfnNtSuspendProcess)GetProcAddress(hNtDll, "NtSuspendProcess");
        NtResumeProcess = (pfnNtResumeProcess)GetProcAddress(hNtDll, "NtResumeProcess");
    }

    initCpuTopology();
}

ProcessManager::~ProcessManager()
{
}

void ProcessManager::initCpuTopology()
{
    DWORD length = 0;
    GetLogicalProcessorInformationEx(RelationProcessorCore, NULL, &length);
    if (length > 0) {
        std::vector<BYTE> buffer(length);
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX info = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data());
        if (GetLogicalProcessorInformationEx(RelationProcessorCore, info, &length)) {
            BYTE* ptr = buffer.data();
            while (ptr < buffer.data() + length) {
                PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX item = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(ptr);
                if (item->Relationship == RelationProcessorCore) {
                    KAFFINITY mask = item->Processor.GroupMask[0].Mask;
                    // In Windows 10/11, EfficiencyClass is byte at offset 1 in PROCESSOR_RELATIONSHIP
                    BYTE efficiencyClass = reinterpret_cast<const BYTE*>(&item->Processor)[1];
                    if (efficiencyClass > 0) {
                        m_pCoreMask |= mask;
                    } else {
                        m_eCoreMask |= mask;
                    }
                    m_allCoresMask |= mask;
                }
                ptr += item->Size;
            }
        }
    }

    if (m_pCoreMask != 0 && m_eCoreMask != 0) {
        m_hasHybridCores = true;
    } else {
        // Fallback for uniform multi-core systems: lower half = P-Cores, upper half = E-Cores
        if (m_allCoresMask == 0) {
            m_allCoresMask = (m_numCores >= 64) ? ~0ULL : ((1ULL << m_numCores) - 1);
        }
        int half = m_numCores / 2;
        if (half >= 1) {
            m_pCoreMask = (1ULL << half) - 1;
            m_eCoreMask = m_allCoresMask & ~m_pCoreMask;
        } else {
            m_pCoreMask = m_allCoresMask;
            m_eCoreMask = m_allCoresMask;
        }
    }
}

QVector<ProcessInfo> ProcessManager::updateProcessList()
{
    QVector<ProcessInfo> processes;
    QHash<unsigned long, ProcessTimeRecord> newHistory;

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return processes;
    }

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            unsigned long pid = pe32.th32ProcessID;
            if (pid == 0) continue; // Skip idle

            QString name = QString::fromWCharArray(pe32.szExeFile);
            double ramUsage = 0.0;
            double privateUsage = 0.0;
            double peakUsage = 0.0;
            double cpuUsage = 0.0;
            int threadsCount = pe32.cntThreads;
            QString username = "SYSTEM";
            QString cmdLine = "N/A";
            QString priority = "Normal";
            bool isEcoQos = false;
            DWORD_PTR affinityMask = 0;

            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
            if (!hProcess) {
                hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
            }

            if (hProcess) {
                // Get memory metrics
                PROCESS_MEMORY_COUNTERS_EX pmc;
                if (GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
                    ramUsage = pmc.WorkingSetSize / (1024.0 * 1024.0);
                    privateUsage = pmc.PrivateUsage / (1024.0 * 1024.0);
                    peakUsage = pmc.PeakWorkingSetSize / (1024.0 * 1024.0);
                }

                // Get CPU load times
                FILETIME creationTime, exitTime, kernelTime, userTime;
                FILETIME currentSystemTime;
                GetSystemTimeAsFileTime(&currentSystemTime);

                if (GetProcessTimes(hProcess, &creationTime, &exitTime, &kernelTime, &userTime)) {
                    if (m_history.contains(pid)) {
                        const auto& record = m_history[pid];
                        ULONGLONG kernelDiff = SubtractFileTime(kernelTime, record.kernelTime);
                        ULONGLONG userDiff = SubtractFileTime(userTime, record.userTime);
                        ULONGLONG timeDiff = SubtractFileTime(currentSystemTime, record.lastQueryTime);

                        if (timeDiff > 0) {
                            double usage = (100.0 * (kernelDiff + userDiff)) / (timeDiff * m_numCores);
                            cpuUsage = (usage < 0.0) ? 0.0 : (usage > 100.0) ? 100.0 : usage;
                        }
                    }

                    ProcessTimeRecord newRecord;
                    newRecord.kernelTime = kernelTime;
                    newRecord.userTime = userTime;
                    newRecord.lastQueryTime = currentSystemTime;
                    newHistory.insert(pid, newRecord);
                }

                // Query username
                username = QueryProcessUsername(hProcess);

                // Query command line
                cmdLine = QueryCommandLine(hProcess);

                // Query priority
                priority = GetPriorityString(hProcess);

                // Query affinity
                DWORD_PTR processAffinity = 0, systemAffinity = 0;
                if (GetProcessAffinityMask(hProcess, &processAffinity, &systemAffinity)) {
                    affinityMask = processAffinity;
                    if (m_eCoreMask != 0 && (affinityMask & ~m_eCoreMask) == 0) {
                        isEcoQos = true;
                    }
                }

                CloseHandle(hProcess);
            }

            ProcessInfo info;
            info.pid = pid;
            info.parentPid = pe32.th32ParentProcessID;
            info.name = name;
            info.cpuUsage = cpuUsage;
            info.ramUsage = ramUsage;
            info.privateUsage = privateUsage;
            info.peakUsage = peakUsage;
            info.threads = threadsCount;
            info.username = username;
            info.cmdLine = cmdLine;
            info.priority = priority;
            info.isSuspended = m_suspendedPids.contains(pid);
            info.isEcoQos = isEcoQos;
            info.affinityMask = affinityMask;
            processes.append(info);

        } while (Process32NextW(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    m_history = newHistory;
    return processes;
}

bool ProcessManager::killProcess(unsigned long pid)
{
    if (pid <= 4) return false;
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!hProcess) return false;
    bool success = TerminateProcess(hProcess, 1);
    CloseHandle(hProcess);
    m_suspendedPids.remove(pid);
    return success;
}

bool ProcessManager::killProcessTree(unsigned long pid)
{
    if (pid <= 4) return false;

    // 1. Take snapshot of all active processes to build parent->children tree
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return killProcess(pid);
    }

    QHash<unsigned long, QVector<unsigned long>> childrenMap;
    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            childrenMap[pe.th32ParentProcessID].append(pe.th32ProcessID);
        } while (Process32NextW(hSnapshot, &pe));
    }
    CloseHandle(hSnapshot);

    // 2. BFS collect all descendant PIDs
    QVector<unsigned long> toKill;
    QQueue<unsigned long> queue;
    queue.enqueue(pid);

    while (!queue.isEmpty()) {
        unsigned long current = queue.dequeue();
        toKill.append(current);

        if (childrenMap.contains(current)) {
            for (unsigned long childPid : childrenMap[current]) {
                if (childPid > 4 && !toKill.contains(childPid)) {
                    queue.enqueue(childPid);
                }
            }
        }
    }

    // 3. Kill in reverse order (bottom-up: children first, parent last)
    bool allSuccess = true;
    for (int i = toKill.size() - 1; i >= 0; --i) {
        if (!killProcess(toKill[i])) {
            allSuccess = false;
        }
    }

    return allSuccess;
}

bool ProcessManager::setPriority(unsigned long pid, int priorityClassValue)
{
    if (pid <= 4) return false;
    HANDLE hProcess = OpenProcess(PROCESS_SET_INFORMATION, FALSE, pid);
    if (!hProcess) return false;

    DWORD winPriorityClass = NORMAL_PRIORITY_CLASS;
    switch (priorityClassValue) {
    case 0: winPriorityClass = IDLE_PRIORITY_CLASS; break;
    case 1: winPriorityClass = BELOW_NORMAL_PRIORITY_CLASS; break;
    case 2: winPriorityClass = NORMAL_PRIORITY_CLASS; break;
    case 3: winPriorityClass = ABOVE_NORMAL_PRIORITY_CLASS; break;
    case 4: winPriorityClass = HIGH_PRIORITY_CLASS; break;
    case 5: winPriorityClass = REALTIME_PRIORITY_CLASS; break;
    }

    bool success = SetPriorityClass(hProcess, winPriorityClass);
    CloseHandle(hProcess);
    return success;
}

bool ProcessManager::suspendProcess(unsigned long pid)
{
    if (pid <= 4) return false;

    // Use native NtSuspendProcess if available
    if (NtSuspendProcess) {
        HANDLE hProcess = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, pid);
        if (hProcess) {
            NTSTATUS status = NtSuspendProcess(hProcess);
            CloseHandle(hProcess);
            if (status == 0) {
                m_suspendedPids.insert(pid);
                return true;
            }
        }
    }

    // Fallback to thread enumeration snapshot
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return false;
    
    THREADENTRY32 te;
    te.dwSize = sizeof(THREADENTRY32);
    if (Thread32First(hSnapshot, &te)) {
        do {
            if (te.th32OwnerProcessID == pid) {
                HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID);
                if (hThread) {
                    SuspendThread(hThread);
                    CloseHandle(hThread);
                }
            }
        } while (Thread32Next(hSnapshot, &te));
    }
    CloseHandle(hSnapshot);
    m_suspendedPids.insert(pid);
    return true;
}

bool ProcessManager::resumeProcess(unsigned long pid)
{
    if (pid <= 4) return false;

    // Use native NtResumeProcess if available
    if (NtResumeProcess) {
        HANDLE hProcess = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, pid);
        if (hProcess) {
            NTSTATUS status = NtResumeProcess(hProcess);
            CloseHandle(hProcess);
            if (status == 0) {
                m_suspendedPids.remove(pid);
                return true;
            }
        }
    }

    // Fallback to thread enumeration snapshot
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return false;
    
    THREADENTRY32 te;
    te.dwSize = sizeof(THREADENTRY32);
    if (Thread32First(hSnapshot, &te)) {
        do {
            if (te.th32OwnerProcessID == pid) {
                HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID);
                if (hThread) {
                    ResumeThread(hThread);
                    CloseHandle(hThread);
                }
            }
        } while (Thread32Next(hSnapshot, &te));
    }
    CloseHandle(hSnapshot);
    m_suspendedPids.remove(pid);
    return true;
}

bool ProcessManager::pinToECores(unsigned long pid)
{
    if (pid <= 4 || m_eCoreMask == 0) return false;
    HANDLE hProcess = OpenProcess(PROCESS_SET_INFORMATION | PROCESS_SET_QUOTA, FALSE, pid);
    if (!hProcess) return false;

    bool affSuccess = SetProcessAffinityMask(hProcess, m_eCoreMask);
    setEcoQos(pid, true);
    SetPriorityClass(hProcess, BELOW_NORMAL_PRIORITY_CLASS);
    CloseHandle(hProcess);
    return affSuccess;
}

bool ProcessManager::pinToPCores(unsigned long pid)
{
    if (pid <= 4 || m_pCoreMask == 0) return false;
    HANDLE hProcess = OpenProcess(PROCESS_SET_INFORMATION | PROCESS_SET_QUOTA, FALSE, pid);
    if (!hProcess) return false;

    bool affSuccess = SetProcessAffinityMask(hProcess, m_pCoreMask);
    setEcoQos(pid, false);
    SetPriorityClass(hProcess, NORMAL_PRIORITY_CLASS);
    CloseHandle(hProcess);
    return affSuccess;
}

bool ProcessManager::resetAffinity(unsigned long pid)
{
    if (pid <= 4 || m_allCoresMask == 0) return false;
    HANDLE hProcess = OpenProcess(PROCESS_SET_INFORMATION | PROCESS_SET_QUOTA, FALSE, pid);
    if (!hProcess) return false;

    bool affSuccess = SetProcessAffinityMask(hProcess, m_allCoresMask);
    setEcoQos(pid, false);
    CloseHandle(hProcess);
    return affSuccess;
}

bool ProcessManager::setEcoQos(unsigned long pid, bool enable)
{
    if (pid <= 4) return false;
    HANDLE hProcess = OpenProcess(PROCESS_SET_INFORMATION, FALSE, pid);
    if (!hProcess) return false;

    PROCESS_POWER_THROTTLING_STATE powerThrottling;
    memset(&powerThrottling, 0, sizeof(powerThrottling));
    powerThrottling.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
    powerThrottling.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;
    powerThrottling.StateMask = enable ? PROCESS_POWER_THROTTLING_EXECUTION_SPEED : 0;

    bool success = SetProcessInformation(hProcess, ProcessPowerThrottling, &powerThrottling, sizeof(powerThrottling));
    CloseHandle(hProcess);
    return success;
}
