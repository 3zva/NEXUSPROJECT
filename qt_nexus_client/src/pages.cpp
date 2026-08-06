#include "pages.h"
#include "nexuswidgets.h"
#include "operatorcatalog.h"
#include "operatorloadoutcatalog.h"
#include "theme.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSlider>
#include <QSet>
#include <QStyle>
#include <QVariantMap>
#include <QVBoxLayout>
#include <functional>
#include <QtMath>

namespace {
QWidget* createScrollableBody(QWidget* page, QVBoxLayout*& bodyLayout) {
    auto* scroll = new QScrollArea(page);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setFrameShape(QFrame::NoFrame);

    auto* body = new QWidget(scroll);
    bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(
        NexusTheme::ContentPadding,
        NexusTheme::ContentPadding,
        NexusTheme::ContentPadding,
        NexusTheme::ContentPadding
    );
    bodyLayout->setSpacing(16);
    scroll->setWidget(body);
    return scroll;
}

QLabel* bodyText(const QString& text, QWidget* parent) {
    auto* label = new QLabel(text, parent);
    label->setProperty("muted", true);
    label->setFont(NexusTheme::font(9));
    label->setWordWrap(true);
    return label;
}

QWidget* createFileAction(
    const QString& iconName,
    const QString& title,
    const QString& description,
    const QString& buttonText,
    QWidget* parent,
    const std::function<void()>& callback
) {
    auto* card = new CardFrame(parent, true);
    auto* layout = new QHBoxLayout(card);
    layout->setContentsMargins(18, 18, 16, 18);
    layout->setSpacing(14);

    auto* icon = new QLabel(card);
    icon->setPixmap(NexusTheme::pixmap(iconName + QStringLiteral("_64.png"), 44, 44));

    auto* text = new QVBoxLayout();
    text->setSpacing(4);
    auto* titleLabel = new QLabel(title, card);
    titleLabel->setFont(NexusTheme::font(11, QFont::Bold));
    text->addWidget(titleLabel);
    text->addWidget(bodyText(description, card));

    auto* button = createAccentButton(buttonText, card);
    button->setFixedWidth(86);
    QObject::connect(button, &QPushButton::clicked, card, callback);

    layout->addWidget(icon);
    layout->addLayout(text, 1);
    layout->addWidget(button);
    return card;
}
}

DashboardPage::DashboardPage(QWidget* parent)
    : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    QVBoxLayout* bodyLayout = nullptr;
    root->addWidget(createScrollableBody(this, bodyLayout));

    auto* hero = new CardFrame(this, true);
    auto* heroLayout = new QHBoxLayout(hero);
    heroLayout->setContentsMargins(28, 24, 36, 24);
    heroLayout->setSpacing(20);

    auto* heroText = new QVBoxLayout();
    auto* welcome = new QLabel(QStringLiteral("WELCOME TO"), hero);
    welcome->setProperty("accent", true);
    welcome->setFont(NexusTheme::font(9, QFont::Bold));
    auto* nexus = new QLabel(QStringLiteral("NEXUS"), hero);
    nexus->setFont(NexusTheme::font(34, QFont::Bold));
    auto* subtitle = new QLabel(QStringLiteral("YOUR CONFIGURATION HUB"), hero);
    subtitle->setProperty("muted", true);
    subtitle->setFont(NexusTheme::font(10, QFont::Bold));

    auto* status = new CardFrame(hero, false);
    status->setMaximumWidth(330);
    auto* statusLayout = new QHBoxLayout(status);
    statusLayout->setContentsMargins(14, 10, 14, 10);
    auto* dot = new QLabel(QStringLiteral("●"), status);
    dot->setProperty("success", true);
    auto* statusText = new QVBoxLayout();
    auto* loaded = new QLabel(QStringLiteral("NEXUS LOADED"), status);
    loaded->setFont(NexusTheme::font(9, QFont::Bold));
    statusText->addWidget(loaded);
    statusText->addWidget(bodyText(QStringLiteral("All systems operational."), status));
    statusLayout->addWidget(dot);
    statusLayout->addLayout(statusText, 1);

    heroText->addWidget(welcome);
    heroText->addWidget(nexus);
    heroText->addWidget(subtitle);
    heroText->addSpacing(16);
    heroText->addWidget(status);
    heroText->addStretch();

    auto* logo = new QLabel(hero);
    logo->setPixmap(NexusTheme::pixmap(QStringLiteral("nexus_logo_256.png"), 190, 190));
    logo->setAlignment(Qt::AlignCenter);

    heroLayout->addLayout(heroText, 1);
    heroLayout->addWidget(logo);
    bodyLayout->addWidget(hero);

    auto* cards = new QWidget(this);
    auto* cardsGrid = new QGridLayout(cards);
    cardsGrid->setContentsMargins(0, 0, 0, 0);
    cardsGrid->setHorizontalSpacing(12);
    cardsGrid->setVerticalSpacing(12);

    struct ActionDefinition {
        QString icon;
        QString title;
        QString description;
        QString key;
    };
    const QList<ActionDefinition> actions{
        {QStringLiteral("operators"), QStringLiteral("OPERATORS"),
         QStringLiteral("Browse the operator library and manage configurations."), QStringLiteral("operators")},
        {QStringLiteral("save"), QStringLiteral("SAVE FILES"),
         QStringLiteral("Import, export, and protect your NEXUS configuration."), QStringLiteral("save_files")},
        {QStringLiteral("settings"), QStringLiteral("SETTINGS"),
         QStringLiteral("Configure keybinds, appearance, and application options."), QStringLiteral("settings")},
        {QStringLiteral("client"), QStringLiteral("CLIENT"),
         QStringLiteral("Control performance, startup, tray, and client behavior."), QStringLiteral("client")},
    };

    for (int index = 0; index < actions.size(); ++index) {
        const auto& action = actions[index];
        auto* card = new ActionCard(action.icon, action.title, action.description, cards);
        connect(card, &ActionCard::activated, this, [this, key = action.key]() {
            Q_EMIT navigateRequested(key);
        });
        cardsGrid->addWidget(card, index / 2, index % 2);
    }
    cardsGrid->setColumnStretch(0, 1);
    cardsGrid->setColumnStretch(1, 1);
    bodyLayout->addWidget(cards);
    bodyLayout->addStretch();
}

OperatorsPage::OperatorsPage(QWidget* parent)
    : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(
        NexusTheme::ContentPadding,
        NexusTheme::ContentPadding,
        NexusTheme::ContentPadding,
        NexusTheme::ContentPadding
    );
    root->setSpacing(14);

    root->addWidget(new PageHeading(
        QStringLiteral("OPERATORS"),
        QStringLiteral("Select any operator to open the shared NEXUS settings workspace."),
        this
    ));

    auto* controls = new QHBoxLayout();
    controls->setSpacing(8);
    m_attackersButton = createAccentButton(QStringLiteral("ATTACKERS"), this);
    m_defendersButton = new QPushButton(QStringLiteral("DEFENDERS"), this);
    m_attackersButton->setFixedWidth(112);
    m_defendersButton->setFixedWidth(112);
    controls->addWidget(m_attackersButton);
    controls->addWidget(m_defendersButton);
    controls->addStretch();

    m_search = new QLineEdit(this);
    m_search->setPlaceholderText(QStringLiteral("Search operator..."));
    m_search->setMaximumWidth(230);
    controls->addWidget(m_search);
    auto* filter = new QPushButton(this);
    filter->setIcon(NexusTheme::icon(QStringLiteral("filter_32.png")));
    filter->setFixedWidth(40);
    filter->setToolTip(QStringLiteral("Filter options are ready for future operator metadata."));
    controls->addWidget(filter);
    root->addLayout(controls);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setFrameShape(QFrame::NoFrame);
    m_gridContent = new QWidget(scroll);
    m_grid = new QGridLayout(m_gridContent);
    m_grid->setContentsMargins(0, 0, 4, 0);
    m_grid->setSpacing(10);
    scroll->setWidget(m_gridContent);
    root->addWidget(scroll, 1);

    auto* footer = new QHBoxLayout();
    m_countLabel = bodyText(QString(), this);
    auto* viewAll = createAccentButton(QStringLiteral("VIEW ALL"), this);
    viewAll->setFixedWidth(90);
    footer->addWidget(m_countLabel, 1);
    footer->addWidget(viewAll);
    root->addLayout(footer);

    connect(m_attackersButton, &QPushButton::clicked, this, [this]() { setSide(QStringLiteral("attackers")); });
    connect(m_defendersButton, &QPushButton::clicked, this, [this]() { setSide(QStringLiteral("defenders")); });
    connect(m_search, &QLineEdit::textChanged, this, [this]() { rebuildGrid(); });
    rebuildGrid();
}

void OperatorsPage::setSide(const QString& side) {
    m_side = side;
    m_attackersButton->setProperty("accentButton", side == QStringLiteral("attackers"));
    m_defendersButton->setProperty("accentButton", side == QStringLiteral("defenders"));
    for (auto* button : {m_attackersButton, m_defendersButton}) {
        button->style()->unpolish(button);
        button->style()->polish(button);
    }
    rebuildGrid();
}

void OperatorsPage::rebuildGrid() {
    while (auto* item = m_grid->takeAt(0)) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    const QString catalogSide = m_side == QStringLiteral("attackers")
        ? QStringLiteral("attacker")
        : QStringLiteral("defender");
    const auto source = OperatorCatalog::forSide(catalogSide);
    const auto search = m_search->text().trimmed();

    int shown = 0;
    constexpr int columns = 6;
    for (const auto& record : source) {
        if (!search.isEmpty()
            && !record.displayName.contains(search, Qt::CaseInsensitive)
            && !record.id.contains(search, Qt::CaseInsensitive)) {
            continue;
        }

        auto* tile = new OperatorTile(
            record.displayName,
            record.iconResource,
            m_gridContent
        );
        connect(tile, &QToolButton::clicked, this, [this, operatorId = record.id]() {
            Q_EMIT operatorSelected(operatorId);
        });
        m_grid->addWidget(tile, shown / columns, shown % columns);
        ++shown;
    }

    for (int column = 0; column < columns; ++column) {
        m_grid->setColumnStretch(column, 1);
    }
    m_grid->setRowStretch((shown + columns - 1) / columns, 1);
    m_countLabel->setText(QStringLiteral(
        "Showing %1 of %2 %3. Every tile opens the same reusable settings page with the matching icon."
    ).arg(shown).arg(source.size()).arg(m_side));
}

