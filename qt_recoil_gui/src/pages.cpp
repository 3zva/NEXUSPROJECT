#include "pages.h"
#include "nexuswidgets.h"
#include "operatorcatalog.h"
#include "theme.h"

#include <QComboBox>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPixmap>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSet>
#include <QSignalBlocker>
#include <QStyle>
#include <QVariantMap>
#include <QVBoxLayout>
#include <QtMath>

namespace {
double normalizeDelaySeconds(double value) {
    return qMax(0.001, value > 1.0 ? value / 1000.0 : value);
}

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
    root->setContentsMargins(18, 16, 18, 14);
    root->setSpacing(8);

    root->addWidget(new PageHeading(
        QStringLiteral("OPERATORS"),
        QStringLiteral("Select any operator to open the shared NEXUS settings workspace."),
        this
    ));

    auto* controls = new QHBoxLayout();
    controls->setSpacing(6);
    m_attackersButton = createAccentButton(QStringLiteral("ATTACKERS"), this);
    m_defendersButton = new QPushButton(QStringLiteral("DEFENDERS"), this);
    m_attackersButton->setFixedWidth(104);
    m_defendersButton->setFixedWidth(104);
    controls->addWidget(m_attackersButton);
    controls->addWidget(m_defendersButton);
    controls->addStretch();

    m_search = new QLineEdit(this);
    m_search->setPlaceholderText(QStringLiteral("Search operator..."));
    m_search->setMaximumWidth(210);
    controls->addWidget(m_search);
    m_filterButton = new QPushButton(this);
    m_filterButton->setIcon(NexusTheme::icon(QStringLiteral("filter_32.png")));
    m_filterButton->setFixedWidth(40);
    m_filterButton->setToolTip(QStringLiteral("Filter operators"));
    controls->addWidget(m_filterButton);
    root->addLayout(controls);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_gridContent = new QWidget(m_scrollArea);
    m_grid = new QGridLayout(m_gridContent);
    m_grid->setContentsMargins(0, 0, 4, 0);
    m_grid->setHorizontalSpacing(7);
    m_grid->setVerticalSpacing(7);
    m_scrollArea->setWidget(m_gridContent);
    root->addWidget(m_scrollArea, 1);

    auto* footer = new QHBoxLayout();
    m_countLabel = bodyText(QString(), this);
    auto* viewAll = createAccentButton(QStringLiteral("VIEW ALL"), this);
    viewAll->setFixedWidth(82);
    footer->addWidget(m_countLabel, 1);
    footer->addWidget(viewAll);
    root->addLayout(footer);

    connect(m_attackersButton, &QPushButton::clicked, this, [this]() { setSide(QStringLiteral("attackers")); });
    connect(m_defendersButton, &QPushButton::clicked, this, [this]() { setSide(QStringLiteral("defenders")); });
    connect(m_search, &QLineEdit::textChanged, this, [this]() { rebuildGrid(); });
    connect(viewAll, &QPushButton::clicked, this, [this]() {
        m_search->clear();
        m_filter = QStringLiteral("all");
        rebuildGrid();
    });
    connect(m_filterButton, &QPushButton::clicked, this, [this]() {
        QMenu menu(this);
        struct FilterDefinition { QString key; QString label; };
        const QList<FilterDefinition> filters{
            {QStringLiteral("all"), QStringLiteral("All operators")},
            {QStringLiteral("configured"), QStringLiteral("Configured")},
            {QStringLiteral("unconfigured"), QStringLiteral("Unconfigured")},
            {QStringLiteral("favorites"), QStringLiteral("Favorites")}
        };
        for (const auto& filter : filters) {
            auto* action = menu.addAction(filter.label);
            action->setCheckable(true);
            action->setChecked(m_filter == filter.key);
            connect(action, &QAction::triggered, this, [this, key = filter.key]() {
                m_filter = key;
                rebuildGrid();
            });
        }
        menu.exec(m_filterButton->mapToGlobal(QPoint(0, m_filterButton->height())));
    });
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
    QList<OperatorRecord> visibleRecords;
    visibleRecords.reserve(source.size());

    for (const auto& record : source) {
        if (!search.isEmpty()
            && !record.displayName.contains(search, Qt::CaseInsensitive)
            && !record.id.contains(search, Qt::CaseInsensitive)) {
            continue;
        }
        if (m_filter == QStringLiteral("favorites")) {
            continue;
        }
        visibleRecords.push_back(record);
    }

    const int viewportWidth = m_scrollArea != nullptr
        ? m_scrollArea->viewport()->width()
        : width();
    const int viewportHeight = m_scrollArea != nullptr
        ? m_scrollArea->viewport()->height()
        : height();
    const GridMetrics metrics = gridMetricsFor(viewportWidth, viewportHeight, visibleRecords.size());
    m_grid->setHorizontalSpacing(metrics.spacing);
    m_grid->setVerticalSpacing(metrics.spacing);
    m_currentColumns = metrics.columns;
    m_currentGridHeight = viewportHeight;

