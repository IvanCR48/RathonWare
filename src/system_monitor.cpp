#include "system_monitor.h"
#include <QDebug>
#include <WbemIdl.h>

// NVML definitions & dynamic loading structures
typedef int nvmlReturn_t;
#define NVML_SUCCESS 0
#define NVML_TEMPERATURE_GPU 0

struct nvmlUtilization_t {
    unsigned int device; // GPU load %
    unsigned int memory; // VRAM access load %
};

struct nvmlMemory_t {
    unsigned long long total;
    unsigned long long free;
    unsigned long long used;
};

typedef nvmlReturn_t (*nvmlInit_v2_t)();
typedef nvmlReturn_t (*nvmlShutdown_t)();
typedef nvmlReturn_t (*nvmlDeviceGetHandleByIndex_v2_t)(unsigned int, void**);
typedef nvmlReturn_t (*nvmlDeviceGetUtilizationRates_t)(void*, nvmlUtilization_t*);
typedef nvmlReturn_t (*nvmlDeviceGetTemperature_t)(void*, int, unsigned int*);
typedef nvmlReturn_t (*nvmlDeviceGetMemoryInfo_t)(void*, nvmlMemory_t*);
typedef nvmlReturn_t (*nvmlDeviceGetName_t)(void*, char*, unsigned int);

static nvmlInit_v2_t pfn_nvmlInit = nullptr;
static nvmlShutdown_t pfn_nvmlShutdown = nullptr;
static nvmlDeviceGetHandleByIndex_v2_t pfn_nvmlDeviceGetHandleByIndex = nullptr;
static nvmlDeviceGetUtilizationRates_t pfn_nvmlDeviceGetUtilizationRates = nullptr;
static nvmlDeviceGetTemperature_t pfn_nvmlDeviceGetTemperature = nullptr;
static nvmlDeviceGetMemoryInfo_t pfn_nvmlDeviceGetMemoryInfo = nullptr;
static nvmlDeviceGetName_t pfn_nvmlDeviceGetName = nullptr;

static ULONGLONG SubtractFileTime(const FILETIME& ftA, const FILETIME& ftB) {
    ULARGE_INTEGER a, b;
    a.LowPart = ftA.dwLowDateTime;
    a.HighPart = ftA.dwHighDateTime;
    b.LowPart = ftB.dwLowDateTime;
    b.HighPart = ftB.dwHighDateTime;
    return a.QuadPart - b.QuadPart;
}

SystemMonitor::SystemMonitor(QObject *parent)
    : QObject(parent)
{
    // Retrieve hardware specifications
    queryCpuModel();
    initGpuQuery();
    queryGpuModel();
    queryMotherboardAndBios();
    queryRamSpeed();

    // Setup initial CPU times
    initCpuQuery();

    // Setup PDH queries for Disk Read/Write rates
    initPdhQueries();

    // Trigger initial stats query
    updateStats();

    // Set up timer to query stats every 1 second
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &SystemMonitor::updateStats);
    m_timer->start(1000);
}

SystemMonitor::~SystemMonitor()
{
    if (m_nvmlInitialized && pfn_nvmlShutdown) {
        pfn_nvmlShutdown();
    }
    if (m_nvmlLib) {
        FreeLibrary(m_nvmlLib);
    }
    if (m_pdhInitialized && m_pdhQuery) {
        PdhCloseQuery(m_pdhQuery);
    }
}

void SystemMonitor::initCpuQuery()
{
    GetSystemTimes(&m_prevIdleTime, &m_prevKernelTime, &m_prevUserTime);
}

double SystemMonitor::calculateCpuUsage()
{
    FILETIME idleTime, kernelTime, userTime;
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        ULONGLONG idleDiff = SubtractFileTime(idleTime, m_prevIdleTime);
        ULONGLONG kernelDiff = SubtractFileTime(kernelTime, m_prevKernelTime);
        ULONGLONG userDiff = SubtractFileTime(userTime, m_prevUserTime);

        ULONGLONG totalDiff = kernelDiff + userDiff;
        m_prevIdleTime = idleTime;
        m_prevKernelTime = kernelTime;
        m_prevUserTime = userTime;

        if (totalDiff > 0) {
            double usage = 100.0 * (1.0 - static_cast<double>(idleDiff) / totalDiff);
            return (usage < 0.0) ? 0.0 : (usage > 100.0) ? 100.0 : usage;
        }
    }
    return 0.0;
}