SaveFilesPage::SaveFilesPage(QWidget* parent)
    : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    QVBoxLayout* bodyLayout = nullptr;
    root->addWidget(createScrollableBody(this, bodyLayout));
    bodyLayout->addWidget(new PageHeading(
        QStringLiteral("CONFIG / SAVE FILES"),
        QStringLiteral("One file stores and restores every NEXUS operator configuration."),
        this
    ));

    auto* summary = new CardFrame(this, true);
    auto* summaryLayout = new QHBoxLayout(summary);
    summaryLayout->setContentsMargins(18, 16, 18, 16);
    summaryLayout->setSpacing(14);
    auto* summaryIcon = new QLabel(summary);
    summaryIcon->setPixmap(NexusTheme::pixmap(QStringLiteral("save_64.png"), 54, 54));
    auto* summaryText = new QVBoxLayout();
    auto* summaryTitle = new QLabel(QStringLiteral("ONE GLOBAL NEXUS CONFIGURATION"), summary);
    summaryTitle->setFont(NexusTheme::font(11, QFont::Bold));
    summaryTitle->setProperty("accent", true);
    summaryText->addWidget(summaryTitle);
    summaryText->addWidget(bodyText(
        QStringLiteral(
            "The file contains the Primary and Secondary values, toggles, notes, "
            "rapid-fire state, and related operator data for all 76 non-Recruit operators. "
            "Importing it replaces the complete operator dataset and immediately refreshes "
            "the reusable Operator Settings page."
        ),
        summary
    ));
    summaryLayout->addWidget(summaryIcon);
    summaryLayout->addLayout(summaryText, 1);
    bodyLayout->addWidget(summary);

    auto* columns = new QHBoxLayout();
    columns->setSpacing(12);

    auto* exportCard = new CardFrame(this, true);
    auto* exportLayout = new QVBoxLayout(exportCard);
    exportLayout->setContentsMargins(18, 18, 18, 18);
    exportLayout->setSpacing(10);
    auto* exportIcon = new QLabel(exportCard);
    exportIcon->setPixmap(NexusTheme::pixmap(QStringLiteral("download_64.png"), 64, 64));
    exportIcon->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    auto* exportTitle = new QLabel(QStringLiteral("EXPORT COMPLETE CONFIG"), exportCard);
    exportTitle->setFont(NexusTheme::font(13, QFont::Bold));
    exportLayout->addWidget(exportIcon);
    exportLayout->addWidget(exportTitle);
    exportLayout->addWidget(bodyText(
        QStringLiteral(
            "Create one .nexus file containing every operator record. There is no "
            "single-operator export anywhere in the application."
        ),
        exportCard
    ));
    exportLayout->addStretch();
    auto* exportButton = createAccentButton(QStringLiteral("EXPORT ALL OPERATOR DATA"), exportCard);
    exportButton->setIcon(NexusTheme::icon(QStringLiteral("download_32.png")));
    connect(exportButton, &QPushButton::clicked, this, &SaveFilesPage::exportRequested);
    exportLayout->addWidget(exportButton);

    auto* importCard = new CardFrame(this, true);
    auto* importLayout = new QVBoxLayout(importCard);
    importLayout->setContentsMargins(18, 18, 18, 18);
    importLayout->setSpacing(10);
    auto* importIcon = new QLabel(importCard);
    importIcon->setPixmap(NexusTheme::pixmap(QStringLiteral("upload_64.png"), 64, 64));
    importIcon->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    auto* importTitle = new QLabel(QStringLiteral("IMPORT COMPLETE CONFIG"), importCard);
    importTitle->setFont(NexusTheme::font(13, QFont::Bold));
    importLayout->addWidget(importIcon);
    importLayout->addWidget(importTitle);
    importLayout->addWidget(bodyText(
        QStringLiteral(
            "Load one compatible .nexus or JSON file. All 76 operator records are "
            "updated together, including operators that are not currently open."
        ),
        importCard
    ));
    importLayout->addStretch();
    auto* importButton = createAccentButton(QStringLiteral("IMPORT ALL OPERATOR DATA"), importCard);
    importButton->setIcon(NexusTheme::icon(QStringLiteral("upload_32.png")));
    connect(importButton, &QPushButton::clicked, this, &SaveFilesPage::importRequested);
    importLayout->addWidget(importButton);

    columns->addWidget(exportCard, 1);
    columns->addWidget(importCard, 1);
    bodyLayout->addLayout(columns);

    auto* rules = new CardFrame(this, false);
    auto* rulesLayout = new QVBoxLayout(rules);
    rulesLayout->setContentsMargins(16, 14, 16, 14);
    rulesLayout->setSpacing(6);
    rulesLayout->addWidget(createSectionLabel(QStringLiteral("CONFIGURATION RULES"), rules));
    rulesLayout->addWidget(bodyText(
        QStringLiteral(
            "• Operator Settings never displays Import or Export buttons.\n"
            "• Save Files is the only import/export location.\n"
            "• Export always writes the full catalog.\n"
            "• Import always refreshes every operator and the currently visible page.\n"
            "• Firebase authentication tokens are never included."
        ),
        rules
    ));
    bodyLayout->addWidget(rules);
    bodyLayout->addStretch();
}

ClientSettingsPage::ClientSettingsPage(QWidget* parent)
    : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    QVBoxLayout* bodyLayout = nullptr;
    root->addWidget(createScrollableBody(this, bodyLayout));
    bodyLayout->addWidget(new PageHeading(
        QStringLiteral("CLIENT SETTINGS"),
        QStringLiteral("Configure how the NEXUS client behaves."),
        this
    ));

    auto* columns = new QHBoxLayout();
    columns->setSpacing(12);
    auto* settings = new QWidget(this);
    auto* settingsLayout = new QVBoxLayout(settings);
    settingsLayout->setContentsMargins(0, 0, 0, 0);
    settingsLayout->setSpacing(10);

    struct ToggleDefinition { QString title; QString description; bool value; QString key; };
    const QList<ToggleDefinition> toggles{
        {QStringLiteral("Mute all client sounds?"), QStringLiteral("Disable interface sounds only."), false, QStringLiteral("mute_sounds")},
        {QStringLiteral("Show FPS of client?"), QStringLiteral("Display the client rendering rate."), true, QStringLiteral("show_fps")},
        {QStringLiteral("Enable performance mode?"), QStringLiteral("Reduce decorative animation and background work."), true, QStringLiteral("performance_mode")},
        {QStringLiteral("Outline crosshairs? (Glitchy)"), QStringLiteral("Experimental client-only outline rendering."), false, QStringLiteral("outline_crosshairs")},
        {QStringLiteral("Minimize to system tray?"), QStringLiteral("Keep NEXUS available without a taskbar window."), true, QStringLiteral("minimize_to_tray")},
        {QStringLiteral("Start NEXUS on system startup?"), QStringLiteral("Launch the client when Windows starts."), true, QStringLiteral("startup")},
        {QStringLiteral("Start game when Load is pressed?"), QStringLiteral("Temporary testing switch. Off skips launching and waiting for Siege."), false, QStringLiteral("start_game_on_load")},
    };
    QSettings storedSettings(QStringLiteral("NEXUS"), QStringLiteral("NEXUS Client"));
    for (const auto& toggle : toggles) {
        auto* row = new ToggleRow(
            toggle.title,
            toggle.description,
            storedSettings.value(QStringLiteral("settings/") + toggle.key, toggle.value).toBool(),
            settings
        );
        connect(row, &ToggleRow::toggled, this, [this, key = toggle.key](bool value) {
            Q_EMIT settingChanged(key, value);
        });
        settingsLayout->addWidget(row);
    }

    auto* rate = new CardFrame(settings, false);
    auto* rateLayout = new QHBoxLayout(rate);
    rateLayout->setContentsMargins(16, 10, 16, 10);
    auto* rateLabel = new QLabel(QStringLiteral("Maximum client refresh rate"), rate);
    rateLabel->setFont(NexusTheme::font(10, QFont::DemiBold));
    auto* rateBox = new QComboBox(rate);
    rateBox->addItems({QStringLiteral("30"), QStringLiteral("60"), QStringLiteral("120"), QStringLiteral("144"), QStringLiteral("240")});
    rateBox->setCurrentText(QStringLiteral("60"));
    rateBox->setFixedWidth(96);
    connect(rateBox, &QComboBox::currentTextChanged, this, [this](const QString& value) {
        Q_EMIT settingChanged(QStringLiteral("refresh_rate"), value.toInt());
    });
    rateLayout->addWidget(rateLabel, 1);
    rateLayout->addWidget(rateBox);
    settingsLayout->addWidget(rate);

    auto* speech = new CardFrame(settings, false);
    auto* speechLayout = new QVBoxLayout(speech);
    speechLayout->setContentsMargins(16, 12, 16, 12);
    speechLayout->setSpacing(10);

    auto* speechHeader = new QHBoxLayout();
    auto* speechText = new QVBoxLayout();
    speechText->setSpacing(2);
    auto* speechLabel = new QLabel(QStringLiteral("TTS audio feedback"), speech);
    speechLabel->setFont(NexusTheme::font(10, QFont::DemiBold));
    auto* speechDescription = new QLabel(QStringLiteral("Announce operator loadouts when they change."), speech);
    speechDescription->setProperty("muted", true);
    speechDescription->setWordWrap(true);
    speechText->addWidget(speechLabel);
    speechText->addWidget(speechDescription);
    auto* speechEnabled = new QCheckBox(speech);
    speechEnabled->setChecked(storedSettings.value(QStringLiteral("settings/tts_enabled"), true).toBool());
    speechEnabled->setCursor(Qt::PointingHandCursor);
    connect(speechEnabled, &QCheckBox::toggled, this, [this](bool enabled) {
        Q_EMIT settingChanged(QStringLiteral("tts_enabled"), enabled);
    });
    speechHeader->addLayout(speechText, 1);
    speechHeader->addWidget(speechEnabled, 0, Qt::AlignTop);
    speechLayout->addLayout(speechHeader);

    auto* volumeRow = new QHBoxLayout();
    auto* volumeLabel = new QLabel(QStringLiteral("Volume"), speech);
    volumeLabel->setProperty("muted", true);
    auto* volumeValue = new QLabel(QStringLiteral("80%"), speech);
    volumeValue->setProperty("accent", true);
    volumeValue->setMinimumWidth(44);
    volumeValue->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    auto* volumeSlider = new QSlider(Qt::Horizontal, speech);
    volumeSlider->setRange(0, 100);
    volumeSlider->setSingleStep(5);
    volumeSlider->setPageStep(10);
    volumeSlider->setValue(storedSettings.value(QStringLiteral("settings/tts_volume"), 80).toInt());
    volumeSlider->setCursor(Qt::PointingHandCursor);
    volumeSlider->setStyleSheet(QStringLiteral(R"QSS(
        QSlider::groove:horizontal {
            height: 8px;
            border-radius: 4px;
            background: #252D40;
        }
        QSlider::sub-page:horizontal {
            border-radius: 4px;
            background: #765BFF;
        }
        QSlider::add-page:horizontal {
            border-radius: 4px;
            background: #131A2A;
        }
        QSlider::handle:horizontal {
            width: 18px;
            height: 18px;
            margin: -6px 0;
            border-radius: 9px;
            background: #F7F9FF;
            border: 2px solid #765BFF;
        }
        QSlider::handle:horizontal:hover {
            background: #A898FF;
        }
    )QSS"));
    volumeValue->setText(QStringLiteral("%1%").arg(volumeSlider->value()));
    connect(volumeSlider, &QSlider::valueChanged, this, [this, volumeValue](int value) {
        volumeValue->setText(QStringLiteral("%1%").arg(value));
        Q_EMIT settingChanged(QStringLiteral("tts_volume"), value);
    });
    volumeRow->addWidget(volumeLabel);
    volumeRow->addWidget(volumeSlider, 1);
    volumeRow->addWidget(volumeValue);
    speechLayout->addLayout(volumeRow);
    settingsLayout->addWidget(speech);

    auto* close = new CardFrame(settings, false);
    auto* closeLayout = new QHBoxLayout(close);
    closeLayout->setContentsMargins(16, 10, 16, 10);
    auto* closeLabel = new QLabel(QStringLiteral("Close client"), close);
    closeLabel->setFont(NexusTheme::font(10, QFont::DemiBold));
    auto* exitButton = new QPushButton(QStringLiteral("EXIT CLIENT"), close);
    exitButton->setProperty("dangerButton", true);
    connect(exitButton, &QPushButton::clicked, this, &ClientSettingsPage::exitRequested);
    closeLayout->addWidget(closeLabel, 1);
    closeLayout->addWidget(exitButton);
    settingsLayout->addSpacing(8);
    settingsLayout->addWidget(close);
    settingsLayout->addStretch();

    auto* about = new CardFrame(this, true);
    auto* aboutLayout = new QVBoxLayout(about);
    aboutLayout->setContentsMargins(18, 18, 18, 18);
    auto* aboutTitle = new QLabel(QStringLiteral("ABOUT"), about);
    aboutTitle->setFont(NexusTheme::font(10, QFont::Bold));
    aboutLayout->addWidget(aboutTitle);
    aboutLayout->addWidget(bodyText(
        QStringLiteral("These settings affect the NEXUS client application only. They do not modify your game or operator configurations."),
        about
    ));
    auto* gear = new QLabel(about);
    gear->setPixmap(NexusTheme::pixmap(QStringLiteral("settings_64.png"), 86, 86));
    gear->setAlignment(Qt::AlignCenter);
    aboutLayout->addStretch();
    aboutLayout->addWidget(gear);
    aboutLayout->addStretch();

    columns->addWidget(settings, 3);
    columns->addWidget(about, 2);
    bodyLayout->addLayout(columns);
    bodyLayout->addStretch();
}

