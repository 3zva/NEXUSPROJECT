#include "authenticatedroot.h"
#include "nexuswidgets.h"
#include "operatorcatalog.h"
#include "pages.h"
#include "pathselectionpage.h"
#include "sensitivityfovconverterpage.h"
#include "theme.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonArray>
#include <QFrame>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSaveFile>
#include <QStandardPaths>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QStyle>
#include <QVBoxLayout>
#include <utility>

namespace {
double legacyDelayToSeconds(double value) {
    return qMax(0.001, value / 1000.0);
}

QVariantMap weaponSettingsFromLegacyRow(const QJsonArray& row, int offset) {
    return {
        {QStringLiteral("x_amount"), row.at(offset).toDouble()},
        {QStringLiteral("y_amount"), row.at(offset + 1).toDouble()},
        {QStringLiteral("time_delay"), legacyDelayToSeconds(row.at(offset + 2).toDouble())},
    };
}

QVariantMap operatorSettingsFromLegacyRow(
    const QString& operatorId,
    const QString& rowText,
    const QString& rapidText
) {
    QJsonParseError rowError;
    const QJsonDocument rowDocument = QJsonDocument::fromJson(rowText.toUtf8(), &rowError);
    if (rowError.error != QJsonParseError::NoError || !rowDocument.isArray()) {
        return {};
    }

    const QJsonArray row = rowDocument.array();
    if (row.size() < 6) {
        return {};
    }

    bool rapidOk = false;
    const int rapidValue = rapidText.toInt(&rapidOk);
    const int preservedRapid = rapidOk ? rapidValue : 0;

    QVariantMap settings;
    settings.insert(QStringLiteral("operator_id"), operatorId);
    settings.insert(QStringLiteral("active_weapon"), QStringLiteral("primary"));
    settings.insert(QStringLiteral("profile_enabled"), true);
    settings.insert(QStringLiteral("auto_load"), true);
    settings.insert(QStringLiteral("show_overlay"), true);
    settings.insert(QStringLiteral("monitor_while_active"), true);
    settings.insert(QStringLiteral("rapid_fire_enabled"), preservedRapid != 0);
    settings.insert(QStringLiteral("rapid_fire_value"), preservedRapid);
    settings.insert(QStringLiteral("notes"), row.at(6).toString());
    settings.insert(QStringLiteral("primary"), weaponSettingsFromLegacyRow(row, 0));
    settings.insert(QStringLiteral("secondary"), weaponSettingsFromLegacyRow(row, 3));
    return settings;
}

bool convertLegacyV5ConfigToGlobal(
    const QByteArray& data,
    QVariantMap& operators,
    QString& errorMessage
) {
    const QByteArray trimmed = data.trimmed();
    constexpr auto prefix = "^^^V5";
    if (!trimmed.startsWith(prefix)) {
        errorMessage = QStringLiteral("This is not a V5 operator config.");
        return false;
    }

    QJsonParseError rootError;
    const QJsonDocument rootDocument = QJsonDocument::fromJson(
        trimmed.mid(static_cast<int>(std::char_traits<char>::length(prefix))),
        &rootError
    );
    if (rootError.error != QJsonParseError::NoError || !rootDocument.isArray()) {
        errorMessage = rootError.errorString();
        return false;
    }

    const QJsonArray root = rootDocument.array();
    QString operatorMapText;
    QString rapidMapText;
    for (int index = 0; index + 1 < root.size(); index += 2) {
        const QString key = root.at(index).toString();
        if (key == QStringLiteral("OPERATOR_SETTINGS_BY_NAME")) {
            operatorMapText = root.at(index + 1).toString();
        } else if (key == QStringLiteral("RAPID_FIRE_BY_NAME")) {
            rapidMapText = root.at(index + 1).toString();
        }
    }

    if (operatorMapText.isEmpty()) {
        errorMessage = QStringLiteral("The V5 config is missing OPERATOR_SETTINGS_BY_NAME.");
        return false;
    }

    QJsonParseError operatorError;
    const QJsonDocument operatorDocument = QJsonDocument::fromJson(
        operatorMapText.toUtf8(),
        &operatorError
    );
    if (operatorError.error != QJsonParseError::NoError || !operatorDocument.isObject()) {
        errorMessage = QStringLiteral("The V5 operator map is invalid.");
        return false;
    }

    QJsonObject rapidObject;
    if (!rapidMapText.isEmpty()) {
        QJsonParseError rapidError;
        const QJsonDocument rapidDocument = QJsonDocument::fromJson(
            rapidMapText.toUtf8(),
            &rapidError
        );
        if (rapidError.error == QJsonParseError::NoError && rapidDocument.isObject()) {
            rapidObject = rapidDocument.object();
        }
    }

    const QJsonObject operatorObject = operatorDocument.object();
    QVariantMap imported;
    for (const auto& record : OperatorCatalog::all()) {
        const QString rowText = operatorObject.value(record.id).toString();
        const QString rapidText = rapidObject.value(record.id).toString(QStringLiteral("0"));
        const QVariantMap settings = operatorSettingsFromLegacyRow(record.id, rowText, rapidText);
        if (!settings.isEmpty()) {
            imported.insert(record.id, settings);
        }
    }

    if (imported.size() != OperatorCatalog::all().size()) {
        errorMessage = QStringLiteral("The V5 config did not contain all 76 operators.");
        return false;
    }

    operators = imported;
    return true;
}
}

