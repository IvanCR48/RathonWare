import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Item {
    id: root
    anchors.fill: parent

    property string searchText: ""
    property int expandedPid: -1 // Track which row is expanded to show command line

    // Context Menu for right-click process actions
    Menu {
        id: processContextMenu
        property int targetPid: 0
        property string targetName: ""

        MenuItem {
            text: "End Task (Kill)"
            onTriggered: processModel.killProcess(processContextMenu.targetPid)
        }
        
        Menu {
            title: "Set Priority Class"
            MenuItem { text: "Realtime"; onTriggered: processModel.setPriority(processContextMenu.targetPid, 5) }
            MenuItem { text: "High"; onTriggered: processModel.setPriority(processContextMenu.targetPid, 4) }
            MenuItem { text: "Above Normal"; onTriggered: processModel.setPriority(processContextMenu.targetPid, 3) }
            MenuItem { text: "Normal"; onTriggered: processModel.setPriority(processContextMenu.targetPid, 2) }
            MenuItem { text: "Below Normal"; onTriggered: processModel.setPriority(processContextMenu.targetPid, 1) }
            MenuItem { text: "Idle"; onTriggered: processModel.setPriority(processContextMenu.targetPid, 0) }
        }

        MenuItem {
            text: "Suspend Execution"
            onTriggered: processModel.suspendProcess(processContextMenu.targetPid)
        }

        MenuItem {
            text: "Resume Execution"
            onTriggered: processModel.resumeProcess(processContextMenu.targetPid)
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
                Layout.preferredHeight: 32
                color: "#ffffff"
                radius: 3
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
                        
                        onTextChanged: root.searchText = text

                        Text {
                            text: "Search processes by executable name..."
                            font.family: "Tahoma"
                            font.pixelSize: 12
                            color: "#92a6b9"
                            visible: !parent.text && !parent.activeFocus
                        }
                    }
                }
            }

            Button {
                id: refreshBtn
                Layout.preferredHeight: 32
                Layout.preferredWidth: 80
                background: Rectangle {
                    color: refreshBtn.hovered ? "#e3ebf4" : window.colorCard
                    radius: 3
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
                onClicked: processModel.refresh()
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
                    text: "Priority"
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
                    
                    property bool matchesFilter: !root.searchText || name.toLowerCase().includes(root.searchText.toLowerCase())
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

                                // Name
                                Row {
                                    width: parent.width * 0.30
                                    height: parent.height
                                    spacing: 8

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