NativeDetectorPage::NativeDetectorPage(QWidget* parent)
    : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    QVBoxLayout* bodyLayout = nullptr;
    root->addWidget(createScrollableBody(this, bodyLayout));
    bodyLayout->addWidget(new PageHeading(
        QStringLiteral("TRIGGER BOT"),
        QStringLiteral("Tune automatic firing behavior while NEXUS runs the trigger engine in the background."),
        this
    ));

    QSettings storedSettings(QStringLiteral("NEXUS"), QStringLiteral("NEXUS Client"));
    auto* detector = new CardFrame(this, true);
    auto* detectorLayout = new QVBoxLayout(detector);
    detectorLayout->setContentsMargins(18, 16, 18, 16);
    detectorLayout->setSpacing(12);

    auto* detectorHeader = new QHBoxLayout();
    auto* detectorText = new QVBoxLayout();
    detectorText->setSpacing(2);
    auto* detectorLabel = new QLabel(QStringLiteral("Trigger bot master switch"), detector);
    detectorLabel->setFont(NexusTheme::font(11, QFont::Bold));
    detectorText->addWidget(detectorLabel);
    detectorText->addWidget(bodyText(
        QStringLiteral("Turn the trigger system on or off. When enabled, it runs silently inside NEXUS with no separate window."),
        detector
    ));
    auto* detectorEnabled = new QCheckBox(detector);
    detectorEnabled->setChecked(storedSettings.value(QStringLiteral("settings/native_detector/enabled"), false).toBool());
    detectorEnabled->setCursor(Qt::PointingHandCursor);
    connect(detectorEnabled, &QCheckBox::toggled, this, [this](bool enabled) {
        Q_EMIT settingChanged(QStringLiteral("native_detector/enabled"), enabled);
    });
    detectorHeader->addLayout(detectorText, 1);
    detectorHeader->addWidget(detectorEnabled, 0, Qt::AlignTop);
    detectorLayout->addLayout(detectorHeader);

    auto* statusGrid = new QGridLayout();
    statusGrid->setContentsMargins(0, 2, 0, 2);
    statusGrid->setHorizontalSpacing(10);
    statusGrid->setVerticalSpacing(8);
    auto createStatusTile = [detector, statusGrid](
        const QString& label,
        const QString& value,
        int column
    ) {
        auto* tile = new CardFrame(detector, false);
        auto* tileLayout = new QVBoxLayout(tile);
        tileLayout->setContentsMargins(14, 10, 14, 10);
        tileLayout->setSpacing(2);
        auto* valueLabel = new QLabel(value, tile);
        valueLabel->setFont(NexusTheme::font(17, QFont::Bold));
        valueLabel->setProperty("accent", true);
        auto* titleLabel = new QLabel(label, tile);
        titleLabel->setProperty("muted", true);
        titleLabel->setFont(NexusTheme::font(8, QFont::DemiBold));
        tileLayout->addWidget(valueLabel);
        tileLayout->addWidget(titleLabel);
        statusGrid->addWidget(tile, 0, column);
        return valueLabel;
    };
    m_fpsValue = createStatusTile(QStringLiteral("TRIGGER FPS"), QStringLiteral("--"), 0);
    m_inferenceValue = createStatusTile(QStringLiteral("FRAME TIME"), QStringLiteral("--"), 1);
    m_detectionValue = createStatusTile(QStringLiteral("TARGETS"), QStringLiteral("--"), 2);
    m_statusValue = createStatusTile(QStringLiteral("STATE"), QStringLiteral("OFF"), 3);
    for (int column = 0; column < 4; ++column) {
        statusGrid->setColumnStretch(column, 1);
    }
    detectorLayout->addLayout(statusGrid);

    auto createToggle = [this, detector, detectorLayout, &storedSettings](
        const QString& label,
        const QString& key,
        bool defaultValue
    ) {
        auto* row = new ToggleRow(
            label,
            QString(),
            storedSettings.value(QStringLiteral("settings/") + key, defaultValue).toBool(),
            detector
        );
        connect(row, &ToggleRow::toggled, this, [this, key](bool value) {
            Q_EMIT settingChanged(key, value);
        });
        detectorLayout->addWidget(row);
    };

    createToggle(QStringLiteral("Fire only when a target is inside the trigger zone"), QStringLiteral("native_detector/trigger_enabled"), true);
    createToggle(QStringLiteral("Allow automatic left-click firing"), QStringLiteral("native_detector/lmb_enabled"), true);
    createToggle(QStringLiteral("Use B key instead of right mouse to arm"), QStringLiteral("native_detector/b_hold_mode_enabled"), false);

    auto createSlider = [this, detector, detectorLayout, &storedSettings](
        const QString& label,
        const QString& key,
        int minimum,
        int maximum,
        int defaultValue,
        const QString& suffix
    ) {
        auto* row = new QHBoxLayout();
        auto* title = new QLabel(label, detector);
        title->setProperty("muted", true);
        auto* valueLabel = new QLabel(detector);
        valueLabel->setProperty("accent", true);
        valueLabel->setMinimumWidth(54);
        valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        auto* slider = new QSlider(Qt::Horizontal, detector);
        slider->setRange(minimum, maximum);
        slider->setValue(storedSettings.value(QStringLiteral("settings/") + key, defaultValue).toInt());
        slider->setCursor(Qt::PointingHandCursor);
        slider->setStyleSheet(QStringLiteral(R"QSS(
            QSlider::groove:horizontal { height: 8px; border-radius: 4px; background: #252D40; }
            QSlider::sub-page:horizontal { border-radius: 4px; background: #765BFF; }
            QSlider::add-page:horizontal { border-radius: 4px; background: #131A2A; }
            QSlider::handle:horizontal {
                width: 18px; height: 18px; margin: -6px 0;
                border-radius: 9px; background: #F7F9FF; border: 2px solid #765BFF;
            }
            QSlider::handle:horizontal:hover { background: #A898FF; }
        )QSS"));
        auto updateValue = [valueLabel, suffix](int value) {
            valueLabel->setText(QStringLiteral("%1%2").arg(value).arg(suffix));
        };
        updateValue(slider->value());
        connect(slider, &QSlider::valueChanged, this, [this, key, updateValue](int value) {
            updateValue(value);
            Q_EMIT settingChanged(key, value);
        });
        row->addWidget(title);
        row->addWidget(slider, 1);
        row->addWidget(valueLabel);
        detectorLayout->addLayout(row);
    };

    createSlider(QStringLiteral("Target confidence required"), QStringLiteral("native_detector/confidence_percent"), 5, 95, 30, QStringLiteral("%"));
    createSlider(QStringLiteral("Maximum trigger FPS"), QStringLiteral("native_detector/fps_cap"), 0, 240, 0, QString());
    createSlider(QStringLiteral("Keep firing after target loss"), QStringLiteral("native_detector/hold_delay_ms"), 0, 500, 185, QStringLiteral("ms"));
    createSlider(QStringLiteral("Delay before first shot"), QStringLiteral("native_detector/trigger_press_delay_ms"), 0, 800, 392, QStringLiteral("ms"));
    createSlider(QStringLiteral("Trigger zone width"), QStringLiteral("native_detector/activation_gate_width"), 20, 420, 120, QString());
    createSlider(QStringLiteral("Trigger zone height"), QStringLiteral("native_detector/activation_gate_height"), 20, 420, 120, QString());

    auto* targetRow = new QHBoxLayout();
    auto* targetLabel = new QLabel(QStringLiteral("Target hitbox"), detector);
    targetLabel->setFont(NexusTheme::font(10, QFont::DemiBold));
    auto* targetBox = new QComboBox(detector);
    targetBox->addItem(QStringLiteral("Any"), -1);
    targetBox->addItem(QStringLiteral("Body"), 0);
    targetBox->addItem(QStringLiteral("Head"), 1);
    const int targetClass = storedSettings.value(QStringLiteral("settings/native_detector/target_class"), 1).toInt();
    const int targetIndex = targetBox->findData(targetClass);
    targetBox->setCurrentIndex(targetIndex >= 0 ? targetIndex : 2);
    targetBox->setFixedWidth(110);
    connect(targetBox, &QComboBox::currentIndexChanged, this, [this, targetBox](int) {
        Q_EMIT settingChanged(QStringLiteral("native_detector/target_class"), targetBox->currentData().toInt());
    });
    targetRow->addWidget(targetLabel, 1);
    targetRow->addWidget(targetBox);
    detectorLayout->addLayout(targetRow);

    bodyLayout->addWidget(detector);
    bodyLayout->addStretch();
}

void NativeDetectorPage::setDetectorStatus(
    bool running,
    double fps,
    double inferenceMs,
    int detections
) {
    if (m_fpsValue == nullptr) {
        return;
    }
    m_fpsValue->setText(running ? QString::number(fps, 'f', 1) : QStringLiteral("--"));
    m_inferenceValue->setText(running ? QStringLiteral("%1 ms").arg(inferenceMs, 0, 'f', 1) : QStringLiteral("--"));
    m_detectionValue->setText(running ? QString::number(detections) : QStringLiteral("--"));
    m_statusValue->setText(running ? QStringLiteral("LIVE") : QStringLiteral("OFF"));
    m_statusValue->setProperty("success", running);
    m_statusValue->setProperty("accent", !running);
    m_statusValue->style()->unpolish(m_statusValue);
    m_statusValue->style()->polish(m_statusValue);
}

SettingsPage::SettingsPage(QWidget* parent)
    : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    QVBoxLayout* bodyLayout = nullptr;
    root->addWidget(createScrollableBody(this, bodyLayout));
    bodyLayout->addWidget(new PageHeading(
        QStringLiteral("SETTINGS"),
        QStringLiteral("Configure keybinds, appearance, and other options."),
        this
    ));

    auto* columns = new QHBoxLayout();
    columns->setSpacing(12);
    auto* keyCard = new CardFrame(this, true);
    auto* keyLayout = new QGridLayout(keyCard);
    keyLayout->setContentsMargins(18, 18, 18, 18);
    keyLayout->setHorizontalSpacing(14);
    keyLayout->setVerticalSpacing(10);
    auto* keyTitle = new QLabel(QStringLiteral("KEYBINDS"), keyCard);
    keyTitle->setFont(NexusTheme::font(10, QFont::Bold));
    keyLayout->addWidget(keyTitle, 0, 0, 1, 2);

    struct KeyDefinition { QString label; QString value; QString key; };
    const QList<KeyDefinition> keys{
        {QStringLiteral("Primary Weapon"), QStringLiteral("1"), QStringLiteral("primary_weapon")},
        {QStringLiteral("Secondary Weapon"), QStringLiteral("2"), QStringLiteral("secondary_weapon")},
        {QStringLiteral("Pause Input Control"), QStringLiteral("N"), QStringLiteral("pause_input")},
    };
    int row = 1;
    for (const auto& definition : keys) {
        auto* label = new QLabel(definition.label, keyCard);
        label->setProperty("muted", true);
        label->setFont(NexusTheme::font(9, QFont::DemiBold));
        auto* entry = new QLineEdit(definition.value, keyCard);
        entry->setMaxLength(8);
        connect(entry, &QLineEdit::editingFinished, this, [this, entry, key = definition.key]() {
            Q_EMIT keybindChanged(key, entry->text().trimmed());
        });
        keyLayout->addWidget(label, row, 0);
        keyLayout->addWidget(entry, row, 1);
        ++row;
    }
    keyLayout->setColumnStretch(1, 1);

    auto* note = new CardFrame(keyCard, false);
    auto* noteLayout = new QVBoxLayout(note);
    noteLayout->setContentsMargins(14, 12, 14, 12);
    noteLayout->addWidget(bodyText(
        QStringLiteral("Keybinds currently accept compact key names. Keep these values synchronized with your in-game controls."),
        note
    ));
    keyLayout->addWidget(note, row++, 0, 1, 2);

    auto* reset = new QPushButton(QStringLiteral("RESET ALL OPERATORS"), keyCard);
    connect(reset, &QPushButton::clicked, this, &SettingsPage::resetOperatorsRequested);
    keyLayout->addWidget(reset, row, 0, 1, 2, Qt::AlignLeft);

    auto* right = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(12);

    auto* appearance = new CardFrame(right, true);
    auto* appearanceLayout = new QGridLayout(appearance);
    appearanceLayout->setContentsMargins(16, 18, 16, 18);
    appearanceLayout->setVerticalSpacing(10);
    auto* appearanceTitle = new QLabel(QStringLiteral("APPEARANCE"), appearance);
    appearanceTitle->setFont(NexusTheme::font(10, QFont::Bold));
    appearanceLayout->addWidget(appearanceTitle, 0, 0, 1, 2);

    struct ComboDefinition { QString label; QStringList values; QString key; };
    const QList<ComboDefinition> combos{
        {QStringLiteral("Theme Color"), {QStringLiteral("NEXUS Purple")}, QStringLiteral("theme")},
        {QStringLiteral("Accent Color"), {QStringLiteral("Purple"), QStringLiteral("Violet"), QStringLiteral("Lavender")}, QStringLiteral("accent")},
        {QStringLiteral("UI Scale"), {QStringLiteral("90%"), QStringLiteral("100%"), QStringLiteral("110%"), QStringLiteral("125%")}, QStringLiteral("ui_scale")},
    };
    int comboRow = 1;
    for (const auto& definition : combos) {
        auto* label = new QLabel(definition.label, appearance);
        label->setProperty("muted", true);
        auto* box = new QComboBox(appearance);
        box->addItems(definition.values);
        connect(box, &QComboBox::currentTextChanged, this, [this, key = definition.key](const QString& value) {
            Q_EMIT settingChanged(key, value);
        });
        appearanceLayout->addWidget(label, comboRow, 0);
        appearanceLayout->addWidget(box, comboRow, 1);
        ++comboRow;
    }
    rightLayout->addWidget(appearance);

    auto* other = new CardFrame(right, true);
    auto* otherLayout = new QVBoxLayout(other);
    otherLayout->setContentsMargins(12, 16, 12, 16);
    otherLayout->setSpacing(10);
    auto* otherTitle = new QLabel(QStringLiteral("OTHER"), other);
    otherTitle->setFont(NexusTheme::font(10, QFont::Bold));
    otherLayout->addWidget(otherTitle);

    auto* languageRow = new QHBoxLayout();
    auto* languageLabel = new QLabel(QStringLiteral("Language"), other);
    languageLabel->setProperty("muted", true);
    auto* language = new QComboBox(other);
    language->addItem(QStringLiteral("English"));
    connect(language, &QComboBox::currentTextChanged, this, [this](const QString& value) {
        Q_EMIT settingChanged(QStringLiteral("language"), value);
    });
    languageRow->addWidget(languageLabel, 1);
    languageRow->addWidget(language);
    otherLayout->addLayout(languageRow);

    m_autoUpdates = new ToggleRow(QStringLiteral("Auto check for updates?"), QString(), true, other);
    auto* anonymous = new ToggleRow(QStringLiteral("Send anonymous data?"), QString(), false, other);
    connect(m_autoUpdates, &ToggleRow::toggled, this, [this](bool value) {
        Q_EMIT settingChanged(QStringLiteral("auto_updates"), value);
    });
    connect(anonymous, &ToggleRow::toggled, this, [this](bool value) {
        Q_EMIT settingChanged(QStringLiteral("anonymous_data"), value);
    });
    otherLayout->addWidget(m_autoUpdates);
    otherLayout->addWidget(anonymous);
    rightLayout->addWidget(other);
    rightLayout->addStretch();

    columns->addWidget(keyCard, 3);
    columns->addWidget(right, 2);
    bodyLayout->addLayout(columns);
    bodyLayout->addStretch();
}

void SettingsPage::setSettings(const QVariantMap& settings) {
    if (m_autoUpdates != nullptr) {
        m_autoUpdates->setChecked(settings.value(QStringLiteral("auto_updates"), true).toBool());
    }
}

OperatorSettingsPage::OperatorSettingsPage(QWidget* parent)
    : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);

    QVBoxLayout* bodyLayout = nullptr;
    root->addWidget(createScrollableBody(this, bodyLayout));

    auto* navigationRow = new QHBoxLayout();
    navigationRow->setSpacing(10);
    m_backButton = new QPushButton(QStringLiteral("←  BACK TO OPERATORS"), this);
    m_backButton->setCursor(Qt::PointingHandCursor);
    m_backButton->setMaximumWidth(190);
    navigationRow->addWidget(m_backButton);
    navigationRow->addStretch();
    bodyLayout->addLayout(navigationRow);

    auto* hero = new CardFrame(this, true);
    auto* heroLayout = new QVBoxLayout(hero);
    heroLayout->setContentsMargins(22, 20, 22, 20);
    heroLayout->setSpacing(14);

    auto* iconCard = new CardFrame(hero, false);
    iconCard->setFixedSize(132, 132);
    auto* iconLayout = new QVBoxLayout(iconCard);
    iconLayout->setContentsMargins(8, 8, 8, 8);
    m_iconLabel = new QLabel(iconCard);
    m_iconLabel->setAlignment(Qt::AlignCenter);
    iconLayout->addWidget(m_iconLabel);

    auto* identity = new QVBoxLayout();
    identity->setSpacing(5);
    auto* eyebrow = new QLabel(QStringLiteral("SELECTED OPERATOR"), hero);
    eyebrow->setProperty("accent", true);
    eyebrow->setFont(NexusTheme::font(9, QFont::Bold));
    m_nameLabel = new QLabel(QStringLiteral("SELECT AN OPERATOR"), hero);
    m_nameLabel->setFont(NexusTheme::font(28, QFont::Bold));
    m_nameLabel->setWordWrap(true);
    m_sideLabel = new QLabel(QStringLiteral("NO OPERATOR LOADED"), hero);
    m_sideLabel->setProperty("accent", true);
    m_sideLabel->setFont(NexusTheme::font(10, QFont::DemiBold));
    m_assetLabel = bodyText(
        QStringLiteral("The matching icon and values are loaded from the shared operator catalog."),
        hero
    );
    identity->addWidget(eyebrow);
    identity->addWidget(m_nameLabel);
    identity->addWidget(m_sideLabel);
    identity->addSpacing(4);
    identity->addWidget(m_assetLabel);
    identity->addStretch();

    auto* notesCard = new CardFrame(hero, false);
    notesCard->setMinimumWidth(0);
    notesCard->setMaximumWidth(QWIDGETSIZE_MAX);
    auto* notesLayout = new QVBoxLayout(notesCard);
    notesLayout->setContentsMargins(14, 12, 14, 12);
    notesLayout->setSpacing(7);
    notesLayout->addWidget(createSectionLabel(QStringLiteral("ADDITIONAL NOTES"), notesCard));
    m_notes = new QPlainTextEdit(notesCard);
    m_notes->setPlaceholderText(QStringLiteral("Click to add operator notes..."));
    m_notes->setMinimumHeight(88);
    notesLayout->addWidget(m_notes);

    auto* identityRow = new QHBoxLayout();
    identityRow->setContentsMargins(0, 0, 0, 0);
    identityRow->setSpacing(16);
    identityRow->addWidget(iconCard);
    identityRow->addLayout(identity, 1);
    heroLayout->addLayout(identityRow);
    heroLayout->addWidget(notesCard);
    bodyLayout->addWidget(hero);

    auto* weaponTabs = new QHBoxLayout();
    weaponTabs->setSpacing(8);
    m_primaryButton = createAccentButton(QStringLiteral("WEAPON 1"), this);
    m_secondaryButton = new QPushButton(QStringLiteral("WEAPON 2"), this);
    m_primaryButton->setMinimumWidth(118);
    m_secondaryButton->setMinimumWidth(118);
    weaponTabs->addWidget(m_primaryButton);
    weaponTabs->addWidget(m_secondaryButton);
    weaponTabs->addStretch();
    bodyLayout->addLayout(weaponTabs);

    auto* workspace = new QVBoxLayout();
    workspace->setContentsMargins(0, 0, 0, 0);
    workspace->setSpacing(12);

    auto* loadoutCard = new CardFrame(this, true);
    auto* loadoutLayout = new QGridLayout(loadoutCard);
    loadoutLayout->setContentsMargins(16, 16, 16, 16);
    loadoutLayout->setHorizontalSpacing(10);
    loadoutLayout->setVerticalSpacing(8);
    loadoutLayout->setColumnStretch(1, 1);
    auto* loadoutHeading = createSectionLabel(QStringLiteral("LOADOUT SELECTION"), loadoutCard);
    loadoutLayout->addWidget(loadoutHeading, 0, 0, 1, 2);

    auto addLoadoutRow = [loadoutCard, loadoutLayout](
        int row,
        const QString& labelText,
        QComboBox*& combo
    ) {
        auto* label = new QLabel(labelText, loadoutCard);
        label->setFont(NexusTheme::font(9, QFont::DemiBold));
        combo = new QComboBox(loadoutCard);
        combo->setMinimumHeight(36);
        combo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        loadoutLayout->addWidget(label, row, 0);
        loadoutLayout->addWidget(combo, row, 1);
    };

    addLoadoutRow(1, QStringLiteral("WEAPON"), m_weaponSelector);
    addLoadoutRow(2, QStringLiteral("OPTIC"), m_opticSelector);
    addLoadoutRow(3, QStringLiteral("BARREL"), m_barrelSelector);
    addLoadoutRow(4, QStringLiteral("GRIP"), m_gripSelector);
    addLoadoutRow(5, QStringLiteral("UNDERBARREL"), m_underbarrelSelector);
    m_adsBindingLabel = bodyText(
        QStringLiteral("Selected optic uses the shared 1x ADS converter value."),
        loadoutCard
    );
    m_adsBindingLabel->setWordWrap(true);
    loadoutLayout->addWidget(m_adsBindingLabel, 6, 0, 1, 2);
    populateAttachmentSelectors();

    auto* inputCard = new CardFrame(this, true);
    auto* inputLayout = new QVBoxLayout(inputCard);
    inputLayout->setContentsMargins(16, 16, 16, 16);
    inputLayout->setSpacing(10);
    inputLayout->addWidget(createSectionLabel(QStringLiteral("INPUT VARIABLES"), inputCard));

    m_xAmount = new NumericStepperRow(
        QStringLiteral("X AMOUNT"),
        -100.0,
        100.0,
        0.05,
        2,
        0.0,
        QString(),
        inputCard
    );
    m_yAmount = new NumericStepperRow(
        QStringLiteral("Y AMOUNT"),
        -100.0,
        100.0,
        0.05,
        2,
        0.0,
        QString(),
        inputCard
    );
    m_horizontalRamp = new NumericStepperRow(
        QStringLiteral("HORIZONTAL AFTER"),
        -100.0,
        100.0,
        0.05,
        2,
        0.0,
        QString(),
        inputCard
    );
    m_verticalRamp = new NumericStepperRow(
        QStringLiteral("VERTICAL AFTER"),
        -100.0,
        100.0,
        0.05,
        2,
        0.0,
        QString(),
        inputCard
    );
    m_rampStartSeconds = new NumericStepperRow(
        QStringLiteral("RAMP START"),
        0.0,
        10.0,
        0.1,
        2,
        0.75,
        QStringLiteral("s"),
        inputCard
    );
    m_timeDelay = new NumericStepperRow(
        QStringLiteral("TIME DELAY"),
        0.0,
        1.0,
        0.000001,
        6,
        0.0,
        QStringLiteral("s"),
        inputCard
    );
    inputLayout->addWidget(m_xAmount);
    inputLayout->addWidget(m_yAmount);
    inputLayout->addWidget(m_horizontalRamp);
    inputLayout->addWidget(m_verticalRamp);
    inputLayout->addWidget(m_rampStartSeconds);
    auto* rampHint = bodyText(
        QStringLiteral(
            "After RAMP START, the pattern switches to the AFTER values. "
            "Leave an AFTER value at 0 to keep that axis on the original amount."
        ),
        inputCard
    );
    rampHint->setWordWrap(true);
    inputLayout->addWidget(rampHint);
    inputLayout->addWidget(m_timeDelay);

    auto* editActions = new QHBoxLayout();
    editActions->setSpacing(8);
    m_copyButton = new QPushButton(QStringLiteral("COPY"), inputCard);
    m_pasteButton = new QPushButton(QStringLiteral("PASTE"), inputCard);
    m_deleteButton = new QPushButton(QStringLiteral("DELETE"), inputCard);
    m_deleteButton->setProperty("dangerButton", true);
    m_undoButton = new QPushButton(QStringLiteral("UNDO"), inputCard);
    for (auto* button : {m_copyButton, m_pasteButton, m_deleteButton, m_undoButton}) {
        button->setCursor(Qt::PointingHandCursor);
        editActions->addWidget(button);
    }
    inputLayout->addLayout(editActions);

    auto* optionsCard = new CardFrame(this, true);
    auto* optionsLayout = new QVBoxLayout(optionsCard);
    optionsLayout->setContentsMargins(16, 16, 16, 16);
    optionsLayout->setSpacing(9);
    optionsLayout->addWidget(createSectionLabel(QStringLiteral("OPERATOR OPTIONS"), optionsCard));
    m_profileEnabled = new ToggleRow(
        QStringLiteral("Enable operator settings"),
        QStringLiteral("Allow this operator record to be used by the existing backend."),
        true,
        optionsCard
    );
    m_autoLoad = new ToggleRow(
        QStringLiteral("Auto-load when detected"),
        QStringLiteral("Load this operator's values when its stable catalog ID is detected."),
        true,
        optionsCard
    );
    m_showRuntimeHelper = new ToggleRow(
        QStringLiteral("Show runtime helper while active"),
        QStringLiteral("Store the runtime helper preference in the single NEXUS configuration."),
        true,
        optionsCard
    );
    m_monitorWhileActive = new ToggleRow(
        QStringLiteral("Continue monitoring"),
        QStringLiteral("Keep the separate monitor active after the operator is identified."),
        true,
        optionsCard
    );
    optionsLayout->addWidget(m_profileEnabled);
    optionsLayout->addWidget(m_autoLoad);
    optionsLayout->addWidget(m_showRuntimeHelper);
    optionsLayout->addWidget(m_monitorWhileActive);
    auto* regionButton = createAccentButton(QStringLiteral("OPEN SCREEN REGION MONITOR"), optionsCard);
    regionButton->setIcon(NexusTheme::icon(QStringLiteral("target_32.png")));
    optionsLayout->addWidget(regionButton);

    auto* leftColumn = new QWidget(this);
    auto* leftLayout = new QVBoxLayout(leftColumn);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(12);
    leftLayout->addWidget(loadoutCard);
    leftLayout->addWidget(inputCard);
    leftLayout->addWidget(optionsCard);
    leftLayout->addStretch();

    auto* visualizationCard = new CardFrame(this, true);
    auto* visualizationLayout = new QVBoxLayout(visualizationCard);
    visualizationLayout->setContentsMargins(16, 16, 16, 16);
    visualizationLayout->setSpacing(8);
    visualizationLayout->addWidget(createSectionLabel(QStringLiteral("VISUALIZATION"), visualizationCard));
    m_vectorPreview = new OperatorVectorPreview(visualizationCard);
    visualizationLayout->addWidget(m_vectorPreview, 1);

    auto* statsCard = new CardFrame(this, true);
    auto* statsLayout = new QVBoxLayout(statsCard);
    statsLayout->setContentsMargins(16, 16, 16, 16);
    statsLayout->setSpacing(12);
    statsLayout->addWidget(createSectionLabel(QStringLiteral("LIVE VALUES"), statsCard));

    auto addStat = [statsCard, statsLayout](const QString& title, QLabel*& output) {
        auto* row = new QHBoxLayout();
        auto* label = new QLabel(title, statsCard);
        label->setProperty("muted", true);
        label->setFont(NexusTheme::font(9, QFont::DemiBold));
        output = new QLabel(QStringLiteral("0"), statsCard);
        output->setFont(NexusTheme::font(11, QFont::Bold));
        output->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        row->addWidget(label, 1);
        row->addWidget(output);
        statsLayout->addLayout(row);
    };
    addStat(QStringLiteral("SLOPE"), m_slopeLabel);
    addStat(QStringLiteral("VECTOR"), m_vectorLabel);
    addStat(QStringLiteral("ANGLE"), m_angleLabel);
    addStat(QStringLiteral("SPEED"), m_speedLabel);
    addStat(QStringLiteral("PULLING"), m_pullLabel);
    addStat(QStringLiteral("RAMP START"), m_rampStartLabel);

    statsLayout->addSpacing(8);
    auto* rapidFireHeading = createSectionLabel(QStringLiteral("RAPID FIRE TARGET"), statsCard);
    statsLayout->addWidget(rapidFireHeading);
    m_rapidFireButton = new QPushButton(QStringLiteral("RAPID FIRE: OFF"), statsCard);
    m_rapidFireButton->setCursor(Qt::PointingHandCursor);
    m_rapidFireButton->setMinimumHeight(44);
    m_rapidFireButton->setToolTip(
        QStringLiteral("Click to cycle: Weapon 1 → Weapon 2 → Both Weapons → Off")
    );
    statsLayout->addWidget(m_rapidFireButton);
    auto* rapidFireHint = bodyText(
        QStringLiteral("Click to cycle Weapon 1, Weapon 2, both weapons, then off."),
        statsCard
    );
    rapidFireHint->setWordWrap(true);
    statsLayout->addWidget(rapidFireHint);
    m_rapidFireStatusLabel = new QLabel(QStringLiteral("RAPID FIRE DISABLED"), statsCard);
    m_rapidFireStatusLabel->setAlignment(Qt::AlignCenter);
    m_rapidFireStatusLabel->setProperty("rapidFireEnabled", false);
    m_rapidFireStatusLabel->setFont(NexusTheme::font(12, QFont::Bold));
    m_rapidFireStatusLabel->setMinimumHeight(52);
    statsLayout->addWidget(m_rapidFireStatusLabel);
    statsLayout->addStretch();

    workspace->addWidget(leftColumn);
    workspace->addWidget(visualizationCard);
    workspace->addWidget(statsCard);
    bodyLayout->addLayout(workspace);

    auto* footerCard = new CardFrame(this, true);
    auto* footerLayout = new QHBoxLayout(footerCard);
    footerLayout->setContentsMargins(16, 14, 16, 14);
    footerLayout->setSpacing(10);
    m_statusLabel = bodyText(
        QStringLiteral("Select an operator from the Operators page to begin."),
        footerCard
    );
    m_resetButton = new QPushButton(QStringLiteral("RESET OPERATOR"), footerCard);
    m_resetButton->setProperty("dangerButton", true);
    m_saveButton = createAccentButton(QStringLiteral("SAVE CHANGES"), footerCard);
    footerLayout->addWidget(m_statusLabel, 1);
    footerLayout->addWidget(m_resetButton);
    footerLayout->addWidget(m_saveButton);
    bodyLayout->addWidget(footerCard);
    bodyLayout->addStretch();

    connect(m_backButton, &QPushButton::clicked, this, [this]() {
        persistCurrentDraft();
        Q_EMIT backRequested();
    });
    connect(m_primaryButton, &QPushButton::clicked, this, [this]() {
        setWeaponSlot(QStringLiteral("primary"));
    });
    connect(m_secondaryButton, &QPushButton::clicked, this, [this]() {
        setWeaponSlot(QStringLiteral("secondary"));
    });

    connect(m_weaponSelector, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        if (m_loading || m_operatorId.isEmpty()) {
            return;
        }
        const QString selectedWeapon = m_weaponSelector->currentData().toString();
        const double defaultDelay = OperatorLoadoutCatalog::delaySecondsForWeapon(selectedWeapon);
        setWeaponField(QStringLiteral("selected_weapon"), selectedWeapon);
        m_timeDelay->setDefaultValue(defaultDelay, true);
        setWeaponField(QStringLiteral("time_delay"), defaultDelay);
        updateStatus(
            defaultDelay > 0.0
                ? QStringLiteral("Weapon selected. TIME DELAY reset to RPM / 60000.")
                : QStringLiteral("Weapon selected. No RPM default is available for this weapon.")
        );
        emitCurrentLoadoutSelection();
    });
    connect(m_opticSelector, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        if (m_loading || m_operatorId.isEmpty()) {
            return;
        }
        setAttachmentField(QStringLiteral("optic"), m_opticSelector->currentData().toString());
        updateAdsBindingLabel();
        emitCurrentLoadoutSelection();
    });
    connect(m_barrelSelector, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        if (!m_loading && !m_operatorId.isEmpty()) {
            setAttachmentField(QStringLiteral("barrel"), m_barrelSelector->currentData().toString());
            emitCurrentLoadoutSelection();
        }
    });
    connect(m_gripSelector, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        if (!m_loading && !m_operatorId.isEmpty()) {
            setAttachmentField(QStringLiteral("grip"), m_gripSelector->currentData().toString());
            emitCurrentLoadoutSelection();
        }
    });
    connect(m_underbarrelSelector, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        if (!m_loading && !m_operatorId.isEmpty()) {
            setAttachmentField(QStringLiteral("underbarrel"), m_underbarrelSelector->currentData().toString());
            emitCurrentLoadoutSelection();
        }
    });

    connect(m_xAmount, &NumericStepperRow::valueChanged, this, [this](double value) {
        setWeaponField(QStringLiteral("x_amount"), value);
    });
    connect(m_yAmount, &NumericStepperRow::valueChanged, this, [this](double value) {
        setWeaponField(QStringLiteral("y_amount"), value);
    });
    connect(m_horizontalRamp, &NumericStepperRow::valueChanged, this, [this](double value) {
        setWeaponField(QStringLiteral("horizontal_ramp"), value);
        emitCurrentLoadoutSelection();
    });
    connect(m_verticalRamp, &NumericStepperRow::valueChanged, this, [this](double value) {
        setWeaponField(QStringLiteral("vertical_ramp"), value);
        emitCurrentLoadoutSelection();
    });
    connect(m_rampStartSeconds, &NumericStepperRow::valueChanged, this, [this](double value) {
        setWeaponField(QStringLiteral("ramp_start_seconds"), value);
        emitCurrentLoadoutSelection();
    });
    connect(m_timeDelay, &NumericStepperRow::valueChanged, this, [this](double value) {
        setWeaponField(QStringLiteral("time_delay"), value);
        emitCurrentLoadoutSelection();
    });

    connect(m_profileEnabled, &ToggleRow::toggled, this, [this](bool value) {
        setOperatorField(QStringLiteral("profile_enabled"), value);
    });
    connect(m_autoLoad, &ToggleRow::toggled, this, [this](bool value) {
        setOperatorField(QStringLiteral("auto_load"), value);
    });
    connect(m_showRuntimeHelper, &ToggleRow::toggled, this, [this](bool value) {
        setOperatorField(QStringLiteral("show_runtime_helper"), value);
    });
    connect(m_monitorWhileActive, &ToggleRow::toggled, this, [this](bool value) {
        setOperatorField(QStringLiteral("monitor_while_active"), value);
    });
    connect(m_rapidFireButton, &QPushButton::clicked, this, [this]() {
        cycleRapidFireValue();
    });
    connect(m_notes, &QPlainTextEdit::textChanged, this, [this]() {
        setOperatorField(QStringLiteral("notes"), m_notes->toPlainText());
    });
    connect(regionButton, &QPushButton::clicked, this, &OperatorSettingsPage::screenRegionPageRequested);

    connect(m_copyButton, &QPushButton::clicked, this, [this]() {
        if (m_operatorId.isEmpty()) {
            return;
        }
        m_internalClipboard = activeWeaponSettings();
        updateStatus(QStringLiteral("Copied the current weapon values inside NEXUS."), true);
    });
    connect(m_pasteButton, &QPushButton::clicked, this, [this]() {
        if (m_operatorId.isEmpty() || m_internalClipboard.isEmpty()) {
            updateStatus(QStringLiteral("Nothing has been copied yet."));
            return;
        }
        m_undoSnapshot = m_drafts.value(m_operatorId, defaultSettingsFor(m_operatorId));
        auto settings = m_drafts.value(m_operatorId, defaultSettingsFor(m_operatorId));
        settings.insert(m_weaponSlot, m_internalClipboard);
        m_drafts.insert(m_operatorId, settings);
        applyActiveWeaponSettings();
        updateStatus(QStringLiteral("Pasted values into the current weapon tab."), true);
    });
    connect(m_deleteButton, &QPushButton::clicked, this, [this]() {
        if (m_operatorId.isEmpty()) {
            return;
        }
        m_undoSnapshot = m_drafts.value(m_operatorId, defaultSettingsFor(m_operatorId));
        auto settings = m_drafts.value(m_operatorId, defaultSettingsFor(m_operatorId));
        settings.insert(m_weaponSlot, defaultWeaponSettingsForSlot(m_weaponSlot));
        m_drafts.insert(m_operatorId, settings);
        applyActiveWeaponSettings();
        updateStatus(QStringLiteral("Cleared the current weapon values."), true);
    });
    connect(m_undoButton, &QPushButton::clicked, this, [this]() {
        if (m_operatorId.isEmpty() || m_undoSnapshot.isEmpty()) {
            updateStatus(QStringLiteral("There is no operator change to undo."));
            return;
        }
        const auto current = currentSettings();
        restoreSnapshot(m_undoSnapshot);
        m_undoSnapshot = current;
        updateStatus(QStringLiteral("Restored the previous operator values."), true);
    });
    connect(m_resetButton, &QPushButton::clicked, this, [this]() {
        if (m_operatorId.isEmpty()) {
            return;
        }
        m_undoSnapshot = m_drafts.value(m_operatorId, defaultSettingsFor(m_operatorId));
        const auto defaults = defaultSettingsFor(m_operatorId);
        m_drafts.insert(m_operatorId, defaults);
        applySettings(defaults);
        updateStatus(QStringLiteral("Reset this operator inside the shared NEXUS configuration."), true);
        Q_EMIT resetRequested(m_operatorId);
    });
    connect(m_saveButton, &QPushButton::clicked, this, [this]() {
        if (m_operatorId.isEmpty()) {
            return;
        }
        persistCurrentDraft();
        updateStatus(
            QStringLiteral("Saved this operator into the one global NEXUS configuration."),
            true
        );
        Q_EMIT saveRequested(m_operatorId, m_drafts.value(m_operatorId));
    });
}