AuthenticatedRoot::AuthenticatedRoot(QWidget* parent)
    : QWidget(parent) {
    setObjectName(QStringLiteral("appRoot"));
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* sidebar = new QFrame(this);
    sidebar->setObjectName(QStringLiteral("sidebar"));
    sidebar->setFixedWidth(NexusTheme::SidebarWidth);
    auto* sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(18, 22, 18, 18);
    sidebarLayout->setSpacing(7);
    sidebar->setLayout(sidebarLayout);

    root->addWidget(sidebar);

    auto* contentShell = new QWidget(this);
    auto* contentLayout = new QVBoxLayout(contentShell);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    auto* topBar = new QHBoxLayout();
    topBar->setContentsMargins(16, 14, 22, 0);
    topBar->addStretch();
    m_moreButton = new QPushButton(QStringLiteral("MORE"), contentShell);
    m_moreButton->setObjectName(QStringLiteral("moreOptionsButton"));
    m_moreButton->setIcon(NexusTheme::icon(QStringLiteral("settings_32.png")));
    m_moreButton->setFixedWidth(96);
    m_moreButton->setToolTip(QStringLiteral("Open More Options"));
    m_moreButton->setCursor(Qt::PointingHandCursor);
    topBar->addWidget(m_moreButton);
    contentLayout->addLayout(topBar);

    m_stack = new QStackedWidget(contentShell);
    contentLayout->addWidget(m_stack, 1);
    root->addWidget(contentShell, 1);

    // Build the persistent sidebar manually so it remains mounted while pages change.
    auto* brandRow = new QHBoxLayout();
    brandRow->setSpacing(10);
    auto* logo = new QLabel(sidebar);
    logo->setPixmap(NexusTheme::pixmap(QStringLiteral("nexus_logo_64.png"), 46, 46));
    auto* brand = new QLabel(QStringLiteral("NEXUS"), sidebar);
    brand->setFont(NexusTheme::font(14, QFont::Bold));
    brandRow->addWidget(logo);
    brandRow->addWidget(brand);
    brandRow->addStretch();
    sidebarLayout->addLayout(brandRow);
    sidebarLayout->addSpacing(18);

    struct NavDefinition { QString key; QString label; QString icon; };
    const QList<NavDefinition> navigation{
        {QStringLiteral("dashboard"), QStringLiteral("Dashboard"), QStringLiteral("dashboard")},
        {QStringLiteral("operators"), QStringLiteral("Operators"), QStringLiteral("operators")},
        {QStringLiteral("save_files"), QStringLiteral("Save Files"), QStringLiteral("save")},
        {QStringLiteral("sensitivity_converter"), QStringLiteral("Sensitivity & FOV"), QStringLiteral("target")},
        {QStringLiteral("client"), QStringLiteral("Client Settings"), QStringLiteral("client")},
        {QStringLiteral("settings"), QStringLiteral("Settings"), QStringLiteral("settings")},
    };
    for (const auto& definition : navigation) {
        auto* button = new SidebarButton(definition.label, definition.icon, sidebar);
        connect(button, &QPushButton::clicked, this, [this, key = definition.key]() {
            showPage(key);
        });
        sidebarLayout->addWidget(button);
        m_navButtons.insert(definition.key, button);
    }

    sidebarLayout->addStretch();

    auto* accountCard = new CardFrame(sidebar, false);
    auto* accountLayout = new QVBoxLayout(accountCard);
    accountLayout->setContentsMargins(12, 12, 12, 12);
    accountLayout->setSpacing(5);
    auto* signedIn = new QLabel(QStringLiteral("SIGNED IN"), accountCard);
    signedIn->setProperty("subtle", true);
    signedIn->setFont(NexusTheme::font(8, QFont::Bold));
    m_emailLabel = new QLabel(accountCard);
    m_emailLabel->setFont(NexusTheme::font(9, QFont::DemiBold));
    m_emailLabel->setWordWrap(true);
    m_logoutButton = new QPushButton(QStringLiteral("Log out"), accountCard);
    m_logoutButton->setIcon(NexusTheme::icon(QStringLiteral("logout_32.png")));
    m_logoutButton->setCursor(Qt::PointingHandCursor);
    connect(m_logoutButton, &QPushButton::clicked, this, &AuthenticatedRoot::requestLogout);
    accountLayout->addWidget(signedIn);
    accountLayout->addWidget(m_emailLabel);
    accountLayout->addSpacing(4);
    accountLayout->addWidget(m_logoutButton);
    sidebarLayout->addWidget(accountCard);

    auto* footer = new QHBoxLayout();
    m_versionLabel = new QLabel(QStringLiteral("NEXUS v") + QString::fromLatin1(NEXUS_APP_VERSION), sidebar);
    m_versionLabel->setProperty("subtle", true);
    m_versionLabel->setFont(NexusTheme::font(8));
    auto* ready = new QLabel(QStringLiteral("● Ready"), sidebar);
    ready->setProperty("success", true);
    ready->setFont(NexusTheme::font(8));
    footer->addWidget(m_versionLabel);
    footer->addStretch();
    footer->addWidget(ready);
    sidebarLayout->addLayout(footer);

    buildPages();
    connect(m_moreButton, &QPushButton::clicked, this, [this]() {
        showPage(QStringLiteral("more_options"));
    });
    m_activeConfigPath = defaultGlobalConfigPath();
    if (QFileInfo::exists(m_activeConfigPath)) {
        readGlobalConfig(m_activeConfigPath, false);
    } else {
        const QString bundledConfigPath = QDir(QCoreApplication::applicationDirPath())
            .filePath(QStringLiteral("Nexus Config.json"));
        if (QFileInfo::exists(bundledConfigPath)
            && readGlobalConfig(bundledConfigPath, false)) {
            writeGlobalConfig(m_activeConfigPath, false);
        }
    }
    resetToPathSelection();
}

