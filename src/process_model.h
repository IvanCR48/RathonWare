#pragma once

#include <QAbstractListModel>
#include <QVector>
#include <QTimer>
#include "process_manager.h"

// Reactive QAbstractListModel bridging Win32 ProcessManager into Qt Quick / QML.
// Maps typed C++ fields directly to QML delegates via named roles to avoid JavaScript runtime reflection overhead.
// Windows process tables have high PID churn (compilers spawning hundreds of cl.exe/gcc processes in seconds);
// this model manages batch updates cleanly without blocking the QML rendering thread.
class ProcessModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool hasHybridCores READ hasHybridCores CONSTANT)

public:
    enum ProcessRoles {
        PidRole = Qt::UserRole + 1,
        ParentPidRole,
        NameRole,
        CpuRole,
        RamRole,
        PrivateRole,
        PeakRole,
        ThreadsRole,
        UsernameRole,
        CmdLineRole,
        PriorityRole,
        IsSuspendedRole,
        IsEcoQosRole
    };

    explicit ProcessModel(QObject *parent = nullptr);
    ~ProcessModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool hasHybridCores() const { return m_manager.hasHybridCores(); }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE bool killProcess(int pid);
    Q_INVOKABLE bool killProcessTree(int pid);
    Q_INVOKABLE bool setPriority(int pid, int priorityClassValue);
    Q_INVOKABLE bool suspendProcess(int pid);
    Q_INVOKABLE bool resumeProcess(int pid);
    Q_INVOKABLE bool pinToECores(int pid);
    Q_INVOKABLE bool pinToPCores(int pid);
    Q_INVOKABLE bool resetAffinity(int pid);

private:
    ProcessManager m_manager;
    QVector<ProcessInfo> m_processes;
    QTimer *m_timer;
};
