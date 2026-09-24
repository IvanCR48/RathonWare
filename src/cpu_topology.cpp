#include "cpu_topology.h"
#include <tlhelp32.h>
#include <psapi.h>
#include <processthreadsapi.h>
#include <vector>
#include <algorithm>
#include <QDebug>

#define SystemProcessorPerformanceInformation 8

typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION {
    LARGE_INTEGER IdleTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER DpcTime;
    LARGE_INTEGER InterruptTime;
    ULONG InterruptCount;
} SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION;

typedef NTSTATUS(NTAPI* pfnNtQuerySystemInformation)(
    ULONG SystemInformationClass,
    PVOID SystemInformation,
    ULONG SystemInformationLength,
    PULONG ReturnLength
);

static pfnNtQuerySystemInformation NtQuerySystemInformation = nullptr;

// --- CpuCoreModel ---

CpuCoreModel::CpuCoreModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

CpuCoreModel::~CpuCoreModel()
{
}

int CpuCoreModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_cores.count();
}

QVariant CpuCoreModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_cores.count()) {
        return QVariant();
    }

    const CpuCoreEntry &entry = m_cores[index.row()];
    switch (role) {
    case CoreIndexRole: return entry.coreIndex;
    case CoreLabelRole: return entry.coreLabel;
    case CoreTypeRole: return entry.coreType;
    case IsPCoreRole: return entry.isPCore;
    case IsECoreRole: return entry.isECore;
    case LoadRole: return entry.load;
    case HeatColorRole: return entry.heatColor;
    default: return QVariant();
    }
}

QHash<int, QByteArray> CpuCoreModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[CoreIndexRole] = "coreIndex";
    roles[CoreLabelRole] = "coreLabel";
    roles[CoreTypeRole] = "coreType";
    roles[IsPCoreRole] = "isPCore";
    roles[IsECoreRole] = "isECore";
    roles[LoadRole] = "load";
    roles[HeatColorRole] = "heatColor";
    return roles;
}

void CpuCoreModel::updateCores(const QVector<CpuCoreEntry>& cores)
{
    beginResetModel();
    m_cores = cores;
    endResetModel();
}

// --- CpuTopology ---

CpuTopology::CpuTopology(CpuCoreModel *coreModel, QObject *parent)
    : QObject(parent)
    , m_coreModel(coreModel)
{
    // Dynamically resolve ntdll!NtQuerySystemInformation.
    // Querying per-core utilization through WMI or PDH takes between 1.5 to 3 seconds of initialization
    // and causes visible UI stuttering in Qt Quick.
    // NtQuerySystemInformation(SystemProcessorPerformanceInformation = 8) pulls the raw tick counts
    // directly from kernel memory in less than 50 microseconds.
    HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");
    if (hNtDll) {
        NtQuerySystemInformation = (pfnNtQuerySystemInformation)GetProcAddress(hNtDll, "NtQuerySystemInformation");
    }

    initTopology();
    updateCoreStats();

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &CpuTopology::updateCoreStats);
    m_timer->start(1000);
}

CpuTopology::~CpuTopology()
{
}

