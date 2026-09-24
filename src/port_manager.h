#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

// Snapshot entry mapping a bound network socket to the offending OS process.
// Windows Task Manager still (in 2026) doesn't show listening ports per process.
// Whenever Vite, Next.js, or Docker dies weirdly and throws EADDRINUSE, this struct
// holds the exact PID, executable name, and memory footprint needed to blast it.
struct PortProcessEntry {
    int port;
    unsigned long pid;
    QString processName;
    QString protocol;       // "TCP", "TCP6", or "UDP"
    QString state;          // "LISTENING", "ESTABLISHED", "BOUND", etc.
    QString localAddress;
    double ramUsageMB;
    QString cmdLine;
};

// Low-level Win32 IP Helper bridge.
// Interrogates GetExtendedTcpTable / GetExtendedUdpTable with TCP_TABLE_OWNER_PID_ALL
// to reconstruct the socket-to-process correlation table without shelling out to netstat.
class PortManager : public QObject
{
    Q_OBJECT

public:
    explicit PortManager(QObject *parent = nullptr);
    ~PortManager();

    // Query active processes binding a specific port (e.g. 3000, 8080, 5432)
    Q_INVOKABLE QVariantList getProcessesByPort(int port);

    // Retrieve all active listening TCP/UDP endpoints for the command palette
    Q_INVOKABLE QVariantList getAllListeningPorts();

    // 1-click killer for developers stuck on EADDRINUSE
    Q_INVOKABLE bool killProcessOnPort(int port);
    Q_INVOKABLE bool killProcessByPid(int pid);

private:
    QVector<PortProcessEntry> queryAllPorts();
    QString getProcessName(unsigned long pid);
    double getProcessMemoryMB(unsigned long pid);
    QString getProcessCommandLine(unsigned long pid);
};
