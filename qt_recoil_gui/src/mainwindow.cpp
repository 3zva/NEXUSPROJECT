#include "mainwindow.h"
#include "authenticatedroot.h"
#include "authpages.h"
#include "firebaseauthclient.h"
#include "LicenseManager.h"
#include "operatorcatalog.h"
#include "theme.h"

#include <QApplication>
#include <QAction>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QCursor>
#include <QDialog>
#include <QDir>
#include <QEvent>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QGuiApplication>
#include <QHostAddress>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMessageBox>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QProcess>
#include <QPushButton>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QResizeEvent>
#include <QSettings>
#include <QScreen>
#include <QSize>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QSystemTrayIcon>
#include <QStringList>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QUrl>

#include <cmath>
#include <optional>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {
QString packageRootDirectory() {
    QDir dir(QCoreApplication::applicationDirPath());
    if (dir.dirName().compare(QStringLiteral("recoil-ui"), Qt::CaseInsensitive) == 0) {
        dir.cdUp();
    }
    return dir.absolutePath();
}

QString runtimeDirectoryForRoot(const QString& rootPath) {
    return QDir(rootPath).filePath(QStringLiteral("runtime"));
}

QString runtimeFileForRoot(const QString& rootPath, const QString& fileName) {
    const QString runtimePath = QDir(runtimeDirectoryForRoot(rootPath)).filePath(fileName);
    if (QFileInfo::exists(runtimePath)) {
        return runtimePath;
    }
    return QDir(rootPath).filePath(fileName);
}

QString writableRuntimeFileForRoot(const QString& rootPath, const QString& fileName) {
    QDir().mkpath(runtimeDirectoryForRoot(rootPath));
    return QDir(runtimeDirectoryForRoot(rootPath)).filePath(fileName);
}

bool savedRegionFromSettings(QRect& region, QString& displayId) {
    QSettings settings(QStringLiteral("NEXUS"), QStringLiteral("NEXUS Client"));
    const bool saved = settings.value(QStringLiteral("overlay/regionSaved"), false).toBool()
        || settings.contains(QStringLiteral("overlay/regionWidth"));
    if (!saved) {
        return false;
    }

    const int x = settings.value(QStringLiteral("overlay/regionX"), 0).toInt();
    const int y = settings.value(QStringLiteral("overlay/regionY"), 0).toInt();
    const int width = settings.value(QStringLiteral("overlay/regionWidth"), 0).toInt();
    const int height = settings.value(QStringLiteral("overlay/regionHeight"), 0).toInt();
    QRect restored(x, y, width, height);
    if (!restored.isValid() || restored.width() <= 0 || restored.height() <= 0) {
        return false;
    }

    region = restored.normalized();
    displayId = settings.value(QStringLiteral("overlay/displayId"), QStringLiteral("primary")).toString();
    if (displayId.trimmed().isEmpty()) {
        displayId = QStringLiteral("primary");
    }
    return true;
}

bool savedOverlayMonitoringEnabled() {
    QSettings settings(QStringLiteral("NEXUS"), QStringLiteral("NEXUS Client"));
    return settings.value(QStringLiteral("overlay/enabled"), true).toBool();
}

std::optional<QString> licenseApiBaseUrl() {
    const QStringList candidates{
        QCoreApplication::applicationDirPath() + QStringLiteral("/config/license_config.json"),
        QDir::currentPath() + QStringLiteral("/config/license_config.json"),
        QCoreApplication::applicationDirPath() + QStringLiteral("/config/license_config.example.json"),
        QDir::currentPath() + QStringLiteral("/config/license_config.example.json"),
    };

    for (const QString& path : candidates) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            continue;
        }
        const auto document = QJsonDocument::fromJson(file.readAll());
        const QString url = document.object().value(QStringLiteral("api_base_url")).toString().trimmed();
        if (!url.isEmpty() && !url.contains(QStringLiteral("YOUR-WORKER-DOMAIN"), Qt::CaseInsensitive)) {
            return url;
        }
    }
    return std::nullopt;
}

bool licensingPaused() {
    const QStringList candidates{
        QCoreApplication::applicationDirPath() + QStringLiteral("/config/license_config.json"),
        QDir::currentPath() + QStringLiteral("/config/license_config.json"),
    };

    for (const QString& path : candidates) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            continue;
        }
        const auto document = QJsonDocument::fromJson(file.readAll());
        if (document.object().value(QStringLiteral("license_paused")).toBool(false)) {
            return true;
        }
    }
    return false;
}

QString licenseMessage(const nexus::license::LicenseResult& result) {
    if (!result.message.empty()) {
        return QString::fromStdString(result.message);
    }
    if (result.valid) {
        const QString licenseType = QString::fromStdString(result.licenseType);
        if (licenseType == QStringLiteral("30_day")) {
            return result.expiresAt.empty()
                ? QStringLiteral("30-day license active. Expiration will be set by the license server.")
                : QStringLiteral("30-day license active. Expires %1.").arg(QString::fromStdString(result.expiresAt));
        }
        if (licenseType == QStringLiteral("lifetime")) {
            return QStringLiteral("Lifetime license active.");
        }
        return QStringLiteral("License active.");
    }
    return QStringLiteral("License validation failed.");
}

double uiScaleFromValue(const QVariant& value) {
    QString text = value.toString().trimmed();
    if (text.isEmpty()) {
        return 1.0;
    }
    text.remove(QLatin1Char('%'));
    bool ok = false;
    double percent = text.toDouble(&ok);
    if (!ok || percent <= 0.0) {
        percent = value.toDouble(&ok) * 100.0;
    }
    if (!ok || percent <= 0.0) {
        percent = 100.0;
    }
    return qBound(0.75, percent / 100.0, 1.25);
}

void applyFontScale(QWidget* widget, double scale) {
    if (widget == nullptr) {
        return;
    }

    QFont font = widget->font();
    QVariant base = widget->property("nexusBasePointSize");
    if (!base.isValid()) {
        const double pointSize = font.pointSizeF() > 0.0 ? font.pointSizeF() : 9.0;
        widget->setProperty("nexusBasePointSize", pointSize);
        base = pointSize;
    }
    font.setPointSizeF(qMax(5.0, base.toDouble() * scale));
    widget->setFont(font);

    const auto children = widget->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    for (auto* child : children) {
        applyFontScale(child, scale);
    }
}

QRect normalizedRegionFromPoints(const QPoint& first, const QPoint& second) {
    const int left = qMin(first.x(), second.x());
    const int top = qMin(first.y(), second.y());
    const int right = qMax(first.x(), second.x());
    const int bottom = qMax(first.y(), second.y());
    return QRect(left, top, qMax(0, right - left), qMax(0, bottom - top));
}

QPoint currentCursorScreenPixelPosition() {
#ifdef Q_OS_WIN
    POINT point{};
    if (GetCursorPos(&point)) {
        return QPoint(point.x, point.y);
    }
#endif
    return QCursor::pos();
}

class RegionSelectionOverlay final : public QDialog {
public:
    explicit RegionSelectionOverlay(QScreen* targetScreen, QWidget* parent = nullptr)
        : QDialog(parent)
        , m_screen(targetScreen != nullptr ? targetScreen : QGuiApplication::primaryScreen()) {
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
        setWindowModality(Qt::ApplicationModal);
        setCursor(Qt::CrossCursor);
        setMouseTracking(true);
        setAttribute(Qt::WA_DeleteOnClose, false);
        setAttribute(Qt::WA_TranslucentBackground, true);
        if (m_screen != nullptr) {
            setGeometry(m_screen->geometry());
        }
    }

