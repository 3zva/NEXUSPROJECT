#include "mainwindow.h"
#include "theme.h"

#include <QApplication>
#include <QCoreApplication>

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("NEXUS"));
    QCoreApplication::setApplicationName(QStringLiteral("NEXUS Client"));
    QCoreApplication::setApplicationVersion(QString::fromLatin1(NEXUS_APP_VERSION));

    application.setStyle(QStringLiteral("Fusion"));
    application.setStyleSheet(NexusTheme::globalStyleSheet());

    MainWindow window;
    window.show();
    return application.exec();
}
