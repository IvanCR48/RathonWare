#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QVector>
#include <QHash>
#include <QTimer>
#include <windows.h>

struct GpuProcessEntry {
    unsigned long pid;
    QString name;
    QString category;      // "CUDA Compute / AI", "DirectX Graphics", etc.
    double vramMB;
    double vramGB;
    double vramPercent;    // % of total VRAM
    bool isCompute;
    bool leakSuspected;
    double growthRateMB;
};

struct VramHistoryEntry {
    QVector<double> samplesMB;
    ULONGLONG firstSeenTime;
    ULONGLONG lastSeenTime;
};

class GpuProcessModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum GpuProcessRoles {
        PidRole = Qt::UserRole + 1,
        NameRole,
        CategoryRole,
        VramMBRole,
        VramGBRole,
        VramPercentRole,
        IsComputeRole,
        LeakSuspectedRole,
        GrowthRateMBRole
    };

    explicit GpuProcessModel(QObject *parent = nullptr);
    ~GpuProcessModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void updateProcesses(const QVector<GpuProcessEntry>& processes);
    Q_INVOKABLE bool killProcess(int pid);

private:
    QVector<GpuProcessEntry> m_processes;
};

class GpuMonitor : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool hasNvidiaGpu READ hasNvidiaGpu NOTIFY statsChanged)
    Q_PROPERTY(QString gpuName READ gpuName NOTIFY statsChanged)
    Q_PROPERTY(double gpuUsage READ gpuUsage NOTIFY statsChanged)
    Q_PROPERTY(double gpuTemp READ gpuTemp NOTIFY statsChanged)
    Q_PROPERTY(double vramTotalGB READ vramTotalGB NOTIFY statsChanged)
    Q_PROPERTY(double vramUsedGB READ vramUsedGB NOTIFY statsChanged)
    Q_PROPERTY(double vramFreeGB READ vramFreeGB NOTIFY statsChanged)
    Q_PROPERTY(double vramUsagePercent READ vramUsagePercent NOTIFY statsChanged)
    Q_PROPERTY(double powerUsageW READ powerUsageW NOTIFY statsChanged)
    Q_PROPERTY(double powerLimitW READ powerLimitW NOTIFY statsChanged)
    Q_PROPERTY(int graphicsClockMHz READ graphicsClockMHz NOTIFY statsChanged)
    Q_PROPERTY(int memoryClockMHz READ memoryClockMHz NOTIFY statsChanged)
    Q_PROPERTY(QString throttleReason READ throttleReason NOTIFY statsChanged)
    Q_PROPERTY(QString throttleStatusLevel READ throttleStatusLevel NOTIFY statsChanged)

public:
    explicit GpuMonitor(GpuProcessModel *processModel, QObject *parent = nullptr);
    ~GpuMonitor();

    bool hasNvidiaGpu() const { return m_hasNvidiaGpu; }
    QString gpuName() const { return m_gpuName; }
    double gpuUsage() const { return m_gpuUsage; }
    double gpuTemp() const { return m_gpuTemp; }
    double vramTotalGB() const { return m_vramTotalGB; }
    double vramUsedGB() const { return m_vramUsedGB; }
    double vramFreeGB() const { return m_vramFreeGB; }
    double vramUsagePercent() const { return m_vramUsagePercent; }
    double powerUsageW() const { return m_powerUsageW; }
    double powerLimitW() const { return m_powerLimitW; }
    int graphicsClockMHz() const { return m_graphicsClockMHz; }
    int memoryClockMHz() const { return m_memoryClockMHz; }
    QString throttleReason() const { return m_throttleReason; }
    QString throttleStatusLevel() const { return m_throttleStatusLevel; }

    Q_INVOKABLE void refresh();

signals:
    void statsChanged();

private slots:
    void updateTelemetry();

private:
    void initNvml();
    void queryGpuProcesses();
    QString decodeThrottleReasons(unsigned long long reasonsMask);
    QString getProcessName(unsigned long pid);
    QString detectAiWorkload(const QString& name, unsigned long pid);

    GpuProcessModel *m_processModel = nullptr;
    QTimer *m_timer = nullptr;

    bool m_hasNvidiaGpu = false;
    QString m_gpuName = "Standard GPU";
    double m_gpuUsage = 0.0;
    double m_gpuTemp = 0.0;
    double m_vramTotalGB = 0.0;
    double m_vramUsedGB = 0.0;
    double m_vramFreeGB = 0.0;
    double m_vramUsagePercent = 0.0;
    double m_powerUsageW = 0.0;
    double m_powerLimitW = 0.0;
    int m_graphicsClockMHz = 0;
    int m_memoryClockMHz = 0;
    QString m_throttleReason = "None / Full Boost";
    QString m_throttleStatusLevel = "normal";

    HMODULE m_nvmlLib = nullptr;
    void *m_nvmlDevice = nullptr;

    // VRAM Leak Tracking history (PID -> Samples)
    QHash<unsigned long, VramHistoryEntry> m_vramHistory;
};
