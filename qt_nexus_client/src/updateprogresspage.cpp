#include "updateprogresspage.h"
#include "nexusprogressview.h"

#include <QVBoxLayout>
#include <algorithm>

UpdateProgressPage::UpdateProgressPage(QWidget* parent)
    : QWidget(parent) {
    setObjectName(QStringLiteral("updateProgressPage"));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_progressView = new NexusProgressView(NexusProgressMode::Update, this);
    root->addWidget(m_progressView);

    connect(m_progressView, &NexusProgressView::actionRequested,
            this, [this]() {
        switch (m_actionMode) {
        case ActionMode::Cancel:
            Q_EMIT cancelUpdateRequested();
            break;
        case ActionMode::Retry:
            Q_EMIT retryUpdateRequested();
            break;
        case ActionMode::Restart:
            Q_EMIT restartRequested();
            break;
        case ActionMode::None:
            break;
        }
    });
}

void UpdateProgressPage::beginUpdate(const QString& versionLabel) {
    m_targetVersion = versionLabel.trimmed();
    m_progressView->reset();
    m_progressView->setTitle(m_targetVersion.isEmpty()
        ? QStringLiteral("Updating NEXUS")
        : QStringLiteral("Updating NEXUS to %1").arg(m_targetVersion));
    m_progressView->setStage(QStringLiteral("CHECKING"));
    m_progressView->setStatus(QStringLiteral("Checking the update package..."));
    m_progressView->setDetail(QStringLiteral(
        "NEXUS will verify the package before installing it."
    ));
    m_progressView->setProgress(2, false);
    setActionMode(ActionMode::Cancel);
}

void UpdateProgressPage::setCheckingStatus(const QString& message) {
    m_progressView->setStage(QStringLiteral("CHECKING"));
    m_progressView->setStatus(message.isEmpty()
        ? QStringLiteral("Checking the update package...")
        : message);
    m_progressView->setProgress(8, true);
}

void UpdateProgressPage::setDownloadProgress(
    qint64 bytesReceived,
    qint64 bytesTotal
) {
    double ratio = 0.0;
    if (bytesTotal > 0) {
        ratio = static_cast<double>(bytesReceived)
            / static_cast<double>(bytesTotal);
    }
    ratio = std::clamp(ratio, 0.0, 1.0);

    // Download occupies 10% through 75% of the visible progress bar.
    const int mappedProgress = 10 + static_cast<int>(ratio * 65.0);
    m_progressView->setStage(QStringLiteral("DOWNLOADING"));
    m_progressView->setStatus(QStringLiteral("Downloading NEXUS update..."));
    m_progressView->setDetail(bytesTotal > 0
        ? QStringLiteral("%1 of %2")
            .arg(formatBytes(bytesReceived), formatBytes(bytesTotal))
        : formatBytes(bytesReceived));
    m_progressView->setProgress(mappedProgress, false);
}

void UpdateProgressPage::setVerificationProgress(int percent) {
    const int bounded = std::clamp(percent, 0, 100);
    const int mappedProgress = 75 + static_cast<int>(bounded * 0.13);
    m_progressView->setStage(QStringLiteral("VERIFYING"));
    m_progressView->setStatus(QStringLiteral("Verifying update integrity..."));
    m_progressView->setDetail(QStringLiteral(
        "The update must pass signature or checksum verification before installation."
    ));
    m_progressView->setProgress(mappedProgress, true);
}

void UpdateProgressPage::setInstallationProgress(int percent) {
    const int bounded = std::clamp(percent, 0, 100);
    const int mappedProgress = 88 + static_cast<int>(bounded * 0.11);
    m_progressView->setStage(QStringLiteral("INSTALLING"));
    m_progressView->setStatus(QStringLiteral("Installing the update..."));
    m_progressView->setDetail(QStringLiteral(
        "Do not close NEXUS while files are being replaced."
    ));
    m_progressView->setProgress(mappedProgress, true);
}

void UpdateProgressPage::finishUpdate(const QString& versionLabel) {
    const QString resolvedVersion = versionLabel.trimmed().isEmpty()
        ? m_targetVersion
        : versionLabel.trimmed();

    m_progressView->setComplete(resolvedVersion.isEmpty()
        ? QStringLiteral("Update complete. Restart NEXUS to continue.")
        : QStringLiteral("NEXUS %1 is ready. Restart to continue.")
            .arg(resolvedVersion));
    m_progressView->setDetail(QStringLiteral(
        "Your settings and global operator configuration were not changed."
    ));
    setActionMode(ActionMode::Restart);
}

void UpdateProgressPage::failUpdate(const QString& message) {
    m_progressView->setStage(QStringLiteral("UPDATE FAILED"));
    m_progressView->setError(message.isEmpty()
        ? QStringLiteral("The update could not be completed.")
        : message);
    m_progressView->setDetail(QStringLiteral(
        "The current installation should remain active. Retry when ready."
    ));
    setActionMode(ActionMode::Retry);
}

void UpdateProgressPage::reset() {
    m_targetVersion.clear();
    m_progressView->reset();
    setActionMode(ActionMode::None);
}

void UpdateProgressPage::setActionMode(ActionMode mode) {
    m_actionMode = mode;
    switch (mode) {
    case ActionMode::Cancel:
        m_progressView->setActionText(QStringLiteral("CANCEL UPDATE"));
        m_progressView->setActionVisible(true);
        break;
    case ActionMode::Retry:
        m_progressView->setActionText(QStringLiteral("RETRY UPDATE"));
        m_progressView->setActionVisible(true);
        break;
    case ActionMode::Restart:
        m_progressView->setActionText(QStringLiteral("RESTART NEXUS"));
        m_progressView->setActionVisible(true);
        break;
    case ActionMode::None:
        m_progressView->setActionVisible(false);
        break;
    }
}

QString UpdateProgressPage::formatBytes(qint64 bytes) {
    constexpr double kilo = 1024.0;
    constexpr double mega = kilo * 1024.0;
    constexpr double giga = mega * 1024.0;

    if (bytes >= static_cast<qint64>(giga)) {
        return QStringLiteral("%1 GB").arg(bytes / giga, 0, 'f', 2);
    }
    if (bytes >= static_cast<qint64>(mega)) {
        return QStringLiteral("%1 MB").arg(bytes / mega, 0, 'f', 1);
    }
    if (bytes >= static_cast<qint64>(kilo)) {
        return QStringLiteral("%1 KB").arg(bytes / kilo, 0, 'f', 1);
    }
    return QStringLiteral("%1 B").arg(bytes);
}