void AuthenticatedRoot::buildPages() {
    m_pathPage = new PathSelectionPage(m_stack);
    m_dashboard = new DashboardPage(m_stack);
    m_operators = new OperatorsPage(m_stack);
    m_saveFiles = new SaveFilesPage(m_stack);
    m_clientSettings = new ClientSettingsPage(m_stack);
    m_settings = new SettingsPage(m_stack);
    // Exactly one shared operator-settings page is created for all operators.
    m_operatorSettings = new OperatorSettingsPage(m_stack);
    m_moreOptions = new MoreOptionsPage(m_stack);
    m_sensitivityConverter = new SensitivityFovConverterPage(m_stack);

    m_stack->addWidget(m_pathPage);
    m_stack->addWidget(m_dashboard);
    m_stack->addWidget(m_operators);
    m_stack->addWidget(m_saveFiles);
    m_stack->addWidget(m_clientSettings);
    m_stack->addWidget(m_settings);
    m_stack->addWidget(m_sensitivityConverter);
    m_stack->addWidget(m_operatorSettings);
    m_stack->addWidget(m_moreOptions);

    m_pages.insert(QStringLiteral("dashboard"), m_dashboard);
    m_pages.insert(QStringLiteral("operators"), m_operators);
    m_pages.insert(QStringLiteral("save_files"), m_saveFiles);
    m_pages.insert(QStringLiteral("client"), m_clientSettings);
    m_pages.insert(QStringLiteral("settings"), m_settings);
    m_pages.insert(QStringLiteral("sensitivity_converter"), m_sensitivityConverter);
    m_pages.insert(QStringLiteral("more_options"), m_moreOptions);

    connect(m_pathPage, &PathSelectionPage::loadRequested,
            this, &AuthenticatedRoot::handlePathLoad);
    connect(m_dashboard, &DashboardPage::navigateRequested,
            this, &AuthenticatedRoot::showPage);
    connect(m_operators, &OperatorsPage::operatorSelected, this, [this](const QString& operatorId) {
        showOperatorSettings(operatorId);
        Q_EMIT operatorSelected(operatorId);
    });

    connect(m_operatorSettings, &OperatorSettingsPage::backRequested, this, [this]() {
        showPage(QStringLiteral("operators"));
    });
    connect(m_operatorSettings, &OperatorSettingsPage::saveRequested,
            this, [this](const QString& operatorId, const QVariantMap& settings) {
        Q_EMIT operatorSettingsSaveRequested(operatorId, settings);
        writeGlobalConfig(m_activeConfigPath, false);
    });
    connect(m_operatorSettings, &OperatorSettingsPage::settingChanged,
            this, &AuthenticatedRoot::operatorSettingsChanged);
    connect(m_operatorSettings, &OperatorSettingsPage::resetRequested,
            this, [this](const QString& operatorId) {
        Q_EMIT operatorSettingsResetRequested(operatorId);
        writeGlobalConfig(m_activeConfigPath, false);
    });
    connect(m_operatorSettings, &OperatorSettingsPage::screenRegionPageRequested, this, [this]() {
        Q_EMIT screenRegionPageRequested();
        showPage(QStringLiteral("more_options"));
    });

    connect(m_moreOptions, &MoreOptionsPage::backRequested, this, [this]() {
        showPage(m_previousAuthenticatedPageKey.isEmpty()
            ? QStringLiteral("dashboard")
            : m_previousAuthenticatedPageKey);
    });
    connect(m_moreOptions, &MoreOptionsPage::regionSelectionRequested,
            this, &AuthenticatedRoot::regionSelectionRequested);
    connect(m_moreOptions, &MoreOptionsPage::regionClearRequested,
            this, &AuthenticatedRoot::regionClearRequested);
    connect(m_moreOptions, &MoreOptionsPage::regionSaveRequested,
            this, &AuthenticatedRoot::regionSaveRequested);
    connect(m_moreOptions, &MoreOptionsPage::overlayMonitoringEnabledChanged,
            this, &AuthenticatedRoot::overlayMonitoringEnabledChanged);
    connect(m_moreOptions, &MoreOptionsPage::showSelectionBorderChanged,
            this, &AuthenticatedRoot::showSelectionBorderChanged);
    connect(m_moreOptions, &MoreOptionsPage::pauseWhenForegroundChanged,
            this, &AuthenticatedRoot::pauseWhenForegroundChanged);
    connect(m_moreOptions, &MoreOptionsPage::lowResourceMonitoringChanged,
            this, &AuthenticatedRoot::lowResourceMonitoringChanged);
    connect(m_sensitivityConverter, &SensitivityFovConverterPage::conversionRequested,
            this, &AuthenticatedRoot::sensitivityConversionRequested);

    connect(m_saveFiles, &SaveFilesPage::settingChanged,
            this, &AuthenticatedRoot::settingChanged);
    connect(m_saveFiles, &SaveFilesPage::exportRequested, this, [this]() {
        const auto path = QFileDialog::getSaveFileName(
            this,
            QStringLiteral("Export complete NEXUS configuration"),
            QStringLiteral("nexus-config.nexus"),
            QStringLiteral("NEXUS configuration (*.nexus);;JSON (*.json);;All files (*.*)")
        );
        if (!path.isEmpty() && writeGlobalConfig(path, true)) {
            Q_EMIT exportPathSelected(path);
        }
    });
    connect(m_saveFiles, &SaveFilesPage::importRequested, this, [this]() {
        const auto path = QFileDialog::getOpenFileName(
            this,
            QStringLiteral("Import complete NEXUS configuration"),
            QString(),
            QStringLiteral("NEXUS configuration (*.nexus *.json *.txt);;Original V5 config (*.txt *.json);;All files (*.*)")
        );
        if (!path.isEmpty() && readGlobalConfig(path, true)) {
            // The selected file updates all operator records, then becomes the
            // active data copied into NEXUS's single local configuration file.
            writeGlobalConfig(m_activeConfigPath, false);
            Q_EMIT importPathSelected(path);
        }
    });

    connect(m_clientSettings, &ClientSettingsPage::settingChanged,
            this, &AuthenticatedRoot::settingChanged);
    connect(m_clientSettings, &ClientSettingsPage::exitRequested,
            this, &AuthenticatedRoot::exitRequested);

    connect(m_settings, &SettingsPage::settingChanged,
            this, &AuthenticatedRoot::settingChanged);
    connect(m_settings, &SettingsPage::keybindChanged,
            this, &AuthenticatedRoot::keybindChanged);
    connect(m_settings, &SettingsPage::resetOperatorsRequested, this, [this]() {
        const auto response = QMessageBox::warning(
            this,
            QStringLiteral("Reset all operators"),
            QStringLiteral(
                "Reset the Primary, Secondary, notes, toggles, and status values "
                "for all 76 operators?"
            ),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );
        if (response != QMessageBox::Yes) {
            return;
        }
        m_operatorSettings->resetAllOperatorSettings();
        writeGlobalConfig(m_activeConfigPath, false);
        Q_EMIT resetOperatorsRequested();
    });
}

