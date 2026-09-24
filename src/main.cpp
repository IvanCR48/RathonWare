#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QIcon>
#include "system_monitor.h"
#include "process_model.h"
#include "port_manager.h"
#include "file_unlocker.h"
#include "gpu_monitor.h"
#include "cpu_topology.h"

int main(int argc, char *argv[])
{
    // Fix High DPI fractional scaling fuzziness on modern Windows displays (125%, 150%, 175%).
    // Rounding policy ensures UI edges snap cleanly to physical device pixels.
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Round);

    QGuiApplication app(argc, argv);

    // Qt Quick Styling Gotcha:
    // By default, Qt Quick Controls picks up the native "Windows" or "Fusion" platform style.
    // Those styles override custom Item delegates and complain with QML console warnings whenever
    // custom background or contentItem properties are bound.
    // Setting style to "Basic" gives our custom retro Y2K theme 100% control over every pixel.
    QQuickStyle::setStyle("Basic");

    app.setWindowIcon(QIcon(":/RathonWare/assets/logo.png"));
    app.setOrganizationName("Rathon");
    app.setApplicationName("RathonWare");

    QQmlApplicationEngine engine;

    // Composition Root: Instantiate C++20 Win32 backend controllers.
    // Kept alive on the main thread stack for the lifetime of the application.
    SystemMonitor monitor;
    ProcessModel processModel;
    PortManager portManager;
    FileUnlocker fileUnlocker;

    // GPU & NVML Superpowers controllers
    GpuProcessModel gpuProcessModel;
    GpuMonitor gpuMonitor(&gpuProcessModel);

    // CPU Topology & P/E-Core controllers
    CpuCoreModel cpuCoreModel;
    CpuTopology cpuTopology(&cpuCoreModel);

    // Expose controllers to QML engine
    engine.rootContext()->setContextProperty("systemMonitor", &monitor);
    engine.rootContext()->setContextProperty("processModel", &processModel);
    engine.rootContext()->setContextProperty("portManager", &portManager);
    engine.rootContext()->setContextProperty("fileUnlocker", &fileUnlocker);
    engine.rootContext()->setContextProperty("gpuProcessModel", &gpuProcessModel);
    engine.rootContext()->setContextProperty("gpuMonitor", &gpuMonitor);
    engine.rootContext()->setContextProperty("cpuCoreModel", &cpuCoreModel);
    engine.rootContext()->setContextProperty("cpuTopology", &cpuTopology);

    // Qt 6 QML module path layout with CMake standard prefix
    const QUrl url(QStringLiteral("qrc:/RathonWare/qml/main.qml"));
    
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    
    engine.load(url);

    return app.exec();
}
