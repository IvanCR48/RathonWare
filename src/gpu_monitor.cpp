#include "gpu_monitor.h"
#include <tlhelp32.h>
#include <psapi.h>
#include <vector>
#include <algorithm>
#include <QDebug>

// NVML definitions & dynamic loading structures
typedef int nvmlReturn_t;
#define NVML_SUCCESS 0
#define NVML_ERROR_INSUFFICIENT_SIZE 7
#define NVML_TEMPERATURE_GPU 0
#define NVML_CLOCK_GRAPHICS 0
#define NVML_CLOCK_SM 1
#define NVML_CLOCK_MEM 2

struct nvmlUtilization_t {
    unsigned int device; // GPU load %
    unsigned int memory; // VRAM access load %
};

struct nvmlMemory_t {
    unsigned long long total;
    unsigned long long free;
    unsigned long long used;
};

struct nvmlProcessInfo_t {
    unsigned int pid;
    unsigned long long usedGpuMemory;
    unsigned int gpuInstanceId;
    unsigned int computeInstanceId;
};

// Throttle reasons bitmask constants
#define NVML_THROTTLE_NONE                     0x0000000000000000ULL
#define NVML_THROTTLE_GPU_HW_SLOWDOWN          0x0000000000000008ULL
#define NVML_THROTTLE_THERMAL                  0x0000000000000020ULL
#define NVML_THROTTLE_SW_POWER_CAP             0x0000000000000004ULL
#define NVML_THROTTLE_HW_SLOWDOWN              0x0000000000000008ULL
#define NVML_THROTTLE_SYNC_BOOST               0x0000000000000010ULL
#define NVML_THROTTLE_SW_THERMAL               0x0000000000000020ULL
#define NVML_THROTTLE_HW_THERMAL               0x0000000000000040ULL
#define NVML_THROTTLE_HW_POWER_BRAKE           0x0000000000000080ULL
#define NVML_THROTTLE_DISPLAY_CLOCK            0x0000000000000100ULL

typedef nvmlReturn_t (*nvmlInit_v2_t)();
typedef nvmlReturn_t (*nvmlShutdown_t)();
typedef nvmlReturn_t (*nvmlDeviceGetHandleByIndex_v2_t)(unsigned int, void**);
typedef nvmlReturn_t (*nvmlDeviceGetUtilizationRates_t)(void*, nvmlUtilization_t*);
typedef nvmlReturn_t (*nvmlDeviceGetTemperature_t)(void*, int, unsigned int*);
typedef nvmlReturn_t (*nvmlDeviceGetMemoryInfo_t)(void*, nvmlMemory_t*);
typedef nvmlReturn_t (*nvmlDeviceGetName_t)(void*, char*, unsigned int);
typedef nvmlReturn_t (*nvmlDeviceGetPowerUsage_t)(void*, unsigned int*);
typedef nvmlReturn_t (*nvmlDeviceGetEnforcedPowerLimit_t)(void*, unsigned int*);
typedef nvmlReturn_t (*nvmlDeviceGetClockInfo_t)(void*, int, unsigned int*);
typedef nvmlReturn_t (*nvmlDeviceGetCurrentClocksThrottleReasons_t)(void*, unsigned long long*);
typedef nvmlReturn_t (*nvmlDeviceGetComputeRunningProcesses_t)(void*, unsigned int*, nvmlProcessInfo_t*);
typedef nvmlReturn_t (*nvmlDeviceGetGraphicsRunningProcesses_t)(void*, unsigned int*, nvmlProcessInfo_t*);

