#pragma once

#include "authsession.h"

#include <QMainWindow>
#include <QRect>
#include <QVariantMap>

class AuthFlowWidget;
class AuthenticatedRoot;
class FirebaseAuthClient;
class QLabel;
class QEvent;
class QResizeEvent;
class QSystemTrayIcon;
class QNetworkAccessManager;
class QTcpServer;
class QTimer;
class QStackedWidget;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void connectAuthentication();
    void connectApplicationPages();
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
    void updateFpsLabel();
    void exitClient();
    void handleSensitivityConversion(const QVariantMap& inputs);
    void restoreSavedScreenRegion();
    void persistScreenRegion(const QRect& region, const QString& displayId);
    bool writeScreenRegionConfig(const QRect& region, const QString& displayId);
    bool writeScreenRegionConfigForRoot(const QString& rootPath, const QRect& region, const QString& displayId);
    void stopVisionAutomationTool();
    void startVisionAutomationTool();
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
    FirebaseAuthClient* m_firebase = nullptr;
    QNetworkAccessManager* m_runtimeNetwork = nullptr;
    QTcpServer* m_detectionServer = nullptr;
    QSystemTrayIcon* m_trayIcon = nullptr;
    QLabel* m_fpsLabel = nullptr;
    QTimer* m_fpsTimer = nullptr;
    AuthSession m_pendingSession;
    QVariantMap m_lastPublishedOperatorSettings;
    QString m_startupInstallPath;
    QString m_handoffEmail;
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
};
