import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 1160
    height: 780
    visible: true
    title: "RathonWare - Hardware Diagnostics, AI/GPU Telemetry & Task Manager"
    color: "#0d0e12"

    // Y2K Retro Light Blue Design System
    readonly property color colorBg: "#dbe3ee"         // Soft classic desktop gray-blue
    readonly property color colorCard: "#f0f4f9"       // Classic window face
    readonly property color colorCardBorder: "#92a6b9" // Windows frame border
    readonly property color colorCpu: "#0a246a"        // Classic active title-bar blue
    readonly property color colorGpu: "#008080"        // Retro teal
    readonly property color colorRam: "#800080"        // Retro purple
    readonly property color colorTextMain: "#000000"   // High contrast black text
    readonly property color colorTextSec: "#4d5b6e"    // Windows secondary label

    property string activePage: "dashboard"

    // Global Command Palette Shortcuts
    Shortcut {
        sequence: "Ctrl+K"
        onActivated: commandPalette.open()
    }
    Shortcut {
        sequence: "Ctrl+F"
        onActivated: commandPalette.open()
    }

    // Custom Navigation Button Component (Retro Bevel/Flat Style)
    component NavButton : Item {
        id: navBtn
        property string label: ""
        property string iconText: ""
        property bool isActive: false
        property color accentColor: "#0a246a"
        signal clicked()

        width: parent.width
        height: 38

        Rectangle {
            anchors.fill: parent
            radius: 4
            color: navBtn.isActive ? "#ffffff" : (mouseArea.containsMouse ? "#e7eef6" : "transparent")
            border.color: navBtn.isActive ? "#92a6b9" : "transparent"
            border.width: 1

            // Active side marker indicator
            Rectangle {
                width: 4
                height: 16
                radius: 1
                anchors.left: parent.left
                anchors.leftMargin: 6
                anchors.verticalCenter: parent.verticalCenter
                color: navBtn.accentColor
                visible: navBtn.isActive
            }

            Row {
                anchors.left: parent.left
                anchors.leftMargin: navBtn.isActive ? 18 : 14
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8

                Text {
                    text: navBtn.iconText
                    font.pixelSize: 11
                    visible: navBtn.iconText !== ""
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    text: navBtn.label
                    font.family: "Tahoma"
                    font.pixelSize: 11
                    font.bold: navBtn.isActive
                    color: navBtn.isActive ? "#0a246a" : (mouseArea.containsMouse ? "#000000" : "#4d5b6e")
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: navBtn.clicked()
        }
    }

    Row {
        anchors.fill: parent
        spacing: 0

        // Sidebar Navigation
        Rectangle {
            id: sidebar
            width: 220
            height: parent.height
            color: "#eceef3"

            // Border on the right
            Rectangle {
                width: 1
                height: parent.height
                anchors.right: parent.right
                color: "#92a6b9"
            }

            Column {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 14

                // Header / Branding (Retro active title-bar gradient feel)
                Rectangle {
                    width: parent.width
                    height: 52
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: "#0a246a" }
                        GradientStop { position: 1.0; color: "#a6caf0" }
                    }
                    radius: 3

                    Column {
                        anchors.fill: parent
                        anchors.margins: 8
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 2

                        Text {
                            text: "RATHONWARE"
                            font.family: "Tahoma"
                            font.pixelSize: 15
                            font.bold: true
                            color: "#ffffff"
                        }

                        Text {
                            text: "SUPERPOWER MONITOR"
                            font.family: "Tahoma"
                            font.pixelSize: 8
                            font.bold: true
                            color: "#dbe3ee"
                            font.letterSpacing: 1
                        }
                    }
                }

                // Divider
                Rectangle {
                    width: parent.width
                    height: 1
                    color: "#b2c3d4"
                }

                // Nav Links
                Column {
                    width: parent.width
                    spacing: 4

                    NavButton {
                        label: "DASHBOARD"
                        iconText: "📊"
                        isActive: window.activePage === "dashboard"
                        accentColor: window.colorCpu
                        onClicked: window.activePage = "dashboard"
                    }

                    NavButton {
                        label: "AI & GPU TELEMETRY"
                        iconText: "⚡"
                        isActive: window.activePage === "gpu"
                        accentColor: window.colorGpu
                        onClicked: window.activePage = "gpu"
                    }

                    NavButton {
                        label: "CPU TOPOLOGY (P/E)"
                        iconText: "🎛️"
                        isActive: window.activePage === "topology"
                        accentColor: window.colorCpu
                        onClicked: window.activePage = "topology"
                    }

                    NavButton {
                        label: "TASK MANAGER"
                        iconText: "⚙️"
                        isActive: window.activePage === "tasks"
                        accentColor: window.colorGpu
                        onClicked: window.activePage = "tasks"
                    }

                    NavButton {
                        label: "SENSORS"
                        iconText: "🌡️"
                        isActive: window.activePage === "sensors"
                        accentColor: window.colorGpu
                        onClicked: window.activePage = "sensors"
                    }

                    NavButton {
                        label: "PERF GRAPHS"
                        iconText: "📈"
                        isActive: window.activePage === "graphs"
                        accentColor: window.colorCpu
                        onClicked: window.activePage = "graphs"
                    }

                    NavButton {
                        label: "HARDWARE INFO"
                        iconText: "💻"
                        isActive: window.activePage === "specs"
                        accentColor: window.colorRam
                        onClicked: window.activePage = "specs"
                    }

                    NavButton {
                        label: "ALERTS"
                        iconText: "🔔"
                        isActive: window.activePage === "alerts"
                        accentColor: window.colorRam
                        onClicked: window.activePage = "alerts"
                    }
                }
            }
        }

        // Main Content Area
        Rectangle {
            width: parent.width - sidebar.width
            height: parent.height
            color: window.colorBg

            // Top Status Bar
            Rectangle {
                id: topBar
                width: parent.width
                height: 52
                color: "#eceef3"
                anchors.top: parent.top

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 20
                    anchors.rightMargin: 20
                    spacing: 15

                    Text {
                        text: window.activePage === "dashboard" ? "System Monitor Dashboard" :
                              window.activePage === "gpu" ? "Modern AI & GPU Telemetry (NVML Superpowers)" :
                              window.activePage === "topology" ? "CPU Topology & Intel P-Core / E-Core Manager" :
                              window.activePage === "sensors" ? "Detailed Hardware Sensors" :
                              window.activePage === "tasks" ? "Active Running Processes & Port Killer" :
                              window.activePage === "graphs" ? "Performance History Charts" :
                              window.activePage === "specs" ? "Hardware Component Details" : "Sensor Alert Configuration"
                        font.family: "Tahoma"
                        font.pixelSize: 14
                        font.bold: true
                        color: window.colorTextMain
                    }

                    Item { Layout.fillWidth: true }

                    // Command Palette Trigger Search Box
                    Rectangle {
                        Layout.preferredWidth: 220
                        Layout.preferredHeight: 30
                        color: "#ffffff"
                        radius: 4
                        border.color: "#92a6b9"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            spacing: 6

                            Text { text: "🔍"; font.pixelSize: 11 }
                            Text {
                                text: "Type ':3000' or Search..."
                                font.family: "Tahoma"
                                font.pixelSize: 10
                                color: "#8fa3b8"
                                Layout.fillWidth: true
                            }
                            Rectangle {
                                Layout.preferredWidth: 46
                                Layout.preferredHeight: 18
                                color: "#e2ebf5"
                                radius: 2
                                Text {
                                    anchors.centerIn: parent
                                    text: "Ctrl + K"
                                    font.family: "Consolas"
                                    font.pixelSize: 9
                                    font.bold: true
                                    color: "#4d5b6e"
                                }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: commandPalette.open()
                        }
                    }

                    // Unlock File Tool Button
                    Button {
                        Layout.preferredHeight: 30
                        implicitWidth: 100
                        background: Rectangle {
                            color: parent.hovered ? "#e2ebf5" : "#ffffff"
                            radius: 4
                            border.color: "#92a6b9"
                        }
                        contentItem: RowLayout {
                            anchors.centerIn: parent
                            spacing: 4
                            Text { text: "🔓"; font.pixelSize: 11 }
                            Text {
                                text: "Unlock File"
                                font.family: "Tahoma"; font.pixelSize: 10; font.bold: true; color: "#0a246a"
                            }
                        }
                        onClicked: fileUnlockerDialog.open()
                    }
                }
            }

            // Divider
            Rectangle {
                id: topDivider
                width: parent.width
                height: 1
                color: "#92a6b9"
                anchors.top: topBar.bottom
            }

            // Active Page Loader
            Loader {
                id: pageLoader
                anchors.top: topDivider.bottom
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 18
                source: window.activePage === "dashboard" ? "DashboardPage.qml" :
                        window.activePage === "gpu" ? "GpuTelemetryPage.qml" :
                        window.activePage === "topology" ? "CpuTopologyPage.qml" :
                        window.activePage === "sensors" ? "SensorsPage.qml" :
                        window.activePage === "tasks" ? "ProcessPage.qml" :
                        window.activePage === "graphs" ? "GraphsPage.qml" :
                        window.activePage === "specs" ? "SpecsPage.qml" : "AlertsPage.qml"
            }
        }
    }

    // Hosted Overlays
    CommandPalette {
        id: commandPalette
    }

    FileUnlockerDialog {
        id: fileUnlockerDialog
    }
}
