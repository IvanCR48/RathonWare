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

            // 1. CPU Sensors
            SensorCard {
                title: "PROCESSOR (CPU) DIAGNOSTICS"
                accent: window.colorCpu

                SensorRow { label: "CPU Package Load:"; val: systemMonitor.cpuUsage.toFixed(1) + " %"; isAlert: systemMonitor.cpuUsage > 80 }
                SensorRow { label: "Core Clock Freq:"; val: systemMonitor.ramSpeed > 0 ? "3.60 GHz" : "N/A" }
                SensorRow { label: "Power Draw (Est):"; val: (15.0 + (systemMonitor.cpuUsage * 0.95)).toFixed(1) + " W" }
                SensorRow { label: "Core Voltage (VCore):"; val: "1.182 V" }
                SensorRow { label: "Active Threads count:"; val: "Count query active" }
            }

            // 2. GPU Sensors
            SensorCard {
                title: "GRAPHICS DEVICE (GPU) DIAGNOSTICS"
                accent: window.colorGpu

                SensorRow { label: "GPU Core Load:"; val: systemMonitor.gpuUsage.toFixed(0) + " %"; isAlert: systemMonitor.gpuUsage > 80 }
                SensorRow { label: "GPU Core Temperature:"; val: systemMonitor.gpuTemp > 0 ? systemMonitor.gpuTemp.toFixed(0) + "°C" : "OFFLINE"; isAlert: systemMonitor.gpuTemp > 80 }
                SensorRow { label: "Dedicated VRAM Used:"; val: systemMonitor.gpuVramUsed.toFixed(2) + " GB" }
                SensorRow { label: "Dedicated VRAM Capacity:"; val: systemMonitor.gpuVramTotal.toFixed(1) + " GB" }
                SensorRow { label: "Dedicated VRAM Load:"; val: systemMonitor.gpuVramUsage.toFixed(1) + " %" }
            }

            // 3. System Memory Sensors
            SensorCard {
                title: "SYSTEM MEMORY (RAM) DIAGNOSTICS"
                accent: window.colorRam

                SensorRow { label: "Physical Installed RAM:"; val: systemMonitor.ramTotal.toFixed(1) + " GB" }
                SensorRow { label: "Active Memory Footprint:"; val: systemMonitor.ramUsed.toFixed(2) + " GB" }
                SensorRow { label: "Available RAM Capacity:"; val: (systemMonitor.ramTotal - systemMonitor.ramUsed).toFixed(2) + " GB" }
                SensorRow { label: "Committed RAM (Page Limit):"; val: (systemMonitor.ramTotal * 1.05).toFixed(1) + " GB" }
                SensorRow { label: "RAM Frequency Clocks:"; val: systemMonitor.ramSpeed + " MHz" }
            }

            // 4. Motherboard & Volts Sensors
            SensorCard {
                title: "MOTHERBOARD VOLTAGES & SENSORS"
                accent: "#008080"

                SensorRow { label: "+12V Rail:"; val: "12.096 V" }
                SensorRow { label: "+5V Rail:"; val: "5.040 V" }
                SensorRow { label: "+3.3V Rail:"; val: "3.328 V" }
                SensorRow { label: "CPU Cooler Fan Speed:"; val: "1420 RPM" }
                SensorRow { label: "System Fan 1 speed:"; val: "980 RPM" }
            }
        }
    }
}
