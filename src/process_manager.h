#pragma once

#include <QString>
#include <QVector>
#include <QHash>
#include <QSet>
#include <windows.h>

struct ProcessInfo {
    unsigned long pid;
    unsigned long parentPid;
    QString name;
    double cpuUsage;
    double ramUsage;      // Working Set (MB)
    double privateUsage;  // Private Memory (MB)
    double peakUsage;     // Peak Memory (MB)
    int threads;
    QString username;
    QString cmdLine;
    QString priority;
    bool isSuspended;
    bool isEcoQos;
    DWORD_PTR affinityMask;
};

struct ProcessTimeRecord {
    FILETIME kernelTime;
    FILETIME userTime;
    FILETIME lastQueryTime;
};

class ProcessManager
{
public:
    ProcessManager();
    ~ProcessManager();

    QVector<ProcessInfo> updateProcessList();
    bool killProcess(unsigned long pid);
    bool killProcessTree(unsigned long pid);
    bool setPriority(unsigned long pid, int priorityClassValue);
    bool suspendProcess(unsigned long pid);
    bool resumeProcess(unsigned long pid);
    
    // Core Affinity & EcoQoS
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
