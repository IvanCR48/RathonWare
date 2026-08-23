import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Item {
    id: root
    anchors.fill: parent

    property string searchText: ""
    property int expandedPid: -1 // Track which row is expanded to show command line
    property var portResults: []

    function checkPortQuery(text) {
        if (text.startsWith(":")) {
            var p = parseInt(text.substring(1));
            if (!isNaN(p) && p > 0) {
                portResults = portManager.getProcessesByPort(p);
                return;
            }
        }
        portResults = [];
    }

    // Context Menu for right-click process actions
    Menu {
        id: processContextMenu
        property int targetPid: 0
        property string targetName: ""

        MenuItem {
            text: "End Task (Kill)"
            onTriggered: processModel.killProcess(processContextMenu.targetPid)
        }

        MenuItem {
            text: "🌲 Kill Entire Process Tree"
            onTriggered: processModel.killProcessTree(processContextMenu.targetPid)
        }

        MenuSeparator {}

        MenuItem {
            text: "🌿 Pin to E-Cores (Efficiency Mode)"
            onTriggered: {
                processModel.pinToECores(processContextMenu.targetPid);
                processModel.refresh();
            }
        }

        MenuItem {
            text: "⚡ Pin to P-Cores (Max Boost)"
            onTriggered: {
                processModel.pinToPCores(processContextMenu.targetPid);
                processModel.refresh();
            }
        }

        MenuItem {
            text: "🔄 Reset Affinity (All Cores)"
            onTriggered: {
                processModel.resetAffinity(processContextMenu.targetPid);
                processModel.refresh();
            }
        }

        MenuSeparator {}

        MenuItem {
            text: "⏸️ Suspend Execution"
            onTriggered: {
                processModel.suspendProcess(processContextMenu.targetPid);
                processModel.refresh();
            }
        }

        MenuItem {
            text: "▶️ Resume Execution"
            onTriggered: {
                processModel.resumeProcess(processContextMenu.targetPid);
                processModel.refresh();
            }
        }

        MenuSeparator {}
        
        Menu {
            title: "Set Priority Class"
            MenuItem { text: "Realtime"; onTriggered: processModel.setPriority(processContextMenu.targetPid, 5) }
            MenuItem { text: "High"; onTriggered: processModel.setPriority(processContextMenu.targetPid, 4) }
            MenuItem { text: "Above Normal"; onTriggered: processModel.setPriority(processContextMenu.targetPid, 3) }
            MenuItem { text: "Normal"; onTriggered: processModel.setPriority(processContextMenu.targetPid, 2) }
            MenuItem { text: "Below Normal"; onTriggered: processModel.setPriority(processContextMenu.targetPid, 1) }
            MenuItem { text: "Idle"; onTriggered: processModel.setPriority(processContextMenu.targetPid, 0) }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        // Search Control Bar
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 34
                color: "#ffffff"
                radius: 4
                border.color: window.colorCardBorder
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    spacing: 8

                    Text {
                        text: "🔍"
                        font.pixelSize: 12
                        color: window.colorTextSec
                    }

                    TextInput {
                        id: searchInput
                        Layout.fillWidth: true
                        font.family: "Tahoma"
                        font.pixelSize: 12
                        color: window.colorTextMain
                        clip: true
                        activeFocusOnTab: true
                        
                        onTextChanged: {
                            root.searchText = text;
                            root.checkPortQuery(text);
                        }

                        Text {
                            text: "Search processes by name, or type ':3000' for port killer..."
                            font.family: "Tahoma"
                            font.pixelSize: 12
                            color: "#92a6b9"
                            visible: !parent.text && !parent.activeFocus
                        }
                    }

                    // Clear button
                    Text {
                        visible: searchInput.text.length > 0
                        text: "✕"
                        font.pixelSize: 12
                        color: "#94a3b8"
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: searchInput.text = ""
                        }
                    }
                }
            }

            Button {
                id: refreshBtn
                Layout.preferredHeight: 34
                Layout.preferredWidth: 80
                background: Rectangle {
                    color: refreshBtn.hovered ? "#e3ebf4" : window.colorCard
                    radius: 4
                    border.color: window.colorCardBorder
                    border.width: 1
                }
                contentItem: Text {
                    text: "Refresh"
                    color: window.colorTextMain
                    font.family: "Tahoma"
                    font.pixelSize: 11
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    processModel.refresh();
                    root.checkPortQuery(searchInput.text);
                }
            }
        }

        // Port Search Match Banner (When :port is typed)
        Rectangle {
            visible: root.searchText.startsWith(":")
            Layout.fillWidth: true
            Layout.preferredHeight: portCol.implicitHeight + 20
            color: "#eef6ff"
            radius: 5
            border.color: "#93c5fd"
            border.width: 1

            ColumnLayout {
                id: portCol
                anchors.fill: parent
                anchors.margins: 10
                spacing: 6

                Text {
                    text: "⚡ PORT-TO-PROCESS KILLER: SEARCH FOR " + root.searchText.toUpperCase()
                    font.family: "Tahoma"
                    font.pixelSize: 11
                    font.bold: true
                    color: "#1d4ed8"
                }

                Repeater {
                    model: root.portResults
                    delegate: Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        color: "#ffffff"
                        radius: 4
                        border.color: "#bfdbfe"

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 6
                            spacing: 10

                            Rectangle {
                                Layout.preferredWidth: 50
                                Layout.preferredHeight: 24
                                color: "#1d4ed8"
                                radius: 3
                                Text {
                                    anchors.centerIn: parent
                                    text: ":" + modelData.port
                                    font.family: "Consolas"; font.pixelSize: 11; font.bold: true; color: "#ffffff"
                                }
                            }

                            Text {
                                text: modelData.name + " (PID: " + modelData.pid + ")"
                                font.family: "Tahoma"; font.pixelSize: 12; font.bold: true; color: "#000000"
                            }

                            Text {
                                text: "Protocol: " + modelData.protocol + "  |  Memory: " + modelData.ramMB.toFixed(1) + " MB"
                                font.family: "Consolas"; font.pixelSize: 11; color: "#555555"
                                Layout.fillWidth: true
                            }

                            Button {
                                Layout.preferredWidth: 80
                                Layout.preferredHeight: 28
                                background: Rectangle {
                                    color: parent.hovered ? "#b91c1c" : "#dc2626"
                                    radius: 3
                                }
                                contentItem: Text {
                                    text: "⚡ Kill"
                                    font.family: "Tahoma"; font.pixelSize: 11; font.bold: true; color: "#ffffff"
                                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                }
                                onClicked: {
                                    portManager.killProcessByPid(modelData.pid);
                                    processModel.refresh();
                                    root.checkPortQuery(searchInput.text);
                                }
                            }
                        }
                    }
                }

                Text {
                    visible: root.portResults.length === 0
                    text: "No active process listening on port " + root.searchText.substring(1)
                    font.family: "Tahoma"
                    font.pixelSize: 11
                    color: "#6b7280"
                }
            }
        }

        // Table Header
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            color: "#d2dce8"
            radius: 3
            border.color: window.colorCardBorder
            border.width: 1

            Row {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10

                Text {
                    width: parent.width * 0.30
                    height: parent.height
                    verticalAlignment: Text.AlignVCenter
                    text: "Process Name (Click to expand Command Line)"
                    font.family: "Tahoma"; font.pixelSize: 11; font.bold: true; color: window.colorTextMain
                }
                Text {
                    width: parent.width * 0.08
                    height: parent.height
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    text: "PID"
                    font.family: "Tahoma"; font.pixelSize: 11; font.bold: true; color: window.colorTextMain
                }
                Text {
                    width: parent.width * 0.08
                    height: parent.height
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignRight
                    text: "CPU"
                    font.family: "Tahoma"; font.pixelSize: 11; font.bold: true; color: window.colorTextMain
                }
                Text {
                    width: parent.width * 0.12
                    height: parent.height
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignRight
                    text: "Working Set"
                    font.family: "Tahoma"; font.pixelSize: 11; font.bold: true; color: window.colorTextMain
                }
                Text {
                    width: parent.width * 0.12
                    height: parent.height
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignRight
                    text: "Private Set"
                    font.family: "Tahoma"; font.pixelSize: 11; font.bold: true; color: window.colorTextMain
                }
                Text {
                    width: parent.width * 0.08
                    height: parent.height
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    text: "Threads"
                    font.family: "Tahoma"; font.pixelSize: 11; font.bold: true; color: window.colorTextMain
                }
                Text {
                    width: parent.width * 0.12
                    height: parent.height
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    text: "Username"
                    font.family: "Tahoma"; font.pixelSize: 11; font.bold: true; color: window.colorTextMain
                }
                Text {
                    width: parent.width * 0.10
                    height: parent.height
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    text: "Priority / State"
                    font.family: "Tahoma"; font.pixelSize: 11; font.bold: true; color: window.colorTextMain
                }
            }
        }

        // ListView container (Pure White base)
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#ffffff"
            radius: 6
            border.color: window.colorCardBorder
            border.width: 1
            clip: true

            ListView {
                id: processListView
                anchors.fill: parent
                anchors.margins: 4
                model: processModel
                spacing: 1
                boundsBehavior: Flickable.StopAtBounds

                ScrollBar.vertical: ScrollBar {
                    active: true
                    policy: ScrollBar.AsNeeded
                }

                delegate: Item {
                    id: delegateItem
                    width: processListView.width
                    
                    property bool matchesFilter: !root.searchText || root.searchText.startsWith(":") || name.toLowerCase().includes(root.searchText.toLowerCase())
                    property bool isExpanded: root.expandedPid === pid
                    
                    height: matchesFilter ? (isExpanded ? 75 : 38) : 0
                    visible: matchesFilter

                    Rectangle {
                        anchors.fill: parent
                        radius: 3
                        color: mouseRowArea.containsMouse ? "#e6eff9" : "transparent"
                        border.color: isExpanded ? "#92a6b9" : "transparent"
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 6
                            spacing: 4

                            // Row elements
                            Row {
                                Layout.fillWidth: true
                                height: 26

                                // Name + Status Badges
                                Row {
                                    width: parent.width * 0.30
                                    height: parent.height
                                    spacing: 6

                                    Rectangle {
                                        width: 6; height: 6; radius: 3
                                        color: cpu > 5.0 ? "#d32f2f" : "#7f9db9"
                                        anchors.verticalCenter: parent.verticalCenter
                                    }

                                    Text {
                                        text: name
                                        font.family: "Tahoma"; font.pixelSize: 12; font.bold: cpu > 5.0
                                        color: window.colorTextMain
                                        elide: Text.ElideRight
                                        anchors.verticalCenter: parent.verticalCenter
                                    }

                                    // Suspended pill badge
                                    Rectangle {
                                        visible: isSuspended
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: 52
                                        height: 16
                                        color: "#fef3c7"
                                        radius: 2
                                        border.color: "#f59e0b"
                                        Text {
                                            anchors.centerIn: parent
                                            text: "PAUSED"
                                            font.family: "Consolas"
                                            font.pixelSize: 8
                                            font.bold: true
                                            color: "#b45309"
                                        }
                                    }

                                    // E-Core pill badge
                                    Rectangle {
                                        visible: isEcoQos
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: 48
                                        height: 16
                                        color: "#ecfdf5"
                                        radius: 2
                                        border.color: "#10b981"
                                        Text {
                                            anchors.centerIn: parent
                                            text: "E-CORE"
                                            font.family: "Consolas"
                                            font.pixelSize: 8
                                            font.bold: true
                                            color: "#047857"
                                        }
                                    }
                                }

                                // PID
                                Text {
                                    width: parent.width * 0.08
                                    height: parent.height
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignHCenter
                                    text: pid.toString()
                                    font.family: "Consolas"; font.pixelSize: 11; color: "#333333"
                                }

                                // CPU Usage
                                Text {
                                    width: parent.width * 0.08
                                    height: parent.height
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignRight
                                    text: cpu > 0.0 ? cpu.toFixed(1) + "%" : "0.0%"
                                    font.family: "Consolas"; font.pixelSize: 12; font.bold: cpu > 10.0
                                    color: cpu > 10.0 ? "#d32f2f" : "#000000"
                                }

                                // RAM Usage (Working Set)
                                Text {
                                    width: parent.width * 0.12
                                    height: parent.height
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignRight
                                    text: ram.toFixed(1) + " MB"
                                    font.family: "Consolas"; font.pixelSize: 11; color: "#000000"
                                }

                                // RAM Private Usage
                                Text {
                                    width: parent.width * 0.12
                                    height: parent.height
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignRight
                                    text: privateUsage.toFixed(1) + " MB"
                                    font.family: "Consolas"; font.pixelSize: 11; color: "#4d5b6e"
                                }

                                // Thread Count
                                Text {
                                    width: parent.width * 0.08
                                    height: parent.height
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignHCenter
                                    text: threads.toString()
                                    font.family: "Consolas"; font.pixelSize: 11; color: "#333333"
                                }

                                // Username
                                Text {
                                    width: parent.width * 0.12
                                    height: parent.height
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignHCenter
                                    text: username
                                    font.family: "Tahoma"; font.pixelSize: 11; color: "#333333"; elide: Text.ElideRight
                                }

                                // Priority
                                Text {
                                    width: parent.width * 0.10
                                    height: parent.height
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignHCenter
                                    text: priority
                                    font.family: "Tahoma"; font.pixelSize: 11
                                    font.bold: priority !== "Normal"
                                    color: priority === "High" || priority === "Realtime" ? "#d32f2f" : "#000000"
                                }
                            }

                            // Command line expansion panel
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 30
                                color: "#f5f6f8"
                                visible: isExpanded
                                border.color: "#e2ebf5"
                                radius: 2

                                Text {
                                    anchors.fill: parent
                                    anchors.margins: 6
                                    text: "Command Line: " + cmdLine
                                    font.family: "Consolas"
                                    font.pixelSize: 10
                                    color: "#555555"
                                    elide: Text.ElideMiddle
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }
                        }

                        MouseArea {
                            id: mouseRowArea
                            anchors.fill: parent
                            hoverEnabled: true
                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                            
                            onClicked: (mouse) => {
                                if (mouse.button === Qt.RightButton) {
                                    processContextMenu.targetPid = pid;
                                    processContextMenu.targetName = name;
                                    processContextMenu.popup();
                                } else {
                                    // Left click toggle command line panel
                                    if (root.expandedPid === pid) {
                                        root.expandedPid = -1;
                                    } else {
                                        root.expandedPid = pid;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
