import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window
    width: 1100
    height: 750
    visible: true
    title: "RathonWare - Hardware Diagnostics & Task Manager"
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

    // Custom Navigation Button Component (Retro Bevel/Flat Style)
    component NavButton : Item {
        id: navBtn
        property string label: ""
        property bool isActive: false
        property color accentColor: "#0a246a"
        signal clicked()

        width: parent.width
        height: 40

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

            Text {
                text: navBtn.label
                anchors.left: parent.left
                anchors.leftMargin: navBtn.isActive ? 18 : 14
                anchors.verticalCenter: parent.verticalCenter
                font.family: "Tahoma"
                font.pixelSize: 12
                font.bold: navBtn.isActive
                color: navBtn.isActive ? "#0a246a" : (mouseArea.containsMouse ? "#000000" : "#4d5b6e")
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
                anchors.margins: 15
                spacing: 20

                // Header / Branding (Retro active title-bar gradient feel)
                Rectangle {
                    width: parent.width
                    height: 54
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
                            font.pixelSize: 16
                            font.bold: true
                            color: "#ffffff"
                        }

                        Text {
                            text: "PC PERFORMANCE GRAPH"
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
                    spacing: 6

                    NavButton {
                        label: "DASHBOARD"
                        isActive: window.activePage === "dashboard"
                        accentColor: window.colorCpu
                        onClicked: window.activePage = "dashboard"
                    }

                    NavButton {
                        label: "SENSORS"
                        isActive: window.activePage === "sensors"
                        accentColor: window.colorGpu
                        onClicked: window.activePage = "sensors"
                    }

                    NavButton {
                        label: "TASK MANAGER"
                        isActive: window.activePage === "tasks"
                        accentColor: window.colorGpu
                        onClicked: window.activePage = "tasks"
                    }

                    NavButton {
                        label: "PERF GRAPHS"
                        isActive: window.activePage === "graphs"
                        accentColor: window.colorCpu
                        onClicked: window.activePage = "graphs"
                    }

                    NavButton {
                        label: "HARDWARE INFO"
                        isActive: window.activePage === "specs"
                        accentColor: window.colorRam
                        onClicked: window.activePage = "specs"
                    }

                    NavButton {
                        label: "ALERTS"
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
                height: 50
                color: "#eceef3"
                anchors.top: parent.top

                Text {
                    text: window.activePage === "dashboard" ? "System Monitor Dashboard" :
                          window.activePage === "sensors" ? "Detailed Hardware Sensors" :
                          window.activePage === "tasks" ? "Active Running Processes" :
                          window.activePage === "graphs" ? "Performance History Charts" :
                          window.activePage === "specs" ? "Hardware Component Details" : "Sensor Alert Configuration"
                    font.family: "Tahoma"
                    font.pixelSize: 15
                    font.bold: true
                    color: window.colorTextMain
                    anchors.left: parent.left
                    anchors.leftMargin: 20
                    anchors.verticalCenter: parent.verticalCenter
                }

                // System Model text display
                Text {
                    text: "Uptime: " + systemMonitor.uptime + "  |  " + systemMonitor.cpuModel
                    font.family: "Tahoma"
                    font.pixelSize: 11
                    color: window.colorTextSec
                    anchors.right: parent.right
                    anchors.rightMargin: 20
                    anchors.verticalCenter: parent.verticalCenter
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
                anchors.margins: 20
                source: window.activePage === "dashboard" ? "DashboardPage.qml" :
                        window.activePage === "sensors" ? "SensorsPage.qml" :
                        window.activePage === "tasks" ? "ProcessPage.qml" :
                        window.activePage === "graphs" ? "GraphsPage.qml" :
                        window.activePage === "specs" ? "SpecsPage.qml" : "AlertsPage.qml"
            }
        }
    }
}
