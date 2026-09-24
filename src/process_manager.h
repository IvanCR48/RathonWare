#pragma once

#include <QString>
#include <QVector>
#include <QHash>
#include <QSet>
#include <windows.h>

// Full process telemetry snapshot consumed by the Qt Quick process table.
// Exposes memory metrics that Task Manager usually hides or conflates (Working Set vs Private Bytes),
// real command-line arguments fetched directly from the process PEB, and scheduling/affinity state.
struct ProcessInfo {
    unsigned long pid;
    unsigned long parentPid;
    QString name;
    double cpuUsage;
    double ramUsage;      // Working Set (physical memory currently resident in RAM) (MB)
    double privateUsage;  // Private commit charge (unshared memory that cannot be paged out to other apps) (MB)
    double peakUsage;     // Peak Working Set over process lifetime (MB)
    int threads;
    QString username;     // Domain\User or SYSTEM
    QString cmdLine;      // Full CLI invocation extracted from the remote Process Environment Block (PEB)
    QString priority;     // "Normal", "High", "Below Normal", "Idle", "Realtime"
    bool isSuspended;     // True if frozen via NtSuspendProcess
    bool isEcoQos;        // True if throttled via Windows 11 EcoQoS (ProcessPowerThrottling)
    DWORD_PTR affinityMask; // Bitmask of logical CPU cores this process is permitted to run on
};

// Internal delta tracker for per-process CPU percentage calculation.
// Windows doesn't give you instant CPU %; you have to sample (KernelTime + UserTime) / WallClockTime across ticks.
struct ProcessTimeRecord {
    FILETIME kernelTime;
    FILETIME userTime;
    FILETIME lastQueryTime;
};

// Heavyweight Win32 & NT kernel process management backend.
// Implements aggressive developer features:
// - Process tree killing (BFS bottom-up kill so child workers don't detach as orphans)
// - Process freezing (NtSuspendProcess from ntdll.dll to halt 100% CPU loops without losing debug state)
// - Intel P-Core / E-Core affinity pinning and EcoQoS throttling
class ProcessManager
{
public:
    ProcessManager();
    ~ProcessManager();

    // Re-enumerates all active processes, computes CPU delta percentages, and builds ProcessInfo table
    QVector<ProcessInfo> updateProcessList();

    // Direct termination of single PID
    bool killProcess(unsigned long pid);

    // Recursively discovers and terminates all child processes (bottom-up) before killing the root
    bool killProcessTree(unsigned long pid);

    // Dynamic priority adjustments (Idle, Below Normal, Normal, Above Normal, High, Realtime)
    bool setPriority(unsigned long pid, int priorityClassValue);

    // Halts all threads in a process atomically via ntdll!NtSuspendProcess
    bool suspendProcess(unsigned long pid);

    // Resumes a previously suspended process via ntdll!NtResumeProcess
    bool resumeProcess(unsigned long pid);
    
    // Core Affinity & Windows 11 EcoQoS (lock background hogs to efficiency cores)
    bool pinToECores(unsigned long pid);
    bool pinToPCores(unsigned long pid);
    bool resetAffinity(unsigned long pid);
    bool setEcoQos(unsigned long pid, bool enable);

    DWORD_PTR getECoreMask() const { return m_eCoreMask; }
    DWORD_PTR getPCoreMask() const { return m_pCoreMask; }
    bool hasHybridCores() const { return m_hasHybridCores; }

private:
    void initCpuTopology();

    int m_numCores = 1;
    DWORD_PTR m_eCoreMask = 0;
    DWORD_PTR m_pCoreMask = 0;
    DWORD_PTR m_allCoresMask = 0;
    bool m_hasHybridCores = false;

    QHash<unsigned long, ProcessTimeRecord> m_history;
    QSet<unsigned long> m_suspendedPids;
};
