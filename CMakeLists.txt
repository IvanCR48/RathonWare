cmake_minimum_required(VERSION 3.16)

project(RathonWare LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Target Windows 10 / 11 to expose modern Win32 APIs (e.g. MIB_IF_TABLE2)
add_compile_definitions(_WIN32_WINNT=0x0A00 WINVER=0x0A00)

# Qt 6 configuration
find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_standard_project_setup()

# Source files
set(SOURCES
    src/main.cpp
    src/system_monitor.h
    src/system_monitor.cpp
    src/process_manager.h
    src/process_manager.cpp
    src/process_model.h
    src/process_model.cpp
    src/resources.rc
)

# QML files
set(QML_FILES
    qml/main.qml
    qml/DashboardPage.qml
    qml/ProcessPage.qml
    qml/SpecsPage.qml
    qml/SensorsPage.qml
    qml/GraphsPage.qml
    qml/AlertsPage.qml
)

qt_add_executable(RathonWare
    ${SOURCES}
)

# Set QML module name
qt_add_qml_module(RathonWare
    URI RathonWare
    VERSION 1.0
    QML_FILES ${QML_FILES}
)

# Windows specific library linkings
if(WIN32)
    target_link_libraries(RathonWare PRIVATE pdh psapi iphlpapi ole32 oleaut32 wbemuuid)
endif()

target_link_libraries(RathonWare PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)

# Compiler flags
if(MSVC)
    target_compile_options(RathonWare PRIVATE /W4)
else()
    target_compile_options(RathonWare PRIVATE -Wall -Wextra)
endif()
