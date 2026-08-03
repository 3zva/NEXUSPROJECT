#include "launchreadinesscontroller.h"
#include "gamelaunchoverlaywindow.h"
#include "siegeprocesswatcher.h"

#include <QTimer>

LaunchReadinessController::LaunchReadinessController(
    GameLaunchOverlayWindow* overlay,
    QObject* parent
)
    : QObject(parent),
      m_overlay(overlay),
      m_watcher(new SiegeProcessWatcher(this)) {
    Q_ASSERT(m_overlay != nullptr);

    connect(m_watcher, &SiegeProcessWatcher::processFound,
            this, [this](qint64 pid, const QString& executableName) {
        if (!m_waiting) {
            return;
        }

        m_siegeDetected = true;
        m_siegePid = pid;
        m_completionScheduled = false;

        // Required behavior: the overlay appears only after a Siege PID exists.
        m_overlay->showForDetectedProcess(pid, executableName);
        m_overlay->setClientReady(m_clientReady);

        Q_EMIT siegeProcessDetected(pid, executableName);
        finishIfReady();
    });

    connect(m_watcher, &SiegeProcessWatcher::processLost,
            this, [this](qint64 previousPid, const QString&) {
        if (!m_waiting) {
            return;
        }

        m_siegeDetected = false;
        m_siegePid = 0;
        m_completionScheduled = false;
        m_overlay->hideImmediately();
        Q_EMIT siegeProcessLost(previousPid);
    });

    connect(m_watcher, &SiegeProcessWatcher::watcherError,
            this, &LaunchReadinessController::errorOccurred);
}

void LaunchReadinessController::setExecutableNames(const QStringList& names) {
    m_watcher->setExecutableNames(names);
}

void LaunchReadinessController::beginWaitingForSiege() {
    m_waiting = true;
    m_siegeDetected = false;
    m_siegePid = 0;
    m_completionScheduled = false;
    m_overlay->resetForWaiting();
    m_watcher->start(750);
    Q_EMIT waitingForSiege();
}

void LaunchReadinessController::setClientReady(bool ready) {
    m_clientReady = ready;
    if (m_siegeDetected) {
        m_overlay->setClientReady(ready);
    }
    finishIfReady();
}

void LaunchReadinessController::stop() {
    m_waiting = false;
    m_completionScheduled = false;
    m_siegeDetected = false;
    m_siegePid = 0;
    m_watcher->stop();
    m_overlay->hideImmediately();
}

bool LaunchReadinessController::clientReady() const {
    return m_clientReady;
}

bool LaunchReadinessController::siegeDetected() const {
    return m_siegeDetected;
}

qint64 LaunchReadinessController::siegePid() const {
    return m_siegePid;
}

void LaunchReadinessController::finishIfReady() {
    if (!m_waiting
        || !m_clientReady
        || !m_siegeDetected
        || m_completionScheduled) {
        return;
    }

    m_completionScheduled = true;
    const qint64 completedPid = m_siegePid;
    m_overlay->complete();

    // Leave 100% visible briefly, then close without stealing game focus.
    QTimer::singleShot(1100, this, [this, completedPid]() {
        if (!m_waiting || m_siegePid != completedPid) {
            return;
        }

        m_overlay->hideImmediately();
        m_watcher->stop();
        m_waiting = false;
        Q_EMIT launchReady(completedPid);
    });
}