void AuthenticatedRoot::setSession(const AuthSession& session) {
    m_session = session;
    m_emailLabel->setText(session.email.isEmpty()
        ? QStringLiteral("Authenticated user")
        : session.email);
}

AuthSession AuthenticatedRoot::session() const {
    return m_session;
}

void AuthenticatedRoot::resetToPathSelection() {
    m_installationPath.clear();
    setNavigationAvailable(false);
    m_previousAuthenticatedPageKey = QStringLiteral("dashboard");
    for (auto* button : std::as_const(m_navButtons)) {
        button->setActive(false);
    }
    m_moreButton->setProperty("navActive", false);
    m_moreButton->style()->unpolish(m_moreButton);
    m_moreButton->style()->polish(m_moreButton);
    m_stack->setCurrentWidget(m_pathPage);
}

void AuthenticatedRoot::showPage(const QString& key) {
    if (!m_pages.contains(key)) {
        qWarning("Unknown NEXUS page requested: %s", qPrintable(key));
        return;
    }
    const QString previousKey = currentPageKey();
    if (key == QStringLiteral("more_options")
        && !previousKey.isEmpty()
        && previousKey != QStringLiteral("more_options")) {
        m_previousAuthenticatedPageKey = previousKey == QStringLiteral("operator_settings")
            ? QStringLiteral("operators")
            : previousKey;
    }
    setNavigationAvailable(true);
    m_stack->setCurrentWidget(m_pages.value(key));
    for (auto iterator = m_navButtons.begin(); iterator != m_navButtons.end(); ++iterator) {
        iterator.value()->setActive(
            iterator.key() == key
            || (key == QStringLiteral("more_options")
                && iterator.key() == m_previousAuthenticatedPageKey)
        );
    }
    m_moreButton->setProperty("navActive", key == QStringLiteral("more_options"));
    m_moreButton->style()->unpolish(m_moreButton);
    m_moreButton->style()->polish(m_moreButton);
}