// Discovers physical and logical CPU layout, detecting Intel Alder Lake / Raptor Lake hybrid cores.
// Uses GetLogicalProcessorInformationEx with RelationProcessorCore:
// - On Windows 10 (21H2+) and Windows 11, Microsoft added EfficiencyClass to PROCESSOR_RELATIONSHIP.
// - Due to MinGW / MSVC header alignment differences across SDK revisions, EfficiencyClass is reliably
//   located at byte offset 1 of the Processor union.
//   Value 0 = Gracemont/Crestmont Efficiency Core (E-Core).
//   Value 1+ = Golden Cove/Raptor Cove Performance Core (P-Core).
void CpuTopology::initTopology()
{
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    m_totalCores = sysInfo.dwNumberOfProcessors;
    if (m_totalCores < 1) m_totalCores = 1;

    m_isPCoreList.resize(m_totalCores);
    m_prevSamples.resize(m_totalCores);

    DWORD length = 0;
    GetLogicalProcessorInformationEx(RelationProcessorCore, NULL, &length);
    if (length > 0) {
        std::vector<BYTE> buffer(length);
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX info = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data());
        if (GetLogicalProcessorInformationEx(RelationProcessorCore, info, &length)) {
            BYTE* ptr = buffer.data();
            int coreIdx = 0;
            while (ptr < buffer.data() + length) {
                PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX item = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(ptr);
                if (item->Relationship == RelationProcessorCore) {
                    KAFFINITY mask = item->Processor.GroupMask[0].Mask;
                    // In Windows 10/11, EfficiencyClass is at byte offset 1 in PROCESSOR_RELATIONSHIP
                    BYTE efficiencyClass = reinterpret_cast<const BYTE*>(&item->Processor)[1];
                    bool isPerformance = (efficiencyClass > 0);
                    if (isPerformance) {
                        m_pCoreMask |= mask;
                    } else {
                        m_eCoreMask |= mask;
                    }
                    m_allCoresMask |= mask;

                    // Set mask bits for logical processors
                    for (int bit = 0; bit < m_totalCores; bit++) {
                        if (mask & (1ULL << bit)) {
                            m_isPCoreList[bit] = isPerformance;
                        }
                    }
                    coreIdx++;
                }
                ptr += item->Size;
            }
        }
    }

    if (m_pCoreMask != 0 && m_eCoreMask != 0) {
        m_hasHybridArchitecture = true;
        for (int i = 0; i < m_totalCores; i++) {
            if (m_isPCoreList[i]) m_pCoreCount++;
            else m_eCoreCount++;
        }
    } else {
        // Fallback for homogeneous processors (AMD Ryzen, Threadripper, Intel 11th Gen and older).
        // Since all cores are identical, we designate the first half as "primary" for affinity grouping.
        if (m_allCoresMask == 0) {
            m_allCoresMask = (m_totalCores >= 64) ? ~0ULL : ((1ULL << m_totalCores) - 1);
        }
        int half = m_totalCores / 2;
        if (half >= 1) {
            m_pCoreMask = (1ULL << half) - 1;
            m_eCoreMask = m_allCoresMask & ~m_pCoreMask;
            for (int i = 0; i < m_totalCores; i++) {
                m_isPCoreList[i] = (i < half);
                if (i < half) m_pCoreCount++;
                else m_eCoreCount++;
            }
        } else {
            m_pCoreMask = m_allCoresMask;
            m_eCoreMask = m_allCoresMask;
            m_pCoreCount = m_totalCores;
            m_eCoreCount = 0;
            for (int i = 0; i < m_totalCores; i++) {
                m_isPCoreList[i] = true;
            }
        }
    }
}

QString CpuTopology::computeHeatColor(double load)
{
    if (load < 20.0) return "#2b6cb0";       // Cool steel blue
    if (load < 40.0) return "#008080";       // Teal
    if (load < 65.0) return "#2e7d32";       // Emerald green
    if (load < 85.0) return "#d97706";       // Amber
    return "#c62828";                        // Crimson Red
}

void CpuTopology::updateCoreStats()
{
    if (!NtQuerySystemInformation || m_totalCores <= 0) return;

    ULONG size = sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * m_totalCores;
    std::vector<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION> perfInfo(m_totalCores);
    ULONG returnLength = 0;

    NTSTATUS status = NtQuerySystemInformation(SystemProcessorPerformanceInformation, perfInfo.data(), size, &returnLength);
    if (status != 0) return;

    QVector<CpuCoreEntry> entries;
    double pCoreSum = 0.0;
    int pCoreActive = 0;
    double eCoreSum = 0.0;
    int eCoreActive = 0;

    for (int i = 0; i < m_totalCores; i++) {
        const auto& info = perfInfo[i];
        const auto& prev = m_prevSamples[i];

        ULONGLONG idleDiff = info.IdleTime.QuadPart - prev.idleTime.QuadPart;
        ULONGLONG kernelDiff = info.KernelTime.QuadPart - prev.kernelTime.QuadPart;
        ULONGLONG userDiff = info.UserTime.QuadPart - prev.userTime.QuadPart;
        ULONGLONG totalDiff = kernelDiff + userDiff;

        double coreLoad = 0.0;
        if (totalDiff > 0 && prev.idleTime.QuadPart > 0) {
            double usage = (100.0 * (totalDiff - idleDiff)) / totalDiff;
            coreLoad = (usage < 0.0) ? 0.0 : (usage > 100.0) ? 100.0 : usage;
        }

        bool isP = m_isPCoreList[i];
        if (isP) {
            pCoreSum += coreLoad;
            pCoreActive++;
        } else {
            eCoreSum += coreLoad;
            eCoreActive++;
        }

        CpuCoreEntry entry;
        entry.coreIndex = i;
        entry.isPCore = isP;
        entry.isECore = !isP;
        entry.coreType = isP ? "P-Core" : "E-Core";
        entry.coreLabel = QString("Core #%1 (%2)").arg(i).arg(entry.coreType);
        entry.load = coreLoad;
        entry.heatColor = computeHeatColor(coreLoad);
        entries.append(entry);

        // Store sample
        m_prevSamples[i].idleTime = info.IdleTime;
        m_prevSamples[i].kernelTime = info.KernelTime;
        m_prevSamples[i].userTime = info.UserTime;
    }

    m_pCoreAvgUsage = (pCoreActive > 0) ? (pCoreSum / pCoreActive) : 0.0;
    m_eCoreAvgUsage = (eCoreActive > 0) ? (eCoreSum / eCoreActive) : 0.0;

    if (m_coreModel) {
        m_coreModel->updateCores(entries);
    }

    emit statsChanged();
}

