#include "nexuswidgets.h"
#include "theme.h"

#include <QHBoxLayout>
#include <QIcon>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QSignalBlocker>
#include <QSize>
#include <QSizePolicy>
#include <QStyle>
#include <QVBoxLayout>
#include <QtMath>

CardFrame::CardFrame(QWidget* parent, bool elevated)
    : QFrame(parent) {
    setProperty("card", elevated ? "elevated" : "true");
    setAttribute(Qt::WA_StyledBackground, true);
}

PageHeading::PageHeading(
    const QString& title,
    const QString& subtitle,
    QWidget* parent
) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    auto* titleLabel = new QLabel(title, this);
    titleLabel->setFont(NexusTheme::font(24, QFont::Bold));
    titleLabel->setWordWrap(true);

    auto* subtitleLabel = new QLabel(subtitle, this);
    subtitleLabel->setProperty("muted", true);
    subtitleLabel->setFont(NexusTheme::font(10));
    subtitleLabel->setWordWrap(true);

    layout->addWidget(titleLabel);
    layout->addWidget(subtitleLabel);
}

SidebarButton::SidebarButton(
    const QString& text,
    const QString& iconName,
    QWidget* parent
) : QPushButton(text, parent) {
    setProperty("navButton", true);
    setProperty("navActive", false);
    setIcon(NexusTheme::icon(iconName + QStringLiteral("_32.png")));
    setIconSize(QSize(18, 18));
    setCursor(Qt::PointingHandCursor);
    setCheckable(false);
}

void SidebarButton::setActive(bool active) {
    setProperty("navActive", active);
    style()->unpolish(this);
    style()->polish(this);
    update();
}

ToggleRow::ToggleRow(
    const QString& title,
    const QString& description,
    bool checked,
    QWidget* parent
) : CardFrame(parent, false) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(12);

    auto* textLayout = new QVBoxLayout();
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(3);

    auto* titleLabel = new QLabel(title, this);
    titleLabel->setFont(NexusTheme::font(10, QFont::DemiBold));
    titleLabel->setWordWrap(true);
    textLayout->addWidget(titleLabel);

    if (!description.isEmpty()) {
        auto* descriptionLabel = new QLabel(description, this);
        descriptionLabel->setProperty("muted", true);
        descriptionLabel->setFont(NexusTheme::font(9));
        descriptionLabel->setWordWrap(true);
        textLayout->addWidget(descriptionLabel);
    }

    m_toggle = new QCheckBox(this);
    m_toggle->setText(QString());
    m_toggle->setChecked(checked);
    m_toggle->setCursor(Qt::PointingHandCursor);

    layout->addLayout(textLayout, 1);
    layout->addWidget(m_toggle, 0, Qt::AlignVCenter);

    connect(m_toggle, &QCheckBox::toggled, this, &ToggleRow::toggled);
}

bool ToggleRow::isChecked() const {
    return m_toggle->isChecked();
}

void ToggleRow::setChecked(bool checked) {
    m_toggle->setChecked(checked);
}

NumericStepperRow::NumericStepperRow(
    const QString& title,
    double minimum,
    double maximum,
    double step,
    int decimals,
    double defaultValue,
    const QString& suffix,
    QWidget* parent
) : CardFrame(parent, false),
    m_minimum(minimum),
    m_maximum(maximum),
    m_step(step),
    m_decimals(decimals),
    m_defaultValue(defaultValue),
    m_value(defaultValue),
    m_suffix(suffix) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(14, 10, 12, 10);
    layout->setSpacing(8);

    auto* label = new QLabel(title, this);
    label->setFont(NexusTheme::font(10, QFont::DemiBold));
    label->setMinimumWidth(110);

    auto createSmallButton = [this](const QString& text, const QString& tooltip) {
        auto* button = new QPushButton(text, this);
        button->setFixedSize(38, 34);
        button->setCursor(Qt::PointingHandCursor);
        button->setToolTip(tooltip);
        button->setProperty("stepperButton", true);
        return button;
    };

    auto* minusButton = createSmallButton(QStringLiteral("−"), QStringLiteral("Decrease value"));
    auto* plusButton = createSmallButton(QStringLiteral("+"), QStringLiteral("Increase value"));
    auto* resetButton = createSmallButton(QStringLiteral("↺"), QStringLiteral("Reset this value"));

    m_valueBox = new QLineEdit(this);
    m_valueBox->setReadOnly(false);
    m_valueBox->setAlignment(Qt::AlignCenter);
    m_valueBox->setMinimumWidth(100);
    m_valueBox->setMaximumWidth(150);
    m_valueBox->setProperty("valueBox", true);
    m_valueBox->setToolTip(QStringLiteral("Type a value, then press Enter or click away."));

    layout->addWidget(label, 1);
    layout->addWidget(minusButton);
    layout->addWidget(m_valueBox);
    layout->addWidget(plusButton);
    layout->addWidget(resetButton);

    connect(minusButton, &QPushButton::clicked, this, [this]() { changeBy(-m_step); });
    connect(plusButton, &QPushButton::clicked, this, [this]() { changeBy(m_step); });
    connect(resetButton, &QPushButton::clicked, this, &NumericStepperRow::resetValue);
    connect(m_valueBox, &QLineEdit::editingFinished, this, &NumericStepperRow::commitText);
    connect(m_valueBox, &QLineEdit::returnPressed, this, &NumericStepperRow::commitText);
    updateDisplay();
}

