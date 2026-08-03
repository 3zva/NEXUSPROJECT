#include "gamelaunchoverlaywindow.h"
#include "nexusprogressview.h"
#include "theme.h"

#include <QApplication>
#include <QGuiApplication>
#include <QCursor>
#include <QHBoxLayout>
#include <QScreen>
#include <QTimer>

GameLaunchOverlayWindow::GameLaunchOverlayWindow(QWidget* parent)
    : QWidget(parent) {
    setObjectName(QStringLiteral("gameLaunchOverlayWindow"));
    setWindowTitle(QStringLiteral("NEXUS Launch Readiness"));
    setWindowIcon(NexusTheme::icon(QStringLiteral("nexus_logo_64.png")));

    setWindowFlags(
        Qt::FramelessWindowHint
        | Qt::Tool
        | Qt::WindowStaysOnTopHint
        | Qt::WindowDoesNotAcceptFocus
        | Qt::WindowTransparentForInput
    );
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAutoFillBackground(true);
    setFixedSize(620, 340);

    setStyleSheet(QStringLiteral(R"QSS(
        QWidget#gameLaunchOverlayWindow {
            background: #070A12;
            border: 1px solid #765BFF;
            border-radius: 18px;
        }
    )QSS"));

    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(1, 1, 1, 1);
    m_progressView = new NexusProgressView(NexusProgressMode::GameLaunch, this);
    root->addWidget(m_progressView);

    hide();
}

void GameLaunchOverlayWindow::resetForWaiting() {
    m_pid = 0;
    m_executableName.clear();
    m_progressView->reset();
    m_progressView->setActionVisible(false);
    hide();
}

void GameLaunchOverlayWindow::showForDetectedProcess(
    qint64 pid,
    const QString& executableName,
    QScreen* preferredScreen
) {
    m_pid = pid;
    m_executableName = executableName;

    m_progressView->reset();
    m_progressView->setStage(QStringLiteral("GAME DETECTED"));
    m_progressView->setStatus(QStringLiteral("Rainbow Six Siege detected."));
    m_progressView->setDetail(
        QStringLiteral("%1 · PID %2").arg(executableName).arg(pid)
    );
    m_progressView->setProgress(64, false);

    Q_UNUSED(preferredScreen);
    hide();
}

void GameLaunchOverlayWindow::setClientReady(bool ready) {
    if (!isVisible()) {
        return;
    }

    if (ready) {
        m_progressView->setStage(QStringLiteral("FINALIZING"));
        m_progressView->setStatus(QStringLiteral("NEXUS client is ready. Finalizing launch..."));
        m_progressView->setProgress(88, true);
    } else {
        m_progressView->setStage(QStringLiteral("CLIENT STARTUP"));
        m_progressView->setStatus(QStringLiteral("Rainbow Six Siege is running. Waiting for NEXUS..."));
        m_progressView->setProgress(72, true);
    }
}

void GameLaunchOverlayWindow::setProgress(
    int percent,
    const QString& status,
    const QString& stage
) {
    if (!stage.isEmpty()) {
        m_progressView->setStage(stage);
    }
    if (!status.isEmpty()) {
        m_progressView->setStatus(status);
    }
    m_progressView->setProgress(percent, true);
}

void GameLaunchOverlayWindow::complete() {
    if (!isVisible()) {
        return;
    }

    m_progressView->setComplete(
        QStringLiteral("NEXUS and Rainbow Six Siege are ready.")
    );
}

void GameLaunchOverlayWindow::hideImmediately() {
    hide();
}

NexusProgressView* GameLaunchOverlayWindow::progressView() const {
    return m_progressView;
}

void GameLaunchOverlayWindow::centerOnScreen(QScreen* screen) {
    QScreen* target = screen;
    if (target == nullptr) {
        target = QGuiApplication::screenAt(QCursor::pos());
    }
    if (target == nullptr) {
        target = QGuiApplication::primaryScreen();
    }
    if (target == nullptr) {
        return;
    }

    const QRect available = target->availableGeometry();
    move(
        available.center().x() - width() / 2,
        available.center().y() - height() / 2
    );
}