    [[nodiscard]] QRect selectedRegion() const {
        const QRect region = normalizedRegionFromPoints(m_originScreenPixels, m_currentScreenPixels);
        if (!region.isValid() || region.width() <= 0 || region.height() <= 0) {
            return {};
        }
        return region;
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.fillRect(rect(), QColor(0, 0, 0, 72));
        if (m_selecting || m_selection.isValid()) {
            const QRect selection = m_selection.normalized();
            painter.setCompositionMode(QPainter::CompositionMode_Clear);
            painter.fillRect(selection, Qt::transparent);
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
            painter.fillRect(selection, QColor(255, 255, 255, 24));
            QPen pen(QColor(118, 91, 255), 2);
            painter.setPen(pen);
            painter.drawRect(selection.adjusted(0, 0, -1, -1));
        }
    }

    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() != Qt::LeftButton) {
            return;
        }
        m_selecting = true;
        m_origin = event->pos();
        m_originScreenPixels = currentCursorScreenPixelPosition();
        m_currentScreenPixels = m_originScreenPixels;
        m_selection = QRect(m_origin, QSize());
        update();
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        if (!m_selecting) {
            return;
        }
        m_currentScreenPixels = currentCursorScreenPixelPosition();
        m_selection = normalizedRegionFromPoints(m_origin, event->pos());
        update();
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        if (event->button() != Qt::LeftButton || !m_selecting) {
            return;
        }
        m_selecting = false;
        m_currentScreenPixels = currentCursorScreenPixelPosition();
        m_selection = normalizedRegionFromPoints(m_origin, event->pos());
        const QRect region = selectedRegion();
        if (region.width() < 4 || region.height() < 4) {
            m_selection = {};
            reject();
            return;
        }
        accept();
    }

    void keyPressEvent(QKeyEvent* event) override {
        if (event->key() == Qt::Key_Escape) {
            reject();
            return;
        }
        QDialog::keyPressEvent(event);
    }

private:
    QScreen* m_screen = nullptr;
    QPoint m_origin;
    QPoint m_originScreenPixels;
    QPoint m_currentScreenPixels;
    QRect m_selection;
    bool m_selecting = false;
};
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("NEXUS"));
    setWindowIcon(NexusTheme::icon(QStringLiteral("nexus_logo_64.png")));
    resize(1086, 1086);
    setMinimumSize(980, 860);

    m_rootStack = new QStackedWidget(this);
    m_authFlow = new AuthFlowWidget(m_rootStack);
    m_authenticatedRoot = new AuthenticatedRoot(m_rootStack);
    m_firebase = new FirebaseAuthClient(this);
    m_runtimeNetwork = new QNetworkAccessManager(this);
    m_detectionServer = new QTcpServer(this);
    m_fpsLabel = new QLabel(this);
    m_fpsLabel->setObjectName(QStringLiteral("clientFpsLabel"));
    m_fpsLabel->setStyleSheet(QStringLiteral(
        "QLabel#clientFpsLabel { background: rgba(7, 10, 18, 180); color: #A898FF; "
        "border: 1px solid #765BFF; border-radius: 5px; padding: 4px 8px; }"
    ));
    m_fpsLabel->hide();
    m_fpsTimer = new QTimer(this);
    connect(m_fpsTimer, &QTimer::timeout, this, &MainWindow::updateFpsLabel);

    m_rootStack->addWidget(m_authFlow);
    m_rootStack->addWidget(m_authenticatedRoot);
    setCentralWidget(m_rootStack);
    setupTrayIcon();

    connectAuthentication();
    connectApplicationPages();
    connect(qApp, &QCoreApplication::aboutToQuit, this, [this]() {
        stopVisionAutomationTool();
    });
    startOperatorDetectionServer();
    readStartupArguments();
    loadFirebaseConfiguration();
    if (!ensureLicense()) {
        QTimer::singleShot(0, qApp, &QCoreApplication::quit);
        return;
    }
    if (m_autoEnterAuthenticatedArea) {
        AuthSession session;
        session.authenticated = true;
        session.emailVerified = true;
        session.email = m_handoffEmail.isEmpty()
            ? QStringLiteral("Authenticated user")
            : m_handoffEmail;
        session.displayName = session.email;
        session.username = session.email.section('@', 0, 0);
        session.localId = QStringLiteral("loader-handoff");
        enterAuthenticatedArea(session);
    } else {
        m_rootStack->setCurrentWidget(m_authFlow);
    }
}

bool MainWindow::ensureLicense() {
    if (licensingPaused()) {
        return true;
    }

    const auto apiBaseUrl = licenseApiBaseUrl();
    if (!apiBaseUrl) {
        QMessageBox::critical(
            this,
            QStringLiteral("NEXUS license required"),
            QStringLiteral("NEXUS licensing is not configured. Add config/license_config.json with your Cloudflare Worker URL.")
        );
        return false;
    }

    try {
        nexus::license::LicenseManager manager{
            nexus::license::LicenseClient(apiBaseUrl->toStdWString())
        };

        if (const auto saved = manager.ValidateSaved()) {
            if (saved->valid) {
                return true;
            }
            manager.ClearSavedLicense();
        }

        for (;;) {
            bool accepted = false;
            const QString key = QInputDialog::getText(
                this,
                QStringLiteral("Activate NEXUS"),
                QStringLiteral("Enter your NEXUS license key:"),
                QLineEdit::Normal,
                QString(),
                &accepted
            ).trimmed();
            if (!accepted) {
                return false;
            }
            if (key.isEmpty()) {
                QMessageBox::warning(this, QStringLiteral("NEXUS license required"), QStringLiteral("Enter a valid NEXUS license key."));
                continue;
            }

            const auto result = manager.ActivateAndSave(key.toStdString());
            if (result.valid) {
                QMessageBox::information(this, QStringLiteral("NEXUS activated"), licenseMessage(result));
                return true;
            }

            QMessageBox::warning(this, QStringLiteral("Activation failed"), licenseMessage(result));
        }
    } catch (const nexus::license::LicenseException& error) {
        QMessageBox::critical(this, QStringLiteral("NEXUS license check failed"), QString::fromStdString(error.what()));
        return false;
    } catch (const std::exception& error) {
        QMessageBox::critical(this, QStringLiteral("NEXUS license check failed"), QString::fromStdString(error.what()));
        return false;
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (m_minimizeToTray && !m_forceExit && m_trayIcon != nullptr && m_trayIcon->isVisible()) {
        hide();
        event->ignore();
        return;
    }

    stopVisionAutomationTool();
    {
        QNetworkAccessManager manager;
        QNetworkRequest request(QUrl(QStringLiteral("http://127.0.0.1:20112/shutdown")));
        request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("text/plain; charset=utf-8"));
        QNetworkReply* reply = manager.post(request, QByteArray());
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(750);
        loop.exec();
        reply->deleteLater();
    }
    QMainWindow::closeEvent(event);
}

void MainWindow::changeEvent(QEvent* event) {
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange
        && m_minimizeToTray
        && isMinimized()
        && m_trayIcon != nullptr
        && m_trayIcon->isVisible()) {
        QTimer::singleShot(0, this, [this]() {
            hide();
            setWindowState(windowState() & ~Qt::WindowMinimized);
        });
    }
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    updateFpsLabel();
}

void MainWindow::loadFirebaseConfiguration() {
    const QStringList candidates{
        QCoreApplication::applicationDirPath() + QStringLiteral("/config/firebase_config.json"),
        QDir::currentPath() + QStringLiteral("/config/firebase_config.json"),
    };

    for (const auto& path : candidates) {
        if (QFile::exists(path) && m_firebase->loadConfiguration(path)) {
            m_firebaseConfigured = true;
            break;
        }
    }

    m_demoMode = false;
    m_authFlow->setDemoMode(false);
    if (!m_firebaseConfigured && !m_autoEnterAuthenticatedArea) {
        m_authFlow->showError(QStringLiteral(
            "Firebase configuration is missing. Add config/firebase_config.json beside the client."
        ));
    }
}

void MainWindow::readStartupArguments() {
    const QStringList arguments = QCoreApplication::arguments();
    auto valueAfter = [&arguments](const QString& key) {
        const int index = arguments.indexOf(key);
        if (index >= 0 && index + 1 < arguments.size()) {
            return arguments.at(index + 1);
        }
        return QString();
    };
    m_startupInstallPath = valueAfter(QStringLiteral("--install-path"));
    m_handoffEmail = valueAfter(QStringLiteral("--authenticated-email"));
    const QString title = valueAfter(QStringLiteral("--window-title")).trimmed();
    if (!title.isEmpty()) {
        m_windowTitle = title;
        setWindowTitle(m_windowTitle);
        if (m_trayIcon != nullptr) {
            m_trayIcon->setToolTip(m_windowTitle);
        }
    }
    m_autoEnterAuthenticatedArea = arguments.contains(QStringLiteral("--trusted-loader-handoff"));
}