    int shown = 0;
    for (const auto& record : visibleRecords) {
        auto* tile = new OperatorTile(
            record.displayName,
            record.iconResource,
            m_gridContent
        );
        tile->setDisplayMetrics(
            metrics.iconSize,
            metrics.tileSize,
            metrics.fontSize,
            metrics.radius,
            metrics.padding
        );
        connect(tile, &QToolButton::clicked, this, [this, operatorId = record.id]() {
            Q_EMIT operatorSelected(operatorId);
        });
        m_grid->addWidget(tile, shown / metrics.columns, shown % metrics.columns);
        ++shown;
    }

    for (int column = 0; column < metrics.columns; ++column) {
        m_grid->setColumnStretch(column, 1);
    }
    m_grid->setRowStretch((shown + metrics.columns - 1) / metrics.columns, 1);
    const QString filterLabel = m_filter == QStringLiteral("all")
        ? QStringLiteral("all")
        : m_filter;
    m_countLabel->setText(QStringLiteral(
        "Showing %1 of %2 %3 (%4). Every tile opens the same reusable settings page with the matching icon."
    ).arg(shown).arg(source.size()).arg(m_side, filterLabel));
}

OperatorsPage::GridMetrics OperatorsPage::gridMetricsFor(int width, int height, int itemCount) const {
    const QList<GridMetrics> modes{
        {qBound(4, width / 82, 12), 7, QSize(72, 78), QSize(42, 42), 8, 9, 5},
        {qBound(5, width / 72, 13), 6, QSize(64, 68), QSize(36, 36), 8, 8, 4},
        {qBound(6, width / 62, 14), 5, QSize(56, 58), QSize(30, 30), 7, 7, 3},
    };

    for (const GridMetrics& metrics : modes) {
        const int rows = qMax(1, (itemCount + metrics.columns - 1) / metrics.columns);
        const int requiredHeight = rows * metrics.tileSize.height() + qMax(0, rows - 1) * metrics.spacing;
        if (requiredHeight <= qMax(180, height - 4)) {
            return metrics;
        }
    }
    return modes.last();
}

void OperatorsPage::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (m_scrollArea == nullptr) {
        return;
    }
    const int viewportWidth = m_scrollArea->viewport()->width();
    const int viewportHeight = m_scrollArea->viewport()->height();
    const GridMetrics metrics = gridMetricsFor(viewportWidth, viewportHeight, 38);
    if (metrics.columns != m_currentColumns || qAbs(viewportHeight - m_currentGridHeight) > 16) {
        rebuildGrid();
    }
}

