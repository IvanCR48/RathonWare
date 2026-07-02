#pragma once

#ifndef WINVER
#define WINVER 0x0A00
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#ifndef NTDDI_VERSION
#define NTDDI_VERSION 0x0A000000
#endif

// Prevent windows.h from including legacy winsock.h (Winsock 1)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <netioapi.h>
#include <pdh.h>

#include <QObject>
#include <QTimer>
#include <QString>

class SystemMonitor : public QObject
{
    Q_OBJECT
    
    // Core monitors
    Q_PROPERTY(double cpuUsage READ cpuUsage NOTIFY cpuUsageChanged)
    Q_PROPERTY(double ramUsage READ ramUsage NOTIFY ramUsageChanged)
    Q_PROPERTY(double ramTotal READ ramTotal NOTIFY ramTotalChanged)
    Q_PROPERTY(double ramUsed READ ramUsed NOTIFY ramUsedChanged)
    Q_PROPERTY(double gpuUsage READ gpuUsage NOTIFY gpuUsageChanged)
    Q_PROPERTY(double gpuTemp READ gpuTemp NOTIFY gpuTempChanged)
    Q_PROPERTY(double gpuVramUsage READ gpuVramUsage NOTIFY gpuVramUsageChanged)
    Q_PROPERTY(double gpuVramTotal READ gpuVramTotal NOTIFY gpuVramTotalChanged)
    Q_PROPERTY(double gpuVramUsed READ gpuVramUsed NOTIFY gpuVramUsedChanged)
    
    // Extended Metrics
    Q_PROPERTY(QString uptime READ uptime NOTIFY uptimeChanged)
    Q_PROPERTY(double diskReadSpeed READ diskReadSpeed NOTIFY diskReadSpeedChanged)
    Q_PROPERTY(double diskWriteSpeed READ diskWriteSpeed NOTIFY diskWriteSpeedChanged)
    Q_PROPERTY(double diskUsage READ diskUsage NOTIFY diskUsageChanged)
    Q_PROPERTY(double netDownloadSpeed READ netDownloadSpeed NOTIFY netDownloadSpeedChanged)
    Q_PROPERTY(double netUploadSpeed READ netUploadSpeed NOTIFY netUploadSpeedChanged)
    
    // Hardware Details
    Q_PROPERTY(QString cpuModel READ cpuModel CONSTANT)
    Q_PROPERTY(QString gpuModel READ gpuModel CONSTANT)
    Q_PROPERTY(QString motherboardModel READ motherboardModel CONSTANT)
    Q_PROPERTY(QString biosVersion READ biosVersion CONSTANT)
    Q_PROPERTY(int ramSpeed READ ramSpeed CONSTANT)

public:
    explicit SystemMonitor(QObject *parent = nullptr);
    ~SystemMonitor();

    // Getters
    double cpuUsage() const { return m_cpuUsage; }
    double ramUsage() const { return m_ramUsage; }
    double ramTotal() const { return m_ramTotal; }
    double ramUsed() const { return m_ramUsed; }
    double gpuUsage() const { return m_gpuUsage; }
    double gpuTemp() const { return m_gpuTemp; }
    double gpuVramUsage() const { return m_gpuVramUsage; }
    double gpuVramTotal() const { return m_gpuVramTotal; }
    double gpuVramUsed() const { return m_gpuVramUsed; }
    
    QString uptime() const { return m_uptime; }
    double diskReadSpeed() const { return m_diskReadSpeed; }
    double diskWriteSpeed() const { return m_diskWriteSpeed; }
    double diskUsage() const { return m_diskUsage; }
    double netDownloadSpeed() const { return m_netDownloadSpeed; }
    double netUploadSpeed() const { return m_netUploadSpeed; }

    QString cpuModel() const { return m_cpuModel; }
    QString gpuModel() const { return m_gpuModel; }
    QString motherboardModel() const { return m_motherboardModel; }
    QString biosVersion() const { return m_biosVersion; }
    int ramSpeed() const { return m_ramSpeed; }

signals:
    void cpuUsageChanged();
    void ramUsageChanged();
    void ramTotalChanged();
    void ramUsedChanged();
    void gpuUsageChanged();
    void gpuTempChanged();
    void gpuVramUsageChanged();
    void gpuVramTotalChanged();
    void gpuVramUsedChanged();
    
    void uptimeChanged();
    void diskReadSpeedChanged();
    void diskWriteSpeedChanged();
    void diskUsageChanged();
    void netDownloadSpeedChanged();
    void netUploadSpeedChanged();

private slots:
    void updateStats();

private:
    void initCpuQuery();
    double calculateCpuUsage();
    
    void initGpuQuery();
    void updateGpuStats();

    void queryCpuModel();
    void queryGpuModel();
    void queryMotherboardAndBios();
    void queryRamSpeed();

    // Extended Sensor queries (PDH & IP Helper)
    void initPdhQueries();
    void queryDiskSpeeds();
    void queryDiskUsage();
    void queryNetworkSpeeds();
    void updateUptime();

    // Core values
    double m_cpuUsage = 0.0;
    double m_ramUsage = 0.0;
    double m_ramTotal = 0.0;
    double m_ramUsed = 0.0;
    double m_gpuUsage = 0.0;
    double m_gpuTemp = 0.0;
    double m_gpuVramUsage = 0.0;
    double m_gpuVramTotal = 0.0;
    double m_gpuVramUsed = 0.0;
    
    // Extended values
    QString m_uptime = "00:00:00";
    double m_diskReadSpeed = 0.0;
    double m_diskWriteSpeed = 0.0;
    double m_diskUsage = 0.0;
    double m_netDownloadSpeed = 0.0;
    double m_netUploadSpeed = 0.0;

    // Spec values
    QString m_cpuModel = "Unknown CPU";
    QString m_gpuModel = "Unknown GPU";
    QString m_motherboardModel = "Unknown Motherboard";
    QString m_biosVersion = "Unknown BIOS";
    int m_ramSpeed = 0;

    QTimer *m_timer;

    // CPU times variables
    FILETIME m_prevIdleTime;
    FILETIME m_prevKernelTime;
    FILETIME m_prevUserTime;

    // NVML Handles
    HMODULE m_nvmlLib = nullptr;
    void* m_nvmlDevice = nullptr;
    bool m_nvmlInitialized = false;

    // PDH Handles for Disk speeds
    PDH_HQUERY m_pdhQuery = nullptr;
    PDH_HCOUNTER m_counterDiskRead = nullptr;
    PDH_HCOUNTER m_counterDiskWrite = nullptr;
    bool m_pdhInitialized = false;

    // Network calculation variables (IP Helper)
    ULONGLONG m_prevNetInBytes = 0;
    ULONGLONG m_prevNetOutBytes = 0;
    ULONGLONG m_lastNetQueryTime = 0;
};
