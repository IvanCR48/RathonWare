#include "process_model.h"
#include <algorithm>
#include <QSet>
#include <QHash>

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
    case ParentPidRole:
        return static_cast<qlonglong>(process.parentPid);
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
    case IsSuspendedRole:
        return process.isSuspended;
    case IsEcoQosRole:
        return process.isEcoQos;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> ProcessModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[PidRole] = "pid";
    roles[ParentPidRole] = "parentPid";
    roles[NameRole] = "name";
    roles[CpuRole] = "cpu";
    roles[RamRole] = "ram";
    roles[PrivateRole] = "privateUsage";
    roles[PeakRole] = "peakUsage";
    roles[ThreadsRole] = "threads";
    roles[UsernameRole] = "username";
    roles[CmdLineRole] = "cmdLine";
    roles[PriorityRole] = "priority";
    roles[IsSuspendedRole] = "isSuspended";
    roles[IsEcoQosRole] = "isEcoQos";
    return roles;
}

void ProcessModel::refresh()
{
    QVector<ProcessInfo> newProcesses = m_manager.updateProcessList();
    
    // Sort new list by CPU usage descending by default
    std::sort(newProcesses.begin(), newProcesses.end(), [](const ProcessInfo& a, const ProcessInfo& b) {
        if (a.cpuUsage != b.cpuUsage) {
            return a.cpuUsage > b.cpuUsage;
        }
        return a.ramUsage > b.ramUsage;
    });

    // Initial load: reset model
    if (m_processes.isEmpty()) {
        beginResetModel();
        m_processes = newProcesses;
        endResetModel();
        return;
    }

    // In-Place Update to prevent resetting scroll position
    QHash<unsigned long, int> oldPidIndex;
    for (int i = 0; i < m_processes.size(); ++i) {
        oldPidIndex[m_processes[i].pid] = i;
    }

    QSet<unsigned long> newPidSet;
    for (const auto& np : newProcesses) {
        newPidSet.insert(np.pid);
    }

    // 1. Remove dead processes
    for (int i = m_processes.size() - 1; i >= 0; --i) {
        if (!newPidSet.contains(m_processes[i].pid)) {
            beginRemoveRows(QModelIndex(), i, i);
            m_processes.removeAt(i);
            endRemoveRows();
        }
    }

    // Rebuild index after removals
    oldPidIndex.clear();
    for (int i = 0; i < m_processes.size(); ++i) {
        oldPidIndex[m_processes[i].pid] = i;
    }

    // 2. Update existing processes in-place & append new processes
    for (const auto& np : newProcesses) {
        if (oldPidIndex.contains(np.pid)) {
            int idx = oldPidIndex[np.pid];
            ProcessInfo& existing = m_processes[idx];
            
            bool changed = false;
            if (existing.cpuUsage != np.cpuUsage || existing.ramUsage != np.ramUsage ||
                existing.privateUsage != np.privateUsage || existing.threads != np.threads ||
                existing.priority != np.priority || existing.isSuspended != np.isSuspended ||
                existing.isEcoQos != np.isEcoQos) 
            {
                existing.cpuUsage = np.cpuUsage;
                existing.ramUsage = np.ramUsage;
                existing.privateUsage = np.privateUsage;
                existing.peakUsage = np.peakUsage;
                existing.threads = np.threads;
                existing.priority = np.priority;
                existing.isSuspended = np.isSuspended;
                existing.isEcoQos = np.isEcoQos;
                changed = true;
            }

            if (changed) {
                emit dataChanged(createIndex(idx, 0), createIndex(idx, 0));
            }
        } else {
            // New process arrived
            int newRow = m_processes.size();
            beginInsertRows(QModelIndex(), newRow, newRow);
            m_processes.append(np);
            endInsertRows();
            oldPidIndex[np.pid] = newRow;
        }
    }
}

bool ProcessModel::killProcess(int pid)
{
    bool success = m_manager.killProcess(pid);
    if (success) {
        refresh();
    }
    return success;
}

bool ProcessModel::killProcessTree(int pid)
{
    bool success = m_manager.killProcessTree(pid);
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

bool ProcessModel::pinToECores(int pid)
{
    bool success = m_manager.pinToECores(pid);
    if (success) {
        refresh();
    }
    return success;
}

bool ProcessModel::pinToPCores(int pid)
{
    bool success = m_manager.pinToPCores(pid);
    if (success) {
        refresh();
    }
    return success;
}

bool ProcessModel::resetAffinity(int pid)
{
    bool success = m_manager.resetAffinity(pid);
    if (success) {
        refresh();
    }
    return success;
}
