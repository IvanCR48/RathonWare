import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Item {
    id: root
    anchors.fill: parent

    // Historical Ring Buffers (60 data points = 60 seconds of live history)
    property var cpuHistory: Array.from({length: 60}, () => 0)
    property var gpuHistory: Array.from({length: 60}, () => 0)
    property var gpuTempHistory: Array.from({length: 60}, () => 0)
    property var ramHistory: Array.from({length: 60}, () => 0)
    property var diskHistory: Array.from({length: 60}, () => 0)
    property var netDownHistory: Array.from({length: 60}, () => 0)
    property var netUpHistory: Array.from({length: 60}, () => 0)

    function pushHistory(array, newValue) {
        var copy = array.slice();
        copy.push(newValue);
        copy.shift();
        return copy;
    }

    function sampleMetrics() {
        var cpu = systemMonitor.cpuUsage;
        var gpu = gpuMonitor.hasNvidiaGpu ? gpuMonitor.gpuUsage : systemMonitor.gpuUsage;
        var gpuTemp = gpuMonitor.hasNvidiaGpu ? gpuMonitor.gpuTemp : systemMonitor.gpuTemp;
        var ram = systemMonitor.ramUsage;
        var disk = systemMonitor.diskUsage;
        var netDown = systemMonitor.netDownloadSpeed;
        var netUp = systemMonitor.netUploadSpeed;

        root.cpuHistory = pushHistory(root.cpuHistory, cpu);
        root.gpuHistory = pushHistory(root.gpuHistory, gpu);
        root.gpuTempHistory = pushHistory(root.gpuTempHistory, gpuTemp);
        root.ramHistory = pushHistory(root.ramHistory, ram);
        root.diskHistory = pushHistory(root.diskHistory, disk);
        root.netDownHistory = pushHistory(root.netDownHistory, netDown);
        root.netUpHistory = pushHistory(root.netUpHistory, netUp);
    }

    Timer {
        id: telemetryTimer
        interval: 1000
        running: true
        repeat: true
        triggeredOnStart: true
        onTriggered: root.sampleMetrics()
    }

    // Reusable Custom Telemetry Chart Component
    component TelemetryChart : Rectangle {
        id: chartBox
        property string chartTitle: ""
        property string firstLabel: "Load"
        property color lineColor: "#0a246a"
        property var historyData: []
        property string currentValue: ""

        // Optional second line
        property bool hasSecondLine: false
        property string secondLabel: ""
        property color secondLineColor: "#d32f2f"
        property var secondHistoryData: []
        property string secondCurrentValue: ""

        // Optional max scale override (default 100 for %)
        property double maxScale: 100.0

        Layout.fillWidth: true
        Layout.preferredHeight: 180
        color: window.colorCard
        radius: 6
        border.color: window.colorCardBorder
        border.width: 1

        onHistoryDataChanged: chartCanvas.requestPaint()
        onSecondHistoryDataChanged: chartCanvas.requestPaint()

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 6

            // Header with title and live badges
            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: chartBox.chartTitle
                    color: window.colorTextMain
                    font.family: "Tahoma"
                    font.pixelSize: 12
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                // Line 1 legend pill
                RowLayout {
                    spacing: 4
                    Rectangle { width: 10; height: 10; radius: 2; color: chartBox.lineColor }
                    Text {
                        text: chartBox.firstLabel + ": " + chartBox.currentValue
                        font.family: "Consolas"
                        font.pixelSize: 11
                        font.bold: true
                        color: chartBox.lineColor
                    }
                }

                // Line 2 legend pill
                RowLayout {
                    visible: chartBox.hasSecondLine
                    spacing: 4
                    Rectangle { width: 10; height: 10; radius: 2; color: chartBox.secondLineColor }
                    Text {
                        text: chartBox.secondLabel + ": " + chartBox.secondCurrentValue
                        font.family: "Consolas"
                        font.pixelSize: 11
                        font.bold: true
                        color: chartBox.secondLineColor
                    }
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: "#dbe3ee" }

            // Chart Canvas
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                Canvas {
                    id: chartCanvas
                    anchors.fill: parent

                    onPaint: {
                        var ctx = getContext("2d");
                        ctx.reset();
                        var w = width;
                        var h = height;

                        if (w <= 0 || h <= 0) return;

                        // 1. Draw Grid Background
                        ctx.fillStyle = "#ffffff";
                        ctx.fillRect(0, 0, w, h);

                        ctx.strokeStyle = "#e8eef5";
                        ctx.lineWidth = 1;

                        // Horizontal grid lines (4 rows)
                        var rows = 4;
                        for (var r = 1; r < rows; r++) {
                            var y = (h / rows) * r;
                            ctx.beginPath();
                            ctx.moveTo(0, y);
                            ctx.lineTo(w, y);
                            ctx.stroke();
                        }

                        // Vertical grid lines (10 columns)
                        var cols = 10;
                        for (var c = 1; c < cols; c++) {
                            var x = (w / cols) * c;
                            ctx.beginPath();
                            ctx.moveTo(x, 0);
                            ctx.lineTo(x, h);
                            ctx.stroke();
                        }

                        var scale = chartBox.maxScale > 0 ? chartBox.maxScale : 100.0;
                        var len = chartBox.historyData ? chartBox.historyData.length : 0;
                        if (len < 2) return;

                        var dx = w / (len - 1);

                        // 2. Draw Line 1 with smooth area gradient fill
                        if (chartBox.historyData && chartBox.historyData.length > 0) {
                            // Path for Stroke
                            ctx.beginPath();
                            for (var i = 0; i < len; i++) {
                                var val = Math.max(0.0, Math.min(scale, chartBox.historyData[i]));
                                var px = i * dx;
                                var py = h - 3 - ((val / scale) * (h - 6));
                                if (i === 0) ctx.moveTo(px, py);
                                else ctx.lineTo(px, py);
                            }
                            ctx.lineWidth = 2.5;
                            ctx.strokeStyle = chartBox.lineColor;
                            ctx.stroke();
                        }

                        // 3. Draw Line 2 (Optional)
                        if (chartBox.hasSecondLine && chartBox.secondHistoryData && chartBox.secondHistoryData.length > 0) {
                            var len2 = chartBox.secondHistoryData.length;
                            var dx2 = w / (len2 - 1);
                            ctx.beginPath();
                            for (var j = 0; j < len2; j++) {
                                var val2 = Math.max(0.0, Math.min(scale, chartBox.secondHistoryData[j]));
                                var px2 = j * dx2;
                                var py2 = h - 3 - ((val2 / scale) * (h - 6));
                                if (j === 0) ctx.moveTo(px2, py2);
                                else ctx.lineTo(px2, py2);
                            }
                            ctx.lineWidth = 2.5;
                            ctx.strokeStyle = chartBox.secondLineColor;
                            ctx.stroke();
                        }
                    }
                }
            }
        }
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: parent.width
        contentHeight: columnLayout.implicitHeight + 25
        clip: true

        ColumnLayout {
            id: columnLayout
            width: parent.width - 15
            spacing: 16

            // 1. CPU History Chart
            TelemetryChart {
                id: cpuChart
                chartTitle: "CPU PACKAGE LOAD HISTORY (LAST 60s)"
                firstLabel: "CPU Load"
                lineColor: window.colorCpu
                historyData: root.cpuHistory
                currentValue: systemMonitor.cpuUsage.toFixed(1) + "%"
            }

            // 2. GPU Load & Temperature Chart
            TelemetryChart {
                id: gpuChart
                chartTitle: "GPU TELEMETRY & TEMPERATURE HISTORY (LAST 60s)"
                firstLabel: "GPU Load"
                lineColor: window.colorGpu
                historyData: root.gpuHistory
                currentValue: (gpuMonitor.hasNvidiaGpu ? gpuMonitor.gpuUsage.toFixed(0) : systemMonitor.gpuUsage.toFixed(0)) + "%"
                hasSecondLine: true
                secondLabel: "Core Temp"
                secondLineColor: "#d32f2f"
                secondHistoryData: root.gpuTempHistory
                secondCurrentValue: (gpuMonitor.hasNvidiaGpu ? gpuMonitor.gpuTemp.toFixed(0) : systemMonitor.gpuTemp.toFixed(0)) + "°C"
            }

            // 3. RAM & Storage History Chart
            TelemetryChart {
                id: ramChart
                chartTitle: "SYSTEM MEMORY & STORAGE (LAST 60s)"
                firstLabel: "RAM Used"
                lineColor: window.colorRam
                historyData: root.ramHistory
                currentValue: systemMonitor.ramUsage.toFixed(1) + "% (" + systemMonitor.ramUsed.toFixed(1) + " GB)"
                hasSecondLine: true
                secondLabel: "C: Drive"
                secondLineColor: "#0284c7"
                secondHistoryData: root.diskHistory
                secondCurrentValue: systemMonitor.diskUsage.toFixed(0) + "%"
            }

            // 4. Network Speeds History Chart
            TelemetryChart {
                id: netChart
                chartTitle: "NETWORK BANDWIDTH REAL-TIME ACTIVITY (LAST 60s)"
                firstLabel: "Download"
                lineColor: "#059669"
                historyData: root.netDownHistory
                maxScale: Math.max(500.0, Math.max(...root.netDownHistory, ...root.netUpHistory))
                currentValue: systemMonitor.netDownloadSpeed > 1024 ? (systemMonitor.netDownloadSpeed / 1024.0).toFixed(2) + " MB/s" : systemMonitor.netDownloadSpeed.toFixed(1) + " KB/s"
                hasSecondLine: true
                secondLabel: "Upload"
                secondLineColor: "#d97706"
                secondHistoryData: root.netUpHistory
                secondCurrentValue: systemMonitor.netUploadSpeed > 1024 ? (systemMonitor.netUploadSpeed / 1024.0).toFixed(2) + " MB/s" : systemMonitor.netUploadSpeed.toFixed(1) + " KB/s"
            }
        }
    }
}