MoreOptionsPage::MoreOptionsPage(QWidget* parent)
    : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    QVBoxLayout* bodyLayout = nullptr;
    root->addWidget(createScrollableBody(this, bodyLayout));

    auto* headerRow = new QHBoxLayout();
    headerRow->setSpacing(10);
    auto* backButton = new QPushButton(QStringLiteral("Back"), this);
    backButton->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));
    backButton->setFixedWidth(92);
    backButton->setCursor(Qt::PointingHandCursor);
    headerRow->addWidget(backButton);
    headerRow->addWidget(new PageHeading(
        QStringLiteral("MORE OPTIONS"),
        QStringLiteral("Screen-region routing and overlay behavior."),
        this
    ), 1);
    bodyLayout->addLayout(headerRow);
    connect(backButton, &QPushButton::clicked, this, &MoreOptionsPage::backRequested);

    auto* statusCard = new CardFrame(this, true);
    auto* statusLayout = new QHBoxLayout(statusCard);
    statusLayout->setContentsMargins(16, 12, 16, 12);
    statusLayout->setSpacing(12);
    m_statusPill = new QLabel(QStringLiteral("IDLE"), statusCard);
    m_statusPill->setProperty("statusPill", true);
    m_statusPill->setAlignment(Qt::AlignCenter);
    m_statusPill->setMinimumWidth(88);
    m_statusText = bodyText(QStringLiteral("No screen region is selected."), statusCard);
    statusLayout->addWidget(m_statusPill);
    statusLayout->addWidget(m_statusText, 1);
    bodyLayout->addWidget(statusCard);

    auto* regionCard = new CardFrame(this, true);
    auto* regionLayout = new QVBoxLayout(regionCard);
    regionLayout->setContentsMargins(18, 16, 18, 16);
    regionLayout->setSpacing(12);
    regionLayout->addWidget(createSectionLabel(QStringLiteral("SCREEN REGION"), regionCard));

    auto* displayRow = new QHBoxLayout();
    auto* displayLabel = new QLabel(QStringLiteral("Display"), regionCard);
    displayLabel->setFont(NexusTheme::font(10, QFont::DemiBold));
    m_displayBox = new QComboBox(regionCard);
    displayRow->addWidget(displayLabel);
    displayRow->addWidget(m_displayBox, 1);
    regionLayout->addLayout(displayRow);

    auto* previewAndFields = new QHBoxLayout();
    previewAndFields->setSpacing(14);
    m_preview = new QLabel(regionCard);
    m_preview->setMinimumSize(300, 170);
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setProperty("previewBox", true);
    m_preview->setText(QStringLiteral("Region preview"));
    previewAndFields->addWidget(m_preview, 1);

    auto* fields = new QWidget(regionCard);
    auto* fieldsLayout = new QGridLayout(fields);
    fieldsLayout->setContentsMargins(0, 0, 0, 0);
    fieldsLayout->setHorizontalSpacing(8);
    fieldsLayout->setVerticalSpacing(8);
    auto makeField = [fields](const QString& label, int row, QGridLayout* layout) {
        auto* fieldLabel = new QLabel(label, fields);
        fieldLabel->setProperty("muted", true);
        auto* field = new QLineEdit(fields);
        field->setReadOnly(true);
        field->setProperty("valueBox", true);
        layout->addWidget(fieldLabel, row, 0);
        layout->addWidget(field, row, 1);
        return field;
    };
    m_xField = makeField(QStringLiteral("X"), 0, fieldsLayout);
    m_yField = makeField(QStringLiteral("Y"), 1, fieldsLayout);
    m_widthField = makeField(QStringLiteral("Width"), 2, fieldsLayout);
    m_heightField = makeField(QStringLiteral("Height"), 3, fieldsLayout);
    m_displayField = makeField(QStringLiteral("Display ID"), 4, fieldsLayout);
    previewAndFields->addWidget(fields);
    regionLayout->addLayout(previewAndFields);

    auto* actions = new QHBoxLayout();
    m_selectButton = createAccentButton(QStringLiteral("SELECT REGION"), regionCard);
    m_clearButton = new QPushButton(QStringLiteral("CLEAR"), regionCard);
    m_saveButton = createAccentButton(QStringLiteral("SAVE REGION"), regionCard);
    actions->addWidget(m_selectButton);
    actions->addWidget(m_clearButton);
    actions->addStretch();
    actions->addWidget(m_saveButton);
    regionLayout->addLayout(actions);
    bodyLayout->addWidget(regionCard);

    auto* monitorCard = new CardFrame(this, true);
    auto* monitorLayout = new QVBoxLayout(monitorCard);
    monitorLayout->setContentsMargins(18, 16, 18, 16);
    monitorLayout->setSpacing(10);
    monitorLayout->addWidget(createSectionLabel(QStringLiteral("OVERLAY BEHAVIOR"), monitorCard));
    m_enableMonitoring = new ToggleRow(
        QStringLiteral("Enable operator detection"),
        QStringLiteral("Use the saved region to route detected operator names to the active settings page."),
        false,
        monitorCard
    );
    m_showBorder = new ToggleRow(
        QStringLiteral("Show selected-region border"),
        QStringLiteral("Draw a lightweight visual border when region selection is active."),
        true,
        monitorCard
    );
    m_pauseWhenForeground = new ToggleRow(
        QStringLiteral("Idle while cursor is hidden"),
        QStringLiteral("Keep monitoring paused whenever the cursor is not visible."),
        false,
        monitorCard
    );
    m_lowResourceMode = new ToggleRow(
        QStringLiteral("Low resource mode"),
        QStringLiteral("Use a lower capture rate while preserving operator routing."),
        false,
        monitorCard
    );
    monitorLayout->addWidget(m_enableMonitoring);
    monitorLayout->addWidget(m_showBorder);
    monitorLayout->addWidget(m_pauseWhenForeground);
    monitorLayout->addWidget(m_lowResourceMode);
    bodyLayout->addWidget(monitorCard);
    bodyLayout->addStretch();

    connect(m_selectButton, &QPushButton::clicked, this, [this]() {
        setSelectionPending();
        Q_EMIT regionSelectionRequested();
    });
    connect(m_clearButton, &QPushButton::clicked, this, [this]() {
        clearSelectedRegion();
        Q_EMIT regionClearRequested();
    });
    connect(m_saveButton, &QPushButton::clicked, this, [this]() {
        if (!m_hasRegion || !m_region.isValid()) {
            setRegionSaveResult(false, QStringLiteral("Select a valid region before saving."));
            return;
        }
        setRegionSaveResult(true, QStringLiteral("Saving region settings..."));
        Q_EMIT regionSaveRequested(m_region, m_displayId);
    });
    connect(m_enableMonitoring, &ToggleRow::toggled,
            this, &MoreOptionsPage::overlayMonitoringEnabledChanged);
    connect(m_showBorder, &ToggleRow::toggled,
            this, &MoreOptionsPage::showSelectionBorderChanged);
    connect(m_pauseWhenForeground, &ToggleRow::toggled,
            this, &MoreOptionsPage::pauseWhenForegroundChanged);
    connect(m_lowResourceMode, &ToggleRow::toggled,
            this, &MoreOptionsPage::lowResourceMonitoringChanged);

    setAvailableDisplays({});
    updateRegionState();
}

