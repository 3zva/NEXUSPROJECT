#pragma once

#include <QWidget>

class NexusProgressView;

/**
 * Full-size update page. Add it to MainWindow's root QStackedWidget so it
 * occupies the exact normal NEXUS application window size and covers the
 * authenticated sidebar while an update is active.
 *
 * This page is UI-only. The native updater supplies real progress signals.
 */
class UpdateProgressPage final : public QWidget {
    Q_OBJECT

public:
    explicit UpdateProgressPage(QWidget* parent = nullptr);

    void beginUpdate(const QString& versionLabel);
    void setCheckingStatus(const QString& message = QString());
    void setDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void setVerificationProgress(int percent);
    void setInstallationProgress(int percent);
    void finishUpdate(const QString& versionLabel);
    void failUpdate(const QString& message);
    void reset();

Q_SIGNALS:
    void cancelUpdateRequested();
    void retryUpdateRequested();
    void restartRequested();

private:
    enum class ActionMode {
        None,
        Cancel,
        Retry,
        Restart
    };

    void setActionMode(ActionMode mode);
    static QString formatBytes(qint64 bytes);

    NexusProgressView* m_progressView = nullptr;
    ActionMode m_actionMode = ActionMode::None;
    QString m_targetVersion;
};
