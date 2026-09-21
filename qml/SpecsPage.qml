import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Item {
    id: root
    anchors.fill: parent

    // Reusable Component for Spec Info Rows
    component SpecRow : RowLayout {
        id: specRow
        property string label: ""
        property string value: ""
        Layout.fillWidth: true
        spacing: 10

        Text {
            text: specRow.label
            font.family: "Tahoma"
            font.pixelSize: 12
            color: window.colorTextSec
            Layout.preferredWidth: 160
        }

        Text {
            text: specRow.value
            font.family: "Tahoma"
            font.pixelSize: 12
            font.bold: true
            color: window.colorTextMain
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }
    }

    // Scrollable View for Specifications
    ScrollView {
        anchors.fill: parent
        contentWidth: parent.width
        contentHeight: contentColumn.implicitHeight + 20
        clip: true

        ColumnLayout {
            id: contentColumn
            width: parent.width - 20
            spacing: 15

            // 1. Processor (CPU) Card
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: cpuCol.implicitHeight + 30
                color: window.colorCard
                radius: 6
                border.color: window.colorCardBorder
                border.width: 1

                ColumnLayout {
                    id: cpuCol
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: "Processor (CPU)"
                            font.family: "Tahoma"
                            font.pixelSize: 14
                            font.bold: true
                            color: window.colorCpu
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: window.colorCardBorder
                    }

                    SpecRow { label: "Model Name:"; value: systemMonitor.cpuModel }
                    SpecRow { label: "Nominal Base Clock:"; value: systemMonitor.cpuBaseClockMHz > 0 ? (systemMonitor.cpuBaseClockMHz / 1000.0).toFixed(2) + " GHz" : "N/A" }
                    SpecRow { label: "Architecture:"; value: "x86_64 (64-bit Desktop Platform)" }
                    SpecRow { label: "Sensor Hook:"; value: "Active - Querying Windows GetSystemTimes & NT Performance APIs" }
                }
            }

            // 2. Graphics (GPU) Card
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: gpuCol.implicitHeight + 30
                color: window.colorCard
                radius: 6
                border.color: window.colorCardBorder
                border.width: 1

                ColumnLayout {
                    id: gpuCol
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: "Graphics Device (GPU)"
                            font.family: "Tahoma"
                            font.pixelSize: 14
                            font.bold: true
                            color: window.colorGpu
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: window.colorCardBorder
                    }

                    SpecRow { label: "Model Name:"; value: systemMonitor.gpuModel }
                    SpecRow { label: "Dedicated VRAM:"; value: systemMonitor.gpuVramTotal > 0 ? systemMonitor.gpuVramTotal.toFixed(1) + " GB Dedicated Video Memory" : "Shared/Integrated Memory" }
                    SpecRow { label: "Interface Driver:"; value: systemMonitor.gpuBackend }
                }
            }

            // 3. Memory & System Card
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: sysCol.implicitHeight + 30
                color: window.colorCard
                radius: 6
                border.color: window.colorCardBorder
                border.width: 1

                ColumnLayout {
                    id: sysCol
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: "System Memory & Platform"
                            font.family: "Tahoma"
                            font.pixelSize: 14
                            font.bold: true
                            color: window.colorRam
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: window.colorCardBorder
                    }

                    SpecRow { label: "Physical RAM Capacity:"; value: systemMonitor.ramTotal.toFixed(1) + " GB Installed System RAM" }
                    SpecRow { label: "RAM Clock Speed:"; value: systemMonitor.ramSpeed > 0 ? systemMonitor.ramSpeed + " MHz" : "N/A" }
                    SpecRow { label: "Motherboard Model:"; value: systemMonitor.motherboardModel }
                    SpecRow { label: "BIOS Firmware:"; value: systemMonitor.biosVersion }
                    SpecRow { label: "OS Environment:"; value: "Microsoft Windows Desktop Platform" }
                    SpecRow { label: "Platform Kernel:"; value: "Windows NT WDM kernel driver" }
                }
            }

            // 4. Fixed Storage & Volumes Card
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: storageCol.implicitHeight + 30
                color: window.colorCard
                radius: 6
                border.color: window.colorCardBorder
                border.width: 1

                ColumnLayout {
                    id: storageCol
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: "Storage & Fixed Volumes"
                            font.family: "Tahoma"
                            font.pixelSize: 14
                            font.bold: true
                            color: "#008080"
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: window.colorCardBorder
                    }

                    SpecRow { label: "Total Storage Capacity:"; value: systemMonitor.diskTotalCapacityGB > 0 ? systemMonitor.diskTotalCapacityGB.toFixed(1) + " GB (" + systemMonitor.diskTotalFreeGB.toFixed(1) + " GB Available)" : "Detecting Storage..." }
                    SpecRow { label: "Mounted Volumes:"; value: systemMonitor.diskDriveSummary !== "" ? systemMonitor.diskDriveSummary : "Scanning local drives..." }
                    SpecRow { label: "Performance Counters:"; value: "Active - Querying Windows PhysicalDisk Read/Write counters" }
                }
            }
        }
    }
}
