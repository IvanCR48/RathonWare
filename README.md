# RathonWare - PC Diagnostics & Task Manager Suite

A high-performance system monitoring dashboard and process manager for Windows, built in **C++20** and **Qt 6 (QML / Qt Quick)**.

## Screenshots & Features
*   **Real-time Dashboard:** Circular load dials and scrolling telemetry charts for **CPU Load**, **GPU Load & Temperature**, and **System RAM**.
*   **Interactive Task Manager:** Lists all active Windows processes sorted by active CPU usage. Includes real-time memory stats, searching, and a clean "End Task" termination button.
*   **Hardware Spec Sheets:** Displays detailed hardware profiles using direct OS query bindings.
*   **Dynamic Graphics Interfacing:** Auto-detects NVIDIA cards by dynamically loading the driver's kernel interface (`nvml.dll`) to display temperatures and dedicated VRAM loads. Falls back gracefully for non-Nvidia GPUs.

---

## Technical Stack
*   **UI Frontend:** Qt Quick / QML 6.x (hardware-accelerated, animated layout, HTML5-like Canvas charts)
*   **System Queries (C++):**
    *   `GetSystemTimes` (CPU load delta calculations)
    *   `GlobalMemoryStatusEx` (Physical RAM)
    *   `CreateToolhelp32Snapshot` (Active processes enumeration)
    *   `GetProcessMemoryInfo` (Individual process RAM allocations)
    *   `TerminateProcess` (Task termination)
    *   `LoadLibrary` / `GetProcAddress` (Dynamic linkage of Nvidia NVML API)

---

## How to Build and Run in Qt Creator

Follow these steps to open, configure, and launch the application:

### Step 1: Open the Project
1. Launch **Qt Creator**.
2. Click on **File** in the top menu, then select **Open File or Project...**
3. Navigate to: `c:\xampp\htdocs\RathonWare`
4. Select the `CMakeLists.txt` file and click **Open**.

### Step 2: Configure the Kit
1. Qt Creator will open the **Configure Project** tab.
2. Under the list of available kits, select the **Desktop Qt 6.x.x MinGW 64-bit** kit.
3. Click the **Configure Project** button.

### Step 3: Compile & Launch
1. On the bottom-left sidebar of Qt Creator, click the green **Run** arrow (or press `Ctrl + R` on your keyboard).
2. The project will compile (takes a few seconds) and launch the styled window!
