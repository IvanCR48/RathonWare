#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

struct PortProcessEntry {
    int port;
    unsigned long pid;
    QString processName;
    QString protocol;       // "TCP" or "UDP"
    QString state;          // "LISTENING", "ESTABLISHED", etc.
    QString localAddress;
    double ramUsageMB;
    QString cmdLine;
};

class PortManager : public QObject
{
    Q_OBJECT

public:
    explicit PortManager(QObject *parent = nullptr);
    ~PortManager();

    Q_INVOKABLE QVariantList getProcessesByPort(int port);
    Q_INVOKABLE QVariantList getAllListeningPorts();
    Q_INVOKABLE bool killProcessOnPort(int port);
    Q_INVOKABLE bool killProcessByPid(int pid);

private:
    QVector<PortProcessEntry> queryAllPorts();
    QString getProcessName(unsigned long pid);
    double getProcessMemoryMB(unsigned long pid);
    QString getProcessCommandLine(unsigned long pid);
};