bool CpuTopology::pinProcessToECores(int pid)
{
    if (pid <= 4 || m_eCoreMask == 0) return false;
    HANDLE hProcess = OpenProcess(PROCESS_SET_INFORMATION | PROCESS_SET_QUOTA, FALSE, pid);
    if (!hProcess) return false;

    bool aff = SetProcessAffinityMask(hProcess, m_eCoreMask);
    
    // Set EcoQoS
    PROCESS_POWER_THROTTLING_STATE powerThrottling;
    memset(&powerThrottling, 0, sizeof(powerThrottling));
    powerThrottling.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
    powerThrottling.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;
    powerThrottling.StateMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;
    SetProcessInformation(hProcess, ProcessPowerThrottling, &powerThrottling, sizeof(powerThrottling));
    SetPriorityClass(hProcess, BELOW_NORMAL_PRIORITY_CLASS);

    CloseHandle(hProcess);
    return aff;
}

bool CpuTopology::pinProcessToPCores(int pid)
{
    if (pid <= 4 || m_pCoreMask == 0) return false;
    HANDLE hProcess = OpenProcess(PROCESS_SET_INFORMATION | PROCESS_SET_QUOTA, FALSE, pid);
    if (!hProcess) return false;

    bool aff = SetProcessAffinityMask(hProcess, m_pCoreMask);

    PROCESS_POWER_THROTTLING_STATE powerThrottling;
    memset(&powerThrottling, 0, sizeof(powerThrottling));
    powerThrottling.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
    powerThrottling.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;
    powerThrottling.StateMask = 0;
    SetProcessInformation(hProcess, ProcessPowerThrottling, &powerThrottling, sizeof(powerThrottling));
    SetPriorityClass(hProcess, NORMAL_PRIORITY_CLASS);

    CloseHandle(hProcess);
    return aff;
}

bool CpuTopology::resetProcessAffinity(int pid)
{
    if (pid <= 4 || m_allCoresMask == 0) return false;
    HANDLE hProcess = OpenProcess(PROCESS_SET_INFORMATION | PROCESS_SET_QUOTA, FALSE, pid);
    if (!hProcess) return false;

    bool aff = SetProcessAffinityMask(hProcess, m_allCoresMask);

    PROCESS_POWER_THROTTLING_STATE powerThrottling;
    memset(&powerThrottling, 0, sizeof(powerThrottling));
    powerThrottling.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
    powerThrottling.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;
    powerThrottling.StateMask = 0;
    SetProcessInformation(hProcess, ProcessPowerThrottling, &powerThrottling, sizeof(powerThrottling));

    CloseHandle(hProcess);
    return aff;
}

QVariantList CpuTopology::getBackgroundApps()
{
    QVariantList list;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return list;

    // Common background apps to suggest for E-Core pinning
    QStringList targets = {
        "discord", "chrome", "slack", "spotify", "node", "python", 
        "qbittorrent", "steam", "epicgameslauncher", "obs64", "vlc", 
        "whatsapp", "telegram", "msedge", "firefox", "devenv", "code"
    };

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            unsigned long pid = pe.th32ProcessID;
            if (pid <= 4) continue;

            QString name = QString::fromWCharArray(pe.szExeFile);
            QString lower = name.toLower();

            bool isTarget = false;
            for (const QString& t : targets) {
                if (lower.contains(t)) {
                    isTarget = true;
                    break;
                }
            }

            if (isTarget) {
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, pid);
                double ramMB = 0.0;
                bool isPinnedToE = false;

                if (hProcess) {
                    PROCESS_MEMORY_COUNTERS pmc;
                    if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
                        ramMB = pmc.WorkingSetSize / (1024.0 * 1024.0);
                    }
                    DWORD_PTR procAff = 0, sysAff = 0;
                    if (GetProcessAffinityMask(hProcess, &procAff, &sysAff)) {
                        if (m_eCoreMask != 0 && (procAff & ~m_eCoreMask) == 0) {
                            isPinnedToE = true;
                        }
                    }
                    CloseHandle(hProcess);
                }

                QVariantMap map;
                map["pid"] = static_cast<qlonglong>(pid);
                map["name"] = name;
                map["ramMB"] = ramMB;
                map["isPinnedToE"] = isPinnedToE;
                list.append(map);
            }
        } while (Process32NextW(hSnapshot, &pe));
    }
    CloseHandle(hSnapshot);
    return list;
}