void AuthenticatedRoot::showOperatorSettings(const QString& operatorId) {
    const QString resolvedId = OperatorCatalog::resolveId(operatorId);
    if (resolvedId.isEmpty() || !m_operatorSettings->setOperator(resolvedId)) {
        return;
    }

    setNavigationAvailable(true);
    m_stack->setCurrentWidget(m_operatorSettings);
    for (auto iterator = m_navButtons.begin(); iterator != m_navButtons.end(); ++iterator) {
        iterator.value()->setActive(iterator.key() == QStringLiteral("operators"));
    }
    m_moreButton->setProperty("navActive", false);
    m_moreButton->style()->unpolish(m_moreButton);
    m_moreButton->style()->polish(m_moreButton);
}

void AuthenticatedRoot::setScreenRegion(const QRect& region, const QString& displayId) {
    m_moreOptions->setSelectedRegion(region, displayId);
}

void AuthenticatedRoot::clearScreenRegion() {
    m_moreOptions->clearSelectedRegion();
}

void AuthenticatedRoot::setScreenRegionSaveResult(bool success, const QString& message) {
    m_moreOptions->setRegionSaveResult(success, message);
}

void AuthenticatedRoot::setOverlayAppSettings(
    bool monitoringEnabled,
    bool showSelectionBorder,
    bool pauseWhenCursorHidden,
    bool lowResourceMode
) {
    m_moreOptions->setOverlaySettings(
        monitoringEnabled,
        showSelectionBorder,
        pauseWhenCursorHidden,
        lowResourceMode
    );
}

