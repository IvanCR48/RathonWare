import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Popup {
    id: palettePopup
    width: 680
    height: 480
    anchors.centerIn: parent
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    background: Rectangle {
        color: "#f8fafc"
        radius: 8
        border.color: "#92a6b9"
        border.width: 2

        // Soft drop shadow effect border
        Rectangle {
            anchors.fill: parent
            anchors.margins: -1
            radius: 9
            color: "transparent"
            border.color: "#0a246a"
            border.width: 1
            opacity: 0.3
        }
    }

    property string queryText: ""
    property var portResults: []
    property var lockResults: []

    function updateQuery(text) {
        queryText = text.trim();
        if (queryText.startsWith(":")) {
            var portNum = parseInt(queryText.substring(1));
            if (!isNaN(portNum) && portNum > 0) {
                portResults = portManager.getProcessesByPort(portNum);
            } else {
                portResults = [];
            }
        } else if (queryText.toLowerCase().startsWith("unlock ") || queryText.includes("\\") || queryText.includes("/")) {
            var path = queryText.toLowerCase().startsWith("unlock ") ? queryText.substring(7).trim() : queryText;
            if (path.length > 2) {
                lockResults = fileUnlocker.findLockingProcesses(path);
            } else {
                lockResults = [];
            }
        }
    }

    onOpened: {
        paletteInput.text = "";
        paletteInput.forceActiveFocus();
        portResults = [];
        lockResults = [];
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        // Top Search Header
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 46
            color: "#ffffff"
            radius: 6
            border.color: "#0a246a"
            border.width: 1.5

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 10

                Text {
                    text: "🔍"
                    font.pixelSize: 16
                }

                TextInput {
                    id: paletteInput
                    Layout.fillWidth: true
                    font.family: "Tahoma"
                    font.pixelSize: 14
                    color: "#000000"
                    clip: true
                    activeFocusOnTab: true

                    onTextChanged: palettePopup.updateQuery(text)

                    Text {
                        text: "Type ':3000' for port killer, 'unlock <path>', or search process..."
                        font.family: "Tahoma"
                        font.pixelSize: 13
                        color: "#8fa3b8"
                        visible: !parent.text && !parent.activeFocus
                    }
                }

                // Quick Esc hint
                Rectangle {
                    Layout.preferredWidth: 38
                    Layout.preferredHeight: 22
                    color: "#e2ebf5"
                    radius: 3
                    border.color: "#b0c4de"
                    Text {
                        anchors.centerIn: parent
                        text: "ESC"
                        font.family: "Consolas"
                        font.pixelSize: 10
                        font.bold: true
                        color: "#4d5b6e"
                    }
                }
            }
        }

        // Quick Filter Chips Bar
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                text: "Quick Shortcuts:"
                font.family: "Tahoma"
                font.pixelSize: 10
                font.bold: true
                color: "#4d5b6e"
            }

            component ShortcutChip : Rectangle {
                id: chip
                property string label: ""
                property string actionQuery: ""
                signal chipClicked()
                Layout.preferredHeight: 22
                implicitWidth: chipText.implicitWidth + 14
                color: mouseChipArea.containsMouse ? "#dbe6f5" : "#eef3f9"
                radius: 3
                border.color: "#b9cde3"
                border.width: 1

                Text {
                    id: chipText
                    anchors.centerIn: parent
                    text: chip.label
                    font.family: "Tahoma"
                    font.pixelSize: 10
                    color: "#0a246a"
                }

                MouseArea {
                    id: mouseChipArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (chip.actionQuery !== "") {
                            paletteInput.text = chip.actionQuery;
                            paletteInput.cursorPosition = paletteInput.text.length;
                        }
                        chip.chipClicked();
                    }
                }
            }

            ShortcutChip { label: ":3000 (Port Killer)"; actionQuery: ":3000" }
            ShortcutChip { label: ":8080 (Web Dev)"; actionQuery: ":8080" }
            ShortcutChip { label: ":5432 (Postgres)"; actionQuery: ":5432" }
            ShortcutChip { label: "GPU Tab"; onChipClicked: { window.activePage = "gpu"; palettePopup.close(); } }
            ShortcutChip { label: "CPU Topology"; onChipClicked: { window.activePage = "topology"; palettePopup.close(); } }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#c9d8e7"
        }

        // Results Container Area
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Mode 1: Port Query Results (:port)
            ColumnLayout {
                anchors.fill: parent
                visible: palettePopup.queryText.startsWith(":")
                spacing: 8

                Text {
                    text: "PORT-TO-PROCESS KILLER (" + palettePopup.queryText + ")"
                    font.family: "Tahoma"
                    font.pixelSize: 11
                    font.bold: true
                    color: "#0a246a"
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: palettePopup.portResults
                    spacing: 6

                    delegate: Rectangle {
                        width: parent.width
                        height: 64
                        color: "#ffffff"
                        radius: 5
                        border.color: "#92a6b9"
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 12

                            // Port Tag
                            Rectangle {
                                Layout.preferredWidth: 65
                                Layout.preferredHeight: 38
                                color: "#0a246a"
                                radius: 4
                                Column {
                                    anchors.centerIn: parent
                                    Text { text: "PORT"; font.family: "Tahoma"; font.pixelSize: 8; font.bold: true; color: "#a6caf0"; anchors.horizontalCenter: parent.horizontalCenter }
                                    Text { text: modelData.port.toString(); font.family: "Consolas"; font.pixelSize: 13; font.bold: true; color: "#ffffff"; anchors.horizontalCenter: parent.horizontalCenter }
                                }
                            }

                            // Details Column
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                RowLayout {
                                    Text {
                                        text: modelData.name
                                        font.family: "Tahoma"
                                        font.pixelSize: 13
                                        font.bold: true
                                        color: "#000000"
                                    }
                                    Rectangle {
                                        Layout.preferredWidth: 50
                                        Layout.preferredHeight: 18
                                        color: "#e8f4fd"
                                        radius: 3
                                        border.color: "#b0d4f1"
                                        Text {
                                            anchors.centerIn: parent
                                            text: modelData.protocol
                                            font.family: "Consolas"
                                            font.pixelSize: 9
                                            font.bold: true
                                            color: "#0284c7"
                                        }
                                    }
                                }

                                Text {
                                    text: "PID: " + modelData.pid + "  |  Memory: " + modelData.ramMB.toFixed(1) + " MB  |  State: " + modelData.state
                                    font.family: "Consolas"
                                    font.pixelSize: 10
                                    color: "#555555"
                                }
                            }

                            // Action Button (⚡ Kill)
                            Button {
                                Layout.preferredWidth: 90
                                Layout.preferredHeight: 36
                                background: Rectangle {
                                    color: parent.hovered ? "#b91c1c" : "#dc2626"
                                    radius: 4
                                }
                                contentItem: Text {
                                    text: "⚡ Kill"
                                    font.family: "Tahoma"
                                    font.pixelSize: 12
                                    font.bold: true
                                    color: "#ffffff"
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                onClicked: {
                                    portManager.killProcessByPid(modelData.pid);
                                    palettePopup.updateQuery(palettePopup.queryText);
                                    processModel.refresh();
                                }
                            }
                        }
                    }
                }

                // Empty state for port
                Text {
                    visible: palettePopup.portResults.length === 0
                    text: "No active process listening on port " + palettePopup.queryText.substring(1)
                    font.family: "Tahoma"
                    font.pixelSize: 12
                    color: "#6b7280"
                    Layout.alignment: Qt.AlignHCenter
                }
            }

            // Mode 2: File Unlocker Results
            ColumnLayout {
                anchors.fill: parent
                visible: !palettePopup.queryText.startsWith(":") && (palettePopup.queryText.toLowerCase().startsWith("unlock ") || palettePopup.queryText.includes("\\") || palettePopup.queryText.includes("/"))
                spacing: 8

                Text {
                    text: "FILE LOCK INSPECTOR & UNLOCKER"
                    font.family: "Tahoma"
                    font.pixelSize: 11
                    font.bold: true
                    color: "#0a246a"
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: palettePopup.lockResults
                    spacing: 6

                    delegate: Rectangle {
                        width: parent.width
                        height: 56
                        color: "#ffffff"
                        radius: 5
                        border.color: "#92a6b9"
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 10

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                Text {
                                    text: modelData.name + " (" + modelData.appName + ")"
                                    font.family: "Tahoma"
                                    font.pixelSize: 12
                                    font.bold: true
                                    color: "#000000"
                                }
                                Text {
                                    text: "PID: " + modelData.pid + "  |  Memory: " + modelData.ramMB.toFixed(1) + " MB"
                                    font.family: "Consolas"
                                    font.pixelSize: 10
                                    color: "#555555"
                                }
                            }

                            Button {
                                Layout.preferredWidth: 110
                                Layout.preferredHeight: 32
                                background: Rectangle {
                                    color: parent.hovered ? "#b91c1c" : "#dc2626"
                                    radius: 4
                                }
                                contentItem: Text {
                                    text: "Unlock (Kill)"
                                    font.family: "Tahoma"
                                    font.pixelSize: 11
                                    font.bold: true
                                    color: "#ffffff"
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                onClicked: {
                                    fileUnlocker.killLockingProcess(modelData.pid);
                                    palettePopup.updateQuery(palettePopup.queryText);
                                    processModel.refresh();
                                }
                            }
                        }
                    }
                }

                Text {
                    visible: palettePopup.lockResults.length === 0
                    text: "No active process locks found on this file path."
                    font.family: "Tahoma"
                    font.pixelSize: 12
                    color: "#059669"
                    Layout.alignment: Qt.AlignHCenter
                }
            }

            // Mode 3: Standard Process Search & Action Chips
            ListView {
                id: processSearchList
                anchors.fill: parent
                visible: !palettePopup.queryText.startsWith(":") && !palettePopup.queryText.toLowerCase().startsWith("unlock ") && !palettePopup.queryText.includes("\\") && !palettePopup.queryText.includes("/")
                clip: true
                model: processModel
                spacing: 4

                delegate: Item {
                    width: processSearchList.width
                    property bool matches: !palettePopup.queryText || name.toLowerCase().includes(palettePopup.queryText.toLowerCase())
                    height: matches ? 50 : 0
                    visible: matches

                    Rectangle {
                        anchors.fill: parent
                        color: "#ffffff"
                        radius: 4
                        border.color: "#c9d8e7"
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            spacing: 8

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                RowLayout {
                                    Text {
                                        text: name
                                        font.family: "Tahoma"
                                        font.pixelSize: 12
                                        font.bold: true
                                        color: "#000000"
                                    }
                                    Rectangle {
                                        visible: isSuspended
                                        Layout.preferredWidth: 64
                                        Layout.preferredHeight: 16
                                        color: "#fef3c7"
                                        radius: 2
                                        border.color: "#f59e0b"
                                        Text {
                                            anchors.centerIn: parent
                                            text: "SUSPENDED"
                                            font.family: "Consolas"
                                            font.pixelSize: 8
                                            font.bold: true
                                            color: "#b45309"
                                        }
                                    }
                                }

                                Text {
                                    text: "PID: " + pid + "  |  CPU: " + cpu.toFixed(1) + "%  |  RAM: " + ram.toFixed(1) + " MB"
                                    font.family: "Consolas"
                                    font.pixelSize: 10
                                    color: "#555555"
                                }
                            }

                            // Quick Action: Pin to E-Cores
                            Button {
                                Layout.preferredWidth: 60
                                Layout.preferredHeight: 28
                                background: Rectangle {
                                    color: parent.hovered ? "#006666" : "#008080"
                                    radius: 3
                                }
                                contentItem: Text {
                                    text: "E-Core"
                                    font.family: "Tahoma"; font.pixelSize: 10; font.bold: true; color: "#ffffff"
                                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                }
                                onClicked: processModel.pinToECores(pid)
                            }

                            // Quick Action: Suspend / Resume
                            Button {
                                Layout.preferredWidth: 60
                                Layout.preferredHeight: 28
                                background: Rectangle {
                                    color: parent.hovered ? "#d97706" : "#f59e0b"
                                    radius: 3
                                }
                                contentItem: Text {
                                    text: isSuspended ? "Resume" : "Pause"
                                    font.family: "Tahoma"; font.pixelSize: 10; font.bold: true; color: "#ffffff"
                                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                }
                                onClicked: {
                                    if (isSuspended) processModel.resumeProcess(pid);
                                    else processModel.suspendProcess(pid);
                                }
                            }

                            // Quick Action: Kill Tree
                            Button {
                                Layout.preferredWidth: 65
                                Layout.preferredHeight: 28
                                background: Rectangle {
                                    color: parent.hovered ? "#7f1d1d" : "#991b1b"
                                    radius: 3
                                }
                                contentItem: Text {
                                    text: "Kill Tree"
                                    font.family: "Tahoma"; font.pixelSize: 10; font.bold: true; color: "#ffffff"
                                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                }
                                onClicked: processModel.killProcessTree(pid)
                            }

                            // Quick Action: Kill
                            Button {
                                Layout.preferredWidth: 50
                                Layout.preferredHeight: 28
                                background: Rectangle {
                                    color: parent.hovered ? "#b91c1c" : "#dc2626"
                                    radius: 3
                                }
                                contentItem: Text {
                                    text: "Kill"
                                    font.family: "Tahoma"; font.pixelSize: 10; font.bold: true; color: "#ffffff"
                                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                }
                                onClicked: processModel.killProcess(pid)
                            }
                        }
                    }
                }
            }
        }
    }
}
