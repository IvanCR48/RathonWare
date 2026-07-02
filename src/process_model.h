#pragma once

#include <QAbstractListModel>
#include <QVector>
#include <QTimer>
#include "process_manager.h"

class ProcessModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum ProcessRoles {
        PidRole = Qt::UserRole + 1,
        NameRole,
        CpuRole,
        RamRole,
        PrivateRole,
        PeakRole,
        ThreadsRole,
        UsernameRole,
        CmdLineRole,
        PriorityRole
    };

    explicit ProcessModel(QObject *parent = nullptr);
    ~ProcessModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE bool killProcess(int pid);
    Q_INVOKABLE bool setPriority(int pid, int priorityClassValue);
    Q_INVOKABLE bool suspendProcess(int pid);
    Q_INVOKABLE bool resumeProcess(int pid);

private:
    ProcessManager m_manager;
    QVector<ProcessInfo> m_processes;
    QTimer *m_timer;
};
