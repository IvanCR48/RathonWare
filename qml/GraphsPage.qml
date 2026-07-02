import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Item {
    id: root
    anchors.fill: parent

    // Caching larger historical charts (up to 100 entries for historic review)
    property var cpuHistory: Array.from({length: 100}, () => 0)
    property var gpuHistory: Array.from({length: 100}, () => 0)
    property var ramHistory: Array.from({length: 100}, () => 0)
    property var diskHistory: Array.from({length: 100}, () => 0)

    function pushHistory(array, newValue) {
        var copy = array;
        copy.push(newValue);
        copy.shift();
        return copy;
    }

    Connections {
        target: systemMonitor
        function onCpuUsageChanged() {
            root.cpuHistory = pushHistory(root.cpuHistory, systemMonitor.cpuUsage);
            cpuChart.requestPaint();
        }
        function onGpuUsageChanged() {
            root.gpuHistory = pushHistory(root.gpuHistory, systemMonitor.gpuUsage);
            gpuChart.requestPaint();
        }
        function onRamUsageChanged() {
            root.ramHistory = pushHistory(root.ramHistory, systemMonitor.ramUsage);
            ramChart.requestPaint();
        }
        function onDiskUsageChanged() {
            root.diskHistory = pushHistory(root.diskHistory, systemMonitor.diskUsage);
            ramChart.requestPaint(); // Re-paint on disk updates too since they share the same card
        }
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: parent.width
        contentHeight: columnLayout.implicitHeight + 20
        clip: true

        ColumnLayout {
            id: columnLayout
            width: parent.width - 15
            spacing: 20

            // Helper component for Charts
            component TelemetryChart : Rectangle {
                id: chartContainer
                property string chartTitle: ""
                property color lineColor: "#0a246a"
                property var historyData: []
                
                // For second optional line
                property bool hasSecondLine: false
                property color secondLineColor: "#ff0000"
                property var secondHistoryData: []

                Layout.fillWidth: true
                Layout.preferredHeight: 180
                color: window.colorCard
                radius: 4
                border.color: window.colorCardBorder

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 8

                    Text {
                        text: chartContainer.chartTitle
                        color: window.colorTextMain
                        font.family: "Tahoma"; font.pixelSize: 11; font.bold: true
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        Canvas {
                            id: chartCanvas
                            anchors.fill: parent

                            onPaint: {
                                var ctx = getContext("2d"); ctx.reset();
                                var w = width; var h = height;

                                // Background grid
                                ctx.fillStyle = "#fcfdfd"; ctx.fillRect(0, 0, w, h);
                                ctx.strokeStyle = "#e5edf5"; ctx.lineWidth = 1;
                                var gridRows = 4;
                                for (var r = 1; r < gridRows; r++) {
                                    var y = (h / gridRows) * r;
                                    ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(w, y); ctx.stroke();
                                }
                                var gridCols = 15;
                                for (var c = 1; c < gridCols; c++) {
                                    var x = (w / gridCols) * c;
                                    ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, h); ctx.stroke();
                                }

                                var dx = w / (100 - 1);

                                // Line 1
                                if (chartContainer.historyData.length > 0) {
                                    ctx.beginPath();
                                    for (var i = 0; i < chartContainer.historyData.length; i++) {
                                        var xPos = i * dx;
                                        var yPos = h - 4 - ((chartContainer.historyData[i]/100) * (h - 8));
                                        if (i === 0) ctx.moveTo(xPos, yPos);
                                        else ctx.lineTo(xPos, yPos);
                                    }
                                    ctx.lineWidth = 2; ctx.strokeStyle = chartContainer.lineColor; ctx.stroke();
                                }

                                // Line 2 (Optional)
                                if (chartContainer.hasSecondLine && chartContainer.secondHistoryData.length > 0) {
                                    ctx.beginPath();
                                    for (var j = 0; j < chartContainer.secondHistoryData.length; j++) {
                                        var xPos2 = j * dx;
                                        var yPos2 = h - 4 - ((chartContainer.secondHistoryData[j]/100) * (h - 8));
                                        if (j === 0) ctx.moveTo(xPos2, yPos2);
                                        else ctx.lineTo(xPos2, yPos2);
                                    }
                                    ctx.lineWidth = 2; ctx.strokeStyle = chartContainer.secondLineColor; ctx.stroke();
                                }
                            }

                            // Force repaint helper
                            function refresh() { requestPaint(); }
                        }
                    }
                }
            }

            // 1. CPU Usage History Chart
            TelemetryChart {
                id: cpuChart
                chartTitle: "CPU Package Load History (%)"
                lineColor: window.colorCpu
                historyData: root.cpuHistory
            }

            // 2. GPU Usage & Temp History Chart
            TelemetryChart {
                id: gpuChart
                chartTitle: "GPU Telemetry (Blue: Load %, Red: Temp °C)"
                lineColor: window.colorGpu
                historyData: root.gpuHistory
                hasSecondLine: true
                secondLineColor: "#d32f2f"
                // Map temp to a scale of 0-100 for graph display
                secondHistoryData: root.gpuHistory.map((v, i) => systemMonitor.gpuTemp) 
            }

            // 3. RAM & Disk History Chart
            TelemetryChart {
                id: ramChart
                chartTitle: "Memory & Storage Allocation (Yellow: RAM %, Purple: C: Disk %)"
                lineColor: window.colorRam
                historyData: root.ramHistory
                hasSecondLine: true
                secondLineColor: "#800080"
                secondHistoryData: root.diskHistory
            }
        }
    }
}