bool OperatorSettingsPage::setOperator(const QString& operatorId) {
    const auto* record = OperatorCatalog::findById(operatorId);
    if (record == nullptr) {
        updateStatus(QStringLiteral("The requested operator ID was not found in OperatorCatalog."));
        return false;
    }

    persistCurrentDraft();
    m_operatorId = record->id;
    m_weaponSlot = QStringLiteral("primary");
    m_undoSnapshot.clear();

    if (!m_drafts.contains(m_operatorId)) {
        m_drafts.insert(m_operatorId, defaultSettingsFor(m_operatorId));
    }

    updateHeader();
    applySettings(m_drafts.value(m_operatorId));
    updateStatus(
        QStringLiteral("Loaded %1 from the shared operator configuration.")
            .arg(record->displayName),
        true
    );
    emitCurrentLoadoutSelection();
    return true;
}

QString OperatorSettingsPage::currentOperatorId() const {
    return m_operatorId;
}

QVariantMap OperatorSettingsPage::currentSettings() const {
    if (m_operatorId.isEmpty()) {
        return {};
    }

    QVariantMap settings = m_drafts.value(m_operatorId, defaultSettingsFor(m_operatorId));
    settings.insert(QStringLiteral("operator_id"), m_operatorId);
    settings.insert(QStringLiteral("active_weapon"), m_weaponSlot);
    settings.insert(QStringLiteral("profile_enabled"), m_profileEnabled->isChecked());
    settings.insert(QStringLiteral("auto_load"), m_autoLoad->isChecked());
    settings.insert(QStringLiteral("show_runtime_helper"), m_showRuntimeHelper->isChecked());
    settings.insert(QStringLiteral("monitor_while_active"), m_monitorWhileActive->isChecked());
    settings.insert(
        QStringLiteral("rapid_fire_enabled"),
        m_rapidFireValue != 0
    );
    settings.insert(QStringLiteral("rapid_fire_value"), m_rapidFireValue);
    // The exported schema intentionally does not contain rapid_fire_target.
    settings.remove(QStringLiteral("rapid_fire_target"));
    settings.insert(QStringLiteral("notes"), m_notes->toPlainText());
    settings.insert(m_weaponSlot, activeWeaponSettings());
    return settings;
}

