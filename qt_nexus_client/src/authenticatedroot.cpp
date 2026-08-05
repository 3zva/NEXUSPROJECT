#include "authenticatedroot.h"
#include "nexuswidgets.h"
#include "moreoptionspage.h"
#include "operatorcatalog.h"
#include "pages.h"
#include "pathselectionpage.h"
#include "sensitivityfovconverterpage.h"
#include "theme.h"

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QStackedWidget>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>
#include <utility>

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
    sidebarLayout->setContentsMargins(18, 18, 18, 24);
    sidebarLayout->setSpacing(10);
    sidebar->setLayout(sidebarLayout);

    root->addWidget(sidebar);

    // Persistent content shell. The ellipsis / More button stays mounted while
    // only the stacked page beneath it changes.
    auto* contentShell = new QWidget(this);
    auto* contentLayout = new QVBoxLayout(contentShell);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    auto* topBar = new QFrame(contentShell);
    topBar->setObjectName(QStringLiteral("authenticatedTopBar"));
    topBar->setFixedHeight(72);
    topBar->setStyleSheet(QStringLiteral(
        "QFrame#authenticatedTopBar { background: transparent; border: none; }"
    ));
    auto* topBarLayout = new QHBoxLayout(topBar);
    topBarLayout->setContentsMargins(20, 18, 22, 8);
    topBarLayout->addStretch();

    m_moreButton = new QPushButton(QStringLiteral("MORE"), topBar);
    m_moreButton->setObjectName(QStringLiteral("moreOptionsButton"));
    m_moreButton->setAccessibleName(QStringLiteral("More options"));
    m_moreButton->setToolTip(QStringLiteral("More options"));
    m_moreButton->setCursor(Qt::PointingHandCursor);
    m_moreButton->setIcon(NexusTheme::icon(QStringLiteral("settings_32.png")));
    m_moreButton->setIconSize(QSize(18, 18));
    m_moreButton->setFixedSize(108, 46);
    m_moreButton->setFont(NexusTheme::font(10, QFont::Bold));
    connect(m_moreButton, &QPushButton::clicked, this, [this]() {
        showPage(QStringLiteral("more_options"));
    });
    topBarLayout->addWidget(m_moreButton);
    contentLayout->addWidget(topBar);

    m_stack = new QStackedWidget(contentShell);
    contentLayout->addWidget(m_stack, 1);
    root->addWidget(contentShell, 1);

    // Build the persistent sidebar manually so it remains mounted while pages change.
    auto* brandRow = new QHBoxLayout();
    brandRow->setSpacing(10);
    auto* logo = new QLabel(sidebar);
    logo->setPixmap(NexusTheme::pixmap(QStringLiteral("nexus_logo_64.png"), 44, 44));
    auto* brand = new QLabel(QStringLiteral("NEXUS"), sidebar);
    brand->setFont(NexusTheme::font(14, QFont::Bold));
    brandRow->addWidget(logo);
    brandRow->addWidget(brand);
    brandRow->addStretch();
    sidebarLayout->addLayout(brandRow);
    sidebarLayout->addSpacing(26);

    struct NavDefinition { QString key; QString label; QString icon; };
    const QList<NavDefinition> navigation{
        {QStringLiteral("dashboard"), QStringLiteral("Dashboard"), QStringLiteral("dashboard")},
        {QStringLiteral("operators"), QStringLiteral("Operators"), QStringLiteral("operators")},
        {QStringLiteral("sensitivity_converter"), QStringLiteral("Sensitivity & FOV"), QStringLiteral("target")},
        {QStringLiteral("save_files"), QStringLiteral("Save Files"), QStringLiteral("save")},
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
    accountLayout->setContentsMargins(14, 14, 14, 14);
    accountLayout->setSpacing(7);
    auto* signedIn = new QLabel(QStringLiteral("SIGNED IN"), accountCard);
    signedIn->setProperty("subtle", true);
    signedIn->setFont(NexusTheme::font(8, QFont::Bold));
    m_emailLabel = new QLabel(accountCard);
    m_emailLabel->setFont(NexusTheme::font(9, QFont::DemiBold));
    m_emailLabel->setWordWrap(true);
    m_logoutButton = new QPushButton(QStringLiteral("Log out"), accountCard);
    m_logoutButton->setIcon(NexusTheme::icon(QStringLiteral("logout_32.png")));
    m_logoutButton->setCursor(Qt::PointingHandCursor);
    m_logoutButton->setMinimumHeight(40);
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

    auto* exitClientButton = new SidebarButton(QStringLiteral("Exit Client"), QStringLiteral("logout"), sidebar);
    exitClientButton->setProperty("navDanger", true);
    connect(exitClientButton, &QPushButton::clicked, this, &AuthenticatedRoot::exitRequested);
    sidebarLayout->addWidget(exitClientButton);
    m_navButtons.insert(QStringLiteral("exit_client"), exitClientButton);

    buildPages();
    m_activeConfigPath = defaultGlobalConfigPath();
    bool configLoaded = false;
    if (QFileInfo::exists(m_activeConfigPath)) {
        configLoaded = readGlobalConfig(m_activeConfigPath, false);
    }
    if (!configLoaded) {
        restoreFromSafeGlobalConfig();
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
    m_moreOptions = new MoreOptionsPage(m_stack);
    m_sensitivityConverter = new SensitivityFovConverterPage(m_stack);
    // Exactly one shared operator-settings page is created for all operators.
    m_operatorSettings = new OperatorSettingsPage(m_stack);

    m_stack->addWidget(m_pathPage);
    m_stack->addWidget(m_dashboard);
    m_stack->addWidget(m_operators);
    m_stack->addWidget(m_saveFiles);
    m_stack->addWidget(m_clientSettings);
    m_stack->addWidget(m_settings);
    m_stack->addWidget(m_moreOptions);
    m_stack->addWidget(m_sensitivityConverter);
    m_stack->addWidget(m_operatorSettings);

    m_pages.insert(QStringLiteral("dashboard"), m_dashboard);
    m_pages.insert(QStringLiteral("operators"), m_operators);
    m_pages.insert(QStringLiteral("save_files"), m_saveFiles);
    m_pages.insert(QStringLiteral("client"), m_clientSettings);
    m_pages.insert(QStringLiteral("settings"), m_settings);
    m_pages.insert(QStringLiteral("more_options"), m_moreOptions);
    m_pages.insert(QStringLiteral("sensitivity_converter"), m_sensitivityConverter);

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
    connect(m_operatorSettings, &OperatorSettingsPage::rapidFireSelectionChanged,
            this, &AuthenticatedRoot::rapidFireSelectionChanged);
    connect(m_operatorSettings, &OperatorSettingsPage::loadoutSelectionChanged,
            this, [this](
                const QString& operatorId,
                const QString& weaponSlot,
                const QString& selectedWeapon,
                const QVariantMap& attachments
            ) {
        QVariantMap resolvedAttachments = attachments;
        const QVariantMap converterInputs = m_sensitivityConverter->currentInputs();
        const QString profileKey = attachments.value(
            QStringLiteral("ads_profile_key"),
            QStringLiteral("ads_1x")
        ).toString();
        resolvedAttachments.insert(
            QStringLiteral("resolved_ads_value"),
            converterInputs.value(
                profileKey == QStringLiteral("ads_2_5x")
                    ? QStringLiteral("ads_2_5x")
                    : QStringLiteral("ads_1x")
            )
        );
        Q_EMIT operatorLoadoutSelectionChanged(
            operatorId,
            weaponSlot,
            selectedWeapon,
            resolvedAttachments,
            converterInputs
        );
    });
    connect(m_operatorSettings, &OperatorSettingsPage::resetRequested,
            this, [this](const QString& operatorId) {
        Q_EMIT operatorSettingsResetRequested(operatorId);
        writeGlobalConfig(m_activeConfigPath, false);
    });
    connect(m_operatorSettings, &OperatorSettingsPage::screenRegionPageRequested,
            this, [this]() {
        Q_EMIT screenRegionPageRequested();
        showPage(QStringLiteral("more_options"));
    });

    connect(m_moreOptions, &MoreOptionsPage::backRequested, this, [this]() {
        const QString destination = m_previousPageKey.isEmpty()
            ? QStringLiteral("dashboard")
            : m_previousPageKey;
        if (destination == QStringLiteral("operator_settings")
            && !m_operatorSettings->currentOperatorId().isEmpty()) {
            showOperatorSettings(m_operatorSettings->currentOperatorId());
        } else {
            showPage(destination);
        }
    });
    connect(m_moreOptions, &MoreOptionsPage::regionSelectionRequested,
            this, &AuthenticatedRoot::regionSelectionRequested);
    connect(m_moreOptions, &MoreOptionsPage::regionClearRequested,
            this, &AuthenticatedRoot::regionClearRequested);
    connect(m_moreOptions, &MoreOptionsPage::regionSaveRequested,
            this, &AuthenticatedRoot::regionSaveRequested);
    connect(m_moreOptions, &MoreOptionsPage::runtimeHelperMonitoringEnabledChanged,
            this, &AuthenticatedRoot::runtimeHelperMonitoringEnabledChanged);
    connect(m_moreOptions, &MoreOptionsPage::settingChanged,
            this, [this](const QString& key, const QVariant& value) {
        if (key == QStringLiteral("runtime_helper/cursor_visible_only")) {
            Q_EMIT pauseWhenForegroundChanged(value.toBool());
        } else if (key == QStringLiteral("runtime_helper/target_window_active_only")) {
            Q_EMIT lowResourceMonitoringChanged(value.toBool());
        } else if (key == QStringLiteral("runtime_helper/show_selection_border")) {
            Q_EMIT showSelectionBorderChanged(value.toBool());
        }
        Q_EMIT settingChanged(key, value);
    });

    connect(m_sensitivityConverter,
            &SensitivityFovConverterPage::conversionRequested,
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
            QStringLiteral("NEXUS configuration (*.nexus *.json);;All files (*.*)")
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
    m_currentPageKey.clear();
    m_previousPageKey = QStringLiteral("dashboard");
    m_pathPage->resetLoading();
    setNavigationAvailable(false);
    for (auto* button : std::as_const(m_navButtons)) {
        button->setActive(false);
    }
    m_stack->setCurrentWidget(m_pathPage);
}

void AuthenticatedRoot::showPage(const QString& key) {
    if (!m_pages.contains(key)) {
        return;
    }

    if (key == QStringLiteral("more_options")) {
        if (!m_currentPageKey.isEmpty()
            && m_currentPageKey != QStringLiteral("more_options")) {
            m_previousPageKey = m_currentPageKey;
        }
    } else {
        m_previousPageKey = key;
    }

    m_currentPageKey = key;
    setNavigationAvailable(true);
    m_stack->setCurrentWidget(m_pages.value(key));

    for (auto iterator = m_navButtons.begin(); iterator != m_navButtons.end(); ++iterator) {
        iterator.value()->setActive(iterator.key() == key);
    }

    const bool moreActive = key == QStringLiteral("more_options");
    m_moreButton->setProperty("accentButton", moreActive);
    m_moreButton->style()->unpolish(m_moreButton);
    m_moreButton->style()->polish(m_moreButton);
}

void AuthenticatedRoot::showOperatorSettings(const QString& operatorId) {
    if (!m_operatorSettings->setOperator(operatorId)) {
        return;
    }

    if (!m_currentPageKey.isEmpty()
        && m_currentPageKey != QStringLiteral("more_options")
        && m_currentPageKey != QStringLiteral("operator_settings")) {
        m_previousPageKey = m_currentPageKey;
    }
    m_currentPageKey = QStringLiteral("operator_settings");
    setNavigationAvailable(true);
    m_stack->setCurrentWidget(m_operatorSettings);
    for (auto iterator = m_navButtons.begin(); iterator != m_navButtons.end(); ++iterator) {
        iterator.value()->setActive(iterator.key() == QStringLiteral("operators"));
    }
    m_moreButton->setProperty("accentButton", false);
    m_moreButton->style()->unpolish(m_moreButton);
    m_moreButton->style()->polish(m_moreButton);
}

void AuthenticatedRoot::setSensitivityScaleFactors(
    double horizontalScale,
    double verticalScale
) {
    if (m_sensitivityConverter != nullptr) {
        m_sensitivityConverter->setScaleFactors(horizontalScale, verticalScale);
    }
}

void AuthenticatedRoot::setSensitivityConversionError(const QString& message) {
    if (m_sensitivityConverter != nullptr) {
        m_sensitivityConverter->setCalculationError(message);
    }
}

void AuthenticatedRoot::showOperatorDetail(const QString& operatorName) {
    const auto* record = OperatorCatalog::findByDisplayName(operatorName);
    if (record != nullptr) {
        showOperatorSettings(record->id);
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
    return m_operatorSettings != nullptr
        ? m_operatorSettings->settingsFor(operatorId)
        : QVariantMap{};
}

void AuthenticatedRoot::setScreenRegion(const QRect& region, const QString& displayId) {
    setSelectedScreenRegion(region, displayId);
}

void AuthenticatedRoot::setSelectedScreenRegion(
    const QRect& region,
    const QString& displayId,
    const QPixmap& preview
) {
    m_moreOptions->setSelectedRegion(region, displayId, preview);
}

void AuthenticatedRoot::clearSelectedScreenRegion() {
    m_moreOptions->clearSelectedRegion();
}

void AuthenticatedRoot::clearScreenRegion() {
    clearSelectedScreenRegion();
}

void AuthenticatedRoot::setScreenRegionSaveResult(
    bool success,
    const QString& message
) {
    m_moreOptions->setRegionSaveResult(success, message);
}

QString AuthenticatedRoot::installationPath() const {
    return m_installationPath;
}

QVariantMap AuthenticatedRoot::allOperatorSettings() const {
    return m_operatorSettings != nullptr
        ? m_operatorSettings->allOperatorSettings()
        : QVariantMap{};
}

void AuthenticatedRoot::setSuggestedInstallationPath(const QString& path) {
    m_pathPage->setSelectedPath(path);
}

void AuthenticatedRoot::setLoadedInstallationPath(const QString& path) {
    m_installationPath = path;
    m_pathPage->setSelectedPath(path);
    if (!path.trimmed().isEmpty()) {
        showPage(QStringLiteral("dashboard"));
    }
}

void AuthenticatedRoot::beginLoadProgress() {
    m_pathPage->beginLoading();
    m_stack->setCurrentWidget(m_pathPage);
}

void AuthenticatedRoot::setLoadWaitingForGame() {
    m_pathPage->setLoadingWaiting();
}

void AuthenticatedRoot::setLoadGameDetected(qint64 pid, const QString& executableName) {
    m_pathPage->setLoadingGameDetected(pid, executableName);
}

void AuthenticatedRoot::setLoadClientReady() {
    m_pathPage->setLoadingClientReady();
}

void AuthenticatedRoot::finishLoadProgress() {
    m_pathPage->finishLoading();
    QTimer::singleShot(650, this, [this]() {
        showPage(QStringLiteral("dashboard"));
    });
}

void AuthenticatedRoot::failLoadProgress(const QString& message) {
    m_pathPage->failLoading(message);
}

void AuthenticatedRoot::setRuntimeHelperAppSettings(
    bool monitoringEnabled,
    bool showSelectionBorder,
    bool pauseWhenCursorHidden,
    bool lowResourceMode
) {
    QSettings settings(QStringLiteral("NEXUS"), QStringLiteral("NEXUS Client"));
    settings.setValue(QStringLiteral("runtime_helper/enabled"), monitoringEnabled);
    settings.setValue(QStringLiteral("runtime_helper/showBorder"), showSelectionBorder);
    settings.setValue(QStringLiteral("runtime_helper/idleWhenCursorHidden"), pauseWhenCursorHidden);
    settings.setValue(QStringLiteral("runtime_helper/lowResourceMode"), lowResourceMode);
}

void AuthenticatedRoot::setClientAppSettings(const QVariantMap& settings) {
    Q_UNUSED(settings);
}

void AuthenticatedRoot::setGeneralAppSettings(const QVariantMap& settings) {
    m_settings->setSettings(settings);
}

bool AuthenticatedRoot::runtimeHelperMonitoringEnabled() const {
    QSettings settings(QStringLiteral("NEXUS"), QStringLiteral("NEXUS Client"));
    return settings.value(QStringLiteral("runtime_helper/enabled"), true).toBool();
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
    beginLoadProgress();
    Q_EMIT loadProgressStarted();
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    QTimer::singleShot(50, this, [this, path]() {
        Q_EMIT installationPathSelected(path);
    });
}


QString AuthenticatedRoot::defaultGlobalConfigPath() const {
    const QString directory = QStandardPaths::writableLocation(
        QStandardPaths::AppConfigLocation
    );
    QDir().mkpath(directory);
    return QDir(directory).filePath(QStringLiteral("nexus-config.json"));
}

QString AuthenticatedRoot::safeGlobalConfigPath() const {
    const QString directory = QStandardPaths::writableLocation(
        QStandardPaths::AppConfigLocation
    );
    QDir().mkpath(directory);
    return QDir(directory).filePath(QStringLiteral("nexus-config.safe.json"));
}

QString AuthenticatedRoot::backupGlobalConfigDirectory() const {
    const QString directory = QDir(QStandardPaths::writableLocation(
        QStandardPaths::AppConfigLocation
    )).filePath(QStringLiteral("config-backups"));
    QDir().mkpath(directory);
    return directory;
}

QJsonObject AuthenticatedRoot::currentGlobalConfigObject() const {
    QJsonObject root;
    root.insert(QStringLiteral("format"), QStringLiteral("nexus-global-config"));
    root.insert(QStringLiteral("schema_version"), 2);
    root.insert(QStringLiteral("source_format"), QStringLiteral("NEXUS_CONFIG_V1"));
    root.insert(QStringLiteral("operator_count"), OperatorCatalog::all().size());
    root.insert(
        QStringLiteral("operators"),
        QJsonObject::fromVariantMap(m_operatorSettings->allOperatorSettings())
    );
    return root;
}

bool AuthenticatedRoot::copyExistingConfigToBackup(
    const QString& path,
    const QString& reason
) const {
    if (!QFileInfo::exists(path)) {
        return false;
    }

    const QString stamp = QDateTime::currentDateTime().toString(
        QStringLiteral("yyyyMMdd-HHmmss-zzz")
    );
    QString safeReason = reason;
    safeReason.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_-]")), QStringLiteral("-"));
    const QString backupPath = QDir(backupGlobalConfigDirectory()).filePath(
        QStringLiteral("nexus-config-%1-%2.json").arg(safeReason, stamp)
    );
    QFile::remove(backupPath);
    return QFile::copy(path, backupPath);
}

bool AuthenticatedRoot::writeConfigObject(
    const QString& path,
    const QJsonObject& root
) const {
    if (path.trimmed().isEmpty()) {
        return false;
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return file.commit();
}

bool AuthenticatedRoot::saveSafeConfigurationSnapshot() {
    if (m_operatorSettings == nullptr || m_activeConfigPath.trimmed().isEmpty()) {
        return false;
    }

    const QJsonObject root = currentGlobalConfigObject();
    const bool activeSaved = writeGlobalConfig(m_activeConfigPath, false);
    const bool safeSaved = writeConfigObject(safeGlobalConfigPath(), root);
    const QString stamp = QDateTime::currentDateTime().toString(
        QStringLiteral("yyyyMMdd-HHmmss-zzz")
    );
    const bool backupSaved = writeConfigObject(
        QDir(backupGlobalConfigDirectory()).filePath(
            QStringLiteral("nexus-config-exit-%1.json").arg(stamp)
        ),
        root
    );
    return activeSaved && safeSaved && backupSaved;
}

bool AuthenticatedRoot::writeGlobalConfig(
    const QString& path,
    bool showFeedback
) {
    if (path.trimmed().isEmpty()) {
        return false;
    }

    const QJsonObject root = currentGlobalConfigObject();
    const bool savingPrimaryConfig = QFileInfo(path).absoluteFilePath()
        == QFileInfo(m_activeConfigPath).absoluteFilePath();
    if (savingPrimaryConfig) {
        copyExistingConfigToBackup(path, QStringLiteral("before-save"));
    }

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

    if (savingPrimaryConfig) {
        writeConfigObject(safeGlobalConfigPath(), root);
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

bool AuthenticatedRoot::restoreFromSafeGlobalConfig() {
    QStringList candidates;
    candidates << safeGlobalConfigPath();

    QDir backupDirectory(backupGlobalConfigDirectory());
    const QFileInfoList backups = backupDirectory.entryInfoList(
        QStringList{QStringLiteral("*.json")},
        QDir::Files,
        QDir::Time
    );
    for (const QFileInfo& backup : backups) {
        candidates << backup.absoluteFilePath();
    }

    for (const QString& candidate : std::as_const(candidates)) {
        if (!QFileInfo::exists(candidate)) {
            continue;
        }
        if (readGlobalConfig(candidate, false)) {
            writeGlobalConfig(m_activeConfigPath, false);
            return true;
        }
    }
    return false;
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

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(
        file.readAll(),
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
    const int schemaVersion = root.value(QStringLiteral("schema_version")).toInt(1);
    const QString sourceFormat = root.value(QStringLiteral("source_format")).toString();
    if (root.value(QStringLiteral("format")).toString()
            != QStringLiteral("nexus-global-config")
        || !root.value(QStringLiteral("operators")).isObject()
        || schemaVersion < 1 || schemaVersion > 2
        || (schemaVersion == 2 && sourceFormat != QStringLiteral("NEXUS_CONFIG_V1"))) {
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

    if (schemaVersion == 2) {
        const int declaredCount = root.value(QStringLiteral("operator_count")).toInt(-1);
        bool hasEveryCatalogOperator = declaredCount == OperatorCatalog::all().size()
            && operators.size() == OperatorCatalog::all().size();
        for (const auto& record : OperatorCatalog::all()) {
            if (!operators.contains(record.id)) {
                hasEveryCatalogOperator = false;
                break;
            }
        }
        if (!hasEveryCatalogOperator) {
            if (showFeedback) {
                QMessageBox::warning(
                    this,
                    QStringLiteral("Incomplete NEXUS configuration"),
                    QStringLiteral(
                        "Schema version 2 requires exactly all 76 non-Recruit operator records."
                    )
                );
            }
            return false;
        }
    }

    m_operatorSettings->replaceAllOperatorSettings(operators);
    const QVariantMap normalizedOperators = m_operatorSettings->allOperatorSettings();
    Q_EMIT globalOperatorConfigurationImported(normalizedOperators);

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
