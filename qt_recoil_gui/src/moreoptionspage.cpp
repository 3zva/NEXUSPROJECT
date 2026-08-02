#include "moreoptionspage.h"

#include "nexuswidgets.h"
#include "theme.h"

#include <QComboBox>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScreen>
#include <QSizePolicy>
#include <QStyle>
#include <QScrollArea>
#include <QVariant>
#include <QVBoxLayout>

namespace {
QLabel* mutedText(const QString& text, QWidget* parent) {
    auto* label = new QLabel(text, parent);
    label->setProperty("muted", true);
    label->setFont(NexusTheme::font(9));
    label->setWordWrap(true);
    return label;
}

QLineEdit* regionValueBox(const QString& placeholder, QWidget* parent) {
    auto* value = new QLineEdit(parent);
    value->setReadOnly(true);
    value->setAlignment(Qt::AlignCenter);
    value->setPlaceholderText(placeholder);
    value->setProperty("valueBox", true);
    value->setMinimumWidth(82);
    return value;
}

QWidget* createRegionField(
    const QString& title,
    QLineEdit*& value,
    QWidget* parent
) {
    auto* field = new QWidget(parent);
    auto* layout = new QVBoxLayout(field);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(5);

    auto* label = new QLabel(title, field);
    label->setProperty("muted", true);
    label->setFont(NexusTheme::font(8, QFont::DemiBold));
    value = regionValueBox(QStringLiteral("—"), field);

    layout->addWidget(label);
    layout->addWidget(value);
    return field;
}
}