QVariantMap OperatorSettingsPage::settingsFor(const QString& operatorId) const {
    const auto normalized = operatorId.trimmed().toLower();
    if (normalized == m_operatorId) {
        return currentSettings();
    }
    if (m_drafts.contains(normalized)) {
        return m_drafts.value(normalized);
    }
    return defaultSettingsFor(normalized);
}

QVariantMap OperatorSettingsPage::allOperatorSettings() const {
    QVariantMap all;
    for (const auto& record : OperatorCatalog::all()) {
        all.insert(record.id, settingsFor(record.id));
    }
    return all;
}

void OperatorSettingsPage::setSettingsFor(
    const QString& operatorId,
    const QVariantMap& settings
) {
    const auto* record = OperatorCatalog::findById(operatorId);
    if (record == nullptr) {
        return;
    }

    QVariantMap merged = defaultSettingsFor(record->id);
    for (auto iterator = settings.constBegin(); iterator != settings.constEnd(); ++iterator) {
        merged.insert(iterator.key(), iterator.value());
    }
    merged.insert(QStringLiteral("operator_id"), record->id);

    const int rapidFireValue = normalizedRapidFireValue(settings);
    merged.remove(QStringLiteral("rapid_fire_target"));
    merged.insert(QStringLiteral("rapid_fire_value"), rapidFireValue);
    merged.insert(
        QStringLiteral("rapid_fire_enabled"),
        rapidFireValue != 0
    );

    for (const QString& slot : {QStringLiteral("primary"), QStringLiteral("secondary")}) {
        QVariantMap weapon = defaultWeaponSettingsForSlot(record->id, slot);
        const auto imported = merged.value(slot).toMap();
        for (auto iterator = imported.constBegin(); iterator != imported.constEnd(); ++iterator) {
            weapon.insert(iterator.key(), iterator.value());
        }
        weapon.insert(
            QStringLiteral("horizontal_ramp"),
            imported.value(
                QStringLiteral("horizontal_ramp"),
                weapon.value(QStringLiteral("horizontal_ramp"), 0.0)
            ).toDouble()
        );
        weapon.insert(
            QStringLiteral("vertical_ramp"),
            imported.value(
                QStringLiteral("vertical_ramp"),
                weapon.value(QStringLiteral("vertical_ramp"), 0.0)
            ).toDouble()
        );
        weapon.insert(
            QStringLiteral("ramp_start_seconds"),
            imported.value(
                QStringLiteral("ramp_start_seconds"),
                weapon.value(QStringLiteral("ramp_start_seconds"), 0.75)
            ).toDouble()
        );
        QVariantMap attachments = OperatorLoadoutCatalog::defaultAttachments();
        const QVariantMap importedAttachments = imported.value(QStringLiteral("attachments")).toMap();
        for (auto iterator = importedAttachments.constBegin(); iterator != importedAttachments.constEnd(); ++iterator) {
            attachments.insert(iterator.key(), iterator.value());
        }
        weapon.insert(QStringLiteral("attachments"), attachments);
        if (weapon.value(QStringLiteral("selected_weapon")).toString().isEmpty()) {
            weapon.insert(
                QStringLiteral("selected_weapon"),
                OperatorLoadoutCatalog::defaultWeapon(record->id, slot)
            );
        }
        merged.insert(slot, weapon);
    }

    m_drafts.insert(record->id, merged);
    if (m_operatorId == record->id) {
        applySettings(merged);
        updateStatus(QStringLiteral("Updated this page from the imported global configuration."), true);
    }
}

