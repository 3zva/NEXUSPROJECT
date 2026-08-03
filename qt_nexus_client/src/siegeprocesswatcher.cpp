#include "siegeprocesswatcher.h"

#include <QSet>
#include <QTimer>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <tlhelp32.h>
#endif

SiegeProcessWatcher::SiegeProcessWatcher(QObject* parent)
    : QObject(parent),
      m_timer(new QTimer(this)),
      m_executableNames(defaultExecutableNames()) {
    m_timer->setTimerType(Qt::CoarseTimer);
    connect(m_timer, &QTimer::timeout, this, &SiegeProcessWatcher::poll);
}

void SiegeProcessWatcher::setExecutableNames(const QStringList& executableNames) {
    QStringList normalized;
    for (const QString& name : executableNames) {
        const QString trimmed = name.trimmed();
        if (!trimmed.isEmpty() && !normalized.contains(trimmed, Qt::CaseInsensitive)) {
            normalized.append(trimmed);
        }
    }
    m_executableNames = normalized.isEmpty()
        ? defaultExecutableNames()
        : normalized;
}

QStringList SiegeProcessWatcher::executableNames() const {
    return m_executableNames;
}

qint64 SiegeProcessWatcher::currentPid() const {
    return m_currentPid;
}

QString SiegeProcessWatcher::currentExecutableName() const {
    return m_currentExecutableName;
}

bool SiegeProcessWatcher::isRunning() const {
    return m_timer->isActive();
}

void SiegeProcessWatcher::start(int pollIntervalMs) {
    const int boundedInterval = qBound(250, pollIntervalMs, 5000);
    m_timer->start(boundedInterval);
    poll();
}

void SiegeProcessWatcher::stop() {
    m_timer->stop();
    m_currentPid = 0;
    m_currentExecutableName.clear();
}

QStringList SiegeProcessWatcher::defaultExecutableNames() {
    return {
        QStringLiteral("RainbowSix.exe"),
        QStringLiteral("RainbowSix_Vulkan.exe"),
        QStringLiteral("RainbowSix_BE.exe")
    };
}

void SiegeProcessWatcher::poll() {
    const ProcessMatch match = findProcess();

    if (match.pid > 0) {
        if (m_currentPid != match.pid) {
            m_currentPid = match.pid;
            m_currentExecutableName = match.executableName;
            Q_EMIT processFound(m_currentPid, m_currentExecutableName);
        }
        return;
    }

    if (m_currentPid > 0) {
        const qint64 previousPid = m_currentPid;
        const QString previousName = m_currentExecutableName;
        m_currentPid = 0;
        m_currentExecutableName.clear();
        Q_EMIT processLost(previousPid, previousName);
    }
}

SiegeProcessWatcher::ProcessMatch SiegeProcessWatcher::findProcess() {
#ifdef Q_OS_WIN
    const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return {};
    }

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    if (!Process32FirstW(snapshot, &entry)) {
        CloseHandle(snapshot);
        return {};
    }

    do {
        const QString executable = QString::fromWCharArray(entry.szExeFile);
        for (const QString& expected : m_executableNames) {
            if (executable.compare(expected, Qt::CaseInsensitive) == 0) {
                const ProcessMatch match{
                    static_cast<qint64>(entry.th32ProcessID),
                    executable
                };
                CloseHandle(snapshot);
                return match;
            }
        }
    } while (Process32NextW(snapshot, &entry));

    CloseHandle(snapshot);
    return {};
#else
    if (!m_reportedUnsupportedPlatform) {
        m_reportedUnsupportedPlatform = true;
        Q_EMIT watcherError(
            QStringLiteral("Siege process detection is implemented for Windows only.")
        );
    }
    return {};
#endif
}