static nvmlInit_v2_t pfn_nvmlInit = nullptr;
static nvmlShutdown_t pfn_nvmlShutdown = nullptr;
static nvmlDeviceGetHandleByIndex_v2_t pfn_nvmlDeviceGetHandleByIndex = nullptr;
static nvmlDeviceGetUtilizationRates_t pfn_nvmlDeviceGetUtilizationRates = nullptr;
static nvmlDeviceGetTemperature_t pfn_nvmlDeviceGetTemperature = nullptr;
static nvmlDeviceGetMemoryInfo_t pfn_nvmlDeviceGetMemoryInfo = nullptr;
static nvmlDeviceGetName_t pfn_nvmlDeviceGetName = nullptr;
static nvmlDeviceGetPowerUsage_t pfn_nvmlDeviceGetPowerUsage = nullptr;
static nvmlDeviceGetEnforcedPowerLimit_t pfn_nvmlDeviceGetEnforcedPowerLimit = nullptr;
static nvmlDeviceGetClockInfo_t pfn_nvmlDeviceGetClockInfo = nullptr;
static nvmlDeviceGetCurrentClocksThrottleReasons_t pfn_nvmlDeviceGetCurrentClocksThrottleReasons = nullptr;
static nvmlDeviceGetComputeRunningProcesses_t pfn_nvmlDeviceGetComputeRunningProcesses = nullptr;
static nvmlDeviceGetGraphicsRunningProcesses_t pfn_nvmlDeviceGetGraphicsRunningProcesses = nullptr;

// --- GpuProcessModel ---

GpuProcessModel::GpuProcessModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

GpuProcessModel::~GpuProcessModel()
{
}

int GpuProcessModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_processes.count();
}

QVariant GpuProcessModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_processes.count()) {
        return QVariant();
    }

    const GpuProcessEntry &entry = m_processes[index.row()];
    switch (role) {
    case PidRole: return static_cast<qlonglong>(entry.pid);
    case NameRole: return entry.name;
    case CategoryRole: return entry.category;
    case VramMBRole: return entry.vramMB;
    case VramGBRole: return entry.vramGB;
    case VramPercentRole: return entry.vramPercent;
    case IsComputeRole: return entry.isCompute;
    case LeakSuspectedRole: return entry.leakSuspected;
    case GrowthRateMBRole: return entry.growthRateMB;
    default: return QVariant();
    }
}

QHash<int, QByteArray> GpuProcessModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[PidRole] = "pid";
    roles[NameRole] = "name";
    roles[CategoryRole] = "category";
    roles[VramMBRole] = "vramMB";
    roles[VramGBRole] = "vramGB";
    roles[VramPercentRole] = "vramPercent";
    roles[IsComputeRole] = "isCompute";
    roles[LeakSuspectedRole] = "leakSuspected";
    roles[GrowthRateMBRole] = "growthRateMB";
    return roles;
}

void GpuProcessModel::updateProcesses(const QVector<GpuProcessEntry>& processes)
{
    beginResetModel();
    m_processes = processes;
    endResetModel();
}

bool GpuProcessModel::killProcess(int pid)
{
    if (pid <= 4) return false;
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, static_cast<DWORD>(pid));
    if (!hProcess) return false;
    bool success = TerminateProcess(hProcess, 1);
    CloseHandle(hProcess);
    return success;
}

// --- GpuMonitor ---

GpuMonitor::GpuMonitor(GpuProcessModel *processModel, QObject *parent)
    : QObject(parent)
    , m_processModel(processModel)
{
    // Attempt high-fidelity NVIDIA Management Library (NVML) telemetry first.
    // If not an NVIDIA GPU (AMD Radeon, Intel Arc, or VM), cleanly fall back to DXGI 1.4.
    initNvml();
    if (!m_hasNvidiaGpu) {
        initDxgi();
    }
    updateTelemetry();

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &GpuMonitor::updateTelemetry);
    m_timer->start(1500);
}

GpuMonitor::~GpuMonitor()
{
    if (m_hasNvidiaGpu && pfn_nvmlShutdown) {
        pfn_nvmlShutdown();
    }
    if (m_nvmlLib) {
        FreeLibrary(m_nvmlLib);
    }
    if (m_dxgiAdapter3) {
        m_dxgiAdapter3->Release();
        m_dxgiAdapter3 = nullptr;
    }
    if (m_dxgiFactory) {
        m_dxgiFactory->Release();
        m_dxgiFactory = nullptr;
    }
}