void OperatorSettingsPage::replaceAllOperatorSettings(const QVariantMap& allSettings) {
    for (const auto& record : OperatorCatalog::all()) {
        const QVariantMap imported = allSettings.value(record.id).toMap();
        setSettingsFor(record.id, imported);
    }
    if (!m_operatorId.isEmpty()) {
        applySettings(m_drafts.value(m_operatorId, defaultSettingsFor(m_operatorId)));
    }
    updateStatus(
        QStringLiteral("Imported the global file and refreshed all 76 operator records."),
        true
    );
}

void OperatorSettingsPage::resetAllOperatorSettings() {
    m_drafts.clear();
    for (const auto& record : OperatorCatalog::all()) {
        m_drafts.insert(record.id, defaultSettingsFor(record.id));
    }
    if (!m_operatorId.isEmpty()) {
        applySettings(m_drafts.value(m_operatorId));
    }
    updateStatus(QStringLiteral("Reset all operator data to NEXUS defaults."), true);
}

QVariantMap OperatorSettingsPage::defaultWeaponSettingsForSlot(const QString& slot) const {
    return defaultWeaponSettingsForSlot(m_operatorId, slot);
}

QVariantMap OperatorSettingsPage::defaultWeaponSettingsForSlot(
    const QString& operatorId,
    const QString& slot
) const {
    return {
        {QStringLiteral("selected_weapon"), OperatorLoadoutCatalog::defaultWeapon(operatorId, slot)},
        {QStringLiteral("attachments"), OperatorLoadoutCatalog::defaultAttachments()},
        {QStringLiteral("x_amount"), 0},
        {QStringLiteral("y_amount"), -1},
        {QStringLiteral("horizontal_ramp"), 0.0},
        {QStringLiteral("vertical_ramp"), 0.0},
        {QStringLiteral("ramp_start_seconds"), 0.75},
        {QStringLiteral("time_delay"), OperatorLoadoutCatalog::defaultDelaySeconds(operatorId, slot)},
    };
}

QVariantMap OperatorSettingsPage::defaultSettingsFor(const QString& operatorId) const {
    const QString normalized = operatorId.trimmed().toLower();
    return {
        {QStringLiteral("operator_id"), normalized},
        {QStringLiteral("active_weapon"), QStringLiteral("primary")},
        {QStringLiteral("profile_enabled"), true},
        {QStringLiteral("auto_load"), true},
        {QStringLiteral("show_runtime_helper"), true},
        {QStringLiteral("monitor_while_active"), true},
        {QStringLiteral("rapid_fire_enabled"), false},
        {QStringLiteral("rapid_fire_value"), 0},
        {QStringLiteral("notes"), QStringLiteral("(Click to Edit)")},
        {QStringLiteral("primary"), defaultWeaponSettingsForSlot(normalized, QStringLiteral("primary"))},
        {QStringLiteral("secondary"), defaultWeaponSettingsForSlot(normalized, QStringLiteral("secondary"))},
    };
}

