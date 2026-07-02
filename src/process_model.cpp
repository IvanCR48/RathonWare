#include "process_model.h"
#include <algorithm>

ProcessModel::ProcessModel(QObject *parent)
    : QAbstractListModel(parent)
{
    refresh();

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &ProcessModel::refresh);
    m_timer->start(2000);
}

ProcessModel::~ProcessModel()
{
}

int ProcessModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_processes.count();
}

QVariant ProcessModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_processes.count()) {
        return QVariant();
    }

    const ProcessInfo &process = m_processes[index.row()];

    switch (role) {
    case PidRole:
        return static_cast<qlonglong>(process.pid);
    case NameRole:
        return process.name;
    case CpuRole:
        return process.cpuUsage;
    case RamRole:
        return process.ramUsage;
    case PrivateRole:
        return process.privateUsage;
    case PeakRole:
        return process.peakUsage;
    case ThreadsRole:
        return process.threads;
    case UsernameRole:
        return process.username;
    case CmdLineRole:
        return process.cmdLine;
    case PriorityRole:
        return process.priority;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> ProcessModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[PidRole] = "pid";
    roles[NameRole] = "name";
    roles[CpuRole] = "cpu";
    roles[RamRole] = "ram";
    roles[PrivateRole] = "privateUsage";
    roles[PeakRole] = "peakUsage";
    roles[ThreadsRole] = "threads";
    roles[UsernameRole] = "username";
    roles[CmdLineRole] = "cmdLine";
    roles[PriorityRole] = "priority";
    return roles;
}

void ProcessModel::refresh()
{
    beginResetModel();
    m_processes = m_manager.updateProcessList();
    
    // Sort processes by CPU usage descending by default
    std::sort(m_processes.begin(), m_processes.end(), [](const ProcessInfo& a, const ProcessInfo& b) {
        if (a.cpuUsage != b.cpuUsage) {
            return a.cpuUsage > b.cpuUsage;
        }
        return a.ramUsage > b.ramUsage;
    });

    endResetModel();
}

bool ProcessModel::killProcess(int pid)
{
    bool success = m_manager.killProcess(pid);
    if (success) {
        refresh();
    }
    return success;
}

bool ProcessModel::setPriority(int pid, int priorityClassValue)
{
    bool success = m_manager.setPriority(pid, priorityClassValue);
    if (success) {
        refresh();
    }
    return success;
}

bool ProcessModel::suspendProcess(int pid)
{
    bool success = m_manager.suspendProcess(pid);
    if (success) {
        refresh();
    }
    return success;
}

bool ProcessModel::resumeProcess(int pid)
{
    bool success = m_manager.resumeProcess(pid);
    if (success) {
        refresh();
    }
    return success;
}