// Dynamically binds to nvml.dll located in System32 or driver store.
// Why dynamic loading? If you link against nvml.lib statically, your executable instantly fails
// to launch on any machine with an AMD Radeon or Intel Arc GPU (STATUS_DLL_NOT_FOUND 0xC0000135).
// Late-binding gives us rock-solid portability across every PC.
void GpuMonitor::initNvml()
{
    m_nvmlLib = LoadLibraryW(L"nvml.dll");
    if (!m_nvmlLib) return;

    pfn_nvmlInit = (nvmlInit_v2_t)GetProcAddress(m_nvmlLib, "nvmlInit_v2");
    pfn_nvmlShutdown = (nvmlShutdown_t)GetProcAddress(m_nvmlLib, "nvmlShutdown");
    pfn_nvmlDeviceGetHandleByIndex = (nvmlDeviceGetHandleByIndex_v2_t)GetProcAddress(m_nvmlLib, "nvmlDeviceGetHandleByIndex_v2");
    pfn_nvmlDeviceGetUtilizationRates = (nvmlDeviceGetUtilizationRates_t)GetProcAddress(m_nvmlLib, "nvmlDeviceGetUtilizationRates");
    pfn_nvmlDeviceGetTemperature = (nvmlDeviceGetTemperature_t)GetProcAddress(m_nvmlLib, "nvmlDeviceGetTemperature");
    pfn_nvmlDeviceGetMemoryInfo = (nvmlDeviceGetMemoryInfo_t)GetProcAddress(m_nvmlLib, "nvmlDeviceGetMemoryInfo");
    pfn_nvmlDeviceGetName = (nvmlDeviceGetName_t)GetProcAddress(m_nvmlLib, "nvmlDeviceGetName");
    pfn_nvmlDeviceGetPowerUsage = (nvmlDeviceGetPowerUsage_t)GetProcAddress(m_nvmlLib, "nvmlDeviceGetPowerUsage");
    pfn_nvmlDeviceGetEnforcedPowerLimit = (nvmlDeviceGetEnforcedPowerLimit_t)GetProcAddress(m_nvmlLib, "nvmlDeviceGetEnforcedPowerLimit");
    pfn_nvmlDeviceGetClockInfo = (nvmlDeviceGetClockInfo_t)GetProcAddress(m_nvmlLib, "nvmlDeviceGetClockInfo");
    pfn_nvmlDeviceGetCurrentClocksThrottleReasons = (nvmlDeviceGetCurrentClocksThrottleReasons_t)GetProcAddress(m_nvmlLib, "nvmlDeviceGetCurrentClocksThrottleReasons");
    
    // NVIDIA driver API version dance:
    // nvmlDeviceGetComputeRunningProcesses_v3 was introduced for modern multi-instance GPU architectures;
    // fall back to v2/v1 on older drivers (e.g. GTX 10-series or legacy workstations).
    pfn_nvmlDeviceGetComputeRunningProcesses = (nvmlDeviceGetComputeRunningProcesses_t)GetProcAddress(m_nvmlLib, "nvmlDeviceGetComputeRunningProcesses_v3");
    if (!pfn_nvmlDeviceGetComputeRunningProcesses) {
        pfn_nvmlDeviceGetComputeRunningProcesses = (nvmlDeviceGetComputeRunningProcesses_t)GetProcAddress(m_nvmlLib, "nvmlDeviceGetComputeRunningProcesses");
    }

    pfn_nvmlDeviceGetGraphicsRunningProcesses = (nvmlDeviceGetGraphicsRunningProcesses_t)GetProcAddress(m_nvmlLib, "nvmlDeviceGetGraphicsRunningProcesses_v3");
    if (!pfn_nvmlDeviceGetGraphicsRunningProcesses) {
        pfn_nvmlDeviceGetGraphicsRunningProcesses = (nvmlDeviceGetGraphicsRunningProcesses_t)GetProcAddress(m_nvmlLib, "nvmlDeviceGetGraphicsRunningProcesses");
    }

    if (pfn_nvmlInit && pfn_nvmlShutdown && pfn_nvmlDeviceGetHandleByIndex &&
        pfn_nvmlDeviceGetUtilizationRates && pfn_nvmlDeviceGetTemperature && 
        pfn_nvmlDeviceGetMemoryInfo && pfn_nvmlDeviceGetName) 
    {
        if (pfn_nvmlInit() == NVML_SUCCESS) {
            if (pfn_nvmlDeviceGetHandleByIndex(0, &m_nvmlDevice) == NVML_SUCCESS) {
                m_hasNvidiaGpu = true;
                m_gpuBackend = "NVIDIA NVML";
                char name[128] = {0};
                if (pfn_nvmlDeviceGetName(m_nvmlDevice, name, sizeof(name)) == NVML_SUCCESS) {
                    m_gpuName = QString::fromUtf8(name);
                }
            } else {
                pfn_nvmlShutdown();
            }
        }
    }
}