QVariantMap OperatorSettingsPage::activeWeaponSettings() const {
    return {
        {QStringLiteral("selected_weapon"), m_weaponSelector->currentData().toString()},
        {QStringLiteral("attachments"), activeAttachmentSettings()},
        {QStringLiteral("x_amount"), m_xAmount->value()},
        {QStringLiteral("y_amount"), m_yAmount->value()},
        {QStringLiteral("horizontal_ramp"), m_horizontalRamp->value()},
        {QStringLiteral("vertical_ramp"), m_verticalRamp->value()},
        {QStringLiteral("ramp_start_seconds"), m_rampStartSeconds->value()},
        {QStringLiteral("time_delay"), m_timeDelay->value()},
    };
}

void OperatorSettingsPage::applySettings(const QVariantMap& settings) {
    m_loading = true;
    m_weaponSlot = settings.value(
        QStringLiteral("active_weapon"),
        QStringLiteral("primary")
    ).toString().toLower();
    if (m_weaponSlot != QStringLiteral("secondary")) {
        m_weaponSlot = QStringLiteral("primary");
    }

    m_profileEnabled->setChecked(settings.value(QStringLiteral("profile_enabled"), true).toBool());
    m_autoLoad->setChecked(settings.value(QStringLiteral("auto_load"), true).toBool());
    m_showRuntimeHelper->setChecked(settings.value(QStringLiteral("show_runtime_helper"), true).toBool());
    m_monitorWhileActive->setChecked(settings.value(QStringLiteral("monitor_while_active"), true).toBool());
    setRapidFireValue(normalizedRapidFireValue(settings), false);
    m_notes->setPlainText(settings.value(QStringLiteral("notes")).toString());

    // Keep loading enabled while the visible weapon tab is hydrated so the
    // previous operator/weapon values are never written into the new record.
    setWeaponSlot(m_weaponSlot);
    m_loading = false;
    updateRapidFireStatus();
}

void OperatorSettingsPage::applyActiveWeaponSettings() {
    if (m_operatorId.isEmpty()) {
        return;
    }
    const auto settings = m_drafts.value(m_operatorId, defaultSettingsFor(m_operatorId));
    const auto weapon = settings.value(m_weaponSlot, defaultWeaponSettingsForSlot(m_weaponSlot)).toMap();
    const bool previousLoadingState = m_loading;
    m_loading = true;
    const auto defaults = defaultWeaponSettingsForSlot(m_weaponSlot);
    m_xAmount->setDefaultValue(defaults.value(QStringLiteral("x_amount")).toDouble());
    m_yAmount->setDefaultValue(defaults.value(QStringLiteral("y_amount")).toDouble());
    m_xAmount->setValue(
        weapon.value(QStringLiteral("x_amount"), defaults.value(QStringLiteral("x_amount"))).toDouble()
    );
    m_yAmount->setValue(
        weapon.value(QStringLiteral("y_amount"), defaults.value(QStringLiteral("y_amount"))).toDouble()
    );
    m_horizontalRamp->setValue(
        weapon.value(
            QStringLiteral("horizontal_ramp"),
            defaults.value(QStringLiteral("horizontal_ramp"))
        ).toDouble()
    );
    m_verticalRamp->setValue(
        weapon.value(
            QStringLiteral("vertical_ramp"),
            defaults.value(QStringLiteral("vertical_ramp"))
        ).toDouble()
    );
    m_rampStartSeconds->setValue(
        weapon.value(
            QStringLiteral("ramp_start_seconds"),
            defaults.value(QStringLiteral("ramp_start_seconds"))
        ).toDouble()
    );
    applyLoadoutSelectors(weapon);
    const QString selectedWeapon = m_weaponSelector->currentData().toString();
    const double selectedWeaponDefaultDelay =
        OperatorLoadoutCatalog::delaySecondsForWeapon(selectedWeapon);
    m_timeDelay->setDefaultValue(selectedWeaponDefaultDelay);
    m_timeDelay->setValue(
        weapon.value(QStringLiteral("time_delay"), selectedWeaponDefaultDelay).toDouble()
    );
    m_loading = previousLoadingState;
    updateVisualization();
    updateAdsBindingLabel();
}

void OperatorSettingsPage::persistCurrentDraft() {
    if (m_operatorId.isEmpty() || m_loading) {
        return;
    }
    m_drafts.insert(m_operatorId, currentSettings());
}

void OperatorSettingsPage::setOperatorField(const QString& key, const QVariant& value) {
    if (m_loading || m_operatorId.isEmpty()) {
        return;
    }
    m_undoSnapshot = m_drafts.value(m_operatorId, defaultSettingsFor(m_operatorId));
    auto settings = m_drafts.value(m_operatorId, defaultSettingsFor(m_operatorId));
    settings.insert(key, value);
    settings.insert(QStringLiteral("operator_id"), m_operatorId);
    m_drafts.insert(m_operatorId, settings);
    updateStatus(QStringLiteral("Unsaved changes are held in the shared NEXUS configuration."));
    Q_EMIT settingChanged(m_operatorId, key, value);
}

void OperatorSettingsPage::setWeaponField(const QString& key, const QVariant& value) {
    if (m_loading || m_operatorId.isEmpty()) {
        return;
    }
    m_undoSnapshot = m_drafts.value(m_operatorId, defaultSettingsFor(m_operatorId));
    auto settings = m_drafts.value(m_operatorId, defaultSettingsFor(m_operatorId));
    auto weapon = settings.value(m_weaponSlot, defaultWeaponSettingsForSlot(m_weaponSlot)).toMap();
    const QVariant normalizedValue = key == QStringLiteral("time_delay")
        ? QVariant(value.toDouble())
        : value;
    weapon.insert(key, normalizedValue);
    settings.insert(m_weaponSlot, weapon);
    settings.insert(QStringLiteral("active_weapon"), m_weaponSlot);
    m_drafts.insert(m_operatorId, settings);
    updateVisualization();
    updateStatus(QStringLiteral("Unsaved weapon and attachment values are held in the shared NEXUS configuration."));
    Q_EMIT settingChanged(
        m_operatorId,
        m_weaponSlot + QStringLiteral(".") + key,
        normalizedValue
    );
}

void OperatorSettingsPage::setAttachmentField(const QString& key, const QVariant& value) {
    if (m_loading || m_operatorId.isEmpty()) {
        return;
    }
    m_undoSnapshot = m_drafts.value(m_operatorId, defaultSettingsFor(m_operatorId));
    auto settings = m_drafts.value(m_operatorId, defaultSettingsFor(m_operatorId));
    auto weapon = settings.value(m_weaponSlot, defaultWeaponSettingsForSlot(m_weaponSlot)).toMap();
    auto attachments = weapon.value(
        QStringLiteral("attachments"),
        OperatorLoadoutCatalog::defaultAttachments()
    ).toMap();
    attachments.insert(key, value);
    if (key == QStringLiteral("optic")) {
        const auto* optic = OperatorLoadoutCatalog::findOptic(value.toString());
        if (optic != nullptr) {
            attachments.insert(QStringLiteral("ads_profile_key"), optic->adsProfileKey);
            attachments.insert(QStringLiteral("optic_magnification"), optic->magnification);
        }
    }
    weapon.insert(QStringLiteral("attachments"), attachments);
    settings.insert(m_weaponSlot, weapon);
    settings.insert(QStringLiteral("active_weapon"), m_weaponSlot);
    m_drafts.insert(m_operatorId, settings);
    updateStatus(QStringLiteral("Attachment selection saved in the shared operator draft."));
    Q_EMIT settingChanged(
        m_operatorId,
        m_weaponSlot + QStringLiteral(".attachments.") + key,
        value
    );
}

void OperatorSettingsPage::populateWeaponSelector() {
    const QSignalBlocker blocker(m_weaponSelector);
    const QString previous = m_weaponSelector->currentData().toString();
    m_weaponSelector->clear();
    const QStringList weapons = OperatorLoadoutCatalog::weaponsFor(m_operatorId, m_weaponSlot);
    if (weapons.isEmpty()) {
        m_weaponSelector->addItem(QStringLiteral("No weapon data configured"), QString());
        m_weaponSelector->setEnabled(false);
        return;
    }
    m_weaponSelector->setEnabled(true);
    for (const QString& weapon : weapons) {
        m_weaponSelector->addItem(weapon, weapon);
    }
    const int previousIndex = m_weaponSelector->findData(previous);
    if (previousIndex >= 0) {
        m_weaponSelector->setCurrentIndex(previousIndex);
    }
}

void OperatorSettingsPage::populateAttachmentSelectors() {
    auto populate = [](QComboBox* combo, const QList<AttachmentOption>& options) {
        const QSignalBlocker blocker(combo);
        combo->clear();
        for (const auto& option : options) {
            combo->addItem(option.displayName, option.id);
        }
    };
    populate(m_opticSelector, OperatorLoadoutCatalog::opticOptions());
    populate(m_barrelSelector, OperatorLoadoutCatalog::barrelOptions());
    populate(m_gripSelector, OperatorLoadoutCatalog::gripOptions());
    populate(m_underbarrelSelector, OperatorLoadoutCatalog::underbarrelOptions());
}

void OperatorSettingsPage::applyLoadoutSelectors(const QVariantMap& weaponSettings) {
    populateWeaponSelector();
    const auto defaults = defaultWeaponSettingsForSlot(m_weaponSlot);
    const QString selectedWeapon = weaponSettings.value(
        QStringLiteral("selected_weapon"),
        defaults.value(QStringLiteral("selected_weapon"))
    ).toString();
    int weaponIndex = m_weaponSelector->findData(selectedWeapon);
    if (weaponIndex < 0 && m_weaponSelector->count() > 0) {
        weaponIndex = 0;
    }

    QVariantMap attachments = OperatorLoadoutCatalog::defaultAttachments();
    const QVariantMap imported = weaponSettings.value(QStringLiteral("attachments")).toMap();
    for (auto iterator = imported.constBegin(); iterator != imported.constEnd(); ++iterator) {
        attachments.insert(iterator.key(), iterator.value());
    }

    auto select = [](QComboBox* combo, const QString& id) {
        const QSignalBlocker blocker(combo);
        const int index = combo->findData(id);
        combo->setCurrentIndex(index >= 0 ? index : 0);
    };
    {
        const QSignalBlocker blocker(m_weaponSelector);
        m_weaponSelector->setCurrentIndex(weaponIndex);
    }
    select(m_opticSelector, attachments.value(QStringLiteral("optic"), QStringLiteral("iron_1x")).toString());
    select(m_barrelSelector, attachments.value(QStringLiteral("barrel"), QStringLiteral("none")).toString());
    select(m_gripSelector, attachments.value(QStringLiteral("grip"), QStringLiteral("none")).toString());
    select(m_underbarrelSelector, attachments.value(QStringLiteral("underbarrel"), QStringLiteral("none")).toString());
}

QVariantMap OperatorSettingsPage::activeAttachmentSettings() const {
    QVariantMap attachments = {
        {QStringLiteral("optic"), m_opticSelector->currentData().toString()},
        {QStringLiteral("barrel"), m_barrelSelector->currentData().toString()},
        {QStringLiteral("grip"), m_gripSelector->currentData().toString()},
        {QStringLiteral("underbarrel"), m_underbarrelSelector->currentData().toString()},
    };
    const auto* optic = OperatorLoadoutCatalog::findOptic(
        m_opticSelector->currentData().toString()
    );
    attachments.insert(
        QStringLiteral("ads_profile_key"),
        optic == nullptr ? QStringLiteral("ads_1x") : optic->adsProfileKey
    );
    attachments.insert(
        QStringLiteral("optic_magnification"),
        optic == nullptr ? 1.0 : optic->magnification
    );
    return attachments;
}