void AuthenticatedRoot::setClientAppSettings(const QVariantMap& settings) {
    m_clientSettings->setSavedSettings(settings);
}

void AuthenticatedRoot::setGeneralAppSettings(const QVariantMap& settings) {
    m_settings->setSavedSettings(settings);
}

void AuthenticatedRoot::setSensitivityScaleFactors(double horizontalScale, double verticalScale) {
    if (m_sensitivityConverter != nullptr) {
        m_sensitivityConverter->setScaleFactors(horizontalScale, verticalScale);
    }
}

void AuthenticatedRoot::setSensitivityConversionError(const QString& message) {
    if (m_sensitivityConverter != nullptr) {
        m_sensitivityConverter->setCalculationError(message);
    }
}

bool AuthenticatedRoot::overlayMonitoringEnabled() const {
    return m_moreOptions->overlayMonitoringEnabled();
}

void AuthenticatedRoot::showOperatorDetail(const QString& operatorName) {
    const QString resolvedId = OperatorCatalog::resolveId(operatorName);
    if (!resolvedId.isEmpty()) {
        showOperatorSettings(resolvedId);
        return;
    }

    // Also accept a stable ID through the old method so existing callers do not break.
    showOperatorSettings(operatorName);
}

void AuthenticatedRoot::setOperatorSettings(
    const QString& operatorId,
    const QVariantMap& settings
) {
    m_operatorSettings->setSettingsFor(operatorId, settings);
}

QVariantMap AuthenticatedRoot::operatorSettingsFor(const QString& operatorId) const {
    return m_operatorSettings->settingsFor(operatorId);
}

QString AuthenticatedRoot::installationPath() const {
    return m_installationPath;
}

void AuthenticatedRoot::setSuggestedInstallationPath(const QString& path) {
    m_pathPage->setSelectedPath(path);
}

void AuthenticatedRoot::setLoadedInstallationPath(const QString& path) {
    m_installationPath = path;
    m_pathPage->setSelectedPath(path);
    m_pathPage->showStatus(QStringLiteral("NEXUS Recoil loaded."));
    showPage(QStringLiteral("dashboard"));
}

void AuthenticatedRoot::setNavigationAvailable(bool available) {
    for (auto* button : std::as_const(m_navButtons)) {
        button->setVisible(available);
        button->setEnabled(available);
    }
    m_moreButton->setVisible(available);
    m_moreButton->setEnabled(available);
}