void SystemMonitor::initGpuQuery()
{
    m_nvmlLib = LoadLibraryW(L"nvml.dll");
    if (!m_nvmlLib) {
        return;
    }

    pfn_nvmlInit = (nvmlInit_v2_t)GetProcAddress(m_nvmlLib, "nvmlInit_v2");
    pfn_nvmlShutdown = (nvmlShutdown_t)GetProcAddress(m_nvmlLib, "nvmlShutdown");
    pfn_nvmlDeviceGetHandleByIndex = (nvmlDeviceGetHandleByIndex_v2_t)GetProcAddress(m_nvmlLib, "nvmlDeviceGetHandleByIndex_v2");
    pfn_nvmlDeviceGetUtilizationRates = (nvmlDeviceGetUtilizationRates_t)GetProcAddress(m_nvmlLib, "nvmlDeviceGetUtilizationRates");
    pfn_nvmlDeviceGetTemperature = (nvmlDeviceGetTemperature_t)GetProcAddress(m_nvmlLib, "nvmlDeviceGetTemperature");
    pfn_nvmlDeviceGetMemoryInfo = (nvmlDeviceGetMemoryInfo_t)GetProcAddress(m_nvmlLib, "nvmlDeviceGetMemoryInfo");
    pfn_nvmlDeviceGetName = (nvmlDeviceGetName_t)GetProcAddress(m_nvmlLib, "nvmlDeviceGetName");

    if (pfn_nvmlInit && pfn_nvmlShutdown && pfn_nvmlDeviceGetHandleByIndex &&
        pfn_nvmlDeviceGetUtilizationRates && pfn_nvmlDeviceGetTemperature && 
        pfn_nvmlDeviceGetMemoryInfo && pfn_nvmlDeviceGetName) 
    {
        if (pfn_nvmlInit() == NVML_SUCCESS) {
            if (pfn_nvmlDeviceGetHandleByIndex(0, &m_nvmlDevice) == NVML_SUCCESS) {
                m_nvmlInitialized = true;
            } else {
                pfn_nvmlShutdown();
            }
        }
    } else {
        FreeLibrary(m_nvmlLib);
        m_nvmlLib = nullptr;
    }
}

void SystemMonitor::initPdhQueries()
{
    if (PdhOpenQueryW(NULL, NULL, &m_pdhQuery) == ERROR_SUCCESS) {
        // PdhAddEnglishCounterW avoids language mismatch problems on non-English Windows
        PdhAddEnglishCounterW(m_pdhQuery, L"\\PhysicalDisk(_Total)\\Disk Read Bytes/sec", NULL, &m_counterDiskRead);
        PdhAddEnglishCounterW(m_pdhQuery, L"\\PhysicalDisk(_Total)\\Disk Write Bytes/sec", NULL, &m_counterDiskWrite);
        
        if (PdhCollectQueryData(m_pdhQuery) == ERROR_SUCCESS) {
            m_pdhInitialized = true;
        }
    }
}

void SystemMonitor::updateStats()
{
    // 1. CPU Load
    double currentCpu = calculateCpuUsage();
    if (m_cpuUsage != currentCpu) {
        m_cpuUsage = currentCpu;
        emit cpuUsageChanged();
    }

    // 2. RAM Stats
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        double currentTotal = memInfo.ullTotalPhys / (1024.0 * 1024.0 * 1024.0);
        double currentUsed = currentTotal - (memInfo.ullAvailPhys / (1024.0 * 1024.0 * 1024.0));
        double currentUsagePercent = memInfo.dwMemoryLoad;

        if (m_ramTotal != currentTotal) {
            m_ramTotal = currentTotal;
            emit ramTotalChanged();
        }
        if (m_ramUsed != currentUsed) {
            m_ramUsed = currentUsed;
            emit ramUsedChanged();
        }
        if (m_ramUsage != currentUsagePercent) {
            m_ramUsage = currentUsagePercent;
            emit ramUsageChanged();
        }
    }

    // 3. GPU Stats
    updateGpuStats();

    // 4. Disk Speeds
    queryDiskSpeeds();

    // 5. Disk Usage
    queryDiskUsage();

    // 6. Network Speeds
    queryNetworkSpeeds();

    // 7. System Uptime
    updateUptime();
}

