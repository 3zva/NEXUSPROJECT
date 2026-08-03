#pragma once

#include <QObject>
#include <QUrl>

class QFile;
class QNetworkAccessManager;
class QNetworkReply;

class AutoUpdater final : public QObject {
    Q_OBJECT

public:
    explicit AutoUpdater(QObject* parent = nullptr);

    void setApiBaseUrl(const QUrl& apiBaseUrl);
    [[nodiscard]] bool isBusy() const;

public Q_SLOTS:
    void checkAndInstall();
    void cancel();
    void retry();

Q_SIGNALS:
    void checking(const QString& message);
    void updateStarted(const QString& version);
    void downloadProgress(qint64 received, qint64 total);
    void verificationProgress(int percent);
    void installationProgress(int percent);
    void noUpdateAvailable(const QString& currentVersion);
    void restartingWithUpdate(const QString& version);
    void failed(const QString& message);
    void canceled();

private:
    struct Manifest {
        QString version;
        QUrl packageUrl;
        QString sha256;
        qint64 packageSize = 0;
        int minimumWindowsBuild = 0;
    };

    void requestManifest();
    void handleManifestReply(QNetworkReply* reply);
    void startDownload();
    void handleDownloadFinished(QNetworkReply* reply);
    bool verifyDownloadedPackage();
    void launchDownloadedPackage();
    void resetActiveTransfer();
    [[nodiscard]] QUrl latestManifestUrl() const;
    [[nodiscard]] QString stagedPackagePath() const;
    [[nodiscard]] bool manifestIsUsable(const Manifest& manifest, QString* error) const;
    static int compareVersions(const QString& left, const QString& right);

    QNetworkAccessManager* m_network = nullptr;
    QNetworkReply* m_activeReply = nullptr;
    QFile* m_downloadFile = nullptr;
    QUrl m_apiBaseUrl;
    Manifest m_manifest;
    bool m_busy = false;
    bool m_installing = false;
};
