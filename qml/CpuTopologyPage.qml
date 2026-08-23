import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Item {
    id: root
    anchors.fill: parent

    property var bgApps: []

    function refreshBackgroundApps() {
        bgApps = cpuTopology.getBackgroundApps();
    }

    Component.onCompleted: {
        refreshBackgroundApps();
    }

    Timer {
        interval: 3000
        running: true
        repeat: true
        onTriggered: root.refreshBackgroundApps()
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: parent.width
        contentHeight: mainCol.implicitHeight + 20
        clip: true

        ColumnLayout {
            id: mainCol
            width: parent.width - 15
            spacing: 16

            // 1. CPU Topology Header Summary
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 100
                color: window.colorCard
                radius: 6
                border.color: window.colorCardBorder
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: systemMonitor.cpuModel
                            font.family: "Tahoma"
                            font.pixelSize: 15
                            font.bold: true
                            color: window.colorCpu
                        }
                        Item { Layout.fillWidth: true }

                        Rectangle {
                            Layout.preferredHeight: 22
                            implicitWidth: hybridLabel.implicitWidth + 14
                            color: cpuTopology.hasHybridArchitecture ? "#e0f2fe" : "#f1f5f9"
                            radius: 3
                            border.color: cpuTopology.hasHybridArchitecture ? "#0284c7" : "#cbd5e1"
                            Text {
                                id: hybridLabel
                                anchors.centerIn: parent
                                text: cpuTopology.hasHybridArchitecture ? "Intel Hybrid (P-Core / E-Core)" : "Symmetric Multi-Core"
                                font.family: "Tahoma"
                                font.pixelSize: 10
                                font.bold: true
                                color: cpuTopology.hasHybridArchitecture ? "#0369a1" : "#475569"
                            }
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: window.colorCardBorder }

                    // Metrics Strip
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 25

                        RowLayout {
                            spacing: 6
                            Text { text: "Total Logical Cores:"; font.family: "Tahoma"; font.pixelSize: 11; color: window.colorTextSec }
                            Text { text: cpuTopology.totalCores.toString(); font.family: "Consolas"; font.pixelSize: 12; font.bold: true; color: window.colorTextMain }
                        }
                        RowLayout {
                            spacing: 6
                            Text { text: "Performance Cores (P):"; font.family: "Tahoma"; font.pixelSize: 11; color: window.colorTextSec }
                            Text { text: cpuTopology.pCoreCount + " Cores (Avg Load: " + cpuTopology.pCoreAvgUsage.toFixed(1) + "%)"; font.family: "Consolas"; font.pixelSize: 12; font.bold: true; color: "#0a246a" }
                        }
                        RowLayout {
                            spacing: 6
                            Text { text: "Efficiency Cores (E):"; font.family: "Tahoma"; font.pixelSize: 11; color: window.colorTextSec }
                            Text { text: cpuTopology.eCoreCount + " Cores (Avg Load: " + cpuTopology.eCoreAvgUsage.toFixed(1) + "%)"; font.family: "Consolas"; font.pixelSize: 12; font.bold: true; color: "#008080" }
                        }
                    }
                }
            }

            // 2. CPU Core Heatmap Grid
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: heatmapCol.implicitHeight + 25
                color: window.colorCard
                radius: 6
                border.color: window.colorCardBorder
                border.width: 1

                ColumnLayout {
                    id: heatmapCol
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: "CPU CORE HEATMAP (REAL-TIME PER-CORE UTILIZATION)"
                            font.family: "Tahoma"
                            font.pixelSize: 12
                            font.bold: true
                            color: window.colorCpu
                        }
                        Item { Layout.fillWidth: true }
                        Text {
                            text: "Total CPU: " + systemMonitor.cpuUsage.toFixed(1) + "%"
                            font.family: "Consolas"
                            font.pixelSize: 12
                            font.bold: true
                            color: window.colorTextMain
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: window.colorCardBorder }

                    // Heatmap Grid
                    GridView {
                        id: coreGrid
                        Layout.fillWidth: true
                        Layout.preferredHeight: Math.ceil(cpuTopology.totalCores / 8.0) * 85
                        cellWidth: width / Math.min(8, Math.max(4, Math.floor(width / 100)))
                        cellHeight: 80
                        interactive: false
                        model: cpuCoreModel

                        delegate: Item {
                            width: coreGrid.cellWidth
                            height: coreGrid.cellHeight

                            Rectangle {
                                anchors.fill: parent
                                anchors.margins: 3
                                radius: 4
                                color: "#ffffff"
                                border.color: window.colorCardBorder
                                border.width: 1

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 6
                                    spacing: 2

                                    RowLayout {
                                        Layout.fillWidth: true
                                        Text {
                                            text: "#" + coreIndex
                                            font.family: "Consolas"
                                            font.pixelSize: 10
                                            font.bold: true
                                            color: "#333333"
                                        }
                                        Item { Layout.fillWidth: true }
                                        Rectangle {
                                            Layout.preferredWidth: 28
                                            Layout.preferredHeight: 14
                                            radius: 2
                                            color: isPCore ? "#e0f2fe" : "#ecfdf5"
                                            border.color: isPCore ? "#0284c7" : "#059669"
                                            Text {
                                                anchors.centerIn: parent
                                                text: isPCore ? "P" : "E"
                                                font.family: "Tahoma"
                                                font.pixelSize: 8
                                                font.bold: true
                                                color: isPCore ? "#0369a1" : "#047857"
                                            }
                                        }
                                    }

                                    // Heat progress bar & load text
                                    Text {
                                        text: load.toFixed(0) + "%"
                                        font.family: "Consolas"
                                        font.pixelSize: 14
                                        font.bold: true
                                        color: heatColor
                                        Layout.alignment: Qt.AlignHCenter
                                    }

                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 6
                                        color: "#e2ebf5"
                                        radius: 2

                                        Rectangle {
                                            height: parent.height
                                            width: parent.width * (Math.min(100.0, load) / 100.0)
                                            radius: 2
                                            color: heatColor
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // 3. 1-Click "Pin to E-Cores" (Efficiency Mode Manager)
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: managerCol.implicitHeight + 30
                color: window.colorCard
                radius: 6
                border.color: window.colorCardBorder
                border.width: 1

                ColumnLayout {
                    id: managerCol
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true
                        ColumnLayout {
                            spacing: 2
                            Text {
                                text: "1-CLICK 'PIN TO E-CORES' (EFFICIENCY MODE MANAGER)"
                                font.family: "Tahoma"
                                font.pixelSize: 12
                                font.bold: true
                                color: "#008080"
                            }
                            Text {
                                text: "Assign background apps (Discord, Chrome, Slack, Torrents, Node) to E-Cores so games/foreground tasks get 100% of P-Cores."
                                font.family: "Tahoma"
                                font.pixelSize: 10
                                color: window.colorTextSec
                            }
                        }
                        Item { Layout.fillWidth: true }
                        Button {
                            Layout.preferredWidth: 80
                            Layout.preferredHeight: 28
                            background: Rectangle {
                                color: parent.hovered ? "#e3ebf4" : "#ffffff"
                                radius: 3
                                border.color: window.colorCardBorder
                            }
                            contentItem: Text {
                                text: "Scan Apps"
                                font.family: "Tahoma"; font.pixelSize: 10; font.bold: true; color: "#0a246a"
                                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: root.refreshBackgroundApps()
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: window.colorCardBorder }

                    // Apps List
                    ListView {
                        id: bgAppList
                        Layout.fillWidth: true
                        Layout.preferredHeight: Math.min(250, count * 44)
                        clip: true
                        interactive: count > 5
                        model: root.bgApps
                        spacing: 4

                        delegate: Rectangle {
                            width: bgAppList.width
                            height: 40
                            color: "#ffffff"
                            radius: 4
                            border.color: modelData.isPinnedToE ? "#6ee7b7" : "#e2ebf5"
                            border.width: 1

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 12
                                spacing: 10

                                Text {
                                    text: modelData.name
                                    font.family: "Tahoma"
                                    font.pixelSize: 12
                                    font.bold: true
                                    color: "#000000"
                                    Layout.preferredWidth: 200
                                    elide: Text.ElideRight
                                }

                                Text {
                                    text: "PID: " + modelData.pid + "  |  " + modelData.ramMB.toFixed(1) + " MB"
                                    font.family: "Consolas"
                                    font.pixelSize: 11
                                    color: "#555555"
                                    Layout.fillWidth: true
                                }

                                // Status Badge
                                Rectangle {
                                    Layout.preferredWidth: 120
                                    Layout.preferredHeight: 20
                                    radius: 3
                                    color: modelData.isPinnedToE ? "#ecfdf5" : "#f1f5f9"
                                    border.color: modelData.isPinnedToE ? "#10b981" : "#cbd5e1"
                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData.isPinnedToE ? "🌿 PINNED TO E-CORES" : "⚙️ ALL CORES"
                                        font.family: "Tahoma"
                                        font.pixelSize: 9
                                        font.bold: true
                                        color: modelData.isPinnedToE ? "#047857" : "#64748b"
                                    }
                                }

                                // Pin Button
                                Button {
                                    Layout.preferredWidth: 110
                                    Layout.preferredHeight: 26
                                    background: Rectangle {
                                        color: parent.hovered ? "#006666" : "#008080"
                                        radius: 3
                                    }
                                    contentItem: Text {
                                        text: "⚡ Pin to E-Cores"
                                        font.family: "Tahoma"; font.pixelSize: 10; font.bold: true; color: "#ffffff"
                                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                    }
                                    onClicked: {
                                        cpuTopology.pinProcessToECores(modelData.pid);
                                        root.refreshBackgroundApps();
                                        processModel.refresh();
                                    }
                                }

                                // Reset Button
                                Button {
                                    Layout.preferredWidth: 65
                                    Layout.preferredHeight: 26
                                    background: Rectangle {
                                        color: parent.hovered ? "#e2ebf5" : "#f0f4f9"
                                        radius: 3
                                        border.color: window.colorCardBorder
                                    }
                                    contentItem: Text {
                                        text: "Reset"
                                        font.family: "Tahoma"; font.pixelSize: 10; font.bold: true; color: "#0a246a"
                                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                    }
                                    onClicked: {
                                        cpuTopology.resetProcessAffinity(modelData.pid);
                                        root.refreshBackgroundApps();
                                        processModel.refresh();
                                    }
                                }
                            }
                        }
                    }

                    Text {
                        visible: root.bgApps.length === 0
                        text: "No common background apps currently detected."
                        font.family: "Tahoma"
                        font.pixelSize: 11
                        color: "#6b7280"
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
            }
        }
    }
}