void GpuMonitor::initDxgi()
{
    if (SUCCEEDED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&m_dxgiFactory))) {
        IDXGIAdapter1* pAdapter = nullptr;
        for (UINT i = 0; m_dxgiFactory->EnumAdapters1(i, &pAdapter) != DXGI_ERROR_NOT_FOUND; ++i) {
            DXGI_ADAPTER_DESC1 desc;
            if (SUCCEEDED(pAdapter->GetDesc1(&desc))) {
                if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) && i > 0) {
                    pAdapter->Release();
                    continue;
                }

                m_gpuName = QString::fromWCharArray(desc.Description).trimmed();
                m_vramTotalGB = desc.DedicatedVideoMemory / (1024.0 * 1024.0 * 1024.0);
                if (desc.VendorId == 0x1002) m_gpuBackend = "AMD Radeon (DXGI)";
                else if (desc.VendorId == 0x8086) m_gpuBackend = "Intel Graphics (DXGI)";
                else if (desc.VendorId == 0x10DE) m_gpuBackend = "NVIDIA (DXGI)";
                else m_gpuBackend = "DirectX Hardware (DXGI)";

                IDXGIAdapter3* pAdapter3 = nullptr;
                if (SUCCEEDED(pAdapter->QueryInterface(__uuidof(IDXGIAdapter3), (void**)&pAdapter3))) {
                    m_dxgiAdapter3 = pAdapter3;
                    m_dxgiInitialized = true;
                }
                pAdapter->Release();
                break;
            }
            pAdapter->Release();
        }
    }
}

QString GpuMonitor::decodeThrottleReasons(unsigned long long mask)
{
    if (mask == NVML_THROTTLE_NONE || mask == 0) {
        m_throttleStatusLevel = "normal";
        return "None / Full Boost";
    }

    if (mask & NVML_THROTTLE_HW_THERMAL || mask & NVML_THROTTLE_SW_THERMAL || mask & NVML_THROTTLE_THERMAL) {
        m_throttleStatusLevel = "critical";
        return "Thermal Limit";
    }
    if (mask & NVML_THROTTLE_GPU_HW_SLOWDOWN || mask & NVML_THROTTLE_HW_SLOWDOWN) {
        m_throttleStatusLevel = "critical";
        return "Hardware Slowdown";
    }
    if (mask & NVML_THROTTLE_SW_POWER_CAP || mask & NVML_THROTTLE_HW_POWER_BRAKE) {
        m_throttleStatusLevel = "warning";
        return "Power Cap Limit";
    }
    if (mask & NVML_THROTTLE_DISPLAY_CLOCK) {
        m_throttleStatusLevel = "normal";
        return "Display Idle Cap";
    }

    m_throttleStatusLevel = "warning";
    return "Clock Throttled";
}

QString GpuMonitor::getProcessName(unsigned long pid)
{
    if (pid <= 4) return "System";
    QString name = "Unknown";
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe;
        pe.dwSize = sizeof(PROCESSENTRY32W);
        if (Process32FirstW(hSnapshot, &pe)) {
            do {
                if (pe.th32ProcessID == pid) {
                    name = QString::fromWCharArray(pe.szExeFile);
                    break;
                }
            } while (Process32NextW(hSnapshot, &pe));
        }
        CloseHandle(hSnapshot);
    }
    return name;
}