void AuthenticatedRoot::requestLogout() {
    const auto response = QMessageBox::question(
        this,
        QStringLiteral("Log out"),
        QStringLiteral("Are you sure you want to log out of NEXUS?"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    if (response == QMessageBox::Yes) {
        m_session.clear();
        m_installationPath.clear();
        Q_EMIT logoutRequested();
    }
}

void AuthenticatedRoot::handlePathLoad(const QString& path) {
    m_installationPath = path;
    m_pathPage->showStatus(QStringLiteral("Path selected. Loading NEXUS client..."));
    Q_EMIT installationPathSelected(path);
    showPage(QStringLiteral("dashboard"));
}

QString AuthenticatedRoot::currentPageKey() const {
    if (m_stack->currentWidget() == m_operatorSettings) {
        return QStringLiteral("operator_settings");
    }
    for (auto iterator = m_pages.constBegin(); iterator != m_pages.constEnd(); ++iterator) {
        if (iterator.value() == m_stack->currentWidget()) {
            return iterator.key();
        }
    }
    return QString();
}

QString AuthenticatedRoot::defaultGlobalConfigPath() const {
    const QString directory = QStandardPaths::writableLocation(
        QStandardPaths::AppConfigLocation
    );
    QDir().mkpath(directory);
    return QDir(directory).filePath(QStringLiteral("nexus-config.json"));
}

bool AuthenticatedRoot::writeGlobalConfig(
    const QString& path,
    bool showFeedback
) {
    if (path.trimmed().isEmpty()) {
        return false;
    }

    QJsonObject root;
    root.insert(QStringLiteral("format"), QStringLiteral("nexus-global-config"));
    root.insert(QStringLiteral("schema_version"), 1);
    root.insert(QStringLiteral("operator_count"), OperatorCatalog::all().size());
    root.insert(
        QStringLiteral("operators"),
        QJsonObject::fromVariantMap(m_operatorSettings->allOperatorSettings())
    );

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (showFeedback) {
            QMessageBox::warning(
                this,
                QStringLiteral("Could not save configuration"),
                file.errorString()
            );
        }
        return false;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        if (showFeedback) {
            QMessageBox::warning(
                this,
                QStringLiteral("Could not finish saving"),
                file.errorString()
            );
        }
        return false;
    }

    if (showFeedback) {
        QMessageBox::information(
            this,
            QStringLiteral("NEXUS configuration saved"),
            QStringLiteral(
                "Saved one configuration containing all %1 operators."
            ).arg(OperatorCatalog::all().size())
        );
    }
    return true;
}

bool AuthenticatedRoot::readGlobalConfig(
    const QString& path,
    bool showFeedback
) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (showFeedback) {
            QMessageBox::warning(
                this,
                QStringLiteral("Could not open configuration"),
                file.errorString()
            );
        }
        return false;
    }

    const QByteArray rawData = file.readAll();
    QVariantMap legacyOperators;
    QString legacyError;
    if (convertLegacyV5ConfigToGlobal(rawData, legacyOperators, legacyError)) {
        m_operatorSettings->replaceAllOperatorSettings(legacyOperators);
        if (showFeedback) {
            QMessageBox::information(
                this,
                QStringLiteral("NEXUS configuration imported"),
                QStringLiteral(
                    "Imported the original V5 operator config and preserved all 76 operator values."
                )
            );
        }
        return true;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(
        rawData,
        &parseError
    );
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (showFeedback) {
            QMessageBox::warning(
                this,
                QStringLiteral("Invalid NEXUS configuration"),
                parseError.errorString()
            );
        }
        return false;
    }

    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("format")).toString()
            != QStringLiteral("nexus-global-config")
        || !root.value(QStringLiteral("operators")).isObject()) {
        if (showFeedback) {
            QMessageBox::warning(
                this,
                QStringLiteral("Unsupported configuration"),
                QStringLiteral(
                    "This is not a complete NEXUS global configuration file."
                )
            );
        }
        return false;
    }

    const QVariantMap operators = root.value(
        QStringLiteral("operators")
    ).toObject().toVariantMap();
    m_operatorSettings->replaceAllOperatorSettings(operators);

    if (showFeedback) {
        QMessageBox::information(
            this,
            QStringLiteral("NEXUS configuration imported"),
            QStringLiteral(
                "All operator data was imported and the shared settings page "
                "was refreshed."
            )
        );
    }
    return true;
}
