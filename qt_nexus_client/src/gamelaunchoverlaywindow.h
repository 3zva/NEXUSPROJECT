#pragma once

#include <QWidget>

class NexusProgressView;
class QScreen;

/**
 * Opaque, frameless, non-activating launch overlay.
 *
 * It is deliberately hidden until the Siege process watcher reports a PID.
 * The window does not read game memory, inject code, or hook game input.
 */
class GameLaunchOverlayWindow final : public QWidget {
    Q_OBJECT

public:
    explicit GameLaunchOverlayWindow(QWidget* parent = nullptr);

    void resetForWaiting();
    void showForDetectedProcess(
        qint64 pid,
        const QString& executableName,
        QScreen* preferredScreen = nullptr
    );
    void setClientReady(bool ready);
    void setProgress(int percent, const QString& status, const QString& stage);
    void complete();
    void hideImmediately();

    [[nodiscard]] NexusProgressView* progressView() const;

private:
    void centerOnScreen(QScreen* screen);

    NexusProgressView* m_progressView = nullptr;
    qint64 m_pid = 0;
    QString m_executableName;
};
