import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Popup {
    id: unlockerDialog
    width: 650
    height: 460
    anchors.centerIn: parent
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    background: Rectangle {
        color: "#f8fafc"
        radius: 8
        border.color: "#92a6b9"
        border.width: 2
    }

    property var lockingProcesses: []
    property string statusMessage: ""
    property bool isSuccess: false

    function inspectPath(path) {
        statusMessage = "";
        lockingProcesses = fileUnlocker.findLockingProcesses(path);
        if (lockingProcesses.length === 0) {
            statusMessage = "✅ No active process locks detected on this file/folder.";
            isSuccess = true;
        } else {
            statusMessage = "⚠️ Found " + lockingProcesses.length + " process(es) holding locks on this resource.";
            isSuccess = false;
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        // Header
        RowLayout {
            Layout.fillWidth: true
            Text {
                text: "🔓 FILE & FOLDER UNLOCKER"
                font.family: "Tahoma"
                font.pixelSize: 14
                font.bold: true
                color: "#0a246a"
            }
            Item { Layout.fillWidth: true }
            Button {
                text: "✕"
                Layout.preferredWidth: 28
                Layout.preferredHeight: 28
                background: Rectangle { color: parent.hovered ? "#fee2e2" : "transparent"; radius: 4 }
                contentItem: Text { text: "✕"; font.bold: true; color: "#6b7280"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                onClicked: unlockerDialog.close()
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#cbd5e1" }

        // Drag and Drop Zone
        Rectangle {
            id: dropZone
            Layout.fillWidth: true
            Layout.preferredHeight: 70
            color: dropArea.containsDrag ? "#e0f2fe" : "#f1f5f9"
            radius: 6
            border.color: dropArea.containsDrag ? "#0284c7" : "#94a3b8"
            border.width: dropArea.containsDrag ? 2 : 1

            DropArea {
                id: dropArea
                anchors.fill: parent
                onDropped: (drop) => {
                    if (drop.hasUrls && drop.urls.length > 0) {
                        var fileUrl = drop.urls[0].toString();
                        pathInput.text = fileUrl;
                        unlockerDialog.inspectPath(fileUrl);
                    }
                }
            }

            Column {
                anchors.centerIn: parent
                spacing: 4
                Text {
                    text: "📁 Drag and Drop Locked File or Folder Here"
                    font.family: "Tahoma"
                    font.pixelSize: 12
                    font.bold: true
                    color: dropArea.containsDrag ? "#0369a1" : "#475569"
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Text {
                    text: "or enter the full path below to inspect and release handles"
                    font.family: "Tahoma"
                    font.pixelSize: 10
                    color: "#64748b"
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        }

        // Path input
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 34
                color: "#ffffff"
                radius: 4
                border.color: "#94a3b8"
                border.width: 1

                TextInput {
                    id: pathInput
                    anchors.fill: parent
                    anchors.margins: 8
                    font.family: "Tahoma"
                    font.pixelSize: 11
                    color: "#000000"
                    clip: true
                    activeFocusOnTab: true
                    onAccepted: unlockerDialog.inspectPath(text)

                    Text {
                        text: "e.g. C:\\Program Files\\app\\locked_file.dll"
                        font.family: "Tahoma"
                        font.pixelSize: 11
                        color: "#94a3b8"
                        visible: !parent.text && !parent.activeFocus
                    }
                }
            }

            Button {
                Layout.preferredWidth: 100
                Layout.preferredHeight: 34
                background: Rectangle {
                    color: parent.hovered ? "#006666" : "#008080"
                    radius: 4
                }
                contentItem: Text {
                    text: "Inspect Locks"
                    font.family: "Tahoma"; font.pixelSize: 11; font.bold: true; color: "#ffffff"
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
                onClicked: unlockerDialog.inspectPath(pathInput.text)
            }
        }

        // Status banner
        Text {
            visible: unlockerDialog.statusMessage !== ""
            text: unlockerDialog.statusMessage
            font.family: "Tahoma"
            font.pixelSize: 11
            font.bold: true
            color: unlockerDialog.isSuccess ? "#059669" : "#b91c1c"
        }

        // Locking processes list
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#ffffff"
            radius: 5
            border.color: "#cbd5e1"
            border.width: 1
            clip: true

            ListView {
                id: lockerList
                anchors.fill: parent
                anchors.margins: 6
                model: unlockerDialog.lockingProcesses
                spacing: 4

                delegate: Rectangle {
                    width: lockerList.width
                    height: 48
                    color: "#f8fafc"
                    radius: 4
                    border.color: "#e2ebf5"

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 10

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: modelData.name + (modelData.appName ? " (" + modelData.appName + ")" : "")
                                font.family: "Tahoma"
                                font.pixelSize: 12
                                font.bold: true
                                color: "#000000"
                            }
                            Text {
                                text: "PID: " + modelData.pid + "  |  RAM: " + modelData.ramMB.toFixed(1) + " MB"
                                font.family: "Consolas"
                                font.pixelSize: 10
                                color: "#64748b"
                            }
                        }

                        Button {
                            Layout.preferredWidth: 90
                            Layout.preferredHeight: 28
                            background: Rectangle {
                                color: parent.hovered ? "#b91c1c" : "#dc2626"
                                radius: 3
                            }
                            contentItem: Text {
                                text: "Kill Process"
                                font.family: "Tahoma"; font.pixelSize: 10; font.bold: true; color: "#ffffff"
                                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: {
                                fileUnlocker.killLockingProcess(modelData.pid);
                                unlockerDialog.inspectPath(pathInput.text);
                                processModel.refresh();
                            }
                        }
                    }
                }
            }

            Text {
                visible: lockerList.count === 0
                anchors.centerIn: parent
                text: "No locking processes loaded. Drop a file or enter a path above."
                font.family: "Tahoma"
                font.pixelSize: 11
                color: "#94a3b8"
            }
        }

        // Bottom Action Bar
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Item { Layout.fillWidth: true }

            Button {
                visible: unlockerDialog.lockingProcesses.length > 0
                Layout.preferredHeight: 34
                implicitWidth: 200
                background: Rectangle {
                    color: parent.hovered ? "#7f1d1d" : "#991b1b"
                    radius: 4
                }
                contentItem: Text {
                    text: "⚡ Unlock File (Kill All Locks)"
                    font.family: "Tahoma"; font.pixelSize: 11; font.bold: true; color: "#ffffff"
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    fileUnlocker.unlockFile(pathInput.text);
                    unlockerDialog.inspectPath(pathInput.text);
                    processModel.refresh();
                }
            }

            Button {
                Layout.preferredWidth: 80
                Layout.preferredHeight: 34
                background: Rectangle {
                    color: parent.hovered ? "#e2ebf5" : "#f1f5f9"
                    radius: 4
                    border.color: "#cbd5e1"
                }
                contentItem: Text {
                    text: "Close"
                    font.family: "Tahoma"; font.pixelSize: 11; font.bold: true; color: "#475569"
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
                onClicked: unlockerDialog.close()
            }
        }
    }
}
