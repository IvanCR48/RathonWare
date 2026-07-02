import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Item {
    id: root
    anchors.fill: parent

    // Layout splits: Grid on left (70%), Top Consumers on right (30%)
    RowLayout {
        anchors.fill: parent
        spacing: 15

        // Left Panel: Metric Cards Grid
        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            columns: 2
            rowSpacing: 12
            columnSpacing: 12

            // 1. CPU Card
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: window.colorCard
                radius: 4
                border.color: window.colorCardBorder

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 4

                    Text {
                        text: "CPU MONITOR"
                        color: window.colorCpu
                        font.family: "Tahoma"
                        font.pixelSize: 11
                        font.bold: true
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Item {
                            Layout.preferredWidth: 60
                            Layout.preferredHeight: 60
                            Canvas {
                                id: cpuCanvas
                                anchors.fill: parent
                                onPaint: {
                                    var ctx = getContext("2d"); ctx.reset();
                                    ctx.beginPath(); ctx.arc(30, 30, 24, 0, 2 * Math.PI);
                                    ctx.lineWidth = 4; ctx.strokeStyle = "#c3cfdd"; ctx.stroke();
                                    ctx.beginPath(); ctx.arc(30, 30, 24, -Math.PI/2, -Math.PI/2 + (2*Math.PI*(systemMonitor.cpuUsage/100)));
                                    ctx.lineWidth = 4; ctx.strokeStyle = window.colorCpu; ctx.stroke();
                                }
                                Connections { target: systemMonitor; function onCpuUsageChanged() { cpuCanvas.requestPaint(); } }
                            }
                            Text {
                                anchors.centerIn: parent
                                text: systemMonitor.cpuUsage.toFixed(0) + "%"
                                font.family: "Tahoma"; font.pixelSize: 10; font.bold: true
                            }
                        }
                        ColumnLayout {
                            Text { text: systemMonitor.cpuModel; font.family: "Tahoma"; font.pixelSize: 10; font.bold: true; elide: Text.ElideRight; Layout.fillWidth: true }
                            Text { text: "Frequency: Dynamic Query"; font.family: "Tahoma"; font.pixelSize: 9; color: window.colorTextSec }
                        }
                    }
                }
            }

            // 2. GPU Card
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: window.colorCard
                radius: 4
                border.color: window.colorCardBorder

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 4

                    Text {
                        text: "GPU MONITOR"
                        color: window.colorGpu
                        font.family: "Tahoma"
                        font.pixelSize: 11
                        font.bold: true
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Item {
                            Layout.preferredWidth: 60
                            Layout.preferredHeight: 60
                            Canvas {
                                id: gpuCanvas
                                anchors.fill: parent
                                onPaint: {
                                    var ctx = getContext("2d"); ctx.reset();
                                    ctx.beginPath(); ctx.arc(30, 30, 24, 0, 2 * Math.PI);
                                    ctx.lineWidth = 4; ctx.strokeStyle = "#c3cfdd"; ctx.stroke();
                                    ctx.beginPath(); ctx.arc(30, 30, 24, -Math.PI/2, -Math.PI/2 + (2*Math.PI*(systemMonitor.gpuUsage/100)));
                                    ctx.lineWidth = 4; ctx.strokeStyle = window.colorGpu; ctx.stroke();
                                }
                                Connections { target: systemMonitor; function onGpuUsageChanged() { gpuCanvas.requestPaint(); } }
                            }
                            Text {
                                anchors.centerIn: parent
                                text: systemMonitor.gpuUsage.toFixed(0) + "%"
                                font.family: "Tahoma"; font.pixelSize: 10; font.bold: true
                            }
                        }
                        ColumnLayout {
                            Text { text: systemMonitor.gpuModel; font.family: "Tahoma"; font.pixelSize: 10; font.bold: true; elide: Text.ElideRight; Layout.fillWidth: true }
                            Text { text: "Temp: " + (systemMonitor.gpuTemp > 0 ? systemMonitor.gpuTemp.toFixed(0) + "°C" : "N/A"); font.family: "Tahoma"; font.pixelSize: 9; color: window.colorTextSec }
                        }
                    }
                }
            }

            // 3. RAM Card
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: window.colorCard
                radius: 4
                border.color: window.colorCardBorder

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 4

                    Text {
                        text: "RAM INSTALLED"
                        color: window.colorRam
                        font.family: "Tahoma"
                        font.pixelSize: 11
                        font.bold: true
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Item {
                            Layout.preferredWidth: 60
                            Layout.preferredHeight: 60
                            Canvas {
                                id: ramCanvas
                                anchors.fill: parent
                                onPaint: {
                                    var ctx = getContext("2d"); ctx.reset();
                                    ctx.beginPath(); ctx.arc(30, 30, 24, 0, 2 * Math.PI);
                                    ctx.lineWidth = 4; ctx.strokeStyle = "#c3cfdd"; ctx.stroke();
                                    ctx.beginPath(); ctx.arc(30, 30, 24, -Math.PI/2, -Math.PI/2 + (2*Math.PI*(systemMonitor.ramUsage/100)));
                                    ctx.lineWidth = 4; ctx.strokeStyle = window.colorRam; ctx.stroke();
                                }
                                Connections { target: systemMonitor; function onRamUsageChanged() { ramCanvas.requestPaint(); } }
                            }
                            Text {
                                anchors.centerIn: parent
                                text: systemMonitor.ramUsage.toFixed(0) + "%"
                                font.family: "Tahoma"; font.pixelSize: 10; font.bold: true
                            }
                        }
                        ColumnLayout {
                            Text { text: "Capacity: " + systemMonitor.ramTotal.toFixed(1) + " GB"; font.family: "Tahoma"; font.pixelSize: 10; font.bold: true }
                            Text { text: "Used: " + systemMonitor.ramUsed.toFixed(1) + " GB"; font.family: "Tahoma"; font.pixelSize: 9; color: window.colorTextSec }
                        }
                    }
                }
            }

            // 4. Storage Card
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: window.colorCard
                radius: 4
                border.color: window.colorCardBorder

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 4

                    Text {
                        text: "HARD DRIVE (C:)"
                        color: "#0a246a"
                        font.family: "Tahoma"
                        font.pixelSize: 11
                        font.bold: true
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Item {
                            Layout.preferredWidth: 60
                            Layout.preferredHeight: 60
                            Canvas {
                                id: diskCanvas
                                anchors.fill: parent
                                onPaint: {
                                    var ctx = getContext("2d"); ctx.reset();
                                    ctx.beginPath(); ctx.arc(30, 30, 24, 0, 2 * Math.PI);
                                    ctx.lineWidth = 4; ctx.strokeStyle = "#c3cfdd"; ctx.stroke();
                                    ctx.beginPath(); ctx.arc(30, 30, 24, -Math.PI/2, -Math.PI/2 + (2*Math.PI*(systemMonitor.diskUsage/100)));
                                    ctx.lineWidth = 4; ctx.strokeStyle = "#0a246a"; ctx.stroke();
                                }
                                Connections { target: systemMonitor; function onDiskUsageChanged() { diskCanvas.requestPaint(); } }
                            }
                            Text {
                                anchors.centerIn: parent
                                text: systemMonitor.diskUsage.toFixed(0) + "%"
                                font.family: "Tahoma"; font.pixelSize: 10; font.bold: true
                            }
                        }
                        ColumnLayout {
                            Text { text: "Read: " + systemMonitor.diskReadSpeed.toFixed(1) + " MB/s"; font.family: "Tahoma"; font.pixelSize: 10; font.bold: true }
                            Text { text: "Write: " + systemMonitor.diskWriteSpeed.toFixed(1) + " MB/s"; font.family: "Tahoma"; font.pixelSize: 9; color: window.colorTextSec }
                        }
                    }
                }
            }

            // 5. Network Speed Card (Spans both columns)
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.columnSpan: 2
                color: window.colorCard
                radius: 4
                border.color: window.colorCardBorder

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 20

                    ColumnLayout {
                        Text { text: "NETWORK BROADBAND"; color: "#008080"; font.family: "Tahoma"; font.pixelSize: 11; font.bold: true }
                        Text { text: "Active Interfaces (Ethernet / WiFi)"; font.family: "Tahoma"; font.pixelSize: 9; color: window.colorTextSec }
                    }

                    Item { Layout.fillWidth: true }

                    // Speeds
                    RowLayout {
                        spacing: 20
                        ColumnLayout {
                            Text { text: "DOWNLOAD"; font.family: "Tahoma"; font.pixelSize: 8; color: window.colorTextSec; font.bold: true }
                            Text { text: systemMonitor.netDownloadSpeed > 1024.0 ? (systemMonitor.netDownloadSpeed / 1024.0).toFixed(2) + " MB/s" : systemMonitor.netDownloadSpeed.toFixed(1) + " KB/s"; font.family: "Consolas"; font.pixelSize: 13; font.bold: true; color: "#0a246a" }
                        }
                        ColumnLayout {
                            Text { text: "UPLOAD"; font.family: "Tahoma"; font.pixelSize: 8; color: window.colorTextSec; font.bold: true }
                            Text { text: systemMonitor.netUploadSpeed > 1024.0 ? (systemMonitor.netUploadSpeed / 1024.0).toFixed(2) + " MB/s" : systemMonitor.netUploadSpeed.toFixed(1) + " KB/s"; font.family: "Consolas"; font.pixelSize: 13; font.bold: true; color: "#008080" }
                        }
                    }
                }
            }
        }

        // Right Panel: Top 5 Heavy CPU Consumer Processes
        Rectangle {
            Layout.preferredWidth: 300
            Layout.fillHeight: true
            color: window.colorCard
            radius: 4
            border.color: window.colorCardBorder

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 15
                spacing: 10

                Text {
                    text: "TOP 5 HEAVY CPU TASKS"
                    color: window.colorCpu
                    font.family: "Tahoma"
                    font.pixelSize: 12
                    font.bold: true
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: window.colorCardBorder
                }

                // Table Layout list for top 5 CPU items
                ListView {
                    id: topFiveList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: processModel
                    interactive: false
                    spacing: 4

                    delegate: Item {
                        width: topFiveList.width
                        height: index < 5 ? 42 : 0 // Show only top 5 rows
                        visible: index < 5

                        Rectangle {
                            anchors.fill: parent
                            color: "#ffffff"
                            border.color: "#e2ebf5"
                            radius: 3

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 8
                                anchors.rightMargin: 8

                                Text {
                                    text: name
                                    font.family: "Tahoma"
                                    font.pixelSize: 11
                                    font.bold: true
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                                Text {
                                    text: cpu.toFixed(1) + "%"
                                    font.family: "Consolas"
                                    font.pixelSize: 11
                                    font.bold: true
                                    color: cpu > 10.0 ? "#d32f2f" : "#000000"
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
