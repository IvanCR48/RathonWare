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

// Win32 Header Hell Workaround:
// If windows.h is included before winsock2.h, it unconditionally includes legacy winsock.h (Winsock 1.1).
// When iphlpapi.h or ws2tcpip.h is later pulled in, the compiler generates 100+ duplicate symbol errors.
// WIN32_LEAN_AND_MEAN and explicit winsock2.h inclusion first is mandatory.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <netioapi.h>
#include <pdh.h>
#include <psapi.h>
#include <dxgi1_4.h>

#include <QObject>
#include <QTimer>
#include <QString>

// System-wide diagnostic monitor.
// Aggregates real-time NT kernel stats, PDH disk counters, network interface deltas,
// and physical memory commit charges into reactive Qt Q_PROPERTY bindings.
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
    Q_PROPERTY(double diskTotalCapacityGB READ diskTotalCapacityGB NOTIFY diskUsageChanged)
    Q_PROPERTY(double diskTotalFreeGB READ diskTotalFreeGB NOTIFY diskUsageChanged)
    Q_PROPERTY(QString diskDriveSummary READ diskDriveSummary NOTIFY diskUsageChanged)
    Q_PROPERTY(double netDownloadSpeed READ netDownloadSpeed NOTIFY netDownloadSpeedChanged)
    Q_PROPERTY(double netUploadSpeed READ netUploadSpeed NOTIFY netUploadSpeedChanged)
    
    // Kernel & Memory Commit Metrics
    Q_PROPERTY(double commitTotalGB READ commitTotalGB NOTIFY commitStatsChanged)
    Q_PROPERTY(double commitLimitGB READ commitLimitGB NOTIFY commitStatsChanged)
    Q_PROPERTY(double commitUsagePercent READ commitUsagePercent NOTIFY commitStatsChanged)
    Q_PROPERTY(double commitPeakGB READ commitPeakGB NOTIFY commitStatsChanged)
    Q_PROPERTY(double kernelPagedMB READ kernelPagedMB NOTIFY commitStatsChanged)
    Q_PROPERTY(double kernelNonpagedMB READ kernelNonpagedMB NOTIFY commitStatsChanged)
    Q_PROPERTY(int processCount READ processCount NOTIFY systemCountsChanged)
    Q_PROPERTY(int threadCount READ threadCount NOTIFY systemCountsChanged)
    Q_PROPERTY(int handleCount READ handleCount NOTIFY systemCountsChanged)

    // Hardware Details
    Q_PROPERTY(QString cpuModel READ cpuModel CONSTANT)
    Q_PROPERTY(int cpuBaseClockMHz READ cpuBaseClockMHz CONSTANT)
    Q_PROPERTY(QString gpuModel READ gpuModel CONSTANT)
    Q_PROPERTY(QString gpuBackend READ gpuBackend CONSTANT)
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
    double diskTotalCapacityGB() const { return m_diskTotalCapacityGB; }
    double diskTotalFreeGB() const { return m_diskTotalFreeGB; }
    QString diskDriveSummary() const { return m_diskDriveSummary; }
    double netDownloadSpeed() const { return m_netDownloadSpeed; }
    double netUploadSpeed() const { return m_netUploadSpeed; }

    double commitTotalGB() const { return m_commitTotalGB; }
    double commitLimitGB() const { return m_commitLimitGB; }
    double commitUsagePercent() const { return m_commitUsagePercent; }
    double commitPeakGB() const { return m_commitPeakGB; }
    double kernelPagedMB() const { return m_kernelPagedMB; }
    double kernelNonpagedMB() const { return m_kernelNonpagedMB; }
    int processCount() const { return m_processCount; }
    int threadCount() const { return m_threadCount; }
    int handleCount() const { return m_handleCount; }

    QString cpuModel() const { return m_cpuModel; }
    int cpuBaseClockMHz() const { return m_cpuBaseClockMHz; }
    QString gpuModel() const { return m_gpuModel; }
    QString gpuBackend() const { return m_gpuBackend; }
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

    void commitStatsChanged();
    void systemCountsChanged();

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

    // Extended Sensor queries (PDH & IP Helper & Win32)
    void initPdhQueries();
    void queryDiskSpeeds();
    void queryDiskUsage();
    void queryNetworkSpeeds();
    void queryPerformanceInfo();
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
    double m_diskTotalCapacityGB = 0.0;
    double m_diskTotalFreeGB = 0.0;
    QString m_diskDriveSummary = "";
    double m_netDownloadSpeed = 0.0;
    double m_netUploadSpeed = 0.0;

    // Kernel & Commit values
    double m_commitTotalGB = 0.0;
    double m_commitLimitGB = 0.0;
    double m_commitUsagePercent = 0.0;
    double m_commitPeakGB = 0.0;
    double m_kernelPagedMB = 0.0;
    double m_kernelNonpagedMB = 0.0;
    int m_processCount = 0;
    int m_threadCount = 0;
    int m_handleCount = 0;

    // Spec values
    QString m_cpuModel = "Unknown CPU";
    int m_cpuBaseClockMHz = 0;
    QString m_gpuModel = "Unknown GPU";
    QString m_gpuBackend = "Standard";
    QString m_motherboardModel = "Unknown Motherboard";
    QString m_biosVersion = "Unknown BIOS";
    int m_ramSpeed = 0;

    QTimer *m_timer = nullptr;

    // CPU times variables
    FILETIME m_prevIdleTime;
    FILETIME m_prevKernelTime;
    FILETIME m_prevUserTime;

    // NVML Handles
    HMODULE m_nvmlLib = nullptr;
    void* m_nvmlDevice = nullptr;
    bool m_nvmlInitialized = false;

    // DXGI Handles for fallback
    IDXGIFactory1* m_dxgiFactory = nullptr;
    IDXGIAdapter3* m_dxgiAdapter3 = nullptr;
    bool m_dxgiInitialized = false;

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
