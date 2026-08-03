#pragma once

#include <QObject>
#include <QStringList>

class QTimer;

/**
 * Polls the Windows process list for configured Rainbow Six Siege executable
 * names and emits a PID when a match is found.
 *
 * This is process-presence detection only. It does not open the process,
 * inspect memory, inject code, or bypass any security system.
 */
class SiegeProcessWatcher final : public QObject {
    Q_OBJECT

public:
    explicit SiegeProcessWatcher(QObject* parent = nullptr);

    void setExecutableNames(const QStringList& executableNames);
    [[nodiscard]] QStringList executableNames() const;
    [[nodiscard]] qint64 currentPid() const;
    [[nodiscard]] QString currentExecutableName() const;
    [[nodiscard]] bool isRunning() const;

    void start(int pollIntervalMs = 750);
    void stop();

    static QStringList defaultExecutableNames();

Q_SIGNALS:
    void processFound(qint64 pid, const QString& executableName);
    void processLost(qint64 previousPid, const QString& executableName);
    void watcherError(const QString& message);

private Q_SLOTS:
    void poll();

private:
    struct ProcessMatch {
        qint64 pid = 0;
        QString executableName;
    };

    [[nodiscard]] ProcessMatch findProcess();

    QTimer* m_timer = nullptr;
    QStringList m_executableNames;
    qint64 m_currentPid = 0;
    QString m_currentExecutableName;
    bool m_reportedUnsupportedPlatform = false;
};