void SystemMonitor::updateGpuStats()
{
    if (!m_nvmlInitialized || !m_nvmlDevice) {
        m_gpuUsage = 0.0;
        m_gpuTemp = 0.0;
        m_gpuVramUsage = 0.0;
        m_gpuVramTotal = 0.0;
        m_gpuVramUsed = 0.0;
        return;
    }

    nvmlUtilization_t utilization;
    if (pfn_nvmlDeviceGetUtilizationRates(m_nvmlDevice, &utilization) == NVML_SUCCESS) {
        if (m_gpuUsage != utilization.device) {
            m_gpuUsage = utilization.device;
            emit gpuUsageChanged();
        }
    }

    unsigned int temp = 0;
    if (pfn_nvmlDeviceGetTemperature(m_nvmlDevice, NVML_TEMPERATURE_GPU, &temp) == NVML_SUCCESS) {
        if (m_gpuTemp != temp) {
            m_gpuTemp = temp;
            emit gpuTempChanged();
        }
    }

    nvmlMemory_t mem;
    if (pfn_nvmlDeviceGetMemoryInfo(m_nvmlDevice, &mem) == NVML_SUCCESS) {
        double totalVram = mem.total / (1024.0 * 1024.0 * 1024.0);
        double usedVram = mem.used / (1024.0 * 1024.0 * 1024.0);
        double vramUsagePercent = (static_cast<double>(mem.used) / mem.total) * 100.0;

        if (m_gpuVramTotal != totalVram) {
            m_gpuVramTotal = totalVram;
            emit gpuVramTotalChanged();
        }
        if (m_gpuVramUsed != usedVram) {
            m_gpuVramUsed = usedVram;
            emit gpuVramUsedChanged();
        }
        if (m_gpuVramUsage != vramUsagePercent) {
            m_gpuVramUsage = vramUsagePercent;
            emit gpuVramUsageChanged();
        }
    }
}

void SystemMonitor::queryDiskSpeeds()
{
    if (!m_pdhInitialized) return;
    
    if (PdhCollectQueryData(m_pdhQuery) == ERROR_SUCCESS) {
        PDH_FMT_COUNTERVALUE valRead, valWrite;
        if (PdhGetFormattedCounterValue(m_counterDiskRead, PDH_FMT_DOUBLE, NULL, &valRead) == ERROR_SUCCESS) {
            double readSpeed = valRead.doubleValue / (1024.0 * 1024.0); // Convert to MB/s
            if (m_diskReadSpeed != readSpeed) {
                m_diskReadSpeed = readSpeed;
                emit diskReadSpeedChanged();
            }
        }
        if (PdhGetFormattedCounterValue(m_counterDiskWrite, PDH_FMT_DOUBLE, NULL, &valWrite) == ERROR_SUCCESS) {
            double writeSpeed = valWrite.doubleValue / (1024.0 * 1024.0); // Convert to MB/s
            if (m_diskWriteSpeed != writeSpeed) {
                m_diskWriteSpeed = writeSpeed;
                emit diskWriteSpeedChanged();
            }
        }
    }
}

void SystemMonitor::queryDiskUsage()
{
    ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
    if (GetDiskFreeSpaceExW(L"C:\\", &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes)) {
        double totalGB = totalNumberOfBytes.QuadPart / (1024.0 * 1024.0 * 1024.0);
        double freeGB = totalNumberOfFreeBytes.QuadPart / (1024.0 * 1024.0 * 1024.0);
        double usedPercent = ((totalGB - freeGB) / totalGB) * 100.0;
        if (m_diskUsage != usedPercent) {
            m_diskUsage = usedPercent;
            emit diskUsageChanged();
        }
    }
}

void SystemMonitor::queryNetworkSpeeds()
{
    MIB_IF_TABLE2* pIfTable = nullptr;
    if (GetIfTable2(&pIfTable) == NO_ERROR) {
        ULONGLONG totalIn = 0;
        ULONGLONG totalOut = 0;
        
        for (ULONG i = 0; i < pIfTable->NumEntries; i++) {
            const MIB_IF_ROW2& row = pIfTable->Table[i];
            if ((row.Type == IF_TYPE_ETHERNET_CSMACD || row.Type == IF_TYPE_IEEE80211) && 
                row.OperStatus == IfOperStatusUp) {
                totalIn += row.InOctets;
                totalOut += row.OutOctets;
            }
        }
        
        ULONGLONG currentTime = GetTickCount64();
        if (m_lastNetQueryTime > 0) {
            double elapsedSecs = (currentTime - m_lastNetQueryTime) / 1000.0;
            if (elapsedSecs > 0.1) {
                double downloadSpeed = ((totalIn - m_prevNetInBytes) / elapsedSecs) / 1024.0; // KB/s
                double uploadSpeed = ((totalOut - m_prevNetOutBytes) / elapsedSecs) / 1024.0; // KB/s
                
                // Prevent negative outliers due to adapter reset
                if (downloadSpeed < 0) downloadSpeed = 0;
                if (uploadSpeed < 0) uploadSpeed = 0;

                if (m_netDownloadSpeed != downloadSpeed) {
                    m_netDownloadSpeed = downloadSpeed;
                    emit netDownloadSpeedChanged();
                }
                if (m_netUploadSpeed != uploadSpeed) {
                    m_netUploadSpeed = uploadSpeed;
                    emit netUploadSpeedChanged();
                }
            }
        }
        
        m_prevNetInBytes = totalIn;
        m_prevNetOutBytes = totalOut;
        m_lastNetQueryTime = currentTime;
        
        FreeMibTable(pIfTable);
    }
}

