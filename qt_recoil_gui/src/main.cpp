#include "mainwindow.h"
#include "theme.h"

#include <QApplication>
#include <QCoreApplication>
#include <QStringList>

#ifdef Q_OS_WIN
#include <windows.h>

namespace {
void enableProcessDpiAwareness() {
    using SetDpiAwarenessContextFn = BOOL(WINAPI*)(HANDLE);
    HMODULE user32 = LoadLibraryW(L"user32.dll");
    if (user32 != nullptr) {
        auto setDpiAwarenessContext = reinterpret_cast<SetDpiAwarenessContextFn>(
            GetProcAddress(user32, "SetProcessDpiAwarenessContext")
        );
        if (setDpiAwarenessContext != nullptr
            && setDpiAwarenessContext(reinterpret_cast<HANDLE>(-4))) {
            FreeLibrary(user32);
            return;
        }
        FreeLibrary(user32);
    }
    SetProcessDPIAware();
}
}
#endif

int main(int argc, char* argv[]) {
#ifdef Q_OS_WIN
    enableProcessDpiAwareness();
#endif
    QApplication application(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("NEXUS"));
    QCoreApplication::setApplicationName(QStringLiteral("NEXUS Client"));
    QCoreApplication::setApplicationVersion(QString::fromLatin1(NEXUS_APP_VERSION));

    application.setStyle(QStringLiteral("Fusion"));
    application.setStyleSheet(NexusTheme::globalStyleSheet());

    MainWindow window;
    const QStringList arguments = QCoreApplication::arguments();
    const int titleIndex = arguments.indexOf(QStringLiteral("--window-title"));
    if (titleIndex >= 0 && titleIndex + 1 < arguments.size()) {
        window.setWindowTitle(arguments.at(titleIndex + 1));
    }
    window.show();
    return application.exec();
}
