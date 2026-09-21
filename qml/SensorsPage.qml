import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Item {
    id: root
    anchors.fill: parent

    // Reusable custom card for sensor listings
    component SensorCard : Rectangle {
        id: card
        property string title: ""
        property color accent: "#0a246a"
        default property alias content: innerCol.data

        Layout.fillWidth: true
        Layout.preferredHeight: innerCol.implicitHeight + 30
        color: window.colorCard
        radius: 4
        border.color: window.colorCardBorder

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8

            Text {
                text: card.title
                color: card.accent
                font.family: "Tahoma"
                font.pixelSize: 11
                font.bold: true
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: window.colorCardBorder
            }

            ColumnLayout {
                id: innerCol
                Layout.fillWidth: true
                spacing: 4
            }
        }
    }

    // Reusable sensor line entry
    component SensorRow : RowLayout {
        id: row
        property string label: ""
        property string val: ""
        property bool isAlert: false
        Layout.fillWidth: true

        Text {
            text: row.label
            font.family: "Tahoma"; font.pixelSize: 11
            color: window.colorTextSec
            Layout.fillWidth: true
        }
        Text {
            text: row.val
            font.family: "Consolas"; font.pixelSize: 11; font.bold: true
            color: row.isAlert ? "#d32f2f" : window.colorTextMain
        }
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: parent.width
        contentHeight: gridLayout.implicitHeight + 20
        clip: true

        GridLayout {
            id: gridLayout
            width: parent.width - 15
            columns: 2
            rowSpacing: 15
            columnSpacing: 15

            // 1. CPU Diagnostics
            SensorCard {
                title: "PROCESSOR (CPU) DIAGNOSTICS"
                accent: window.colorCpu

                SensorRow { label: "CPU Package Load:"; val: systemMonitor.cpuUsage.toFixed(1) + " %"; isAlert: systemMonitor.cpuUsage > 80 }
                SensorRow { label: "Base Clock Frequency:"; val: systemMonitor.cpuBaseClockMHz > 0 ? (systemMonitor.cpuBaseClockMHz / 1000.0).toFixed(2) + " GHz" : "N/A" }
                SensorRow { label: "Active System Threads:"; val: systemMonitor.threadCount > 0 ? systemMonitor.threadCount.toLocaleString() : "Querying..." }
                SensorRow { label: "Active System Processes:"; val: systemMonitor.processCount > 0 ? systemMonitor.processCount.toLocaleString() : "Querying..." }
                SensorRow { label: "Open System Handles:"; val: systemMonitor.handleCount > 0 ? systemMonitor.handleCount.toLocaleString() : "Querying..." }
            }

            // 2. GPU Diagnostics
            SensorCard {
                title: "GRAPHICS DEVICE (GPU) DIAGNOSTICS"
                accent: window.colorGpu

                SensorRow { label: "Driver Interface:"; val: systemMonitor.gpuBackend }
                SensorRow { label: "GPU Core Load:"; val: systemMonitor.gpuUsage.toFixed(0) + " %"; isAlert: systemMonitor.gpuUsage > 80 }
                SensorRow { label: "GPU Core Temperature:"; val: systemMonitor.gpuTemp > 0 ? systemMonitor.gpuTemp.toFixed(0) + "°C" : "N/A (DXGI Mode)"; isAlert: systemMonitor.gpuTemp > 80 }
                SensorRow { label: "Dedicated VRAM Used:"; val: systemMonitor.gpuVramUsed.toFixed(2) + " GB" }
                SensorRow { label: "Dedicated VRAM Capacity:"; val: systemMonitor.gpuVramTotal.toFixed(1) + " GB" }
                SensorRow { label: "Dedicated VRAM Load:"; val: systemMonitor.gpuVramUsage.toFixed(1) + " %" }
                SensorRow { label: "Power Draw:"; val: gpuMonitor.powerUsageW > 0 ? gpuMonitor.powerUsageW.toFixed(1) + " W" : "N/A (Requires NVML)" }
            }

            // 3. System Memory Sensors
            SensorCard {
                title: "PHYSICAL MEMORY (RAM) DIAGNOSTICS"
                accent: window.colorRam

                SensorRow { label: "Physical Installed RAM:"; val: systemMonitor.ramTotal.toFixed(1) + " GB" }
                SensorRow { label: "Active Memory Footprint:"; val: systemMonitor.ramUsed.toFixed(2) + " GB" }
                SensorRow { label: "Available RAM Capacity:"; val: (systemMonitor.ramTotal - systemMonitor.ramUsed).toFixed(2) + " GB" }
                SensorRow { label: "Physical RAM Load:"; val: systemMonitor.ramUsage.toFixed(1) + " %"; isAlert: systemMonitor.ramUsage > 85 }
                SensorRow { label: "RAM Frequency Clocks:"; val: systemMonitor.ramSpeed > 0 ? systemMonitor.ramSpeed + " MHz" : "N/A" }
            }

            // 4. Kernel & Memory Commit Diagnostics (Verified Win32 Performance Data)
            SensorCard {
                title: "SYSTEM KERNEL & MEMORY COMMIT DIAGNOSTICS"
                accent: "#008080"

                SensorRow { label: "Memory Commit Charge:"; val: systemMonitor.commitTotalGB.toFixed(2) + " / " + systemMonitor.commitLimitGB.toFixed(2) + " GB" }
                SensorRow { label: "Commit Charge Load:"; val: systemMonitor.commitUsagePercent.toFixed(1) + " %"; isAlert: systemMonitor.commitUsagePercent > 85 }
                SensorRow { label: "Commit Peak Reached:"; val: systemMonitor.commitPeakGB.toFixed(2) + " GB" }
                SensorRow { label: "Kernel Paged Pool:"; val: systemMonitor.kernelPagedMB.toFixed(0) + " MB" }
                SensorRow { label: "Kernel Non-Paged Pool:"; val: systemMonitor.kernelNonpagedMB.toFixed(0) + " MB"; isAlert: systemMonitor.kernelNonpagedMB > 1500 }
                SensorRow { label: "System Uptime:"; val: systemMonitor.uptime }
            }
        }
    }
}
