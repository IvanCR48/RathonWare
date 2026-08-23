#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

struct LockingProcessEntry {
    unsigned long pid;
    QString processName;
    QString appName;
    QString cmdLine;
    double ramMB;
    bool isService;
};

class FileUnlocker : public QObject
{
    Q_OBJECT

public:
    explicit FileUnlocker(QObject *parent = nullptr);
    ~FileUnlocker();

    Q_INVOKABLE QVariantList findLockingProcesses(const QString& filePath);
    Q_INVOKABLE bool unlockFile(const QString& filePath);
    Q_INVOKABLE bool killLockingProcess(int pid);

private:
    QString normalizePath(const QString& path);
    QString getProcessName(unsigned long pid);
    double getProcessMemoryMB(unsigned long pid);
    QString getProcessCommandLine(unsigned long pid);
};