void MainWindow::connectAuthentication() {
    connect(m_authFlow, &AuthFlowWidget::signInRequested,
            this, [this](const QString& email, const QString& password) {
        m_authFlow->setBusy(true, QStringLiteral("Signing in..."));
        if (!m_firebaseConfigured) {
            m_authFlow->setBusy(false);
            m_authFlow->showError(QStringLiteral("Firebase login is not configured on this build."));
            return;
        }
        m_firebase->signIn(email, password);
    });

    connect(m_authFlow, &AuthFlowWidget::createAccountRequested,
            this, [this](const QString& fullName, const QString& username,
                         const QString& email, const QString& password) {
        m_authFlow->setBusy(true, QStringLiteral("Creating account..."));
        if (!m_firebaseConfigured) {
            m_authFlow->setBusy(false);
            m_authFlow->showError(QStringLiteral("Firebase account creation is not configured on this build."));
            return;
        }
        m_firebase->createAccount(fullName, username, email, password);
    });

    connect(m_authFlow, &AuthFlowWidget::verificationCheckRequested, this, [this]() {
        m_authFlow->setBusy(true, QStringLiteral("Checking verification status..."));
        if (m_pendingSession.idToken.isEmpty()) {
            m_authFlow->showError(QStringLiteral("The current verification session is missing."));
            return;
        }
        m_reloadAfterSignIn = false;
        m_firebase->reloadUser(m_pendingSession.idToken);
    });

    connect(m_authFlow, &AuthFlowWidget::resendVerificationRequested, this, [this]() {
        if (m_pendingSession.idToken.isEmpty()) {
            m_authFlow->showError(QStringLiteral("The current verification session is missing."));
            return;
        }
        m_authFlow->setBusy(true, QStringLiteral("Sending another verification link..."));
        m_firebase->sendVerificationEmail(m_pendingSession.idToken);
    });

    connect(m_authFlow, &AuthFlowWidget::passwordResetRequested,
            this, [this](const QString& email) {
        if (!m_firebaseConfigured) {
            QMessageBox::warning(
                this,
                QStringLiteral("Password reset"),
                QStringLiteral("Firebase password reset is not configured on this build.")
            );
            return;
        }
        m_firebase->sendPasswordReset(email);
    });

    connect(m_firebase, &FirebaseAuthClient::accountCreated,
            this, [this](const AuthSession& session) {
        m_pendingSession = session;
        m_authFlow->showVerifyEmail(session.email);
        m_authFlow->setBusy(true, QStringLiteral("Sending verification link..."));
        m_firebase->sendVerificationEmail(session.idToken);
    });

    connect(m_firebase, &FirebaseAuthClient::verificationEmailSent,
            this, [this]() {
        m_authFlow->setBusy(false, QStringLiteral(
            "Verification link sent. Check your email, then return here."
        ));
    });

    connect(m_firebase, &FirebaseAuthClient::signedIn,
            this, [this](const AuthSession& session) {
        m_pendingSession = session;
        m_reloadAfterSignIn = true;
        m_firebase->reloadUser(session.idToken);
    });

    connect(m_firebase, &FirebaseAuthClient::userReloaded,
            this, [this](const AuthSession& lookupSession) {
        AuthSession session = m_pendingSession;
        session.email = lookupSession.email.isEmpty() ? session.email : lookupSession.email;
        session.displayName = lookupSession.displayName.isEmpty()
            ? session.displayName : lookupSession.displayName;
        session.localId = lookupSession.localId.isEmpty() ? session.localId : lookupSession.localId;
        session.emailVerified = lookupSession.emailVerified;
        session.authenticated = true;
        m_pendingSession = session;

        if (!session.emailVerified) {
            m_authFlow->showVerifyEmail(session.email);
            m_authFlow->setBusy(false, QStringLiteral(
                "Your email is not verified yet. Use the link Firebase sent."
            ));
            return;
        }

        if (m_reloadAfterSignIn) {
            m_reloadAfterSignIn = false;
            enterAuthenticatedArea(session);
        } else {
            m_authFlow->showAccountCreated(session.email);
            m_authFlow->setBusy(false);
        }
    });

    connect(m_firebase, &FirebaseAuthClient::passwordResetSent, this, [this]() {
        QMessageBox::information(
            this,
            QStringLiteral("Password reset"),
            QStringLiteral("Firebase sent a password-reset email if the account exists.")
        );
    });

    connect(m_firebase, &FirebaseAuthClient::requestFailed,
            m_authFlow, &AuthFlowWidget::showError);
}