QString GpuMonitor::detectAiWorkload(const QString& name, unsigned long pid)
{
    QString lower = name.toLower();
    
    // Check executable names
    if (lower.contains("ollama")) return "Ollama / LLM Runner";
    if (lower.contains("python")) {
        // Query command line if available to get model / script name
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (hProcess) {
            wchar_t path[MAX_PATH] = {0};
            DWORD size = MAX_PATH;
            if (QueryFullProcessImageNameW(hProcess, 0, path, &size)) {
                QString full = QString::fromWCharArray(path).toLower();
                if (full.contains("torch") || full.contains("cuda") || full.contains("comfy") || full.contains("stable")) {
                    CloseHandle(hProcess);
                    return "Python (PyTorch / AI Model)";
                }
            }
            CloseHandle(hProcess);
        }
        return "Python (CUDA Compute / AI)";
    }
    if (lower.contains("llama") || lower.contains("vllm") || lower.contains("text-generation")) {
        return "LLM Inference Engine";
    }
    if (lower.contains("blender")) return "Blender 3D (CUDA/OptiX)";
    if (lower.contains("unreal") || lower.contains("unity")) return "Game Engine Render";
    if (lower.contains("dwm.exe")) return "Windows Desktop Window Manager";
    if (lower.contains("chrome") || lower.contains("msedge") || lower.contains("firefox")) {
        return "Web Browser (GPU Acceleration)";
    }

    return "CUDA / GPU Process";
}