void OperatorSettingsPage::updateAdsBindingLabel() {
    const auto attachments = activeAttachmentSettings();
    const QString profile = attachments.value(QStringLiteral("ads_profile_key")).toString();
    const QString display = profile == QStringLiteral("ads_2_5x")
        ? QStringLiteral("2.5x Optics ADS")
        : QStringLiteral("1x Optics ADS");
    m_adsBindingLabel->setText(
        QStringLiteral("Selected optic is linked to the shared %1 converter value. The backend receives the complete converter input map.")
            .arg(display)
    );
}

void OperatorSettingsPage::emitCurrentLoadoutSelection() {
    if (m_loading || m_operatorId.isEmpty()) {
        return;
    }
    const QString selectedWeapon = m_weaponSelector->currentData().toString();
    QVariantMap loadoutMetadata = activeAttachmentSettings();
    loadoutMetadata.insert(
        QStringLiteral("weapon_rpm"),
        OperatorLoadoutCatalog::weaponRpm(selectedWeapon)
    );
    loadoutMetadata.insert(
        QStringLiteral("default_delay_seconds"),
        OperatorLoadoutCatalog::delaySecondsForWeapon(selectedWeapon)
    );
    loadoutMetadata.insert(
        QStringLiteral("configured_time_delay_seconds"),
        m_timeDelay->value()
    );
    loadoutMetadata.insert(
        QStringLiteral("horizontal_ramp"),
        m_horizontalRamp->value()
    );
    loadoutMetadata.insert(
        QStringLiteral("vertical_ramp"),
        m_verticalRamp->value()
    );
    loadoutMetadata.insert(
        QStringLiteral("ramp_start_seconds"),
        m_rampStartSeconds->value()
    );

    Q_EMIT loadoutSelectionChanged(
        m_operatorId,
        m_weaponSlot,
        selectedWeapon,
        loadoutMetadata
    );
}

void OperatorSettingsPage::setWeaponSlot(const QString& slot) {
    const bool previousLoadingState = m_loading;
    if (!m_operatorId.isEmpty() && !previousLoadingState) {
        auto settings = m_drafts.value(m_operatorId, defaultSettingsFor(m_operatorId));
        settings.insert(m_weaponSlot, activeWeaponSettings());
        m_drafts.insert(m_operatorId, settings);
    }

    m_weaponSlot = slot == QStringLiteral("secondary")
        ? QStringLiteral("secondary")
        : QStringLiteral("primary");

    m_primaryButton->setProperty("accentButton", m_weaponSlot == QStringLiteral("primary"));
    m_secondaryButton->setProperty("accentButton", m_weaponSlot == QStringLiteral("secondary"));
    for (auto* button : {m_primaryButton, m_secondaryButton}) {
        button->style()->unpolish(button);
        button->style()->polish(button);
    }

    if (!m_operatorId.isEmpty() && !previousLoadingState) {
        auto settings = m_drafts.value(m_operatorId, defaultSettingsFor(m_operatorId));
        settings.insert(QStringLiteral("active_weapon"), m_weaponSlot);
        m_drafts.insert(m_operatorId, settings);
        Q_EMIT settingChanged(
            m_operatorId,
            QStringLiteral("active_weapon"),
            m_weaponSlot
        );
    }

    applyActiveWeaponSettings();
    m_loading = previousLoadingState;
    if (!m_loading) {
        emitCurrentLoadoutSelection();
    }
}

void OperatorSettingsPage::recordUndoSnapshot() {
    if (!m_operatorId.isEmpty()) {
        m_undoSnapshot = m_drafts.value(m_operatorId, defaultSettingsFor(m_operatorId));
    }
}

void OperatorSettingsPage::restoreSnapshot(const QVariantMap& snapshot) {
    if (m_operatorId.isEmpty() || snapshot.isEmpty()) {
        return;
    }
    m_drafts.insert(m_operatorId, snapshot);
    applySettings(snapshot);
}

void OperatorSettingsPage::updateHeader() {
    const auto* record = OperatorCatalog::findById(m_operatorId);
    if (record == nullptr) {
        return;
    }
    m_iconLabel->setPixmap(NexusTheme::pixmap(record->iconResource, 108, 108));
    m_nameLabel->setText(record->displayName.toUpper());
    m_sideLabel->setText(
        record->side.compare(QStringLiteral("attacker"), Qt::CaseInsensitive) == 0
            ? QStringLiteral("ATTACKER")
            : QStringLiteral("DEFENDER")
    );
    m_assetLabel->setText(
        QStringLiteral("Catalog ID: %1  ·  Existing icon: :/assets/%2")
            .arg(record->id, record->iconResource)
    );
}

void OperatorSettingsPage::updateVisualization() {
    const double x = m_xAmount->value();
    const double y = m_yAmount->value();
    const double horizontalRamp = m_horizontalRamp->value();
    const double verticalRamp = m_verticalRamp->value();
    const double rampStartSeconds = m_rampStartSeconds->value();
    const double rampedX = qFuzzyIsNull(horizontalRamp) ? x : horizontalRamp;
    const double rampedY = qFuzzyIsNull(verticalRamp) ? y : verticalRamp;
    const double delay = qMax(0.000001, m_timeDelay->value());
    m_vectorPreview->setValues(x, y, delay, horizontalRamp, verticalRamp, rampStartSeconds);

    const double baseMagnitude = qSqrt((x * x) + (y * y));
    const double rampedMagnitude = qSqrt((rampedX * rampedX) + (rampedY * rampedY));
    const double angle = qRadiansToDegrees(qAtan2(rampedY, rampedX));
    const double speed = rampedMagnitude / delay;
    const QString slope = qFuzzyIsNull(rampedX)
        ? (qFuzzyIsNull(rampedY) ? QStringLiteral("0") : QStringLiteral("∞"))
        : QString::number(rampedY / rampedX, 'f', 2);

    m_slopeLabel->setText(slope);
    m_vectorLabel->setText(
        QStringLiteral("%1, %2 -> %3, %4")
            .arg(QString::number(x, 'f', 0))
            .arg(QString::number(y, 'f', 0))
            .arg(QString::number(rampedX, 'f', 2))
            .arg(QString::number(rampedY, 'f', 2))
    );
    m_angleLabel->setText(QString::number(angle, 'f', 1) + QStringLiteral("°"));
    m_speedLabel->setText(QString::number(speed, 'f', 1));
    m_pullLabel->setText(
        QStringLiteral("%1 -> %2")
            .arg(QString::number(baseMagnitude, 'f', 2))
            .arg(QString::number(rampedMagnitude, 'f', 2))
    );
    m_rampStartLabel->setText(QString::number(rampStartSeconds, 'f', 2) + QStringLiteral("s"));
}

void OperatorSettingsPage::updateStatus(const QString& text, bool success) {
    m_statusLabel->setText(text);
    m_statusLabel->setProperty("success", success);
    m_statusLabel->setProperty("muted", !success);
    m_statusLabel->style()->unpolish(m_statusLabel);
    m_statusLabel->style()->polish(m_statusLabel);
}

int OperatorSettingsPage::normalizedRapidFireValue(
    const QVariantMap& settings
) const {
    // Exact schema-v2 format:
    //   rapid_fire_enabled: bool
    //   rapid_fire_value: 0..3
    // 0 = off, 1 = Weapon 1, 2 = Weapon 2, 3 = both weapons.
    const bool hasEnabled = settings.contains(QStringLiteral("rapid_fire_enabled"));
    const bool enabled = settings.value(
        QStringLiteral("rapid_fire_enabled"),
        false
    ).toBool();

    bool hasNumericValue = settings.contains(QStringLiteral("rapid_fire_value"));
    int value = settings.value(QStringLiteral("rapid_fire_value"), 0).toInt();

    // Migration support for NEXUS.4.2 development files. Export never writes this key.
    if (!hasNumericValue && settings.contains(QStringLiteral("rapid_fire_target"))) {
        const QString target = settings.value(QStringLiteral("rapid_fire_target"))
                                   .toString()
                                   .trimmed()
                                   .toLower();
        if (target == QStringLiteral("weapon_1") ||
            target == QStringLiteral("weapon1") ||
            target == QStringLiteral("primary")) {
            value = 1;
        } else if (target == QStringLiteral("weapon_2") ||
                   target == QStringLiteral("weapon2") ||
                   target == QStringLiteral("secondary")) {
            value = 2;
        } else if (target == QStringLiteral("both") ||
                   target == QStringLiteral("both_weapons")) {
            value = 3;
        } else {
            value = 0;
        }
        hasNumericValue = true;
    }

    value = qBound(0, value, 3);

    // In schema v2 the boolean is authoritative when explicitly false.
    if (hasEnabled && !enabled) {
        return 0;
    }

    if (value > 0) {
        return value;
    }

    // Old files that only had rapid_fire_enabled=true map safely to BOTH.
    return enabled ? 3 : 0;
}

void OperatorSettingsPage::setRapidFireValue(
    int value,
    bool persist
) {
    const int normalized = qBound(0, value, 3);
    m_rapidFireValue = normalized;

    if (persist && !m_loading && !m_operatorId.isEmpty()) {
        m_undoSnapshot = m_drafts.value(
            m_operatorId,
            defaultSettingsFor(m_operatorId)
        );
        auto settings = m_drafts.value(
            m_operatorId,
            defaultSettingsFor(m_operatorId)
        );
        settings.remove(QStringLiteral("rapid_fire_target"));
        settings.insert(QStringLiteral("rapid_fire_value"), normalized);
        settings.insert(QStringLiteral("rapid_fire_enabled"), normalized != 0);
        settings.insert(QStringLiteral("operator_id"), m_operatorId);
        m_drafts.insert(m_operatorId, settings);

        updateStatus(
            QStringLiteral("Rapid fire target updated inside the shared NEXUS configuration.")
        );
        Q_EMIT settingChanged(
            m_operatorId,
            QStringLiteral("rapid_fire_value"),
            normalized
        );
        Q_EMIT settingChanged(
            m_operatorId,
            QStringLiteral("rapid_fire_enabled"),
            normalized != 0
        );
        Q_EMIT rapidFireSelectionChanged(
            m_operatorId,
            normalized,
            normalized != 0
        );
    }

    updateRapidFireStatus();
}

void OperatorSettingsPage::cycleRapidFireValue() {
    // Fourth click returns to Off. The third enabled state is BOTH weapons;
    // it is not a third weapon profile.
    setRapidFireValue((m_rapidFireValue + 1) % 4, true);
}

void OperatorSettingsPage::updateRapidFireStatus() {
    QString buttonText;
    QString statusText;

    if (m_rapidFireValue == 1) {
        buttonText = QStringLiteral("RAPID FIRE: WEAPON 1");
        statusText = QStringLiteral("RAPID FIRE · WEAPON 1");
    } else if (m_rapidFireValue == 2) {
        buttonText = QStringLiteral("RAPID FIRE: WEAPON 2");
        statusText = QStringLiteral("RAPID FIRE · WEAPON 2");
    } else if (m_rapidFireValue == 3) {
        buttonText = QStringLiteral("RAPID FIRE: BOTH WEAPONS");
        statusText = QStringLiteral("RAPID FIRE · BOTH WEAPONS");
    } else {
        buttonText = QStringLiteral("RAPID FIRE: OFF");
        statusText = QStringLiteral("RAPID FIRE DISABLED");
    }

    const bool enabled = m_rapidFireValue != 0;
    m_rapidFireButton->setText(buttonText);
    m_rapidFireButton->setProperty("accentButton", enabled);
    m_rapidFireButton->style()->unpolish(m_rapidFireButton);
    m_rapidFireButton->style()->polish(m_rapidFireButton);

    m_rapidFireStatusLabel->setText(statusText);
    m_rapidFireStatusLabel->setProperty("rapidFireEnabled", enabled);
    m_rapidFireStatusLabel->style()->unpolish(m_rapidFireStatusLabel);
    m_rapidFireStatusLabel->style()->polish(m_rapidFireStatusLabel);
}
