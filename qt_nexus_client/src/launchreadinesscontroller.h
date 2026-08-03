#pragma once

#include <QObject>
#include <QStringList>

class GameLaunchOverlayWindow;
class SiegeProcessWatcher;

/**
 * Coordinates two independent readiness conditions:
 *   1. The NEXUS client/backend has finished initialization.
 *   2. A configured Rainbow Six Siege process has been detected.
 *
 * The compact overlay remains completely hidden until condition 2 is true.
 * Progress reaches 100% only when both conditions are true.
 */
class LaunchReadinessController final : public QObject {
    Q_OBJECT

public:
    explicit LaunchReadinessController(
        GameLaunchOverlayWindow* overlay,
        QObject* parent = nullptr
    );

    void setExecutableNames(const QStringList& names);
    void beginWaitingForSiege();
    void setClientReady(bool ready);
    void stop();

    [[nodiscard]] bool clientReady() const;
    [[nodiscard]] bool siegeDetected() const;
    [[nodiscard]] qint64 siegePid() const;

Q_SIGNALS:
    void waitingForSiege();
    void siegeProcessDetected(qint64 pid, const QString& executableName);
    void siegeProcessLost(qint64 previousPid);
    void launchReady(qint64 siegePid);
    void errorOccurred(const QString& message);

private:
    void finishIfReady();

    GameLaunchOverlayWindow* m_overlay = nullptr;
    SiegeProcessWatcher* m_watcher = nullptr;
    bool m_waiting = false;
    bool m_clientReady = false;
    bool m_siegeDetected = false;
    bool m_completionScheduled = false;
    qint64 m_siegePid = 0;
};
