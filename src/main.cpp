#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "system_monitor.h"
#include "process_model.h"

int main(int argc, char *argv[])
{
    // Enable High DPI scaling
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Round);

    QGuiApplication app(argc, argv);

    app.setOrganizationName("Rathon");
    app.setApplicationName("RathonWare");

    QQmlApplicationEngine engine;

    // Instantiate backend controllers
    SystemMonitor monitor;
    ProcessModel processModel;

    // Expose controllers to QML engine
    engine.rootContext()->setContextProperty("systemMonitor", &monitor);
    engine.rootContext()->setContextProperty("processModel", &processModel);

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
