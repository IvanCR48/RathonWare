#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

// Information about a process holding an exclusive handle or memory-mapped file lock.
struct LockingProcessEntry {
    unsigned long pid;
    QString processName;
    QString appName;
    QString cmdLine;
    double ramMB;
    bool isService;         // True if the holding process is a background Windows Service (RmService)
};

// Windows Restart Manager (rstrtmgr.dll) wrapper.
// Solves the notorious "The action can't be completed because the folder or a file in it is open in another program".
// Instead of writing a kernel filter driver or scanning all handle tables in userland (which freezes the OS for 5 seconds),
// we leverage the native Windows Restart Manager API designed for Windows Update installers.
class FileUnlocker : public QObject
{
    Q_OBJECT

public:
    explicit FileUnlocker(QObject *parent = nullptr);
    ~FileUnlocker();

    // Queries which processes hold a lock on filePath
    Q_INVOKABLE QVariantList findLockingProcesses(const QString& filePath);

    // 1-Click unlocker: terminates holding processes so the user or compiler can delete/overwrite the file
    Q_INVOKABLE bool unlockFile(const QString& filePath);

    // Kills a specific holding process by PID
    Q_INVOKABLE bool killLockingProcess(int pid);

private:
    QString normalizePath(const QString& path);
    QString getProcessName(unsigned long pid);
    double getProcessMemoryMB(unsigned long pid);
    QString getProcessCommandLine(unsigned long pid);
};