void MainWindow::connectApplicationPages() {
    connect(m_authenticatedRoot, &AuthenticatedRoot::logoutRequested,
            this, &MainWindow::handleLogout);

    connect(m_authenticatedRoot, &AuthenticatedRoot::installationPathSelected,
            this, [this](const QString& path) {
        QSettings settings(QStringLiteral("NEXUS"), QStringLiteral("NEXUS Client"));
        settings.setValue(QStringLiteral("installationPath"), path);
        restoreSavedScreenRegion();
        launchRainbowSixSiege();
    });

    // V5.4: AuthenticatedRoot owns the one global operator configuration.
    // Do not add QSettings persistence or per-operator import/export here.
    // The typed operator signals remain available for the existing backend,
    // while the complete 76-record file is handled by SaveFilesPage.
    connect(m_authenticatedRoot, &AuthenticatedRoot::operatorSelected,
            this, &MainWindow::publishOperatorProfile);
    connect(m_authenticatedRoot, &AuthenticatedRoot::operatorSettingsSaveRequested,
            this, [this](const QString&, const QVariantMap& settings) {
        publishOperatorSettings(settings);
    });
    connect(m_authenticatedRoot, &AuthenticatedRoot::operatorSettingsChanged,
            this, [this](const QString& operatorId, const QString&, const QVariant&) {
        publishOperatorProfile(operatorId);
    });
    connect(m_authenticatedRoot, &AuthenticatedRoot::operatorSettingsResetRequested,
            this, &MainWindow::publishOperatorProfile);
    connect(m_authenticatedRoot, &AuthenticatedRoot::resetOperatorsRequested,
            this, [this]() {
        publishRuntimeCommand(QStringLiteral("settings"), QStringLiteral("DISABLE"));
    });
    connect(m_authenticatedRoot, &AuthenticatedRoot::sensitivityConversionRequested,
            this, &MainWindow::handleSensitivityConversion);

    connect(m_authenticatedRoot, &AuthenticatedRoot::screenRegionPageRequested,
            this, []() {});

    connect(m_authenticatedRoot, &AuthenticatedRoot::regionSelectionRequested,
            this, [this]() {
        QScreen* targetScreen = QGuiApplication::screenAt(QCursor::pos());
        if (targetScreen == nullptr) {
            targetScreen = screen() != nullptr ? screen() : QGuiApplication::primaryScreen();
        }
        if (targetScreen == nullptr) {
            m_authenticatedRoot->setScreenRegionSaveResult(false, QStringLiteral("No display was available for region selection."));
            return;
        }

        RegionSelectionOverlay selector(targetScreen, this);
        if (selector.exec() != QDialog::Accepted) {
            m_authenticatedRoot->setScreenRegionSaveResult(false, QStringLiteral("Region selection cancelled."));
            return;
        }

        const QRect region = selector.selectedRegion();
        if (!region.isValid()) {
            m_authenticatedRoot->setScreenRegionSaveResult(false, QStringLiteral("Selected region was too small."));
            return;
        }

        const QString displayId = targetScreen->name().isEmpty()
            ? QStringLiteral("primary")
            : targetScreen->name();
        m_authenticatedRoot->setScreenRegion(region, displayId);
        persistScreenRegion(region, displayId);
        if (writeScreenRegionConfig(region, displayId)) {
            stopVisionAutomationTool();
            if (savedOverlayMonitoringEnabled()) {
                startVisionAutomationTool();
            }
            m_authenticatedRoot->setScreenRegionSaveResult(true, QStringLiteral("Region saved and operator detection updated."));
        } else {
            m_authenticatedRoot->setScreenRegionSaveResult(false, QStringLiteral("Region saved in the app, but the overlay config file could not be updated."));
        }
    });

    connect(m_authenticatedRoot, &AuthenticatedRoot::regionSaveRequested,
            this, [this](const QRect& region, const QString& displayId) {
        persistScreenRegion(region, displayId);
        if (writeScreenRegionConfig(region, displayId)) {
            stopVisionAutomationTool();
            if (savedOverlayMonitoringEnabled()) {
                startVisionAutomationTool();
            }
            m_authenticatedRoot->setScreenRegionSaveResult(true, QStringLiteral("Region saved and overlay config updated."));
        } else {
            m_authenticatedRoot->setScreenRegionSaveResult(false, QStringLiteral("Region saved, but the overlay config file could not be updated."));
        }
    });

    connect(m_authenticatedRoot, &AuthenticatedRoot::regionClearRequested,
            this, [this]() {
        QSettings settings(QStringLiteral("NEXUS"), QStringLiteral("NEXUS Client"));
        settings.setValue(QStringLiteral("overlay/regionSaved"), false);
        settings.remove(QStringLiteral("overlay/regionX"));
        settings.remove(QStringLiteral("overlay/regionY"));
        settings.remove(QStringLiteral("overlay/regionWidth"));
        settings.remove(QStringLiteral("overlay/regionHeight"));
        settings.remove(QStringLiteral("overlay/displayId"));
        const QString rootPath = m_authenticatedRoot->installationPath().isEmpty()
            ? packageRootDirectory()
            : m_authenticatedRoot->installationPath();
        const QString configPath = writableRuntimeFileForRoot(rootPath, QStringLiteral("vision_overlay_config.txt"));
        QStringList lines;
        QFile existing(configPath);
        if (existing.open(QIODevice::ReadOnly | QIODevice::Text)) {
            lines = QString::fromUtf8(existing.readAll()).split('\n');
            existing.close();
        }
        bool updated = false;
        for (auto& line : lines) {
            if (line.startsWith(QStringLiteral("region_configured="))) {
                line = QStringLiteral("region_configured=false");
                updated = true;
                break;
            }
        }
        if (!updated) {
            lines.prepend(QStringLiteral("region_configured=false"));
        }
        QFile config(configPath);
        if (config.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            config.write(lines.join('\n').toUtf8());
            config.close();
        }
        stopVisionAutomationTool();
        if (savedOverlayMonitoringEnabled()) {
            startVisionAutomationTool();
        }
    });

    connect(m_authenticatedRoot, &AuthenticatedRoot::overlayMonitoringEnabledChanged,
            this, [this](bool enabled) {
        QSettings settings(QStringLiteral("NEXUS"), QStringLiteral("NEXUS Client"));
        settings.setValue(QStringLiteral("overlay/enabled"), enabled);
        settings.sync();
        if (enabled) {
            QRect region;
            QString displayId;
            if (savedRegionFromSettings(region, displayId) && writeScreenRegionConfig(region, displayId)) {
                stopVisionAutomationTool();
                startVisionAutomationTool();
            }
        } else {
            stopVisionAutomationTool();
        }
    });
    connect(m_authenticatedRoot, &AuthenticatedRoot::showSelectionBorderChanged,
            this, [](bool enabled) {
        QSettings settings(QStringLiteral("NEXUS"), QStringLiteral("NEXUS Client"));
        settings.setValue(QStringLiteral("overlay/showBorder"), enabled);
        settings.sync();
    });
    connect(m_authenticatedRoot, &AuthenticatedRoot::pauseWhenForegroundChanged,
            this, [this](bool enabled) {
        QSettings settings(QStringLiteral("NEXUS"), QStringLiteral("NEXUS Client"));
        settings.setValue(QStringLiteral("overlay/idleWhenCursorHidden"), enabled);
        settings.sync();
        QRect region;
        QString displayId;
        if (savedOverlayMonitoringEnabled() && savedRegionFromSettings(region, displayId) && writeScreenRegionConfig(region, displayId)) {
            stopVisionAutomationTool();
            startVisionAutomationTool();
        }
    });
    connect(m_authenticatedRoot, &AuthenticatedRoot::lowResourceMonitoringChanged,
            this, [this](bool enabled) {
        QSettings settings(QStringLiteral("NEXUS"), QStringLiteral("NEXUS Client"));
        settings.setValue(QStringLiteral("overlay/lowResourceMode"), enabled);
        settings.sync();
        QRect region;
        QString displayId;
        if (savedOverlayMonitoringEnabled() && savedRegionFromSettings(region, displayId) && writeScreenRegionConfig(region, displayId)) {
            stopVisionAutomationTool();
            startVisionAutomationTool();
        }
    });

    connect(m_authenticatedRoot, &AuthenticatedRoot::settingChanged,
            this, [this](const QString& key, const QVariant& value) {
        QSettings settings(QStringLiteral("NEXUS"), QStringLiteral("NEXUS Client"));
        settings.setValue(QStringLiteral("settings/") + key, value);
        applyClientSetting(key, value);
        publishRuntimeSetting(key, value);
    });

    connect(m_authenticatedRoot, &AuthenticatedRoot::keybindChanged,
            this, [this](const QString& key, const QString& value) {
        QSettings settings(QStringLiteral("NEXUS"), QStringLiteral("NEXUS Client"));
        settings.setValue(QStringLiteral("keybinds/") + key, value);
        publishRuntimeKeybind(key, value);
    });

    connect(m_authenticatedRoot, &AuthenticatedRoot::exitRequested,
            this, &MainWindow::exitClient);
}

void MainWindow::startOperatorDetectionServer() {
    connect(m_detectionServer, &QTcpServer::newConnection,
            this, &MainWindow::handleOperatorDetectionConnection);
    m_detectionServer->listen(QHostAddress::LocalHost, 20113);
}

void MainWindow::handleOperatorDetectionConnection() {
    while (m_detectionServer->hasPendingConnections()) {
        auto* socket = m_detectionServer->nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            QByteArray request = socket->property("requestBuffer").toByteArray();
            request += socket->readAll();
            socket->setProperty("requestBuffer", request);
            const int headerEnd = request.indexOf("\r\n\r\n");
            if (headerEnd < 0) {
                return;
            }
            const QByteArray headers = request.left(headerEnd);
            int contentLength = 0;
            for (const QByteArray& line : headers.split('\n')) {
                const QByteArray trimmed = line.trimmed();
                if (trimmed.toLower().startsWith("content-length:")) {
                    contentLength = trimmed.mid(trimmed.indexOf(':') + 1).trimmed().toInt();
                    break;
                }
            }
            if (request.size() < headerEnd + 4 + contentLength) {
                return;
            }
            const QByteArray body = request.mid(headerEnd + 4, contentLength);
            const QJsonDocument document = QJsonDocument::fromJson(body);
            const QString detectedOperator = document.object()
                .value(QStringLiteral("operator"))
                .toString()
                .trimmed();
            const QString operatorId = OperatorCatalog::resolveId(detectedOperator);

            bool accepted = false;
            if (!operatorId.isEmpty()) {
                m_authenticatedRoot->showOperatorSettings(operatorId);
                publishOperatorProfile(operatorId);
                accepted = true;
                raise();
                activateWindow();
            }

            const QByteArray responseBody = accepted ? "OK" : "Invalid operator";
            const QByteArray status = accepted ? "200 OK" : "400 Bad Request";
            QByteArray response;
            response += QByteArray("HTTP/1.1 ") + status + "\r\n";
            response += QByteArray("Content-Type: text/plain; charset=utf-8\r\n");
            response += QByteArray("Content-Length: ") + QByteArray::number(responseBody.size()) + "\r\n";
            response += "Connection: close\r\n\r\n";
            response += responseBody;
            socket->write(response);
            socket->disconnectFromHost();
        });
        connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
    }
}

