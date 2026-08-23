#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QVector>
#include <QTimer>
#include <windows.h>

struct CpuCoreEntry {
    int coreIndex;
    QString coreLabel;
    QString coreType;     // "P-Core", "E-Core", or "Core"
    bool isPCore;
    bool isECore;
    double load;          // 0.0 - 100.0%
    QString heatColor;    // Dynamic hex color code
};

struct CoreTimeSample {
    LARGE_INTEGER idleTime;
    LARGE_INTEGER kernelTime;
    LARGE_INTEGER userTime;
};

class CpuCoreModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum CpuCoreRoles {
        CoreIndexRole = Qt::UserRole + 1,
        CoreLabelRole,
        CoreTypeRole,
        IsPCoreRole,
        IsECoreRole,
        LoadRole,
        HeatColorRole
    };

    explicit CpuCoreModel(QObject *parent = nullptr);
    ~CpuCoreModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void updateCores(const QVector<CpuCoreEntry>& cores);

private:
    QVector<CpuCoreEntry> m_cores;
};

class CpuTopology : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int totalCores READ totalCores NOTIFY statsChanged)
    Q_PROPERTY(int pCoreCount READ pCoreCount NOTIFY statsChanged)
    Q_PROPERTY(int eCoreCount READ eCoreCount NOTIFY statsChanged)
    Q_PROPERTY(bool hasHybridArchitecture READ hasHybridArchitecture NOTIFY statsChanged)
    Q_PROPERTY(double pCoreAvgUsage READ pCoreAvgUsage NOTIFY statsChanged)
    Q_PROPERTY(double eCoreAvgUsage READ eCoreAvgUsage NOTIFY statsChanged)

public:
    explicit CpuTopology(CpuCoreModel *coreModel, QObject *parent = nullptr);
    ~CpuTopology();

    int totalCores() const { return m_totalCores; }
    int pCoreCount() const { return m_pCoreCount; }
    int eCoreCount() const { return m_eCoreCount; }
    bool hasHybridArchitecture() const { return m_hasHybridArchitecture; }
    double pCoreAvgUsage() const { return m_pCoreAvgUsage; }
    double eCoreAvgUsage() const { return m_eCoreAvgUsage; }

    Q_INVOKABLE bool pinProcessToECores(int pid);
    Q_INVOKABLE bool pinProcessToPCores(int pid);
    Q_INVOKABLE bool resetProcessAffinity(int pid);
    Q_INVOKABLE QVariantList getBackgroundApps();

signals:
    void statsChanged();

private slots:
    void updateCoreStats();

private:
    void initTopology();
    QString computeHeatColor(double load);

    CpuCoreModel *m_coreModel = nullptr;
    QTimer *m_timer = nullptr;

    int m_totalCores = 0;
    int m_pCoreCount = 0;
    int m_eCoreCount = 0;
    bool m_hasHybridArchitecture = false;
    double m_pCoreAvgUsage = 0.0;
    double m_eCoreAvgUsage = 0.0;

    DWORD_PTR m_pCoreMask = 0;
    DWORD_PTR m_eCoreMask = 0;
    DWORD_PTR m_allCoresMask = 0;

    QVector<bool> m_isPCoreList;
    QVector<CoreTimeSample> m_prevSamples;
};
