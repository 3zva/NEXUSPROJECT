#pragma once

#include "authsession.h"

#include <QHash>
#include <QJsonObject>
#include <QPixmap>
#include <QRect>
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QWidget>

class QLabel;
class QPushButton;
class QStackedWidget;
class SidebarButton;
class PathSelectionPage;
class DashboardPage;
class OperatorsPage;
class SaveFilesPage;
class ClientSettingsPage;
class NativeDetectorPage;
class SettingsPage;
class OperatorSettingsPage;
class MoreOptionsPage;
class SensitivityFovConverterPage;

class AuthenticatedRoot final : public QWidget {
    Q_OBJECT

public:
    explicit AuthenticatedRoot(QWidget* parent = nullptr);

    void setSession(const AuthSession& session);
    [[nodiscard]] AuthSession session() const;
    void resetToPathSelection();
    void showPage(const QString& key);
    void showOperatorSettings(const QString& operatorId);

    // Compatibility wrapper for older integrations that still pass a display name.
    void showOperatorDetail(const QString& operatorName);

    [[nodiscard]] QString installationPath() const;
    [[nodiscard]] QVariantMap allOperatorSettings() const;
    void setSuggestedInstallationPath(const QString& path);
    void setLoadedInstallationPath(const QString& path);
    void setOperatorSettings(
        const QString& operatorId,
        const QVariantMap& settings
    );
    [[nodiscard]] QVariantMap operatorSettingsFor(const QString& operatorId) const;
    void setScreenRegion(const QRect& region, const QString& displayId);
    void setSelectedScreenRegion(
        const QRect& region,
        const QString& displayId,
        const QPixmap& preview = QPixmap()
    );
    void clearScreenRegion();
    void clearSelectedScreenRegion();
    void setScreenRegionSaveResult(bool success, const QString& message = QString());
    void setRuntimeHelperAppSettings(
        bool monitoringEnabled,
        bool showSelectionBorder,
        bool pauseWhenCursorHidden,
        bool lowResourceMode
    );
    void setClientAppSettings(const QVariantMap& settings);
    void setGeneralAppSettings(const QVariantMap& settings);
    void setSensitivityScaleFactors(double horizontalScale, double verticalScale);
    void setSensitivityConversionError(const QString& message);
    void setNativeDetectorStatus(bool running, double fps, double inferenceMs, int detections);
    void beginLoadProgress();
    void setLoadWaitingForGame();
    void setLoadGameDetected(qint64 pid, const QString& executableName);
    void setLoadClientReady();
    void finishLoadProgress();
    void failLoadProgress(const QString& message);
    bool saveSafeConfigurationSnapshot();
    [[nodiscard]] bool runtimeHelperMonitoringEnabled() const;

Q_SIGNALS:
    void logoutRequested();
    void loadProgressStarted();
    void installationPathSelected(const QString& path);
    void operatorSelected(const QString& operatorId);
    void operatorSettingsSaveRequested(
        const QString& operatorId,
        const QVariantMap& settings
    );
    void operatorSettingsChanged(
        const QString& operatorId,
        const QString& key,
        const QVariant& value
    );
    void operatorSettingsResetRequested(const QString& operatorId);
    // Typed GUI-to-existing-backend signal. Value mapping is schema v2:
    // 0 off, 1 Weapon 1, 2 Weapon 2, 3 both weapons.
    void rapidFireSelectionChanged(
        const QString& operatorId,
        int rapidFireValue,
        bool enabled
    );
    void operatorLoadoutSelectionChanged(
        const QString& operatorId,
        const QString& weaponSlot,
        const QString& selectedWeapon,
        const QVariantMap& attachments,
        const QVariantMap& converterInputs
    );
    // Emitted after one global config imports and normalizes all 76 records.
    void globalOperatorConfigurationImported(const QVariantMap& operators);
    // UI integration surface for the built-in runtime helper controller.
    void screenRegionPageRequested(); // compatibility notification
    void regionSelectionRequested();
    void regionClearRequested();
    void regionSaveRequested(const QRect& region, const QString& displayId);
    void runtimeHelperMonitoringEnabledChanged(bool enabled);
    void showSelectionBorderChanged(bool enabled);
    void pauseWhenForegroundChanged(bool enabled);
    void lowResourceMonitoringChanged(bool enabled);
    void sensitivityConversionRequested(const QVariantMap& inputs);
    void exportPathSelected(const QString& path);
    void importPathSelected(const QString& path);
    void settingChanged(const QString& key, const QVariant& value);
    void keybindChanged(const QString& key, const QString& value);
    void resetOperatorsRequested();
    void exitRequested();

private:
    void buildSidebar();
    void buildPages();
    void setNavigationAvailable(bool available);
    void requestLogout();
    void handlePathLoad(const QString& path);
    [[nodiscard]] QString durableConfigDirectory() const;
    [[nodiscard]] QString desktopBackupConfigDirectory() const;
    [[nodiscard]] QString defaultGlobalConfigPath() const;
    [[nodiscard]] QString safeGlobalConfigPath() const;
    [[nodiscard]] QString backupGlobalConfigDirectory() const;
    [[nodiscard]] QStringList legacyGlobalConfigCandidates() const;
    [[nodiscard]] QJsonObject currentGlobalConfigObject() const;
    bool copyExistingConfigToBackup(const QString& path, const QString& reason) const;
    bool mirrorConfigBackup(const QJsonObject& root, const QString& reason) const;
    bool writeConfigObject(const QString& path, const QJsonObject& root) const;
    bool writeGlobalConfig(const QString& path, bool showFeedback);
    bool readGlobalConfig(const QString& path, bool showFeedback);
    bool restoreFromSafeGlobalConfig();

    AuthSession m_session;
    QString m_installationPath;
    QString m_activeConfigPath;
    QStackedWidget* m_stack = nullptr;
    PathSelectionPage* m_pathPage = nullptr;
    DashboardPage* m_dashboard = nullptr;
    OperatorsPage* m_operators = nullptr;
    SaveFilesPage* m_saveFiles = nullptr;
    ClientSettingsPage* m_clientSettings = nullptr;
    NativeDetectorPage* m_nativeDetector = nullptr;
    SettingsPage* m_settings = nullptr;
    OperatorSettingsPage* m_operatorSettings = nullptr;
    MoreOptionsPage* m_moreOptions = nullptr;
    SensitivityFovConverterPage* m_sensitivityConverter = nullptr;
    QLabel* m_emailLabel = nullptr;
    QLabel* m_versionLabel = nullptr;
    QPushButton* m_logoutButton = nullptr;
    QPushButton* m_moreButton = nullptr;
    QHash<QString, SidebarButton*> m_navButtons;
    QHash<QString, QWidget*> m_pages;
    QString m_currentPageKey;
    QString m_previousPageKey = QStringLiteral("dashboard");
};