double NumericStepperRow::value() const {
    return m_value;
}

double NumericStepperRow::defaultValue() const {
    return m_defaultValue;
}

void NumericStepperRow::setValue(double value, bool emitSignal) {
    const double next = bounded(value);
    if (qFuzzyCompare(m_value + 1.0, next + 1.0)) {
        updateDisplay();
        return;
    }
    m_value = next;
    updateDisplay();
    if (emitSignal) {
        Q_EMIT valueChanged(m_value);
    }
}

void NumericStepperRow::setDefaultValue(double defaultValue, bool updateCurrent) {
    m_defaultValue = bounded(defaultValue);
    if (updateCurrent) {
        setValue(m_defaultValue, false);
    }
}

void NumericStepperRow::resetValue() {
    setValue(m_defaultValue, true);
}

void NumericStepperRow::changeBy(double delta) {
    setValue(m_value + delta, true);
}

void NumericStepperRow::commitText() {
    QString text = m_valueBox->text().trimmed();
    if (!m_suffix.isEmpty() && text.endsWith(m_suffix, Qt::CaseInsensitive)) {
        text.chop(m_suffix.size());
        text = text.trimmed();
    }

    bool ok = false;
    const double typedValue = text.toDouble(&ok);
    if (!ok) {
        updateDisplay();
        return;
    }
    setValue(typedValue, true);
}

void NumericStepperRow::updateDisplay() {
    QString text = QString::number(m_value, 'f', m_decimals);
    if (!m_suffix.isEmpty()) {
        text += QStringLiteral(" ") + m_suffix;
    }
    const QSignalBlocker blocker(m_valueBox);
    m_valueBox->setText(text);
}

double NumericStepperRow::bounded(double value) const {
    return qBound(m_minimum, value, m_maximum);
}