void MoreOptionsPage::setAvailableDisplays(const QList<DisplayOption>& displays) {
    m_displays = displays;
    QSignalBlocker blocker(m_displayBox);
    m_displayBox->clear();
    if (m_displays.isEmpty()) {
        m_displayBox->addItem(QStringLiteral("Primary display"), QStringLiteral("primary"));
        return;
    }
    for (const auto& display : m_displays) {
        m_displayBox->addItem(display.displayName, display.id);
    }
}

void MoreOptionsPage::setSelectedRegion(const QRect& region, const QString& displayId) {
    m_region = region.normalized();
    m_displayId = displayId.isEmpty() ? QStringLiteral("primary") : displayId;
    m_hasRegion = m_region.isValid();
    for (int index = 0; index < m_displayBox->count(); ++index) {
        if (m_displayBox->itemData(index).toString() == m_displayId) {
            m_displayBox->setCurrentIndex(index);
            break;
        }
    }
    refreshRegionFields();
    updateRegionPreview();
    updateRegionState();
}

void MoreOptionsPage::clearSelectedRegion() {
    m_region = QRect();
    m_displayId.clear();
    m_hasRegion = false;
    m_preview->setPixmap(QPixmap());
    m_preview->setText(QStringLiteral("Region preview"));
    refreshRegionFields();
    updateRegionState();
}

void MoreOptionsPage::setSelectionPending() {
    m_statusPill->setText(QStringLiteral("SELECTING"));
    m_statusText->setText(QStringLiteral("Drag across the screen to choose the reading area."));
}

void MoreOptionsPage::setSelectionError(const QString& message) {
    m_statusPill->setText(QStringLiteral("ERROR"));
    m_statusText->setText(message);
}

void MoreOptionsPage::setRegionSaveResult(bool success, const QString& message) {
    m_statusPill->setText(success ? QStringLiteral("SAVED") : QStringLiteral("ERROR"));
    m_statusText->setText(message);
}

void MoreOptionsPage::updateRegionPreview() {
    if (!m_hasRegion || !m_region.isValid()) {
        m_preview->setPixmap(QPixmap());
        m_preview->setText(QStringLiteral("Region preview"));
        return;
    }

    QSize targetSize = m_preview->contentsRect().size();
    if (targetSize.isEmpty()) {
        targetSize = m_preview->size();
    }
    targetSize = targetSize.expandedTo(QSize(160, 90));

    QPixmap preview(targetSize);
    preview.fill(Qt::transparent);

    const QSize regionSize = m_region.normalized().size();
    QSize fitted = regionSize;
    fitted.scale(targetSize - QSize(20, 20), Qt::KeepAspectRatio);
    fitted = fitted.expandedTo(QSize(18, 18));
    const QRect regionRect(
        QPoint((targetSize.width() - fitted.width()) / 2, (targetSize.height() - fitted.height()) / 2),
        fitted
    );

    QPainter painter(&preview);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(regionRect, QColor(118, 91, 255, 34));
    painter.setPen(QPen(QColor(118, 91, 255), 2));
    painter.drawRect(regionRect.adjusted(1, 1, -2, -2));
    painter.end();

    m_preview->setText(QString());
    m_preview->setPixmap(preview);
}

void MoreOptionsPage::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    updateRegionPreview();
}

void MoreOptionsPage::setOverlaySettings(
    bool monitoringEnabled,
    bool showSelectionBorder,
    bool pauseWhenCursorHidden,
    bool lowResourceMode
) {
    const QSignalBlocker monitorBlocker(m_enableMonitoring);
    const QSignalBlocker borderBlocker(m_showBorder);
    const QSignalBlocker pauseBlocker(m_pauseWhenForeground);
    const QSignalBlocker lowResourceBlocker(m_lowResourceMode);
    m_enableMonitoring->setChecked(monitoringEnabled && m_hasRegion);
    m_showBorder->setChecked(showSelectionBorder);
    m_pauseWhenForeground->setChecked(pauseWhenCursorHidden);
    m_lowResourceMode->setChecked(lowResourceMode);
    updateRegionState();
}

QRect MoreOptionsPage::selectedRegion() const {
    return m_region;
}

QString MoreOptionsPage::selectedDisplayId() const {
    return m_displayId;
}

bool MoreOptionsPage::overlayMonitoringEnabled() const {
    return m_enableMonitoring->isChecked();
}

void MoreOptionsPage::refreshRegionFields() {
    m_xField->setText(m_hasRegion ? QString::number(m_region.x()) : QStringLiteral("-"));
    m_yField->setText(m_hasRegion ? QString::number(m_region.y()) : QStringLiteral("-"));
    m_widthField->setText(m_hasRegion ? QString::number(m_region.width()) : QStringLiteral("-"));
    m_heightField->setText(m_hasRegion ? QString::number(m_region.height()) : QStringLiteral("-"));
    m_displayField->setText(m_hasRegion ? m_displayId : QStringLiteral("-"));
}

