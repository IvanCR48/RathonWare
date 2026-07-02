#pragma once

#include <QString>
#include <QVector>
#include <QHash>
#include <windows.h>

struct ProcessInfo {
    unsigned long pid;
    QString name;
    double cpuUsage;
    double ramUsage;      // Working Set (MB)
    double privateUsage;  // Private Memory (MB)
    double peakUsage;     // Peak Memory (MB)
    int threads;
    QString username;
    QString cmdLine;
    QString priority;
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
    bool setPriority(unsigned long pid, int priorityClassValue);
    bool suspendProcess(unsigned long pid);
    bool resumeProcess(unsigned long pid);

private:
    int m_numCores = 1;
    QHash<unsigned long, ProcessTimeRecord> m_history;
};
