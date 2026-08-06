#pragma once

#include "authsession.h"

#include <QMainWindow>
#include <QRect>
#include <QVariantMap>

class AuthFlowWidget;
class AuthenticatedRoot;
class AutoUpdater;
class FirebaseAuthClient;
class GameLaunchOverlayWindow;
class LaunchReadinessController;
class QLabel;
class NexusProgressView;
class QEvent;
class QResizeEvent;
class QSystemTrayIcon;
class QNetworkAccessManager;
class QTcpServer;
class QTimer;
class QStackedWidget;
class UpdateProgressPage;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void connectAuthentication();
    void connectApplicationPages();
    void connectUpdater();
    void enterAuthenticatedArea(const AuthSession& session);
    void handleLogout();
    void loadFirebaseConfiguration();
    bool ensureLicense();
    void readStartupArguments();
    void startOperatorDetectionServer();
    void handleOperatorDetectionConnection();
    void restoreSavedAppSettings();
    void restoreSavedClientSettings();
    void applyClientSetting(const QString& key, const QVariant& value);
    void setupTrayIcon();
    void updateStartupRegistration(bool enabled);
    void setTrayEnabled(bool enabled);
    void setFpsVisible(bool visible);
    void setClientRefreshRate(int refreshRate);
    void setUiScale(const QVariant& value);
    void setAuthWindowMode(bool enabled);
    void updateFpsLabel();
    int confirmExitSaveChoice();
    void exitClient();
    void checkForClientUpdatesIfEnabled();
    void beginClientUpdate(const QString& versionLabel);
    void finishClientUpdate(const QString& versionLabel);
    void failClientUpdate(const QString& message);
    void handleSensitivityConversion(const QVariantMap& inputs);
    void beginLoadProgress();
    void setLoadWaitingForGame();
    void setLoadGameDetected(qint64 pid, const QString& executableName);
    void setLoadClientReady();
    void finishLoadProgress();
    void failLoadProgress(const QString& message);
    bool saveSafeApplicationSettingsSnapshot() const;
    void restoreSavedScreenRegion();
    void persistScreenRegion(const QRect& region, const QString& displayId);
    bool writeScreenRegionConfig(const QRect& region, const QString& displayId);
    bool writeScreenRegionConfigForRoot(const QString& rootPath, const QRect& region, const QString& displayId);
    void stopRuntimeHelper();
    void startRuntimeHelper();
    void stopNativeDetector();
    void startNativeDetector();
    void restartNativeDetector();
    bool writeNativeDetectorConfig();
    void launchRainbowSixSiege();
    bool shouldStartWindowDrag(QObject* watched, QEvent* event) const;
    void publishOperatorProfile(const QString& operatorId);
    void publishOperatorSettings(const QVariantMap& settings);
    void publishRuntimeSetting(const QString& key, const QVariant& value);
    void publishRuntimeKeybind(const QString& key, const QString& value);
    void publishRuntimeCommand(const QString& endpoint, const QString& payload);
    [[nodiscard]] QString automationPayloadForSettings(const QVariantMap& settings) const;
    [[nodiscard]] double aspectRatioValue(const QString& text) const;

    QStackedWidget* m_rootStack = nullptr;
    AuthFlowWidget* m_authFlow = nullptr;
    AuthenticatedRoot* m_authenticatedRoot = nullptr;
    NexusProgressView* m_loadProgressView = nullptr;
    UpdateProgressPage* m_updateProgressPage = nullptr;
    GameLaunchOverlayWindow* m_gameLaunchOverlay = nullptr;
    LaunchReadinessController* m_launchReadiness = nullptr;
    FirebaseAuthClient* m_firebase = nullptr;
    AutoUpdater* m_autoUpdater = nullptr;
    QNetworkAccessManager* m_runtimeNetwork = nullptr;
    QTcpServer* m_detectionServer = nullptr;
    QSystemTrayIcon* m_trayIcon = nullptr;
    QLabel* m_fpsLabel = nullptr;
    QTimer* m_fpsTimer = nullptr;
    AuthSession m_pendingSession;
    QVariantMap m_lastPublishedOperatorSettings;
    QString m_startupInstallPath;
    QString m_handoffEmail;
    QString m_windowTitle = QStringLiteral("NEXUS");
    int m_clientRefreshRate = 60;
    double m_uiScale = 1.0;
    double m_sensitivityScaleX = 1.0;
    double m_sensitivityScaleY = 1.0;
    bool m_minimizeToTray = true;
    bool m_forceExit = false;
    bool m_demoMode = false;
    bool m_firebaseConfigured = false;
    bool m_autoEnterAuthenticatedArea = false;
    bool m_reloadAfterSignIn = false;
    bool m_authWindowMode = false;
    bool m_updateCheckStarted = false;
    bool m_exitSavePromptHandled = false;
    bool m_saveSettingsOnExit = true;
};
