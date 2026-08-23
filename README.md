# RathonWare - Hardware Diagnostics, AI/GPU Telemetry & Task Manager Suite

A high-performance system monitoring dashboard and superpower developer utility for Windows, built with **C++20** and **Qt 6 (QML / Qt Quick)**.

---

## ⚡ Flagship Features

### 🛠️ Pillar 1: Developer Life-Saver Features
*   **Port-to-Process Killer (`:port` search):** Type `:3000`, `:8080`, or `:5432` into the search bar or Command Palette to instantly find the exact listening process with a 1-click **⚡ Kill** button (resolves `EADDRINUSE` in seconds via Win32 `GetExtendedTcpTable` / `GetExtendedUdpTable`).
*   **"Unlock File / Who is Locking This?":** Drag & drop any locked file/folder or enter its path to inspect holding processes and terminate them with 1-click using the Windows Restart Manager API (`rstrtmgr.lib`).
*   **Process Suspend & Resume (`NtSuspendProcess` / `NtResumeProcess`):** Freeze runaway 100% CPU processes without losing memory or state.
*   **"Kill Entire Process Tree":** Recursively terminate spawned child processes (Node workers, Python runners, build tools) cleanly.
*   **Global Ctrl + K Command Palette:** Spotlight-style quick launcher for instant port killing, file unlocking, and process actions.

### 🤖 Pillar 2: Modern AI & GPU Telemetry Tab (NVML Superpowers)
*   **Per-Process VRAM & CUDA Breakdown:** Dedicated tab utilizing NVML to display dedicated VRAM per process, compute vs graphics tags (Ollama, PyTorch, ComfyUI, 3D engines), and VRAM share percentage.
*   **GPU Throttle Reason Indicator:** Real-time hardware status tag showing downclock reasons (e.g. *Full Boost / None*, *Power Cap Limit*, *Thermal Limit*, *Hardware Slowdown*).
*   **Real-time Power & Clocks:** Live wattage draw (e.g., `280W / 450W`), GPU core temperature, graphics and memory clocks.
*   **VRAM Memory Leak Detector:** Automated heuristic that tracks time-series VRAM allocation trends and alerts with ⚠️ *VRAM Leak Suspected* badges.

### 🎛️ Pillar 3: CPU Topology & Intel P-Core / E-Core Manager
*   **1-Click "Pin to E-Cores" (Efficiency Mode on Steroids):** Automatically scan background apps (Discord, Chrome, Slack, Spotify, Torrents, Node, Python) and lock them to E-Cores with Windows EcoQoS, freeing 100% of P-Cores for gaming and heavy foreground compute.
*   **Per-Core CPU Heatmap Grid:** Real-time visual grid displaying every logical core with load %, P-Core/E-Core identification, and dynamic heat color gradient using low-overhead NT kernel `NtQuerySystemInformation`.

---

## 🛠️ Technical Stack
*   **UI Frontend:** Qt Quick / QML 6.x (hardware-accelerated, responsive layout, dynamic Canvas telemetry charts)
*   **System & Kernel Queries (C++20 / Win32):**
    *   `GetExtendedTcpTable` / `GetExtendedUdpTable` (IPv4 & IPv6 Port-to-Process correlation)
    *   `RestartManager` (`RmStartSession`, `RmRegisterResources`, `RmGetList`, `RmEndSession`)
    *   `NtQuerySystemInformation(SystemProcessorPerformanceInformation)` (Per-core CPU telemetry)
    *   `NtSuspendProcess` / `NtResumeProcess` (`ntdll.dll`)
    *   `GetLogicalProcessorInformationEx` (CPU P-Core / E-Core topology discovery)
    *   `SetProcessAffinityMask` & `SetProcessInformation(ProcessPowerThrottling)` (EcoQoS)
    *   `GetSystemTimes` & `GlobalMemoryStatusEx`
    *   `CreateToolhelp32Snapshot` & `GetProcessMemoryInfo`
    *   `LoadLibrary` / `GetProcAddress` (NVIDIA NVML dynamic bindings)

---

## 🚀 How to Build and Run

### Prerequisites
*   Windows 10 or Windows 11 (64-bit)
*   **Qt 6.x** with MinGW 64-bit (or MSVC)
*   **CMake 3.16+** & **Ninja**

### Command Line Build
```powershell
# Set Qt toolchain paths
$env:PATH = "C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\CMake_64\bin;C:\Qt\Tools\Ninja;C:\Qt\6.11.1\mingw_64\bin;" + $env:PATH

# Configure and compile in Release mode
cmake -B build -G "Ninja" -DCMAKE_PREFIX_PATH="C:\Qt\6.11.1\mingw_64" -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Launch RathonWare
./build/RathonWare.exe
```
