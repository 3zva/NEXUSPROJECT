#include "autoupdater.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTimer>
#include <memory>

namespace {
QString updateDirectory() {
    const QString root = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    const QString path = QDir(root).filePath(QStringLiteral("updates"));
    QDir().mkpath(path);
    return path;
}

QString cleanVersionLabel(const QString& version) {
    QString label = version.trimmed();
    if (label.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
        label.remove(0, 1);
    }
    return label;
}
}

AutoUpdater::AutoUpdater(QObject* parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this)) {
}

void AutoUpdater::setApiBaseUrl(const QUrl& apiBaseUrl) {
    m_apiBaseUrl = apiBaseUrl;
}

bool AutoUpdater::isBusy() const {
    return m_busy;
}

void AutoUpdater::checkAndInstall() {
    if (m_busy) {
        return;
    }
    m_busy = true;
    m_installing = false;
    m_manifest = {};
    Q_EMIT checking(QStringLiteral("Checking GitHub releases for a newer NEXUS build..."));
    requestManifest();
}

void AutoUpdater::cancel() {
    if (!m_busy || m_installing) {
        return;
    }
    if (m_activeReply != nullptr) {
        auto* reply = m_activeReply;
        m_activeReply = nullptr;
        reply->disconnect(this);
        reply->abort();
        reply->deleteLater();
    }
    resetActiveTransfer();
    m_busy = false;
    Q_EMIT canceled();
}

void AutoUpdater::retry() {
    if (m_busy) {
        return;
    }
    checkAndInstall();
}

void AutoUpdater::requestManifest() {
    const QUrl url = latestManifestUrl();
    if (!url.isValid()) {
        m_busy = false;
        Q_EMIT failed(QStringLiteral("The release manifest URL is not configured."));
        return;
    }

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("NEXUS Client/%1").arg(QCoreApplication::applicationVersion()));
    m_activeReply = m_network->get(request);
    connect(m_activeReply, &QNetworkReply::finished, this, [this]() {
        auto* reply = m_activeReply;
        m_activeReply = nullptr;
        handleManifestReply(reply);
    });
}

void AutoUpdater::handleManifestReply(QNetworkReply* reply) {
    const std::unique_ptr<QNetworkReply, void(*)(QNetworkReply*)> guard(reply, [](QNetworkReply* item) {
        if (item != nullptr) {
            item->deleteLater();
        }
    });

    if (reply == nullptr) {
        m_busy = false;
        Q_EMIT failed(QStringLiteral("The release check did not return a response."));
        return;
    }
    if (reply->error() != QNetworkReply::NoError) {
        m_busy = false;
        Q_EMIT failed(QStringLiteral("Release check failed: %1").arg(reply->errorString()));
        return;
    }

    const QJsonDocument document = QJsonDocument::fromJson(reply->readAll());
    const QJsonObject root = document.object();
    const QJsonObject manifest = root.value(QStringLiteral("manifest")).toObject();
    Manifest parsed;
    parsed.version = cleanVersionLabel(manifest.value(QStringLiteral("version")).toString());
    parsed.packageUrl = QUrl(manifest.value(QStringLiteral("package_url")).toString());
    parsed.sha256 = manifest.value(QStringLiteral("package_sha256")).toString().trimmed().toLower();
    parsed.packageSize = static_cast<qint64>(manifest.value(QStringLiteral("package_size")).toDouble());
    parsed.minimumWindowsBuild = manifest.value(QStringLiteral("minimum_windows_build")).toInt();

    QString error;
    if (!manifestIsUsable(parsed, &error)) {
        m_busy = false;
        Q_EMIT failed(error);
        return;
    }

    const QString currentVersion = cleanVersionLabel(QCoreApplication::applicationVersion());
    if (compareVersions(parsed.version, currentVersion) <= 0) {
        m_busy = false;
        Q_EMIT noUpdateAvailable(currentVersion);
        return;
    }

    m_manifest = parsed;
    Q_EMIT updateStarted(m_manifest.version);
    startDownload();
}

void AutoUpdater::startDownload() {
    QNetworkRequest request(m_manifest.packageUrl);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("NEXUS Client/%1").arg(QCoreApplication::applicationVersion()));
    m_activeReply = m_network->get(request);

    m_downloadFile = new QFile(stagedPackagePath(), this);
    if (!m_downloadFile->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        resetActiveTransfer();
        m_busy = false;
        Q_EMIT failed(QStringLiteral("Unable to create the staged update file."));
        return;
    }

    connect(m_activeReply, &QNetworkReply::readyRead, this, [this]() {
        if (m_downloadFile != nullptr && m_activeReply != nullptr) {
            m_downloadFile->write(m_activeReply->readAll());
        }
    });
    connect(m_activeReply, &QNetworkReply::downloadProgress,
            this, &AutoUpdater::downloadProgress);
    connect(m_activeReply, &QNetworkReply::finished, this, [this]() {
        auto* reply = m_activeReply;
        m_activeReply = nullptr;
        handleDownloadFinished(reply);
    });
}

