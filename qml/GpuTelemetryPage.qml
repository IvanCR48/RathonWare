import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Item {
    id: root
    anchors.fill: parent

    ColumnLayout {
        anchors.fill: parent
        spacing: 14

        // 1. GPU Hardware & Telemetry Header Banner
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 105
            color: window.colorCard
            radius: 6
            border.color: window.colorCardBorder
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 8

                // Title row + Throttle Status Tag
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    Text {
                        text: gpuMonitor.hasNvidiaGpu ? gpuMonitor.gpuName : systemMonitor.gpuModel
                        font.family: "Tahoma"
                        font.pixelSize: 15
                        font.bold: true
                        color: window.colorGpu
                    }

                    // Throttle reason pill
                    Rectangle {
                        Layout.preferredHeight: 22
                        implicitWidth: throttleText.implicitWidth + 16
                        radius: 4
                        color: gpuMonitor.throttleStatusLevel === "normal" ? "#ecfdf5" :
                               gpuMonitor.throttleStatusLevel === "warning" ? "#fffbeb" : "#fef2f2"
                        border.color: gpuMonitor.throttleStatusLevel === "normal" ? "#10b981" :
                                      gpuMonitor.throttleStatusLevel === "warning" ? "#f59e0b" : "#ef4444"
                        border.width: 1

                        RowLayout {
                            anchors.centerIn: parent
                            spacing: 4
                            Text {
                                text: gpuMonitor.throttleStatusLevel === "normal" ? "⚡" :
                                      gpuMonitor.throttleStatusLevel === "warning" ? "⚠️" : "🔥"
                                font.pixelSize: 10
                            }
                            Text {
                                id: throttleText
                                text: "Throttling: " + gpuMonitor.throttleReason
                                font.family: "Tahoma"
                                font.pixelSize: 10
                                font.bold: true
                                color: gpuMonitor.throttleStatusLevel === "normal" ? "#047857" :
                                       gpuMonitor.throttleStatusLevel === "warning" ? "#b45309" : "#b91c1c"
                            }
                        }
                    }

                    Item { Layout.fillWidth: true }

                    // GPU Load Badge
                    Rectangle {
                        Layout.preferredHeight: 24
                        implicitWidth: loadText.implicitWidth + 16
                        color: "#008080"
                        radius: 3
                        Text {
                            id: loadText
                            anchors.centerIn: parent
                            text: "GPU LOAD: " + (gpuMonitor.hasNvidiaGpu ? gpuMonitor.gpuUsage.toFixed(0) : systemMonitor.gpuUsage.toFixed(0)) + "%"
                            font.family: "Consolas"
                            font.pixelSize: 11
                            font.bold: true
                            color: "#ffffff"
                        }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: window.colorCardBorder }

                // Telemetry metrics row
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 25

                    // Temp
                    RowLayout {
                        spacing: 6
                        Text { text: "🌡️ Core Temp:"; font.family: "Tahoma"; font.pixelSize: 11; color: window.colorTextSec }
                        Text {
                            text: (gpuMonitor.hasNvidiaGpu ? gpuMonitor.gpuTemp.toFixed(0) : systemMonitor.gpuTemp.toFixed(0)) + " °C"
                            font.family: "Consolas"; font.pixelSize: 12; font.bold: true
                            color: (gpuMonitor.gpuTemp > 80) ? "#d32f2f" : window.colorTextMain
                        }
                    }

                    // Power Draw
                    RowLayout {
                        spacing: 6
                        Text { text: "⚡ Power Draw:"; font.family: "Tahoma"; font.pixelSize: 11; color: window.colorTextSec }
                        Text {
                            text: gpuMonitor.powerUsageW > 0 ? gpuMonitor.powerUsageW.toFixed(1) + " W / " + gpuMonitor.powerLimitW.toFixed(0) + " W" : "N/A"
                            font.family: "Consolas"; font.pixelSize: 12; font.bold: true
                            color: window.colorTextMain
                        }
                    }

                    // Graphics Clock
                    RowLayout {
                        spacing: 6
                        Text { text: "📊 Core Clock:"; font.family: "Tahoma"; font.pixelSize: 11; color: window.colorTextSec }
                        Text {
                            text: gpuMonitor.graphicsClockMHz > 0 ? gpuMonitor.graphicsClockMHz + " MHz" : "N/A"
                            font.family: "Consolas"; font.pixelSize: 12; font.bold: true
                            color: window.colorTextMain
                        }
                    }

                    // Memory Clock
                    RowLayout {
                        spacing: 6
                        Text { text: "💾 Memory Clock:"; font.family: "Tahoma"; font.pixelSize: 11; color: window.colorTextSec }
                        Text {
                            text: gpuMonitor.memoryClockMHz > 0 ? gpuMonitor.memoryClockMHz + " MHz" : "N/A"
                            font.family: "Consolas"; font.pixelSize: 12; font.bold: true
                            color: window.colorTextMain
                        }
                    }
                }
            }
        }

        // 2. VRAM Allocation Breakdown Bar
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 70
            color: window.colorCard
            radius: 6
            border.color: window.colorCardBorder
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: "DEDICATED VRAM ALLOCATION"
                        font.family: "Tahoma"
                        font.pixelSize: 11
                        font.bold: true
                        color: "#0a246a"
                    }
                    Item { Layout.fillWidth: true }
                    Text {
                        text: (gpuMonitor.hasNvidiaGpu ? gpuMonitor.vramUsedGB.toFixed(2) : systemMonitor.gpuVramUsed.toFixed(2)) + " GB / " + 
                              (gpuMonitor.hasNvidiaGpu ? gpuMonitor.vramTotalGB.toFixed(1) : systemMonitor.gpuVramTotal.toFixed(1)) + " GB (" +
                              (gpuMonitor.hasNvidiaGpu ? gpuMonitor.vramUsagePercent.toFixed(1) : systemMonitor.gpuVramUsage.toFixed(1)) + "%)"
                        font.family: "Consolas"
                        font.pixelSize: 11
                        font.bold: true
                        color: window.colorTextMain
                    }
                }

                // Progress Bar Container
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 18
                    color: "#e2ebf5"
                    radius: 3
                    border.color: "#92a6b9"

                    Rectangle {
                        height: parent.height
                        width: parent.width * (Math.min(100.0, Math.max(0.0, (gpuMonitor.hasNvidiaGpu ? gpuMonitor.vramUsagePercent : systemMonitor.gpuVramUsage))) / 100.0)
                        radius: 3
                        gradient: Gradient {
                            orientation: Gradient.Horizontal
                            GradientStop { position: 0.0; color: "#008080" }
                            GradientStop { position: 0.7; color: "#0a246a" }
                            GradientStop { position: 1.0; color: (gpuMonitor.vramUsagePercent > 90) ? "#d32f2f" : "#6366f1" }
                        }
                    }
                }
            }
        }

        // 3. Running CUDA & GPU Processes Table
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#ffffff"
            radius: 6
            border.color: window.colorCardBorder
            border.width: 1
            clip: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // Table Header
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 32
                    color: "#d2dce8"
                    border.color: window.colorCardBorder
                    border.width: 1

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12

                        Text {
                            width: parent.width * 0.38
                            height: parent.height
                            verticalAlignment: Text.AlignVCenter
                            text: "Process / AI Model Name"
                            font.family: "Tahoma"; font.pixelSize: 11; font.bold: true; color: window.colorTextMain
                        }
                        Text {
                            width: parent.width * 0.10
                            height: parent.height
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                            text: "PID"
                            font.family: "Tahoma"; font.pixelSize: 11; font.bold: true; color: window.colorTextMain
                        }
                        Text {
                            width: parent.width * 0.18
                            height: parent.height
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                            text: "Engine / Category"
                            font.family: "Tahoma"; font.pixelSize: 11; font.bold: true; color: window.colorTextMain
                        }
                        Text {
                            width: parent.width * 0.14
                            height: parent.height
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignRight
                            text: "VRAM Used"
                            font.family: "Tahoma"; font.pixelSize: 11; font.bold: true; color: window.colorTextMain
                        }
                        Text {
                            width: parent.width * 0.20
                            height: parent.height
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                            text: "VRAM Share & Actions"
                            font.family: "Tahoma"; font.pixelSize: 11; font.bold: true; color: window.colorTextMain
                        }
                    }
                }

                // Process ListView
                ListView {
                    id: gpuProcList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: gpuProcessModel
                    clip: true
                    spacing: 2
                    boundsBehavior: Flickable.StopAtBounds

                    ScrollBar.vertical: ScrollBar {
                        active: true
                        policy: ScrollBar.AsNeeded
                    }

                    delegate: Rectangle {
                        width: gpuProcList.width
                        height: 44
                        color: mouseArea.containsMouse ? "#f0f6ff" : (index % 2 === 0 ? "#ffffff" : "#fcfdfe")
                        border.color: leakSuspected ? "#fca5a5" : "transparent"
                        border.width: leakSuspected ? 1 : 0

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12

                            // Name + Leak Warning
                            RowLayout {
                                width: parent.width * 0.38
                                height: parent.height
                                spacing: 6

                                Text {
                                    text: name
                                    font.family: "Tahoma"
                                    font.pixelSize: 12
                                    font.bold: isCompute
                                    color: isCompute ? "#0a246a" : "#000000"
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                // VRAM Leak Badge
                                Rectangle {
                                    visible: leakSuspected
                                    Layout.preferredHeight: 18
                                    implicitWidth: leakLabel.implicitWidth + 8
                                    color: "#fee2e2"
                                    radius: 2
                                    border.color: "#ef4444"
                                    Text {
                                        id: leakLabel
                                        anchors.centerIn: parent
                                        text: "⚠️ VRAM Leak (+" + growthRateMB.toFixed(0) + "MB/m)"
                                        font.family: "Consolas"
                                        font.pixelSize: 8
                                        font.bold: true
                                        color: "#b91c1c"
                                    }
                                }
                            }

                            // PID
                            Text {
                                width: parent.width * 0.10
                                height: parent.height
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignHCenter
                                text: pid.toString()
                                font.family: "Consolas"
                                font.pixelSize: 11
                                color: "#4d5b6e"
                            }

                            // Category Badge
                            Item {
                                width: parent.width * 0.18
                                height: parent.height
                                Rectangle {
                                    anchors.centerIn: parent
                                    width: parent.width * 0.90
                                    height: 20
                                    radius: 3
                                    color: isCompute ? "#e0e7ff" : "#ecfdf5"
                                    border.color: isCompute ? "#818cf8" : "#6ee7b7"
                                    Text {
                                        anchors.centerIn: parent
                                        text: category
                                        font.family: "Tahoma"
                                        font.pixelSize: 9
                                        font.bold: true
                                        color: isCompute ? "#3730a3" : "#065f46"
                                    }
                                }
                            }

                            // VRAM Used
                            Text {
                                width: parent.width * 0.14
                                height: parent.height
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignRight
                                text: vramGB >= 1.0 ? vramGB.toFixed(2) + " GB" : vramMB.toFixed(1) + " MB"
                                font.family: "Consolas"
                                font.pixelSize: 12
                                font.bold: true
                                color: "#000000"
                            }

                            // Share Bar & Kill Button
                            RowLayout {
                                width: parent.width * 0.20
                                height: parent.height
                                spacing: 8

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 12
                                    color: "#e2ebf5"
                                    radius: 2

                                    Rectangle {
                                        height: parent.height
                                        width: parent.width * (Math.min(100.0, vramPercent) / 100.0)
                                        radius: 2
                                        color: isCompute ? "#4f46e5" : "#008080"
                                    }
                                }

                                Button {
                                    Layout.preferredWidth: 48
                                    Layout.preferredHeight: 24
                                    background: Rectangle {
                                        color: parent.hovered ? "#b91c1c" : "#dc2626"
                                        radius: 3
                                    }
                                    contentItem: Text {
                                        text: "Kill"
                                        font.family: "Tahoma"; font.pixelSize: 10; font.bold: true; color: "#ffffff"
                                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                    }
                                    onClicked: {
                                        gpuProcessModel.killProcess(pid);
                                        gpuMonitor.refresh();
                                    }
                                }
                            }
                        }

                        MouseArea {
                            id: mouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                        }
                    }
                }

                // Fallback message if no GPU processes found
                Rectangle {
                    visible: gpuProcList.count === 0
                    Layout.fillWidth: true
                    Layout.preferredHeight: 100
                    color: "transparent"

                    Column {
                        anchors.centerIn: parent
                        spacing: 6
                        Text {
                            text: gpuMonitor.hasNvidiaGpu ? "No active CUDA or DirectX processes detected on this GPU" : "NVML is unavailable (running on integrated graphics or non-NVIDIA device)"
                            font.family: "Tahoma"
                            font.pixelSize: 12
                            color: "#6b7280"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                }
            }
        }
    }
}
