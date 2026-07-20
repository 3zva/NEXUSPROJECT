#pragma once

#include "authsession.h"

#include <QHash>
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
    void setScreenRegion(const QRect& region, const QString& displayId);
    void clearScreenRegion();
    void setScreenRegionSaveResult(bool success, const QString& message);
    void setOverlayAppSettings(
        bool monitoringEnabled,
        bool showSelectionBorder,
        bool pauseWhenCursorHidden,
        bool lowResourceMode
    );
    void setClientAppSettings(const QVariantMap& settings);
    void setGeneralAppSettings(const QVariantMap& settings);
    void setSensitivityScaleFactors(double horizontalScale, double verticalScale);
    void setSensitivityConversionError(const QString& message);
    [[nodiscard]] bool overlayMonitoringEnabled() const;

    [[nodiscard]] QString installationPath() const;
    void setSuggestedInstallationPath(const QString& path);
    void setLoadedInstallationPath(const QString& path);
    void setOperatorSettings(
        const QString& operatorId,
        const QVariantMap& settings
    );
    [[nodiscard]] QVariantMap operatorSettingsFor(const QString& operatorId) const;

Q_SIGNALS:
    void logoutRequested();
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
    void screenRegionPageRequested();
    void regionSelectionRequested();
    void regionClearRequested();
    void regionSaveRequested(const QRect& region, const QString& displayId);
    void overlayMonitoringEnabledChanged(bool enabled);
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
    [[nodiscard]] QString currentPageKey() const;
    [[nodiscard]] QString defaultGlobalConfigPath() const;
    bool writeGlobalConfig(const QString& path, bool showFeedback);
    bool readGlobalConfig(const QString& path, bool showFeedback);

    AuthSession m_session;
    QString m_installationPath;
    QString m_activeConfigPath;
    QStackedWidget* m_stack = nullptr;
    PathSelectionPage* m_pathPage = nullptr;
    DashboardPage* m_dashboard = nullptr;
    OperatorsPage* m_operators = nullptr;
    SaveFilesPage* m_saveFiles = nullptr;
    ClientSettingsPage* m_clientSettings = nullptr;
    SettingsPage* m_settings = nullptr;
    OperatorSettingsPage* m_operatorSettings = nullptr;
    MoreOptionsPage* m_moreOptions = nullptr;
    SensitivityFovConverterPage* m_sensitivityConverter = nullptr;
    QLabel* m_emailLabel = nullptr;
    QLabel* m_versionLabel = nullptr;
    QPushButton* m_logoutButton = nullptr;
    QPushButton* m_moreButton = nullptr;
    QString m_previousAuthenticatedPageKey = QStringLiteral("dashboard");
    QHash<QString, SidebarButton*> m_navButtons;
    QHash<QString, QWidget*> m_pages;
};