void AutoUpdater::handleDownloadFinished(QNetworkReply* reply) {
    const std::unique_ptr<QNetworkReply, void(*)(QNetworkReply*)> guard(reply, [](QNetworkReply* item) {
        if (item != nullptr) {
            item->deleteLater();
        }
    });

    if (m_downloadFile != nullptr && reply != nullptr) {
        m_downloadFile->write(reply->readAll());
        m_downloadFile->flush();
        m_downloadFile->close();
    }

    if (reply == nullptr || reply->error() != QNetworkReply::NoError) {
        const QString message = reply == nullptr
            ? QStringLiteral("The update download did not return a response.")
            : QStringLiteral("Update download failed: %1").arg(reply->errorString());
        resetActiveTransfer();
        m_busy = false;
        Q_EMIT failed(message);
        return;
    }

    resetActiveTransfer();
    if (!verifyDownloadedPackage()) {
        m_busy = false;
        return;
    }

    Q_EMIT installationProgress(35);
    QTimer::singleShot(250, this, [this]() {
        Q_EMIT installationProgress(75);
        launchDownloadedPackage();
    });
}

bool AutoUpdater::verifyDownloadedPackage() {
    Q_EMIT verificationProgress(10);
    QFileInfo info(stagedPackagePath());
    if (!info.exists() || !info.isFile() || info.size() <= 0) {
        Q_EMIT failed(QStringLiteral("The downloaded update package is missing."));
        return false;
    }
    if (m_manifest.packageSize > 0 && info.size() != m_manifest.packageSize) {
        QFile::remove(stagedPackagePath());
        Q_EMIT failed(QStringLiteral("The downloaded update package size does not match the release manifest."));
        return false;
    }

    QFile file(stagedPackagePath());
    if (!file.open(QIODevice::ReadOnly)) {
        Q_EMIT failed(QStringLiteral("Unable to read the downloaded update package."));
        return false;
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (!file.atEnd()) {
        hash.addData(file.read(1024 * 1024));
    }
    Q_EMIT verificationProgress(85);
    const QString actual = QString::fromLatin1(hash.result().toHex()).toLower();
    if (!m_manifest.sha256.isEmpty() && actual != m_manifest.sha256) {
        QFile::remove(stagedPackagePath());
        Q_EMIT failed(QStringLiteral("The downloaded update package failed SHA-256 verification."));
        return false;
    }

    Q_EMIT verificationProgress(100);
    return true;
}

void AutoUpdater::launchDownloadedPackage() {
    m_installing = true;
    const QString packagePath = stagedPackagePath();
    if (!QFileInfo::exists(packagePath)) {
        m_busy = false;
        Q_EMIT failed(QStringLiteral("The staged update package is no longer available."));
        return;
    }

    if (!QProcess::startDetached(packagePath, QStringList(), QFileInfo(packagePath).absolutePath())) {
        m_busy = false;
        Q_EMIT failed(QStringLiteral("NEXUS could not start the downloaded update package."));
        return;
    }

    Q_EMIT installationProgress(100);
    Q_EMIT restartingWithUpdate(m_manifest.version);
}

void AutoUpdater::resetActiveTransfer() {
    if (m_downloadFile != nullptr) {
        if (m_downloadFile->isOpen()) {
            m_downloadFile->close();
        }
        m_downloadFile->deleteLater();
        m_downloadFile = nullptr;
    }
}

QUrl AutoUpdater::latestManifestUrl() const {
    QUrl url = m_apiBaseUrl;
    if (!url.isValid() || url.isEmpty()) {
        return {};
    }
    QString path = url.path();
    if (!path.endsWith(QLatin1Char('/'))) {
        path.append(QLatin1Char('/'));
    }
    path.append(QStringLiteral("v1/releases/latest"));
    url.setPath(path);
    url.setQuery(QString());
    url.setFragment(QString());
    return url;
}

QString AutoUpdater::stagedPackagePath() const {
    const QString version = m_manifest.version.isEmpty()
        ? QStringLiteral("latest")
        : m_manifest.version;
    return QDir(updateDirectory()).filePath(QStringLiteral("NEXUS-%1.exe").arg(version));
}

bool AutoUpdater::manifestIsUsable(const Manifest& manifest, QString* error) const {
    if (manifest.version.isEmpty()) {
        if (error != nullptr) *error = QStringLiteral("The release manifest did not include a version.");
        return false;
    }
    if (!manifest.packageUrl.isValid() || manifest.packageUrl.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) != 0) {
        if (error != nullptr) *error = QStringLiteral("The release manifest did not include a valid HTTPS GitHub download URL.");
        return false;
    }
    if (manifest.packageSize <= 0) {
        if (error != nullptr) *error = QStringLiteral("The release manifest did not include a valid package size.");
        return false;
    }
    static const QRegularExpression shaPattern(QStringLiteral("^[0-9a-f]{64}$"));
    if (!shaPattern.match(manifest.sha256).hasMatch()) {
        if (error != nullptr) *error = QStringLiteral("The release manifest did not include a valid SHA-256 hash.");
        return false;
    }
    return true;
}

int AutoUpdater::compareVersions(const QString& left, const QString& right) {
    const QStringList leftParts = cleanVersionLabel(left).split(QLatin1Char('.'));
    const QStringList rightParts = cleanVersionLabel(right).split(QLatin1Char('.'));
    const int count = qMax(leftParts.size(), rightParts.size());
    for (int index = 0; index < count; ++index) {
        const int leftValue = index < leftParts.size() ? leftParts.at(index).toInt() : 0;
        const int rightValue = index < rightParts.size() ? rightParts.at(index).toInt() : 0;
        if (leftValue < rightValue) return -1;
        if (leftValue > rightValue) return 1;
    }
    return 0;
}