void MoreOptionsPage::updateRegionState() {
    m_saveButton->setEnabled(m_hasRegion);
    m_clearButton->setEnabled(m_hasRegion);
    m_enableMonitoring->setEnabled(m_hasRegion);
    if (!m_hasRegion) {
        m_enableMonitoring->setChecked(false);
        m_statusPill->setText(QStringLiteral("IDLE"));
        m_statusText->setText(QStringLiteral("No screen region is selected."));
        return;
    }
    m_statusPill->setText(QStringLiteral("READY"));
    m_statusText->setText(QStringLiteral("Region is ready for operator-name routing."));
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
    };
    for (const auto& toggle : toggles) {
        auto* row = new ToggleRow(toggle.title, toggle.description, toggle.value, settings);
        m_toggles.insert(toggle.key, row);
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
    m_refreshRateBox = new QComboBox(rate);
    m_refreshRateBox->addItems({QStringLiteral("30"), QStringLiteral("60"), QStringLiteral("120"), QStringLiteral("144"), QStringLiteral("240")});
    m_refreshRateBox->setCurrentText(QStringLiteral("60"));
    m_refreshRateBox->setFixedWidth(96);
    connect(m_refreshRateBox, &QComboBox::currentTextChanged, this, [this](const QString& value) {
        Q_EMIT settingChanged(QStringLiteral("refresh_rate"), value.toInt());
    });
    rateLayout->addWidget(rateLabel, 1);
    rateLayout->addWidget(m_refreshRateBox);
    settingsLayout->addWidget(rate);

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

void ClientSettingsPage::setSavedSettings(const QVariantMap& settings) {
    const QHash<QString, QVariant> defaults{
        {QStringLiteral("mute_sounds"), false},
        {QStringLiteral("show_fps"), true},
        {QStringLiteral("performance_mode"), true},
        {QStringLiteral("outline_crosshairs"), false},
        {QStringLiteral("minimize_to_tray"), true},
        {QStringLiteral("startup"), true},
    };

    for (auto iterator = m_toggles.begin(); iterator != m_toggles.end(); ++iterator) {
        const QSignalBlocker blocker(iterator.value());
        iterator.value()->setChecked(settings.value(
            iterator.key(),
            defaults.value(iterator.key(), false)
        ).toBool());
    }

    if (m_refreshRateBox != nullptr) {
        const QSignalBlocker blocker(m_refreshRateBox);
        m_refreshRateBox->setCurrentText(QString::number(settings.value(
            QStringLiteral("refresh_rate"),
            60
        ).toInt()));
    }
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
        {QStringLiteral("Crouchspam"), QStringLiteral("K"), QStringLiteral("crouchspam")},
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
        QStringLiteral("Keybinds currently accept compact key names. Keep your in-game crouch key synchronized with the Crouchspam setting."),
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
        {QStringLiteral("UI Scale"), {QStringLiteral("75%"), QStringLiteral("90%"), QStringLiteral("100%"), QStringLiteral("110%"), QStringLiteral("125%")}, QStringLiteral("ui_scale")},
    };
    int comboRow = 1;
    for (const auto& definition : combos) {
        auto* label = new QLabel(definition.label, appearance);
        label->setProperty("muted", true);
        auto* box = new QComboBox(appearance);
        box->addItems(definition.values);
        m_combos.insert(definition.key, box);
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

    auto* updates = new ToggleRow(QStringLiteral("Auto check for updates?"), QString(), true, other);
    auto* anonymous = new ToggleRow(QStringLiteral("Send anonymous data?"), QString(), false, other);
    connect(updates, &ToggleRow::toggled, this, [this](bool value) {
        Q_EMIT settingChanged(QStringLiteral("auto_updates"), value);
    });
    connect(anonymous, &ToggleRow::toggled, this, [this](bool value) {
        Q_EMIT settingChanged(QStringLiteral("anonymous_data"), value);
    });
    otherLayout->addWidget(updates);
    otherLayout->addWidget(anonymous);
    rightLayout->addWidget(other);
    rightLayout->addStretch();

    columns->addWidget(keyCard, 3);
    columns->addWidget(right, 2);
    bodyLayout->addLayout(columns);
    bodyLayout->addStretch();
}

void SettingsPage::setSavedSettings(const QVariantMap& settings) {
    for (auto iterator = m_combos.begin(); iterator != m_combos.end(); ++iterator) {
        QString value = settings.value(iterator.key()).toString();
        if (value.isEmpty()) {
            continue;
        }
        if (iterator.key() == QStringLiteral("ui_scale") && value == QStringLiteral("50%")) {
            value = QStringLiteral("75%");
        }
        const QSignalBlocker blocker(iterator.value());
        const int index = iterator.value()->findText(value);
        if (index >= 0) {
            iterator.value()->setCurrentIndex(index);
        }
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
    auto* heroLayout = new QHBoxLayout(hero);
    heroLayout->setContentsMargins(22, 20, 22, 20);
    heroLayout->setSpacing(20);

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
    notesCard->setMinimumWidth(260);
    notesCard->setMaximumWidth(330);
    auto* notesLayout = new QVBoxLayout(notesCard);
    notesLayout->setContentsMargins(14, 12, 14, 12);
    notesLayout->setSpacing(7);
    notesLayout->addWidget(createSectionLabel(QStringLiteral("ADDITIONAL NOTES"), notesCard));
    m_notes = new QPlainTextEdit(notesCard);
    m_notes->setPlaceholderText(QStringLiteral("Click to add operator notes..."));
    m_notes->setMinimumHeight(88);
    notesLayout->addWidget(m_notes);

    heroLayout->addWidget(iconCard);
    heroLayout->addLayout(identity, 1);
    heroLayout->addWidget(notesCard);
    bodyLayout->addWidget(hero);

    auto* weaponTabs = new QHBoxLayout();
    weaponTabs->setSpacing(8);
    m_primaryButton = createAccentButton(QStringLiteral("WEAPON 1"), this);
    m_secondaryButton = new QPushButton(QStringLiteral("WEAPON 2"), this);
    m_primaryButton->setFixedWidth(150);
    m_secondaryButton->setFixedWidth(150);
    weaponTabs->addWidget(m_primaryButton);
    weaponTabs->addWidget(m_secondaryButton);
    weaponTabs->addStretch();
    bodyLayout->addLayout(weaponTabs);

    auto* workspace = new QGridLayout();
    workspace->setContentsMargins(0, 0, 0, 0);
    workspace->setHorizontalSpacing(12);
    workspace->setVerticalSpacing(12);
    workspace->setColumnStretch(0, 4);
    workspace->setColumnStretch(1, 3);
    workspace->setColumnStretch(2, 2);

    auto* inputCard = new CardFrame(this, true);
    auto* inputLayout = new QVBoxLayout(inputCard);
    inputLayout->setContentsMargins(16, 16, 16, 16);
    inputLayout->setSpacing(10);
    inputLayout->addWidget(createSectionLabel(QStringLiteral("INPUT VARIABLES"), inputCard));

    m_xAmount = new NumericStepperRow(
        QStringLiteral("X AMOUNT"),
        -100.0,
        100.0,
        1.0,
        0,
        0.0,
        QString(),
        inputCard
    );
    m_yAmount = new NumericStepperRow(
        QStringLiteral("Y AMOUNT"),
        -100.0,
        100.0,
        1.0,
        0,
        0.0,
        QString(),
        inputCard
    );
    m_timeDelay = new NumericStepperRow(
        QStringLiteral("TIME DELAY"),
        0.001,
        1.0,
        0.001,
        3,
        0.030,
        QStringLiteral("s"),
        inputCard
    );
    inputLayout->addWidget(m_xAmount);
    inputLayout->addWidget(m_yAmount);
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
    m_showOverlay = new ToggleRow(
        QStringLiteral("Show overlay while active"),
        QStringLiteral("Store the overlay preference in the single NEXUS configuration."),
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
    optionsLayout->addWidget(m_showOverlay);
    optionsLayout->addWidget(m_monitorWhileActive);
    auto* regionButton = createAccentButton(QStringLiteral("OPEN SCREEN REGION MONITOR"), optionsCard);
    regionButton->setIcon(NexusTheme::icon(QStringLiteral("target_32.png")));
    optionsLayout->addWidget(regionButton);

    auto* leftColumn = new QWidget(this);
    auto* leftLayout = new QVBoxLayout(leftColumn);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(12);
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

    statsLayout->addSpacing(8);
    statsLayout->addWidget(createSectionLabel(QStringLiteral("RAPID FIRE TARGET"), statsCard));
    m_rapidFireButton = new QPushButton(QStringLiteral("RAPID FIRE: OFF"), statsCard);
    m_rapidFireButton->setCursor(Qt::PointingHandCursor);
    m_rapidFireButton->setMinimumHeight(44);
    m_rapidFireButton->setToolTip(QStringLiteral("Click to cycle: Weapon 1, Weapon 2, both weapons, off."));
    statsLayout->addWidget(m_rapidFireButton);
    m_rapidFireStatusLabel = new QLabel(QStringLiteral("RAPID FIRE DISABLED"), statsCard);
    m_rapidFireStatusLabel->setAlignment(Qt::AlignCenter);
    m_rapidFireStatusLabel->setProperty("rapidFireEnabled", false);
    m_rapidFireStatusLabel->setFont(NexusTheme::font(12, QFont::Bold));
    m_rapidFireStatusLabel->setMinimumHeight(52);
    statsLayout->addWidget(m_rapidFireStatusLabel);
    statsLayout->addStretch();

    workspace->addWidget(leftColumn, 0, 0);
    workspace->addWidget(visualizationCard, 0, 1);
    workspace->addWidget(statsCard, 0, 2);
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

    connect(m_xAmount, &NumericStepperRow::valueChanged, this, [this](double value) {
        setWeaponField(QStringLiteral("x_amount"), value);
    });
    connect(m_yAmount, &NumericStepperRow::valueChanged, this, [this](double value) {
        setWeaponField(QStringLiteral("y_amount"), value);
    });
    connect(m_timeDelay, &NumericStepperRow::valueChanged, this, [this](double value) {
        setWeaponField(QStringLiteral("time_delay"), value);
    });

    connect(m_profileEnabled, &ToggleRow::toggled, this, [this](bool value) {
        setOperatorField(QStringLiteral("profile_enabled"), value);
    });
    connect(m_autoLoad, &ToggleRow::toggled, this, [this](bool value) {
        setOperatorField(QStringLiteral("auto_load"), value);
    });
    connect(m_showOverlay, &ToggleRow::toggled, this, [this](bool value) {
        setOperatorField(QStringLiteral("show_overlay"), value);
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
        settings.insert(m_weaponSlot, defaultWeaponSettings());
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
    settings.insert(QStringLiteral("show_overlay"), m_showOverlay->isChecked());
    settings.insert(QStringLiteral("monitor_while_active"), m_monitorWhileActive->isChecked());
    settings.insert(QStringLiteral("rapid_fire_enabled"), m_rapidFireValue != 0);
    settings.insert(QStringLiteral("rapid_fire_value"), m_rapidFireValue);
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
    const int rapidValue = normalizedRapidFireValue(settings);
    merged.remove(QStringLiteral("rapid_fire_target"));
    merged.insert(QStringLiteral("rapid_fire_value"), rapidValue);
    merged.insert(QStringLiteral("rapid_fire_enabled"), rapidValue != 0);

    for (const QString& slot : {QStringLiteral("primary"), QStringLiteral("secondary")}) {
        QVariantMap weapon = defaultWeaponSettings();
        const auto imported = merged.value(slot).toMap();
        for (auto iterator = imported.constBegin(); iterator != imported.constEnd(); ++iterator) {
            weapon.insert(iterator.key(), iterator.value());
        }
        weapon.insert(
            QStringLiteral("time_delay"),
            normalizeDelaySeconds(weapon.value(QStringLiteral("time_delay"), 0.030).toDouble())
        );
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

QVariantMap OperatorSettingsPage::defaultWeaponSettings() const {
    return {
        {QStringLiteral("x_amount"), 0.0},
        {QStringLiteral("y_amount"), 0.0},
        {QStringLiteral("time_delay"), 0.030},
    };
}

QVariantMap OperatorSettingsPage::defaultSettingsFor(const QString& operatorId) const {
    return {
        {QStringLiteral("operator_id"), operatorId.trimmed().toLower()},
        {QStringLiteral("active_weapon"), QStringLiteral("primary")},
        {QStringLiteral("profile_enabled"), true},
        {QStringLiteral("auto_load"), true},
        {QStringLiteral("show_overlay"), true},
        {QStringLiteral("monitor_while_active"), true},
        {QStringLiteral("rapid_fire_enabled"), false},
        {QStringLiteral("rapid_fire_value"), 0},
        {QStringLiteral("notes"), QString()},
        {QStringLiteral("primary"), defaultWeaponSettings()},
        {QStringLiteral("secondary"), defaultWeaponSettings()},
    };
}

QVariantMap OperatorSettingsPage::activeWeaponSettings() const {
    return {
        {QStringLiteral("x_amount"), m_xAmount->value()},
        {QStringLiteral("y_amount"), m_yAmount->value()},
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
    m_showOverlay->setChecked(settings.value(QStringLiteral("show_overlay"), true).toBool());
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
    const auto weapon = settings.value(m_weaponSlot, defaultWeaponSettings()).toMap();
    const bool previousLoadingState = m_loading;
    m_loading = true;
    m_xAmount->setValue(weapon.value(QStringLiteral("x_amount"), 0.0).toDouble());
    m_yAmount->setValue(weapon.value(QStringLiteral("y_amount"), 0.0).toDouble());
    m_timeDelay->setValue(normalizeDelaySeconds(weapon.value(QStringLiteral("time_delay"), 0.030).toDouble()));
    m_loading = previousLoadingState;
    updateVisualization();
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
    auto weapon = settings.value(m_weaponSlot, defaultWeaponSettings()).toMap();
    weapon.insert(key, value);
    settings.insert(m_weaponSlot, weapon);
    settings.insert(QStringLiteral("active_weapon"), m_weaponSlot);
    m_drafts.insert(m_operatorId, settings);
    updateVisualization();
    updateStatus(QStringLiteral("Unsaved input values are held in the shared NEXUS configuration."));
    Q_EMIT settingChanged(
        m_operatorId,
        m_weaponSlot + QStringLiteral(".") + key,
        value
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
    applyActiveWeaponSettings();
    m_loading = previousLoadingState;
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
    const double delay = qMax(0.001, m_timeDelay->value());
    m_vectorPreview->setValues(x, y, delay);

    const double magnitude = qSqrt((x * x) + (y * y));
    const double angle = qRadiansToDegrees(qAtan2(y, x));
    const double speed = magnitude / delay;
    const QString slope = qFuzzyIsNull(x)
        ? (qFuzzyIsNull(y) ? QStringLiteral("0") : QStringLiteral("∞"))
        : QString::number(y / x, 'f', 2);

    m_slopeLabel->setText(slope);
    m_vectorLabel->setText(
        QStringLiteral("%1, %2")
            .arg(QString::number(x, 'f', 0))
            .arg(QString::number(y, 'f', 0))
    );
    m_angleLabel->setText(QString::number(angle, 'f', 1) + QStringLiteral("°"));
    m_speedLabel->setText(QString::number(speed, 'f', 1));
}

void OperatorSettingsPage::updateStatus(const QString& text, bool success) {
    m_statusLabel->setText(text);
    m_statusLabel->setProperty("success", success);
    m_statusLabel->setProperty("muted", !success);
    m_statusLabel->style()->unpolish(m_statusLabel);
    m_statusLabel->style()->polish(m_statusLabel);
}

int OperatorSettingsPage::normalizedRapidFireValue(const QVariantMap& settings) const {
    const bool hasEnabled = settings.contains(QStringLiteral("rapid_fire_enabled"));
    const bool enabled = settings.value(QStringLiteral("rapid_fire_enabled"), false).toBool();

    int value = settings.value(QStringLiteral("rapid_fire_value"), 0).toInt();
    bool hasValue = settings.contains(QStringLiteral("rapid_fire_value"));

    if (!hasValue && settings.contains(QStringLiteral("rapid_fire_target"))) {
        const QString target = settings.value(QStringLiteral("rapid_fire_target")).toString().trimmed().toLower();
        if (target == QStringLiteral("weapon_1") || target == QStringLiteral("weapon1") || target == QStringLiteral("primary")) {
            value = 1;
        } else if (target == QStringLiteral("weapon_2") || target == QStringLiteral("weapon2") || target == QStringLiteral("secondary")) {
            value = 2;
        } else if (target == QStringLiteral("both") || target == QStringLiteral("both_weapons")) {
            value = 3;
        } else {
            value = 0;
        }
        hasValue = true;
    }

    value = qBound(0, value, 3);
    if (hasEnabled && !enabled) {
        return 0;
    }
    if (value > 0) {
        return value;
    }
    return enabled ? 3 : 0;
}

void OperatorSettingsPage::setRapidFireValue(int value, bool persist) {
    const int normalized = qBound(0, value, 3);
    m_rapidFireValue = normalized;

    if (persist && !m_loading && !m_operatorId.isEmpty()) {
        m_undoSnapshot = m_drafts.value(m_operatorId, defaultSettingsFor(m_operatorId));
        auto settings = m_drafts.value(m_operatorId, defaultSettingsFor(m_operatorId));
        settings.remove(QStringLiteral("rapid_fire_target"));
        settings.insert(QStringLiteral("rapid_fire_value"), normalized);
        settings.insert(QStringLiteral("rapid_fire_enabled"), normalized != 0);
        settings.insert(QStringLiteral("operator_id"), m_operatorId);
        m_drafts.insert(m_operatorId, settings);
        updateStatus(QStringLiteral("Rapid fire target updated inside the shared NEXUS configuration."));
        Q_EMIT settingChanged(m_operatorId, QStringLiteral("rapid_fire_value"), normalized);
        Q_EMIT settingChanged(m_operatorId, QStringLiteral("rapid_fire_enabled"), normalized != 0);
        Q_EMIT rapidFireSelectionChanged(m_operatorId, normalized, normalized != 0);
    }

    updateRapidFireStatus();
}

void OperatorSettingsPage::cycleRapidFireValue() {
    setRapidFireValue((m_rapidFireValue + 1) % 4, true);
}

void OperatorSettingsPage::updateRapidFireStatus() {
    QString buttonText = QStringLiteral("RAPID FIRE: OFF");
    QString statusText = QStringLiteral("RAPID FIRE DISABLED");
    if (m_rapidFireValue == 1) {
        buttonText = QStringLiteral("RAPID FIRE: WEAPON 1");
        statusText = QStringLiteral("RAPID FIRE - WEAPON 1");
    } else if (m_rapidFireValue == 2) {
        buttonText = QStringLiteral("RAPID FIRE: WEAPON 2");
        statusText = QStringLiteral("RAPID FIRE - WEAPON 2");
    } else if (m_rapidFireValue == 3) {
        buttonText = QStringLiteral("RAPID FIRE: BOTH WEAPONS");
        statusText = QStringLiteral("RAPID FIRE - BOTH WEAPONS");
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