void MainWindow::restoreSavedAppSettings() {
    QSettings settings(QStringLiteral("NEXUS"), QStringLiteral("NEXUS Client"));
    m_authenticatedRoot->setOverlayAppSettings(
        settings.value(QStringLiteral("overlay/enabled"), true).toBool(),
        settings.value(QStringLiteral("overlay/showBorder"), true).toBool(),
        settings.value(QStringLiteral("overlay/idleWhenCursorHidden"), false).toBool(),
        settings.value(QStringLiteral("overlay/lowResourceMode"), false).toBool()
    );
}

void MainWindow::restoreSavedClientSettings() {
    QSettings settings(QStringLiteral("NEXUS"), QStringLiteral("NEXUS Client"));
    const QVariantMap values{
        {QStringLiteral("mute_sounds"), settings.value(QStringLiteral("settings/mute_sounds"), false)},
        {QStringLiteral("show_fps"), settings.value(QStringLiteral("settings/show_fps"), true)},
        {QStringLiteral("performance_mode"), settings.value(QStringLiteral("settings/performance_mode"), true)},
        {QStringLiteral("outline_crosshairs"), settings.value(QStringLiteral("settings/outline_crosshairs"), false)},
        {QStringLiteral("minimize_to_tray"), settings.value(QStringLiteral("settings/minimize_to_tray"), true)},
        {QStringLiteral("startup"), settings.value(QStringLiteral("settings/startup"), true)},
        {QStringLiteral("refresh_rate"), settings.value(QStringLiteral("settings/refresh_rate"), 60)},
        {QStringLiteral("tts_enabled"), settings.value(QStringLiteral("settings/tts_enabled"), true)},
        {QStringLiteral("tts_volume"), settings.value(QStringLiteral("settings/tts_volume"), 80)},
        {QStringLiteral("theme"), settings.value(QStringLiteral("settings/theme"), QStringLiteral("NEXUS Purple"))},
        {QStringLiteral("accent"), settings.value(QStringLiteral("settings/accent"), QStringLiteral("Purple"))},
        {QStringLiteral("ui_scale"), settings.value(QStringLiteral("settings/ui_scale"), QStringLiteral("100%"))},
        {QStringLiteral("language"), settings.value(QStringLiteral("settings/language"), QStringLiteral("English"))},
    };

    m_authenticatedRoot->setClientAppSettings(values);
    m_authenticatedRoot->setGeneralAppSettings(values);
    for (auto iterator = values.begin(); iterator != values.end(); ++iterator) {
        applyClientSetting(iterator.key(), iterator.value());
        if (iterator.key() == QStringLiteral("tts_enabled") || iterator.key() == QStringLiteral("tts_volume")) {
            publishRuntimeSetting(iterator.key(), iterator.value());
        }
    }
}

void MainWindow::applyClientSetting(const QString& key, const QVariant& value) {
    if (key == QStringLiteral("startup")) {
        updateStartupRegistration(value.toBool());
    } else if (key == QStringLiteral("minimize_to_tray")) {
        setTrayEnabled(value.toBool());
    } else if (key == QStringLiteral("show_fps")) {
        setFpsVisible(value.toBool());
    } else if (key == QStringLiteral("refresh_rate")) {
        setClientRefreshRate(value.toInt());
    } else if (key == QStringLiteral("ui_scale")) {
        setUiScale(value);
    } else if (key == QStringLiteral("performance_mode")) {
        setUpdatesEnabled(true);
    } else if (key == QStringLiteral("mute_sounds")) {
        // Stored setting. The current client has no sound playback path to mute.
    } else if (key == QStringLiteral("tts_enabled") || key == QStringLiteral("tts_volume")) {
        // Stored setting. Runtime audio feedback is handled by the native backend.
    } else if (key == QStringLiteral("outline_crosshairs")) {
        // Stored setting. Crosshair drawing is not part of the current Qt client surface.
    }
}

void MainWindow::setupTrayIcon() {
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        return;
    }

    m_trayIcon = new QSystemTrayIcon(windowIcon(), this);
    m_trayIcon->setToolTip(m_windowTitle);

    auto* trayMenu = new QMenu(this);
    auto* showAction = trayMenu->addAction(QStringLiteral("Open ") + m_windowTitle);
    connect(showAction, &QAction::triggered, this, [this]() {
        showNormal();
        raise();
        activateWindow();
    });
    auto* exitAction = trayMenu->addAction(QStringLiteral("Exit Client"));
    connect(exitAction, &QAction::triggered, this, &MainWindow::exitClient);

    m_trayIcon->setContextMenu(trayMenu);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
            showNormal();
            raise();
            activateWindow();
        }
    });
}

void MainWindow::updateStartupRegistration(bool enabled) {
    QSettings runKey(
        QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
        QSettings::NativeFormat
    );
    constexpr auto startupName = "NEXUS Client";
    if (enabled) {
        const QString command = QStringLiteral("\"%1\"").arg(QDir::toNativeSeparators(QCoreApplication::applicationFilePath()));
        runKey.setValue(QString::fromLatin1(startupName), command);
    } else {
        runKey.remove(QString::fromLatin1(startupName));
    }
    runKey.sync();
}

void MainWindow::setTrayEnabled(bool enabled) {
    m_minimizeToTray = enabled && m_trayIcon != nullptr;
    if (m_trayIcon == nullptr) {
        return;
    }
    if (m_minimizeToTray) {
        m_trayIcon->show();
    } else {
        m_trayIcon->hide();
    }
}

void MainWindow::setFpsVisible(bool visible) {
    if (m_fpsLabel == nullptr || m_fpsTimer == nullptr) {
        return;
    }
    m_fpsLabel->setVisible(visible);
    if (visible) {
        updateFpsLabel();
        m_fpsTimer->start(1000);
    } else {
        m_fpsTimer->stop();
    }
}

void MainWindow::setClientRefreshRate(int refreshRate) {
    if (refreshRate <= 0) {
        refreshRate = 60;
    }
    m_clientRefreshRate = refreshRate;
    updateFpsLabel();
}

void MainWindow::setUiScale(const QVariant& value) {
    const double scale = uiScaleFromValue(value);
    if (qFuzzyCompare(m_uiScale, scale)) {
        return;
    }
    m_uiScale = scale;

    const QSize minimumSize(
        qMax(900, qRound(980 * scale)),
        qMax(760, qRound(860 * scale))
    );
    const QSize targetSize(
        qMax(minimumSize.width(), qRound(1086 * scale)),
        qMax(minimumSize.height(), qRound(1086 * scale))
    );
    QSize adjustedTargetSize = targetSize;
    if (scale <= 0.76) {
        adjustedTargetSize = QSize(
            qMax(targetSize.width(), 940),
            qMax(targetSize.height(), 840)
        );
    }
    setMinimumSize(minimumSize);
    if (!isMaximized() && !isFullScreen()) {
        resize(adjustedTargetSize);
    }

    applyFontScale(this, scale);
    updateFpsLabel();
}

void MainWindow::updateFpsLabel() {
    if (m_fpsLabel == nullptr || !m_fpsLabel->isVisible()) {
        return;
    }
    m_fpsLabel->setText(QStringLiteral("Client FPS: %1").arg(m_clientRefreshRate));
    m_fpsLabel->adjustSize();
    m_fpsLabel->move(
        width() - m_fpsLabel->width() - 18,
        height() - m_fpsLabel->height() - 18
    );
    m_fpsLabel->raise();
}

void MainWindow::exitClient() {
    m_forceExit = true;
    stopVisionAutomationTool();
    if (m_trayIcon != nullptr) {
        m_trayIcon->hide();
    }
    close();
    QCoreApplication::quit();
}