void SystemMonitor::updateUptime()
{
    ULONGLONG ms = GetTickCount64();
    ULONGLONG secs = ms / 1000;
    ULONGLONG mins = secs / 60;
    ULONGLONG hours = mins / 60;
    ULONGLONG days = hours / 24;
    
    secs %= 60;
    mins %= 60;
    hours %= 24;

    QString currentUptime = QString("%1d %2h %3m %4s")
            .arg(days)
            .arg(hours, 2, 10, QChar('0'))
            .arg(mins, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'));

    if (m_uptime != currentUptime) {
        m_uptime = currentUptime;
        emit uptimeChanged();
    }
}

void SystemMonitor::queryCpuModel()
{
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        wchar_t m_cpuName[256];
        DWORD size = sizeof(m_cpuName);
        if (RegQueryValueExW(hKey, L"ProcessorNameString", nullptr, nullptr, (LPBYTE)m_cpuName, &size) == ERROR_SUCCESS) {
            m_cpuModel = QString::fromWCharArray(m_cpuName).trimmed();
        }
        RegCloseKey(hKey);
    }
}

void SystemMonitor::queryGpuModel()
{
    if (m_nvmlInitialized && m_nvmlDevice && pfn_nvmlDeviceGetName) {
        char name[128];
        if (pfn_nvmlDeviceGetName(m_nvmlDevice, name, sizeof(name)) == NVML_SUCCESS) {
            m_gpuModel = QString::fromUtf8(name);
            return;
        }
    }
    m_gpuModel = "Standard GPU Renderer";
}

void SystemMonitor::queryMotherboardAndBios()
{
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        wchar_t manufacturer[256] = {0};
        wchar_t product[256] = {0};
        wchar_t biosVer[256] = {0};
        DWORD size = sizeof(manufacturer);
        
        if (RegQueryValueExW(hKey, L"BaseBoardManufacturer", nullptr, nullptr, (LPBYTE)manufacturer, &size) == ERROR_SUCCESS) {
            m_motherboardModel = QString::fromWCharArray(manufacturer).trimmed();
        }
        size = sizeof(product);
        if (RegQueryValueExW(hKey, L"BaseBoardProduct", nullptr, nullptr, (LPBYTE)product, &size) == ERROR_SUCCESS) {
            m_motherboardModel += " " + QString::fromWCharArray(product).trimmed();
        }
        size = sizeof(biosVer);
        if (RegQueryValueExW(hKey, L"BIOSVersion", nullptr, nullptr, (LPBYTE)biosVer, &size) == ERROR_SUCCESS) {
            m_biosVersion = QString::fromWCharArray(biosVer).trimmed();
        }
        RegCloseKey(hKey);
    }
}

void SystemMonitor::queryRamSpeed()
{
    m_ramSpeed = 3200; // Standard fallback speed in MHz

    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    bool coInit = SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE;

    if (coInit) {
        IWbemLocator *pLoc = NULL;
        hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *)&pLoc);
        if (SUCCEEDED(hr)) {
            IWbemServices *pSvc = NULL;
            BSTR bstrNamespace = SysAllocString(L"ROOT\\CIMV2");
            hr = pLoc->ConnectServer(bstrNamespace, NULL, NULL, 0, NULL, 0, 0, &pSvc);
            SysFreeString(bstrNamespace);
            
            if (SUCCEEDED(hr)) {
                CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
                IEnumWbemClassObject* pEnumerator = NULL;
                BSTR bstrWQL = SysAllocString(L"WQL");
                BSTR bstrQuery = SysAllocString(L"SELECT Speed FROM Win32_PhysicalMemory");
                hr = pSvc->ExecQuery(bstrWQL, bstrQuery, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
                SysFreeString(bstrWQL);
                SysFreeString(bstrQuery);
                
                if (SUCCEEDED(hr)) {
                    IWbemClassObject *pclsObj = NULL;
                    ULONG uReturn = 0;
                    if (pEnumerator) {
                        hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
                        if (uReturn != 0) {
                            VARIANT vtProp;
                            hr = pclsObj->Get(L"Speed", 0, &vtProp, 0, 0);
                            if (SUCCEEDED(hr) && (vtProp.vt == VT_I4 || vtProp.vt == VT_UI4)) {
                                m_ramSpeed = vtProp.uintVal;
                            }
                            VariantClear(&vtProp);
                            pclsObj->Release();
                        }
                    }
                    pEnumerator->Release();
                }
                pSvc->Release();
            }
            pLoc->Release();
        }
        CoUninitialize();
    }
}