MoreOptionsPage::MoreOptionsPage(QWidget* parent)
    : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setFrameShape(QFrame::NoFrame);

    auto* content = new QWidget(scroll);
    auto* body = new QVBoxLayout(content);
    body->setContentsMargins(
        NexusTheme::ContentPadding,
        18,
        NexusTheme::ContentPadding,
        NexusTheme::ContentPadding
    );
    body->setSpacing(14);

    auto* topRow = new QHBoxLayout();
    auto* backButton = new QPushButton(QStringLiteral("←  Back"), content);
    backButton->setCursor(Qt::PointingHandCursor);
    backButton->setMaximumWidth(110);
    connect(backButton, &QPushButton::clicked, this, &MoreOptionsPage::backRequested);
    topRow->addWidget(backButton);
    topRow->addStretch();
    body->addLayout(topRow);

    body->addWidget(new PageHeading(
        QStringLiteral("MORE OPTIONS"),
        QStringLiteral(
            "Choose the screen region used by the separate NEXUS overlay monitor "
            "and manage its UI-facing preferences."
        ),
        content
    ));

    auto* monitorCard = new CardFrame(content, true);
    auto* monitorLayout = new QVBoxLayout(monitorCard);
    monitorLayout->setContentsMargins(18, 18, 18, 18);
    monitorLayout->setSpacing(13);

    auto* cardTitle = new QLabel(QStringLiteral("SCREEN REGION MONITOR"), monitorCard);
    cardTitle->setFont(NexusTheme::font(12, QFont::Bold));
    cardTitle->setProperty("accent", true);
    monitorLayout->addWidget(cardTitle);
    monitorLayout->addWidget(mutedText(
        QStringLiteral(
            "Select the exact area that your existing overlay module should watch. "
            "This page stores and exposes the region; it does not perform capture or detection."
        ),
        monitorCard
    ));

    auto* displayRow = new QHBoxLayout();
    displayRow->setSpacing(10);
    auto* displayLabel = new QLabel(QStringLiteral("Display"), monitorCard);
    displayLabel->setFont(NexusTheme::font(9, QFont::DemiBold));
    m_displayCombo = new QComboBox(monitorCard);
    m_displayCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    refreshDisplays();
    connect(m_displayCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (index < 0) {
            return;
        }
        m_displayId = m_displayCombo->currentData().toString();
        Q_EMIT settingChanged(QStringLiteral("overlay/display_id"), m_displayId);
    });
    displayRow->addWidget(displayLabel);
    displayRow->addWidget(m_displayCombo, 1);
    monitorLayout->addLayout(displayRow);

    m_previewLabel = new QLabel(monitorCard);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setMinimumHeight(210);
    m_previewLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_previewLabel->setProperty("regionPreview", true);
    m_previewLabel->setStyleSheet(QStringLiteral(
        "QLabel[regionPreview=\"true\"] {"
        " background: #090D17; border: 1px dashed #39445A;"
        " border-radius: 12px; color: #68738B; padding: 18px; }"
    ));
    monitorLayout->addWidget(m_previewLabel);

    auto* valuesRow = new QHBoxLayout();
    valuesRow->setSpacing(8);
    valuesRow->addWidget(createRegionField(QStringLiteral("X"), m_xValue, monitorCard), 1);
    valuesRow->addWidget(createRegionField(QStringLiteral("Y"), m_yValue, monitorCard), 1);
    valuesRow->addWidget(createRegionField(QStringLiteral("WIDTH"), m_widthValue, monitorCard), 1);
    valuesRow->addWidget(createRegionField(QStringLiteral("HEIGHT"), m_heightValue, monitorCard), 1);
    monitorLayout->addLayout(valuesRow);

    auto* actions = new QHBoxLayout();
    actions->setSpacing(8);
    auto* selectButton = createAccentButton(QStringLiteral("SELECT REGION"), monitorCard);
    selectButton->setToolTip(QStringLiteral(
        "Request region selection from the existing overlay controller."
    ));
    m_clearButton = new QPushButton(QStringLiteral("CLEAR REGION"), monitorCard);
    m_saveButton = new QPushButton(QStringLiteral("SAVE REGION"), monitorCard);
    connect(selectButton, &QPushButton::clicked,
            this, &MoreOptionsPage::regionSelectionRequested);
    connect(m_clearButton, &QPushButton::clicked, this, [this]() {
        clearSelectedRegion();
        Q_EMIT regionClearRequested();
    });
    connect(m_saveButton, &QPushButton::clicked, this, [this]() {
        if (!hasValidRegion()) {
            updateStatus(QStringLiteral("Select a valid region before saving."));
            return;
        }
        updateStatus(QStringLiteral("Waiting for the overlay integration to confirm the save..."));
        Q_EMIT regionSaveRequested(m_region, m_displayId);
    });
    actions->addWidget(selectButton, 1);
    actions->addWidget(m_clearButton, 1);
    actions->addWidget(m_saveButton, 1);
    monitorLayout->addLayout(actions);

    m_statusLabel = new QLabel(monitorCard);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setProperty("muted", true);
    m_statusLabel->setFont(NexusTheme::font(9));
    monitorLayout->addWidget(m_statusLabel);
    body->addWidget(monitorCard);

    auto* preferencesCard = new CardFrame(content, false);
    auto* preferencesLayout = new QVBoxLayout(preferencesCard);
    preferencesLayout->setContentsMargins(16, 16, 16, 16);
    preferencesLayout->setSpacing(10);
    preferencesLayout->addWidget(createSectionLabel(
        QStringLiteral("MONITORING PREFERENCES"),
        preferencesCard
    ));

    m_monitoringToggle = new ToggleRow(
        QStringLiteral("Enable region monitoring"),
        QStringLiteral(
            "Allows the separate monitoring controller to use the saved region. "
            "Unavailable until a valid region has been selected."
        ),
        false,
        preferencesCard
    );
    auto* cursorToggle = new ToggleRow(
        QStringLiteral("Check only while the cursor is visible"),
        QStringLiteral(
            "UI preference for your existing monitor. This page does not inspect the cursor."
        ),
        true,
        preferencesCard
    );
    auto* activeWindowToggle = new ToggleRow(
        QStringLiteral("Require the target window to be active"),
        QStringLiteral(
            "UI preference for pausing the separate monitor when the target is not focused."
        ),
        true,
        preferencesCard
    );

    connect(m_monitoringToggle, &ToggleRow::toggled, this, [this](bool enabled) {
        if (enabled && !hasValidRegion()) {
            m_monitoringToggle->setChecked(false);
            updateStatus(QStringLiteral("Select and save a valid region before enabling monitoring."));
            return;
        }
        Q_EMIT overlayMonitoringEnabledChanged(enabled);
        Q_EMIT settingChanged(QStringLiteral("overlay/enabled"), enabled);
    });
    connect(cursorToggle, &ToggleRow::toggled, this, [this](bool enabled) {
        Q_EMIT settingChanged(QStringLiteral("overlay/cursor_visible_only"), enabled);
    });
    connect(activeWindowToggle, &ToggleRow::toggled, this, [this](bool enabled) {
        Q_EMIT settingChanged(QStringLiteral("overlay/target_window_active_only"), enabled);
    });

    preferencesLayout->addWidget(m_monitoringToggle);
    preferencesLayout->addWidget(cursorToggle);
    preferencesLayout->addWidget(activeWindowToggle);
    body->addWidget(preferencesCard);

    auto* handoff = new CardFrame(content, false);
    auto* handoffLayout = new QVBoxLayout(handoff);
    handoffLayout->setContentsMargins(16, 14, 16, 14);
    handoffLayout->setSpacing(5);
    handoffLayout->addWidget(createSectionLabel(
        QStringLiteral("OVERLAY INTEGRATION STATUS"),
        handoff
    ));
    handoffLayout->addWidget(mutedText(
        QStringLiteral(
            "The page is fully routed and exposes typed Qt signals. Connect "
            "regionSelectionRequested() to the region-selection overlay you are already building, "
            "then call setSelectedRegion() with the result."
        ),
        handoff
    ));
    body->addWidget(handoff);
    body->addStretch();

    scroll->setWidget(content);
    root->addWidget(scroll);
    updateRegionControls();
}