void GpuMonitor::queryGpuProcesses()
{
    if (!m_hasNvidiaGpu || !m_nvmlDevice || !m_processModel) return;

    QVector<GpuProcessEntry> entries;
    QHash<unsigned long, GpuProcessEntry> processMap;

    unsigned int procCount = 64;
    std::vector<nvmlProcessInfo_t> computeProcs(procCount);

    if (pfn_nvmlDeviceGetComputeRunningProcesses) {
        nvmlReturn_t ret = pfn_nvmlDeviceGetComputeRunningProcesses(m_nvmlDevice, &procCount, computeProcs.data());
        if (ret == NVML_ERROR_INSUFFICIENT_SIZE && procCount > 0) {
            computeProcs.resize(procCount);
            pfn_nvmlDeviceGetComputeRunningProcesses(m_nvmlDevice, &procCount, computeProcs.data());
        }
        for (unsigned int i = 0; i < procCount; i++) {
            unsigned long pid = computeProcs[i].pid;
            if (pid <= 0) continue;

            double vramBytes = static_cast<double>(computeProcs[i].usedGpuMemory);
            double vramMB = vramBytes / (1024.0 * 1024.0);
            double vramGB = vramBytes / (1024.0 * 1024.0 * 1024.0);
            double vramPct = (m_vramTotalGB > 0) ? (vramGB / m_vramTotalGB) * 100.0 : 0.0;

            QString baseName = getProcessName(pid);
            QString workload = detectAiWorkload(baseName, pid);
            QString displayName = QString("%1 (%2)").arg(baseName, workload);

            GpuProcessEntry entry;
            entry.pid = pid;
            entry.name = displayName;
            entry.category = "CUDA Compute / AI";
            entry.vramMB = vramMB;
            entry.vramGB = vramGB;
            entry.vramPercent = vramPct;
            entry.isCompute = true;
            entry.leakSuspected = false;
            entry.growthRateMB = 0.0;

            processMap[pid] = entry;
        }
    }

    // Query graphics processes
    procCount = 64;
    std::vector<nvmlProcessInfo_t> graphicsProcs(procCount);
    if (pfn_nvmlDeviceGetGraphicsRunningProcesses) {
        nvmlReturn_t ret = pfn_nvmlDeviceGetGraphicsRunningProcesses(m_nvmlDevice, &procCount, graphicsProcs.data());
        if (ret == NVML_ERROR_INSUFFICIENT_SIZE && procCount > 0) {
            graphicsProcs.resize(procCount);
            pfn_nvmlDeviceGetGraphicsRunningProcesses(m_nvmlDevice, &procCount, graphicsProcs.data());
        }
        for (unsigned int i = 0; i < procCount; i++) {
            unsigned long pid = graphicsProcs[i].pid;
            if (pid <= 0) continue;

            double vramBytes = static_cast<double>(graphicsProcs[i].usedGpuMemory);
            double vramMB = vramBytes / (1024.0 * 1024.0);
            double vramGB = vramBytes / (1024.0 * 1024.0 * 1024.0);
            double vramPct = (m_vramTotalGB > 0) ? (vramGB / m_vramTotalGB) * 100.0 : 0.0;

            if (processMap.contains(pid)) {
                // Already listed as compute, add graphics VRAM
                processMap[pid].vramMB += vramMB;
                processMap[pid].vramGB += vramGB;
                processMap[pid].vramPercent = (m_vramTotalGB > 0) ? (processMap[pid].vramGB / m_vramTotalGB) * 100.0 : 0.0;
            } else {
                QString baseName = getProcessName(pid);
                QString workload = detectAiWorkload(baseName, pid);
                QString displayName = (baseName == "dwm.exe" || baseName == "csrss.exe") ? baseName : QString("%1 (%2)").arg(baseName, workload);

                GpuProcessEntry entry;
                entry.pid = pid;
                entry.name = displayName;
                entry.category = "DirectX / Graphics";
                entry.vramMB = vramMB;
                entry.vramGB = vramGB;
                entry.vramPercent = vramPct;
                entry.isCompute = false;
                entry.leakSuspected = false;
                entry.growthRateMB = 0.0;

                processMap[pid] = entry;
            }
        }
    }

    // VRAM Leak Detector Analysis
    ULONGLONG currentTime = GetTickCount64();
    for (auto it = processMap.begin(); it != processMap.end(); ++it) {
        unsigned long pid = it.key();
        double currentMB = it.value().vramMB;

        if (!m_vramHistory.contains(pid)) {
            VramHistoryEntry hist;
            hist.samplesMB.append(currentMB);
            hist.firstSeenTime = currentTime;
            hist.lastSeenTime = currentTime;
            m_vramHistory[pid] = hist;
        } else {
            VramHistoryEntry& hist = m_vramHistory[pid];
            hist.samplesMB.append(currentMB);
            if (hist.samplesMB.size() > 60) hist.samplesMB.removeFirst();
            hist.lastSeenTime = currentTime;

            // Analyze leak if at least 8 samples and at least 15 seconds
            if (hist.samplesMB.size() >= 8) {
                double firstMB = hist.samplesMB.first();
                double lastMB = hist.samplesMB.last();
                double diffMB = lastMB - firstMB;
                double elapsedMins = (currentTime - hist.firstSeenTime) / 60000.0;

                if (diffMB > 50.0 && firstMB > 0.0 && (diffMB / firstMB) > 0.25 && elapsedMins > 0.2) {
                    // Check if strictly non-decreasing trend
                    int increases = 0;
                    for (int s = 1; s < hist.samplesMB.size(); s++) {
                        if (hist.samplesMB[s] >= hist.samplesMB[s-1]) increases++;
                    }
                    if (increases >= hist.samplesMB.size() * 0.85) {
                        it.value().leakSuspected = true;
                        it.value().growthRateMB = (elapsedMins > 0.0) ? (diffMB / elapsedMins) : 0.0;
                    }
                }
            }
        }
    }

    // Clean up stale history entries
    QList<unsigned long> keys = m_vramHistory.keys();
    for (unsigned long k : keys) {
        if (!processMap.contains(k)) {
            m_vramHistory.remove(k);
        }
    }

    for (const auto& val : processMap) {
        entries.append(val);
    }

    // Sort descending by VRAM usage
    std::sort(entries.begin(), entries.end(), [](const GpuProcessEntry& a, const GpuProcessEntry& b) {
        return a.vramMB > b.vramMB;
    });

    m_processModel->updateProcesses(entries);
}

