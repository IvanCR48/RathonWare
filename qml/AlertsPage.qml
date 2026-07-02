import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Item {
    id: root
    anchors.fill: parent

    // Alert threshold states
    property bool cpuAlertEnabled: true
    property int cpuThreshold: 80
    
    property bool gpuAlertEnabled: true
    property int gpuThreshold: 75

    property bool ramAlertEnabled: false
    property int ramThreshold: 90

    // List of active notifications log
    property var alertLogs: []

    function addLog(msg) {
        var logs = alertLogs;
        var time = new Date().toLocaleTimeString();
        logs.unshift("[" + time + "] " + msg);
        if (logs.length > 8) logs.pop();
        alertLogs = logs;
        logList.model = alertLogs;
    }

    Connections {
        target: systemMonitor
        
        function onCpuUsageChanged() {
            if (cpuAlertEnabled && systemMonitor.cpuUsage > cpuThreshold) {
                var msg = "WARNING: CPU Usage at " + systemMonitor.cpuUsage.toFixed(1) + "% (Threshold: " + cpuThreshold + "%)";
                if (alertLogs.length === 0 || !alertLogs[0].includes("CPU Usage")) {
                    addLog(msg);
                }
            }
        }

        function onGpuTempChanged() {
            if (gpuAlertEnabled && systemMonitor.gpuTemp > gpuThreshold) {
                var msg = "CRITICAL: GPU Temp at " + systemMonitor.gpuTemp.toFixed(0) + "°C (Threshold: " + gpuThreshold + "°C)";
                if (alertLogs.length === 0 || !alertLogs[0].includes("GPU Temp")) {
                    addLog(msg);
                }
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 20

        // Left Column: Threshold Slider Rules
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: window.colorCard
            radius: 4
            border.color: window.colorCardBorder
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 15

                Text {
                    text: "SYSTEM ALERTS & THRESHOLDS"
                    color: window.colorCpu
                    font.family: "Tahoma"; font.pixelSize: 13; font.bold: true
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: window.colorCardBorder }

                // CPU Load Rule
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    RowLayout {
                        Layout.fillWidth: true
                        CheckBox {
                            id: cpuChk
                            text: "Monitor CPU Load Threshold"
                            font.family: "Tahoma"; font.pixelSize: 11
                            checked: root.cpuAlertEnabled
                            onCheckedChanged: root.cpuAlertEnabled = checked
                        }
                        Item { Layout.fillWidth: true }
                        Text {
                            text: root.cpuThreshold + "%"
                            font.family: "Consolas"; font.pixelSize: 11; font.bold: true
                            color: window.colorCpu
                        }
                    }
                    Slider {
                        Layout.fillWidth: true
                        from: 50; to: 99
                        value: root.cpuThreshold
                        enabled: cpuChk.checked
                        onMoved: root.cpuThreshold = value
                    }
                }

                // GPU Temp Rule
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    RowLayout {
                        Layout.fillWidth: true
                        CheckBox {
                            id: gpuChk
                            text: "Monitor GPU Temperature Alert"
                            font.family: "Tahoma"; font.pixelSize: 11
                            checked: root.gpuAlertEnabled
                            onCheckedChanged: root.gpuAlertEnabled = checked
                        }
                        Item { Layout.fillWidth: true }
                        Text {
                            text: root.gpuThreshold + "°C"
                            font.family: "Consolas"; font.pixelSize: 11; font.bold: true
                            color: window.colorGpu
                        }
                    }
                    Slider {
                        Layout.fillWidth: true
                        from: 50; to: 95
                        value: root.gpuThreshold
                        enabled: gpuChk.checked
                        onMoved: root.gpuThreshold = value
                    }
                }

                // RAM Load Rule
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    RowLayout {
                        Layout.fillWidth: true
                        CheckBox {
                            id: ramChk
                            text: "Monitor RAM Utilization Peak"
                            font.family: "Tahoma"; font.pixelSize: 11
                            checked: root.ramAlertEnabled
                            onCheckedChanged: root.ramAlertEnabled = checked
                        }
                        Item { Layout.fillWidth: true }
                        Text {
                            text: root.ramThreshold + "%"
                            font.family: "Consolas"; font.pixelSize: 11; font.bold: true
                            color: window.colorRam
                        }
                    }
                    Slider {
                        Layout.fillWidth: true
                        from: 60; to: 95
                        value: root.ramThreshold
                        enabled: ramChk.checked
                        onMoved: root.ramThreshold = value
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }

        // Right Column: Log panel
        Rectangle {
            Layout.preferredWidth: 320
            Layout.fillHeight: true
            color: window.colorCard
            radius: 4
            border.color: window.colorCardBorder
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 12

                Text {
                    text: "DIAGNOSTIC LOG (ALERTS)"
                    color: "#d32f2f"
                    font.family: "Tahoma"; font.pixelSize: 13; font.bold: true
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: window.colorCardBorder }

                // Logs list
                ListView {
                    id: logList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: root.alertLogs
                    spacing: 4

                    delegate: Item {
                        width: logList.width
                        height: logLabel.implicitHeight + 10

                        Rectangle {
                            anchors.fill: parent
                            color: "#ffebee"
                            radius: 3
                            border.color: "#ffcdd2"
                            border.width: 1

                            Text {
                                id: logLabel
                                anchors.fill: parent
                                anchors.margins: 6
                                text: modelData
                                font.family: "Consolas"; font.pixelSize: 10
                                color: "#c62828"
                                wrapMode: Text.Wrap
                            }
                        }
                    }
                }
            }
        }
    }
}