void MainWindow::restoreSavedScreenRegion() {
    QRect region;
    QString displayId;
    if (!savedRegionFromSettings(region, displayId)) {
        if (savedOverlayMonitoringEnabled()) {
            startVisionAutomationTool();
        }
        return;
    }

    m_authenticatedRoot->setScreenRegion(region, displayId);
    if (writeScreenRegionConfig(region, displayId)) {
        stopVisionAutomationTool();
        if (savedOverlayMonitoringEnabled()) {
            startVisionAutomationTool();
        }
        m_authenticatedRoot->setScreenRegionSaveResult(true, QStringLiteral("Saved region restored."));
    } else {
        m_authenticatedRoot->setScreenRegionSaveResult(true, QStringLiteral("Saved region restored in the app."));
    }
}

void MainWindow::persistScreenRegion(const QRect& region, const QString& displayId) {
    if (!region.isValid()) {
        return;
    }

    QSettings settings(QStringLiteral("NEXUS"), QStringLiteral("NEXUS Client"));
    const QRect normalized = region.normalized();
    if (!settings.contains(QStringLiteral("overlay/enabled"))) {
        settings.setValue(QStringLiteral("overlay/enabled"), true);
    }
    settings.setValue(QStringLiteral("overlay/regionSaved"), true);
    settings.setValue(QStringLiteral("overlay/regionX"), normalized.x());
    settings.setValue(QStringLiteral("overlay/regionY"), normalized.y());
    settings.setValue(QStringLiteral("overlay/regionWidth"), normalized.width());
    settings.setValue(QStringLiteral("overlay/regionHeight"), normalized.height());
    settings.setValue(
        QStringLiteral("overlay/displayId"),
        displayId.trimmed().isEmpty() ? QStringLiteral("primary") : displayId
    );
    settings.sync();
    restoreSavedAppSettings();
}

bool MainWindow::writeScreenRegionConfig(const QRect& region, const QString& displayId) {
    if (!region.isValid()) {
        return false;
    }

    const QString rootPath = m_authenticatedRoot->installationPath().isEmpty()
        ? packageRootDirectory()
        : m_authenticatedRoot->installationPath();
    bool wroteAny = writeScreenRegionConfigForRoot(rootPath, region, displayId);
    const QString packageRoot = packageRootDirectory();
    if (QDir(packageRoot).absolutePath().compare(QDir(rootPath).absolutePath(), Qt::CaseInsensitive) != 0) {
        wroteAny = writeScreenRegionConfigForRoot(packageRoot, region, displayId) || wroteAny;
    }
    return wroteAny;
}

bool MainWindow::writeScreenRegionConfigForRoot(const QString& rootPath, const QRect& region, const QString& displayId) {
    if (!region.isValid() || rootPath.trimmed().isEmpty()) {
        return false;
    }

    const QString configPath = writableRuntimeFileForRoot(rootPath, QStringLiteral("vision_overlay_config.txt"));
    QStringList lines;
    QFile existing(configPath);
    if (existing.open(QIODevice::ReadOnly | QIODevice::Text)) {
        lines = QString::fromUtf8(existing.readAll()).split('\n');
        existing.close();
    }
    auto upsertLine = [&lines](const QString& key, const QString& value) {
        const QString prefix = key + QStringLiteral("=");
        for (auto& line : lines) {
            if (line.startsWith(prefix)) {
                line = prefix + value;
                return;
            }
        }
        lines.prepend(prefix + value);
    };

    const QRect normalized = region.normalized();
    upsertLine(QStringLiteral("region_configured"), QStringLiteral("true"));
    upsertLine(QStringLiteral("region_x"), QString::number(normalized.x()));
    upsertLine(QStringLiteral("region_y"), QString::number(normalized.y()));
    upsertLine(QStringLiteral("region_width"), QString::number(normalized.width()));
    upsertLine(QStringLiteral("region_height"), QString::number(normalized.height()));
    upsertLine(QStringLiteral("region_display"), displayId.trimmed().isEmpty() ? QStringLiteral("primary") : displayId);
    upsertLine(QStringLiteral("server_port"), QStringLiteral("20112"));

    QSettings settings(QStringLiteral("NEXUS"), QStringLiteral("NEXUS Client"));
    const bool pauseWhenHidden = settings.value(QStringLiteral("overlay/idleWhenCursorHidden"), false).toBool();
    const bool lowResourceMode = settings.value(QStringLiteral("overlay/lowResourceMode"), false).toBool();
    const double currentAspectRatio = settings.value(QStringLiteral("converter/currentAspectRatio"), 4.0 / 3.0).toDouble();
    const double nativeAspectRatio = settings.value(QStringLiteral("converter/nativeAspectRatio"), 16.0 / 9.0).toDouble();
    upsertLine(QStringLiteral("pause_when_cursor_hidden"), pauseWhenHidden ? QStringLiteral("true") : QStringLiteral("false"));
    upsertLine(QStringLiteral("fps"), lowResourceMode ? QStringLiteral("6") : QStringLiteral("12"));
    upsertLine(QStringLiteral("current_aspect_ratio"), QString::number(currentAspectRatio, 'g', 12));
    upsertLine(QStringLiteral("native_aspect_ratio"), QString::number(nativeAspectRatio, 'g', 12));

    QFile config(configPath);
    if (!config.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return false;
    }
    config.write(lines.join('\n').toUtf8());
    config.close();
    return true;
}

void MainWindow::stopVisionAutomationTool() {
    const QString installPath = m_authenticatedRoot->installationPath().isEmpty()
        ? packageRootDirectory()
        : m_authenticatedRoot->installationPath();
    const QStringList pidMarkers{
        QDir(runtimeDirectoryForRoot(installPath)).filePath(QStringLiteral("current_overlay_pid.txt")),
        QDir(installPath).filePath(QStringLiteral("current_overlay_pid.txt")),
    };
    for (const QString& pidMarker : pidMarkers) {
        QFile pidFile(pidMarker);
        if (pidFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            const QString pid = QString::fromUtf8(pidFile.readAll()).trimmed();
            if (!pid.isEmpty()) {
                QProcess::execute(QStringLiteral("taskkill.exe"), {
                    QStringLiteral("/PID"),
                    pid,
                    QStringLiteral("/F"),
                    QStringLiteral("/T")
                });
            }
        }
    }

    QProcess::execute(QStringLiteral("powershell.exe"), {
        QStringLiteral("-NoProfile"),
        QStringLiteral("-ExecutionPolicy"),
        QStringLiteral("Bypass"),
        QStringLiteral("-Command"),
        QStringLiteral(
            "$self=$PID; Get-CimInstance Win32_Process | "
            "Where-Object { "
            "($_.ProcessId -ne $self) -and ("
            "($_.ExecutablePath -like '*VisionAutomationTool.exe') -or "
            "($_.CommandLine -like '*vision_overlay_config.txt*')"
            ") "
            "} | ForEach-Object { Invoke-CimMethod -InputObject $_ -MethodName Terminate | Out-Null }"
        )
    });

    const QStringList previousOverlayMarkers{
        QDir(runtimeDirectoryForRoot(installPath)).filePath(QStringLiteral("current_overlay.txt")),
        QDir(installPath).filePath(QStringLiteral("current_overlay.txt")),
    };
    for (const QString& previousOverlayMarker : previousOverlayMarkers) {
        QFile previousOverlay(previousOverlayMarker);
        if (previousOverlay.open(QIODevice::ReadOnly | QIODevice::Text)) {
            const QString previousPath = QString::fromUtf8(previousOverlay.readAll()).trimmed();
            const QString previousName = QFileInfo(previousPath).fileName();
            if (!previousName.isEmpty()) {
                QProcess::execute(QStringLiteral("taskkill.exe"), {
                    QStringLiteral("/IM"),
                    previousName,
                    QStringLiteral("/F")
                });
            }
            if (!previousPath.isEmpty()) {
                QString escapedPreviousPath = previousPath;
                escapedPreviousPath.replace(QStringLiteral("'"), QStringLiteral("''"));
                QProcess::execute(QStringLiteral("powershell.exe"), {
                    QStringLiteral("-NoProfile"),
                    QStringLiteral("-ExecutionPolicy"),
                    QStringLiteral("Bypass"),
                    QStringLiteral("-Command"),
                    QStringLiteral(
                        "Get-CimInstance Win32_Process | "
                        "Where-Object { $_.ExecutablePath -eq '%1' } | "
                        "ForEach-Object { Invoke-CimMethod -InputObject $_ -MethodName Terminate | Out-Null }"
                    ).arg(escapedPreviousPath)
                });
            }
        }
    }
    QProcess::execute(QStringLiteral("taskkill.exe"), {
        QStringLiteral("/IM"),
        QStringLiteral("VisionAutomationTool.exe"),
        QStringLiteral("/F")
    });
}