void MoreOptionsPage::refreshDisplays() {
    const QString previous = m_displayId;
    m_displayCombo->clear();

    const auto screens = QGuiApplication::screens();
    for (int index = 0; index < screens.size(); ++index) {
        const auto* screen = screens.at(index);
        const QRect geometry = screen->geometry();
        const QString id = screen->name().isEmpty()
            ? QStringLiteral("display_%1").arg(index)
            : screen->name();
        const QString label = QStringLiteral("%1 — %2 × %3 at %4, %5")
            .arg(id)
            .arg(geometry.width())
            .arg(geometry.height())
            .arg(geometry.x())
            .arg(geometry.y());
        m_displayCombo->addItem(label, id);
    }

    int selected = previous.isEmpty() ? 0 : m_displayCombo->findData(previous);
    if (selected < 0) {
        selected = 0;
    }
    if (m_displayCombo->count() > 0) {
        m_displayCombo->setCurrentIndex(selected);
        m_displayId = m_displayCombo->currentData().toString();
    }
}

void MoreOptionsPage::setSelectedRegion(
    const QRect& region,
    const QString& displayId,
    const QPixmap& preview
) {
    m_region = region.normalized();
    m_displayId = displayId.trimmed();
    if (m_displayId.isEmpty() && m_displayCombo->currentIndex() >= 0) {
        m_displayId = m_displayCombo->currentData().toString();
    }

    const int displayIndex = m_displayCombo->findData(m_displayId);
    if (displayIndex >= 0) {
        m_displayCombo->setCurrentIndex(displayIndex);
    }

    updatePreview(preview);
    updateRegionControls();
    updateStatus(
        hasValidRegion()
            ? QStringLiteral("Region received. Review the values, then press Save Region.")
            : QStringLiteral("The overlay controller returned an invalid region.")
    );
}

void MoreOptionsPage::clearSelectedRegion() {
    m_region = QRect();
    updatePreview();
    updateRegionControls();
    updateStatus(QStringLiteral("Region cleared."));
}

void MoreOptionsPage::setRegionSaveResult(bool success, const QString& message) {
    const QString fallback = success
        ? QStringLiteral("Region settings saved.")
        : QStringLiteral("The region could not be saved.");
    updateStatus(message.isEmpty() ? fallback : message, success);
}

QRect MoreOptionsPage::selectedRegion() const {
    return m_region;
}

QString MoreOptionsPage::selectedDisplayId() const {
    return m_displayId;
}

bool MoreOptionsPage::hasValidRegion() const {
    return m_region.isValid() && m_region.width() > 0 && m_region.height() > 0;
}

void MoreOptionsPage::updateRegionControls() {
    const bool valid = hasValidRegion();
    m_xValue->setText(valid ? QString::number(m_region.x()) : QString());
    m_yValue->setText(valid ? QString::number(m_region.y()) : QString());
    m_widthValue->setText(valid ? QString::number(m_region.width()) : QString());
    m_heightValue->setText(valid ? QString::number(m_region.height()) : QString());
    m_clearButton->setEnabled(valid);
    m_saveButton->setEnabled(valid);
    m_monitoringToggle->setEnabled(valid);
    if (!valid && m_monitoringToggle->isChecked()) {
        m_monitoringToggle->setChecked(false);
    }
}

void MoreOptionsPage::updatePreview(const QPixmap& preview) {
    if (preview.isNull()) {
        m_previewLabel->setPixmap(QPixmap());
        m_previewLabel->setText(
            hasValidRegion()
                ? QStringLiteral("Selected region\n%1 × %2")
                      .arg(m_region.width())
                      .arg(m_region.height())
                : QStringLiteral("No region selected\nPress Select Region to open your overlay selector.")
        );
        return;
    }

    m_previewLabel->setText(QString());
    m_previewLabel->setPixmap(preview.scaled(
        760,
        260,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    ));
}

void MoreOptionsPage::updateStatus(const QString& text, bool success) {
    m_statusLabel->setText(text);
    m_statusLabel->setProperty("success", success);
    m_statusLabel->setProperty("muted", !success);
    m_statusLabel->style()->unpolish(m_statusLabel);
    m_statusLabel->style()->polish(m_statusLabel);
}