OperatorVectorPreview::OperatorVectorPreview(QWidget* parent)
    : QWidget(parent) {
    setMinimumSize(250, 250);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void OperatorVectorPreview::setValues(
    double xAmount,
    double yAmount,
    double timeDelay,
    double horizontalRamp,
    double verticalRamp,
    double rampStartSeconds
) {
    m_xAmount = xAmount;
    m_yAmount = yAmount;
    m_timeDelay = timeDelay;
    m_horizontalRamp = horizontalRamp;
    m_verticalRamp = verticalRamp;
    m_rampStartSeconds = rampStartSeconds;
    update();
}

void OperatorVectorPreview::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF bounds = rect().adjusted(18, 18, -18, -18);
    const QPointF center = bounds.center();
    const double radius = qMin(bounds.width(), bounds.height()) * 0.36;

    painter.setPen(QPen(QColor(QStringLiteral("#252D40")), 1.4));
    painter.setBrush(Qt::NoBrush);
    for (double scale : {1.0, 0.70, 0.40}) {
        painter.drawEllipse(center, radius * scale, radius * scale);
    }
    painter.drawLine(QPointF(center.x() - radius, center.y()), QPointF(center.x() + radius, center.y()));
    painter.drawLine(QPointF(center.x(), center.y() - radius), QPointF(center.x(), center.y() + radius));

    const bool rampActive = qAbs(m_horizontalRamp) > 0.000001 || qAbs(m_verticalRamp) > 0.000001;
    const double rampedX = qAbs(m_horizontalRamp) > 0.000001 ? m_horizontalRamp : m_xAmount;
    const double rampedY = qAbs(m_verticalRamp) > 0.000001 ? m_verticalRamp : m_yAmount;
    const double previewSeconds = qMax(3.0, m_rampStartSeconds + 1.0);
    const double baseSeconds = rampActive
        ? qBound(0.0, m_rampStartSeconds, previewSeconds)
        : previewSeconds;
    const double rampSeconds = rampActive ? qMax(0.0, previewSeconds - baseSeconds) : 0.0;

    const QPointF baseDisplacement(m_xAmount * baseSeconds, -m_yAmount * baseSeconds);
    const QPointF rampedDisplacement(
        baseDisplacement.x() + (rampedX * rampSeconds),
        baseDisplacement.y() - (rampedY * rampSeconds)
    );
    const double largestComponent = qMax(
        10.0,
        qMax(
            qMax(qAbs(baseDisplacement.x()), qAbs(baseDisplacement.y())),
            qMax(qAbs(rampedDisplacement.x()), qAbs(rampedDisplacement.y()))
        )
    );
    const QPointF baseEndpoint(
        center.x() + qBound(-1.0, baseDisplacement.x() / largestComponent, 1.0) * radius,
        center.y() + qBound(-1.0, baseDisplacement.y() / largestComponent, 1.0) * radius
    );
    const QPointF rampedEndpoint(
        center.x() + qBound(-1.0, rampedDisplacement.x() / largestComponent, 1.0) * radius,
        center.y() + qBound(-1.0, rampedDisplacement.y() / largestComponent, 1.0) * radius
    );

    painter.setPen(QPen(QColor(QStringLiteral("#765BFF")), 3.0, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(center, baseEndpoint);
    if (rampActive) {
        painter.setPen(QPen(QColor(QStringLiteral("#4ED49A")), 3.0, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(baseEndpoint, rampedEndpoint);
    }
    painter.setBrush(QColor(QStringLiteral("#A898FF")));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(baseEndpoint, 6.0, 6.0);
    if (rampActive) {
        painter.setBrush(QColor(QStringLiteral("#4ED49A")));
        painter.drawEllipse(rampedEndpoint, 7.0, 7.0);
    }
    painter.setBrush(QColor(QStringLiteral("#4ED49A")));
    painter.drawEllipse(center, 5.0, 5.0);

    painter.setPen(QColor(QStringLiteral("#98A2B7")));
    painter.setFont(NexusTheme::font(8, QFont::DemiBold));
    painter.drawText(
        QRectF(bounds.left(), bounds.bottom() - 24, bounds.width(), 22),
        Qt::AlignCenter,
        QStringLiteral("X %1 Y %2  ->  X %3 Y %4   start %5 s")
            .arg(QString::number(m_xAmount, 'f', 0))
            .arg(QString::number(m_yAmount, 'f', 0))
            .arg(QString::number(rampedX, 'f', 2))
            .arg(QString::number(rampedY, 'f', 2))
            .arg(QString::number(m_rampStartSeconds, 'f', 2))
    );
}

ActionCard::ActionCard(
    const QString& iconName,
    const QString& title,
    const QString& description,
    QWidget* parent
) : CardFrame(parent, true) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(18, 16, 16, 16);
    layout->setSpacing(14);

    auto* iconLabel = new QLabel(this);
    iconLabel->setPixmap(NexusTheme::pixmap(iconName + QStringLiteral("_64.png"), 44, 44));
    iconLabel->setFixedSize(46, 46);
    iconLabel->setAlignment(Qt::AlignCenter);

    auto* textLayout = new QVBoxLayout();
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(4);

    auto* titleLabel = new QLabel(title, this);
    titleLabel->setFont(NexusTheme::font(11, QFont::Bold));
    auto* descriptionLabel = new QLabel(description, this);
    descriptionLabel->setProperty("muted", true);
    descriptionLabel->setFont(NexusTheme::font(9));
    descriptionLabel->setWordWrap(true);

    textLayout->addWidget(titleLabel);
    textLayout->addWidget(descriptionLabel);

    auto* openButton = createAccentButton(QStringLiteral("Open"), this);
    openButton->setFixedWidth(78);

    layout->addWidget(iconLabel);
    layout->addLayout(textLayout, 1);
    layout->addWidget(openButton, 0, Qt::AlignVCenter);

    connect(openButton, &QPushButton::clicked, this, &ActionCard::activated);
}

OperatorTile::OperatorTile(
    const QString& name,
    const QString& iconResource,
    QWidget* parent
) : QToolButton(parent), m_name(name), m_iconResource(iconResource) {
    QIcon operatorIcon(QStringLiteral(":/assets/") + iconResource);
    if (operatorIcon.isNull()) {
        operatorIcon = NexusTheme::icon(QStringLiteral("target_64.png"));
    }

    setText(name.toUpper());
    setIcon(operatorIcon);
    setIconSize(QSize(58, 58));
    setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setMinimumSize(96, 108);
    setCursor(Qt::PointingHandCursor);
    setToolTip(name);
    setAccessibleName(name + QStringLiteral(" operator"));
    setStyleSheet(QStringLiteral(R"QSS(
        QToolButton {
            background: #0E1320;
            border: 1px solid #252D40;
            border-radius: 14px;
            color: #F7F9FF;
            font-size: 9px;
            font-weight: 700;
            padding: 9px 6px 8px 6px;
        }
        QToolButton:hover,
        QToolButton:focus {
            background: #182136;
            border-color: #765BFF;
            color: #A898FF;
        }
        QToolButton:pressed {
            background: #28204D;
            border-color: #A898FF;
        }
    )QSS"));
}

QString OperatorTile::operatorName() const {
    return m_name;
}

QString OperatorTile::iconResource() const {
    return m_iconResource;
}

QScrollArea* createPageScrollArea(QWidget* content, QWidget* parent) {
    auto* scroll = new QScrollArea(parent);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(content);
    return scroll;
}

QLabel* createSectionLabel(const QString& text, QWidget* parent) {
    auto* label = new QLabel(text, parent);
    label->setProperty("muted", true);
    label->setFont(NexusTheme::font(8, QFont::Bold));
    return label;
}

QPushButton* createAccentButton(const QString& text, QWidget* parent) {
    auto* button = new QPushButton(text, parent);
    button->setProperty("accentButton", true);
    button->setCursor(Qt::PointingHandCursor);
    return button;
}