void GpuMonitor::updateTelemetry()
{
    if (m_hasNvidiaGpu && m_nvmlDevice) {
        // 1. Utilization
        nvmlUtilization_t util = {0, 0};
        if (pfn_nvmlDeviceGetUtilizationRates(m_nvmlDevice, &util) == NVML_SUCCESS) {
            m_gpuUsage = util.device;
        }

        // 2. Temperature
        unsigned int temp = 0;
        if (pfn_nvmlDeviceGetTemperature(m_nvmlDevice, NVML_TEMPERATURE_GPU, &temp) == NVML_SUCCESS) {
            m_gpuTemp = temp;
        }

        // 3. VRAM
        nvmlMemory_t mem = {0, 0, 0};
        if (pfn_nvmlDeviceGetMemoryInfo(m_nvmlDevice, &mem) == NVML_SUCCESS) {
            m_vramTotalGB = mem.total / (1024.0 * 1024.0 * 1024.0);
            m_vramUsedGB = mem.used / (1024.0 * 1024.0 * 1024.0);
            m_vramFreeGB = mem.free / (1024.0 * 1024.0 * 1024.0);
            m_vramUsagePercent = (mem.total > 0) ? (static_cast<double>(mem.used) / mem.total) * 100.0 : 0.0;
        }

        // 4. Power
        if (pfn_nvmlDeviceGetPowerUsage) {
            unsigned int powerMW = 0;
            if (pfn_nvmlDeviceGetPowerUsage(m_nvmlDevice, &powerMW) == NVML_SUCCESS) {
                m_powerUsageW = powerMW / 1000.0;
            }
        }
        if (pfn_nvmlDeviceGetEnforcedPowerLimit) {
            unsigned int limitMW = 0;
            if (pfn_nvmlDeviceGetEnforcedPowerLimit(m_nvmlDevice, &limitMW) == NVML_SUCCESS) {
                m_powerLimitW = limitMW / 1000.0;
            }
        }

        // 5. Clocks
        if (pfn_nvmlDeviceGetClockInfo) {
            unsigned int clockMhz = 0;
            if (pfn_nvmlDeviceGetClockInfo(m_nvmlDevice, NVML_CLOCK_GRAPHICS, &clockMhz) == NVML_SUCCESS) {
                m_graphicsClockMHz = clockMhz;
            }
            if (pfn_nvmlDeviceGetClockInfo(m_nvmlDevice, NVML_CLOCK_MEM, &clockMhz) == NVML_SUCCESS) {
                m_memoryClockMHz = clockMhz;
            }
        }

        // 6. Throttle Reasons
        if (pfn_nvmlDeviceGetCurrentClocksThrottleReasons) {
            unsigned long long reasons = 0;
            if (pfn_nvmlDeviceGetCurrentClocksThrottleReasons(m_nvmlDevice, &reasons) == NVML_SUCCESS) {
                m_throttleReason = decodeThrottleReasons(reasons);
            }
        }

        // 7. Per-process Breakdown
        queryGpuProcesses();

        emit statsChanged();
        return;
    }

    // DXGI fallback for AMD / Intel / generic GPUs
    if (m_dxgiInitialized && m_dxgiAdapter3) {
        DXGI_QUERY_VIDEO_MEMORY_INFO memInfo;
        if (SUCCEEDED(m_dxgiAdapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &memInfo))) {
            m_vramUsedGB = memInfo.CurrentUsage / (1024.0 * 1024.0 * 1024.0);
            m_vramFreeGB = (m_vramTotalGB > m_vramUsedGB) ? (m_vramTotalGB - m_vramUsedGB) : 0.0;
            m_vramUsagePercent = (m_vramTotalGB > 0) ? (m_vramUsedGB / m_vramTotalGB) * 100.0 : 0.0;
            m_throttleReason = "Active DirectX Adapter (Telemetry via DXGI)";
            m_throttleStatusLevel = "normal";
        }
        emit statsChanged();
    }
}

void GpuMonitor::refresh()
{
    updateTelemetry();
}