void MainWindow::startVisionAutomationTool() {
    const QString rootPath = m_authenticatedRoot->installationPath().isEmpty()
        ? packageRootDirectory()
        : m_authenticatedRoot->installationPath();
    const QString toolPath = runtimeFileForRoot(rootPath, QStringLiteral("VisionAutomationTool.exe"));
    if (!QFileInfo::exists(toolPath)) {
        return;
    }

    QRect savedRegion;
    QString savedDisplayId;
    if (savedRegionFromSettings(savedRegion, savedDisplayId)) {
        writeScreenRegionConfigForRoot(rootPath, savedRegion, savedDisplayId);
    }

    const QString configPath = writableRuntimeFileForRoot(rootPath, QStringLiteral("vision_overlay_config.txt"));
    QProcess::startDetached(
        toolPath,
        {QStringLiteral("--config"), configPath},
        QFileInfo(toolPath).absolutePath()
    );
}

void MainWindow::launchRainbowSixSiege() {
    QSettings settings(QStringLiteral("NEXUS"), QStringLiteral("NEXUS Client"));
    const QString savedPath = settings.value(QStringLiteral("gamePath")).toString();
    if (!savedPath.isEmpty() && QFileInfo::exists(savedPath)) {
        QProcess::startDetached(savedPath);
        return;
    }

    const QString steamPath = QStringLiteral(
        "C:/Program Files (x86)/Steam/steamapps/common/Tom Clancy's Rainbow Six Siege/RainbowSix.exe"
    );
    const QString ubisoftPath = QStringLiteral(
        "C:/Program Files (x86)/Ubisoft/Ubisoft Game Launcher/games/Tom Clancy's Rainbow Six Siege/RainbowSix.exe"
    );
    const QString steamSubPath = QStringLiteral(
        "Steam/steamapps/common/Tom Clancy's Rainbow Six Siege/RainbowSix.exe"
    );
    const QString ubisoftSubPath = QStringLiteral(
        "Ubisoft/Ubisoft Game Launcher/games/Tom Clancy's Rainbow Six Siege/RainbowSix.exe"
    );

    const QStringList candidates{steamPath, ubisoftPath};
    for (const QString& candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            settings.setValue(QStringLiteral("gamePath"), candidate);
            settings.sync();
            QProcess::startDetached(candidate);
            return;
        }
    }

    const QFileInfoList drives = QDir::drives();
    for (const QFileInfo& driveInfo : drives) {
        const QString drive = driveInfo.absoluteFilePath();
        if (drive.startsWith(QStringLiteral("C:"), Qt::CaseInsensitive)) {
            continue;
        }

        const QStringList driveCandidates{
            QDir(drive).filePath(QStringLiteral("Program Files (x86)/") + steamSubPath),
            QDir(drive).filePath(QStringLiteral("Program Files (x86)/") + ubisoftSubPath),
            QDir(drive).filePath(steamSubPath),
        };

        for (const QString& candidate : driveCandidates) {
            if (QFileInfo::exists(candidate)) {
                settings.setValue(QStringLiteral("gamePath"), candidate);
                settings.sync();
                QProcess::startDetached(candidate);
                return;
            }
        }
    }

    QMessageBox::warning(
        this,
        QStringLiteral("Game not found"),
        QStringLiteral("Rainbow Six Siege was not found on any available drive.")
    );
}

void MainWindow::enterAuthenticatedArea(const AuthSession& session) {
    m_pendingSession = session;
    m_authFlow->setBusy(false);
    m_authenticatedRoot->setSession(session);
    m_authenticatedRoot->resetToPathSelection();

    QSettings settings(QStringLiteral("NEXUS"), QStringLiteral("NEXUS Client"));
    auto previousPath = m_startupInstallPath.isEmpty()
        ? settings.value(QStringLiteral("installationPath")).toString()
        : m_startupInstallPath;
    if (previousPath.isEmpty()) {
        QDir applicationDirectory(QCoreApplication::applicationDirPath());
        if (applicationDirectory.dirName().compare(QStringLiteral("recoil-ui"), Qt::CaseInsensitive) == 0) {
            applicationDirectory.cdUp();
        }
        previousPath = applicationDirectory.absolutePath();
    }
    if (!previousPath.isEmpty()) {
        if (m_autoEnterAuthenticatedArea) {
            m_authenticatedRoot->setLoadedInstallationPath(previousPath);
            restoreSavedAppSettings();
            restoreSavedClientSettings();
            restoreSavedScreenRegion();
        } else {
            m_authenticatedRoot->setSuggestedInstallationPath(previousPath);
            restoreSavedAppSettings();
            restoreSavedClientSettings();
            restoreSavedScreenRegion();
        }
    } else {
        restoreSavedAppSettings();
        restoreSavedClientSettings();
        restoreSavedScreenRegion();
    }

    m_rootStack->setCurrentWidget(m_authenticatedRoot);
}

void MainWindow::handleLogout() {
    m_pendingSession.clear();
    m_authenticatedRoot->resetToPathSelection();
    m_authFlow->showSignIn();
    m_authFlow->setDemoMode(false);
    m_rootStack->setCurrentWidget(m_authFlow);
}

double MainWindow::aspectRatioValue(const QString& text) const {
    const QString normalized = text.trimmed();
    const QStringList parts = normalized.split(QLatin1Char(':'));
    if (parts.size() == 2) {
        bool widthOk = false;
        bool heightOk = false;
        const double width = parts.at(0).toDouble(&widthOk);
        const double height = parts.at(1).toDouble(&heightOk);
        if (widthOk && heightOk && width > 0.0 && height > 0.0) {
            return width / height;
        }
    }
    bool ok = false;
    const double parsed = normalized.toDouble(&ok);
    return ok && parsed > 0.0 ? parsed : 16.0 / 9.0;
}

void MainWindow::handleSensitivityConversion(const QVariantMap& inputs) {
    constexpr double pi = 3.14159265358979323846;
    constexpr double baselineSens = 48.0;
    constexpr double baselineFovDeg = 86.0;
    constexpr double baselineRenderedAr = 4.0 / 3.0;
    constexpr double baselineNativeAr = 16.0 / 9.0;
    constexpr double baselineMouseMultiplier = 0.002;
    constexpr double baselineAds1x = 58.0;
    constexpr double opticModifier = 0.90;
    auto number = [&inputs](const QString& key, double fallback) {
        bool ok = false;
        const double parsed = inputs.value(key, fallback).toDouble(&ok);
        return ok ? parsed : fallback;
    };

    const double baseFovDeg = number(QStringLiteral("vertical_fov"), baselineFovDeg);
    const double targetAr = aspectRatioValue(inputs.value(QStringLiteral("aspect_ratio"), QStringLiteral("4:3")).toString());
    const double nativeAr = aspectRatioValue(inputs.value(QStringLiteral("native_monitor_ratio"), QStringLiteral("16:9")).toString());
    const double adsSlider = number(QStringLiteral("ads_1x"), baselineAds1x);
    const double baseSens = number(QStringLiteral("in_game_sensitivity"), baselineSens);
    const double mouseMultiplier = number(QStringLiteral("mouse_multiplier"), baselineMouseMultiplier);

    if (baseFovDeg < 60.0 || baseFovDeg > 90.0
        || targetAr <= 0.0
        || nativeAr <= 0.0
        || adsSlider <= 0.0
        || baseSens <= 0.0
        || mouseMultiplier <= 0.0) {
        m_authenticatedRoot->setSensitivityConversionError(QStringLiteral("Check the converter inputs and use positive values in the allowed ranges."));
        return;
    }

    auto calculateModifier = [](double fovDeg, double renderedAr, double monitorAr, double ads, double sens, double multiplier) {
        const double baseVRad = fovDeg * (pi / 180.0);
        const double baseHRad = 2.0 * std::atan(std::tan(baseVRad / 2.0) * monitorAr);
        const double activeVRad = (fovDeg * opticModifier) * (pi / 180.0);
        const double activeHRad = 2.0 * std::atan(std::tan(activeVRad / 2.0) * renderedAr);

        const double baseFocalX = 1.0 / std::tan(baseHRad / 2.0);
        const double activeFocalX = 1.0 / std::tan(activeHRad / 2.0);
        const double baseFocalY = 1.0 / std::tan(baseVRad / 2.0);
        const double activeFocalY = 1.0 / std::tan(activeVRad / 2.0);
        const double scaleX = baseFocalX / activeFocalX;
        const double scaleY = baseFocalY / activeFocalY;
        const double stretchFactor = monitorAr / renderedAr;
        const double adsWeight = (sens * multiplier) * (ads / 50.0);
        return std::pair<double, double>{scaleX * stretchFactor * adsWeight, scaleY * adsWeight};
    };

    const auto [currentX, currentY] = calculateModifier(baseFovDeg, targetAr, nativeAr, adsSlider, baseSens, mouseMultiplier);
    const auto [baselineX, baselineY] = calculateModifier(
        baselineFovDeg,
        baselineRenderedAr,
        baselineNativeAr,
        baselineAds1x,
        baselineSens,
        baselineMouseMultiplier
    );
    const double horizontalScale = baselineX / currentX;
    const double verticalScale = baselineY / currentY;
    if (!std::isfinite(horizontalScale) || !std::isfinite(verticalScale)) {
        m_authenticatedRoot->setSensitivityConversionError(QStringLiteral("The converter produced an invalid scale. Check the entered values."));
        return;
    }

    m_sensitivityScaleX = horizontalScale;
    m_sensitivityScaleY = verticalScale;
    QSettings settings(QStringLiteral("NEXUS"), QStringLiteral("NEXUS Client"));
    settings.setValue(QStringLiteral("converter/currentAspectRatio"), targetAr);
    settings.setValue(QStringLiteral("converter/nativeAspectRatio"), nativeAr);
    settings.sync();
    m_authenticatedRoot->setSensitivityScaleFactors(horizontalScale, verticalScale);
    QRect savedRegion;
    QString savedDisplayId;
    if (savedRegionFromSettings(savedRegion, savedDisplayId)) {
        writeScreenRegionConfig(savedRegion, savedDisplayId);
    }
    if (!m_lastPublishedOperatorSettings.isEmpty()) {
        publishOperatorSettings(m_lastPublishedOperatorSettings);
    }
}

QString MainWindow::automationPayloadForSettings(const QVariantMap& settings) const {
    if (!settings.value(QStringLiteral("profile_enabled"), true).toBool()) {
        return QStringLiteral("DISABLE");
    }

    const auto primary = settings.value(QStringLiteral("primary")).toMap();
    const auto secondary = settings.value(QStringLiteral("secondary")).toMap();
    auto value = [](const QVariantMap& weapon, const QString& key, double fallback) {
        bool ok = false;
        const double parsed = weapon.value(key, fallback).toDouble(&ok);
        return ok ? parsed : fallback;
    };
    auto speedSeconds = [](double value) {
        return qMax(0.001, value > 1.0 ? value / 1000.0 : value);
    };
    auto convertedAxis = [](double value, double scale) {
        if (std::abs(scale - 1.0) < 0.0000001) {
            return value;
        }
        // Keep fractional pixels for the recoil engine's accumulator; X is applied on a half-cadence.
        return value * scale;
    };

    int rapid = 0;
    if (settings.value(QStringLiteral("rapid_fire_enabled"), false).toBool()) {
        rapid = settings.value(QStringLiteral("rapid_fire_value"), 0).toInt();
        if (rapid != 1 && rapid != 2 && rapid != 3) {
            rapid = settings.value(QStringLiteral("active_weapon")).toString().compare(
                QStringLiteral("secondary"),
                Qt::CaseInsensitive
            ) == 0 ? 2 : 1;
        }
    }

    const QList<double> values{
        speedSeconds(value(primary, QStringLiteral("time_delay"), 0.030)),
        convertedAxis(value(primary, QStringLiteral("x_amount"), 0.0), m_sensitivityScaleX),
        -convertedAxis(value(primary, QStringLiteral("y_amount"), 0.0), m_sensitivityScaleY),
        speedSeconds(value(secondary, QStringLiteral("time_delay"), 0.040)),
        convertedAxis(value(secondary, QStringLiteral("x_amount"), 0.0), m_sensitivityScaleX),
        -convertedAxis(value(secondary, QStringLiteral("y_amount"), 0.0), m_sensitivityScaleY),
        static_cast<double>(rapid),
        convertedAxis(value(primary, QStringLiteral("horizontal_ramp"), 0.0), m_sensitivityScaleX),
        -convertedAxis(value(primary, QStringLiteral("vertical_ramp"), 0.0), m_sensitivityScaleY),
        convertedAxis(value(secondary, QStringLiteral("horizontal_ramp"), 0.0), m_sensitivityScaleX),
        -convertedAxis(value(secondary, QStringLiteral("vertical_ramp"), 0.0), m_sensitivityScaleY),
        value(primary, QStringLiteral("ramp_start_seconds"), 0.75),
        value(secondary, QStringLiteral("ramp_start_seconds"), 0.75),
    };

    QStringList parts;
    parts.reserve(values.size());
    for (double number : values) {
        parts.append(QString::number(number, 'g', 12));
    }
    return parts.join(QLatin1Char(','));
}

void MainWindow::publishOperatorProfile(const QString& operatorId) {
    publishOperatorSettings(m_authenticatedRoot->operatorSettingsFor(operatorId));
}

void MainWindow::publishOperatorSettings(const QVariantMap& settings) {
    if (settings.isEmpty() || m_runtimeNetwork == nullptr) {
        return;
    }

    m_lastPublishedOperatorSettings = settings;
    publishRuntimeCommand(QStringLiteral("settings"), automationPayloadForSettings(settings));

    const QVariantMap primary = settings.value(QStringLiteral("primary")).toMap();
    const QVariantMap secondary = settings.value(QStringLiteral("secondary")).toMap();
    QJsonObject loadout{
        {QStringLiteral("operator"), settings.value(QStringLiteral("operator_id")).toString()},
        {QStringLiteral("primary"), primary.value(QStringLiteral("selected_weapon")).toString()},
        {QStringLiteral("secondary"), secondary.value(QStringLiteral("selected_weapon")).toString()}
    };
    publishRuntimeCommand(
        QStringLiteral("loadout"),
        QString::fromUtf8(QJsonDocument(loadout).toJson(QJsonDocument::Compact))
    );
}

void MainWindow::publishRuntimeSetting(const QString& key, const QVariant& value) {
    publishRuntimeCommand(
        QStringLiteral("app-setting"),
        key + QStringLiteral("=") + value.toString()
    );
}

void MainWindow::publishRuntimeKeybind(const QString& key, const QString& value) {
    publishRuntimeCommand(
        QStringLiteral("keybind"),
        key + QStringLiteral("=") + value.trimmed()
    );
}

void MainWindow::publishRuntimeCommand(const QString& endpoint, const QString& payload) {
    if (m_runtimeNetwork == nullptr) {
        return;
    }

    QNetworkRequest request(QUrl(QStringLiteral("http://127.0.0.1:20112/") + endpoint));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("text/plain; charset=utf-8"));
    auto* reply = m_runtimeNetwork->post(request, payload.toUtf8());
    connect(reply, &QNetworkReply::finished, reply, &QObject::deleteLater);
}
