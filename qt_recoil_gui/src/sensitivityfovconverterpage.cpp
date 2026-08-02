#include "sensitivityfovconverterpage.h"

#include "nexuswidgets.h"
#include "theme.h"

#include <QComboBox>
#include <QDoubleValidator>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSlider>
#include <QStyle>
#include <QVBoxLayout>

#include <cmath>

namespace {
QLabel* mutedText(const QString& text, QWidget* parent) {
    auto* label = new QLabel(text, parent);
    label->setProperty("muted", true);
    label->setFont(NexusTheme::font(9));
    label->setWordWrap(true);
    return label;
}

QLabel* fieldLabel(const QString& text, QWidget* parent) {
    auto* label = new QLabel(text, parent);
    label->setFont(NexusTheme::font(9, QFont::DemiBold));
    return label;
}

void configureNumericInput(QLineEdit* input) {
    input->setMinimumHeight(38);
    input->setClearButtonEnabled(true);
    input->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
}

QWidget* createSliderField(
    const QString& title,
    const QString& description,
    QSlider*& slider,
    QLineEdit*& valueBox,
    int minimum,
    int maximum,
    int defaultValue,
    QWidget* parent
) {
    auto* container = new QWidget(parent);
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(7);

    auto* heading = new QHBoxLayout();
    heading->setSpacing(10);
    heading->addWidget(fieldLabel(title, container));
    heading->addStretch();

    valueBox = new QLineEdit(QString::number(defaultValue), container);
    valueBox->setProperty("valueBox", true);
    valueBox->setAlignment(Qt::AlignCenter);
    valueBox->setFixedWidth(72);
    valueBox->setValidator(new QIntValidator(minimum, maximum, valueBox));
    valueBox->setAccessibleName(title + QStringLiteral(" numeric value"));
    heading->addWidget(valueBox);
    layout->addLayout(heading);

    if (!description.isEmpty()) {
        layout->addWidget(mutedText(description, container));
    }

    slider = new QSlider(Qt::Horizontal, container);
    slider->setRange(minimum, maximum);
    slider->setValue(defaultValue);
    slider->setSingleStep(1);
    slider->setPageStep(5);
    slider->setCursor(Qt::PointingHandCursor);
    slider->setAccessibleName(title + QStringLiteral(" slider"));
    slider->setStyleSheet(QStringLiteral(R"QSS(
        QSlider::groove:horizontal {
            height: 6px;
            border-radius: 3px;
            background: #252D40;
        }
        QSlider::sub-page:horizontal {
            border-radius: 3px;
            background: #765BFF;
        }
        QSlider::add-page:horizontal {
            border-radius: 3px;
            background: #252D40;
        }
        QSlider::handle:horizontal {
            width: 18px;
            height: 18px;
            margin: -6px 0;
            border-radius: 9px;
            border: 2px solid #A898FF;
            background: #765BFF;
        }
        QSlider::handle:horizontal:hover {
            border-color: #F7F9FF;
            background: #6848F4;
        }
    )QSS"));
    layout->addWidget(slider);

    return container;
}

CardFrame* createSectionCard(
    const QString& eyebrow,
    const QString& title,
    const QString& description,
    QWidget* parent,
    QVBoxLayout*& contentLayout
) {
    auto* card = new CardFrame(parent, true);
    contentLayout = new QVBoxLayout(card);
    contentLayout->setContentsMargins(18, 18, 18, 18);
    contentLayout->setSpacing(13);

    auto* eyebrowLabel = new QLabel(eyebrow, card);
    eyebrowLabel->setProperty("accent", true);
    eyebrowLabel->setFont(NexusTheme::font(8, QFont::Bold));

    auto* titleLabel = new QLabel(title, card);
    titleLabel->setFont(NexusTheme::font(13, QFont::Bold));

    contentLayout->addWidget(eyebrowLabel);
    contentLayout->addWidget(titleLabel);
    contentLayout->addWidget(mutedText(description, card));

    auto* divider = new QFrame(card);
    divider->setFrameShape(QFrame::HLine);
    divider->setStyleSheet(QStringLiteral("background: #252D40; max-height: 1px; border: none;"));
    contentLayout->addWidget(divider);

    return card;
}

CardFrame* createMetricCard(
    const QString& title,
    const QString& description,
    QLabel*& valueLabel,
    QWidget* parent
) {
    auto* card = new CardFrame(parent, false);
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(6);

    auto* titleLabel = new QLabel(title, card);
    titleLabel->setProperty("muted", true);
    titleLabel->setFont(NexusTheme::font(9, QFont::DemiBold));

    valueLabel = new QLabel(QStringLiteral("—"), card);
    valueLabel->setFont(NexusTheme::font(25, QFont::Bold));
    valueLabel->setProperty("accent", true);
    valueLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    layout->addWidget(titleLabel);
    layout->addWidget(valueLabel);
    layout->addWidget(mutedText(description, card));
    return card;
}
}

SensitivityFovConverterPage::SensitivityFovConverterPage(QWidget* parent)
    : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setFrameShape(QFrame::NoFrame);

    auto* body = new QWidget(scroll);
    auto* bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(
        NexusTheme::ContentPadding,
        NexusTheme::ContentPadding,
        NexusTheme::ContentPadding,
        NexusTheme::ContentPadding
    );
    bodyLayout->setSpacing(16);
    scroll->setWidget(body);
    root->addWidget(scroll);

    bodyLayout->addWidget(new PageHeading(
        QStringLiteral("SENSITIVITY & FOV CONVERTER"),
        QStringLiteral("Normalize base sensitivity, screen geometry, and optics values through the existing NEXUS converter backend."),
        body
    ));

    auto* intro = new CardFrame(body, true);
    auto* introLayout = new QHBoxLayout(intro);
    introLayout->setContentsMargins(20, 18, 22, 18);
    introLayout->setSpacing(16);

    auto* icon = new QLabel(intro);
    icon->setPixmap(NexusTheme::pixmap(QStringLiteral("target_64.png"), 54, 54));
    icon->setFixedSize(58, 58);
    icon->setAlignment(Qt::AlignCenter);

    auto* introText = new QVBoxLayout();
    introText->setSpacing(4);
    auto* introTitle = new QLabel(QStringLiteral("PRECISION CONVERSION WORKSPACE"), intro);
    introTitle->setFont(NexusTheme::font(11, QFont::Bold));
    introTitle->setProperty("accent", true);
    introText->addWidget(introTitle);
    introText->addWidget(mutedText(
        QStringLiteral("Enter the source engine, monitor, and optic values. NEXUS sends one normalized input object to the existing calculation layer and displays the returned X and Y scale factors below."),
        intro
    ));

    introLayout->addWidget(icon, 0, Qt::AlignTop);
    introLayout->addLayout(introText, 1);
    bodyLayout->addWidget(intro);

    auto* inputGrid = new QGridLayout();
    inputGrid->setContentsMargins(0, 0, 0, 0);
    inputGrid->setHorizontalSpacing(14);
    inputGrid->setVerticalSpacing(14);
    inputGrid->setColumnStretch(0, 1);
    inputGrid->setColumnStretch(1, 1);

    QVBoxLayout* baseLayout = nullptr;
    auto* baseCard = createSectionCard(
        QStringLiteral("INPUT GROUP 01"),
        QStringLiteral("Base Engine"),
        QStringLiteral("Define the source sensitivity scale and vertical field of view."),
        body,
        baseLayout
    );

    baseLayout->addWidget(fieldLabel(QStringLiteral("In-Game Sensitivity"), baseCard));
    m_sensitivityInput = new QLineEdit(QStringLiteral("48"), baseCard);
    m_sensitivityInput->setPlaceholderText(QStringLiteral("Enter sensitivity"));
    m_sensitivityInput->setValidator(new QDoubleValidator(0.001, 1000.0, 4, m_sensitivityInput));
    m_sensitivityInput->setAccessibleName(QStringLiteral("In-game sensitivity"));
    configureNumericInput(m_sensitivityInput);
    baseLayout->addWidget(m_sensitivityInput);

    baseLayout->addWidget(fieldLabel(QStringLiteral("Mouse Multiplier"), baseCard));
    m_multiplierInput = new QLineEdit(QStringLiteral("0.002"), baseCard);
    m_multiplierInput->setPlaceholderText(QStringLiteral("0.002"));
    auto* multiplierValidator = new QDoubleValidator(0.000001, 10.0, 6, m_multiplierInput);
    multiplierValidator->setNotation(QDoubleValidator::StandardNotation);
    m_multiplierInput->setValidator(multiplierValidator);
    m_multiplierInput->setAccessibleName(QStringLiteral("Mouse multiplier"));
    configureNumericInput(m_multiplierInput);
    baseLayout->addWidget(m_multiplierInput);

    baseLayout->addWidget(createSliderField(
        QStringLiteral("Vertical FOV"),
        QStringLiteral("Allowed range: 60° to 90°."),
        m_verticalFovSlider,
        m_verticalFovValue,
        60,
        90,
        86,
        baseCard
    ));

    QVBoxLayout* screenLayout = nullptr;
    auto* screenCard = createSectionCard(
        QStringLiteral("INPUT GROUP 02"),
        QStringLiteral("Screen Configuration"),
        QStringLiteral("Describe the rendered aspect ratio and the monitor's native shape."),
        body,
        screenLayout
    );

    screenLayout->addWidget(fieldLabel(QStringLiteral("Aspect Ratio"), screenCard));
    m_aspectRatio = new QComboBox(screenCard);
    m_aspectRatio->addItems({
        QStringLiteral("16:9"),
        QStringLiteral("16:10"),
        QStringLiteral("4:3"),
        QStringLiteral("3:2"),
        QStringLiteral("21:9")
    });
    m_aspectRatio->setAccessibleName(QStringLiteral("Rendered aspect ratio"));
    screenLayout->addWidget(m_aspectRatio);

    screenLayout->addWidget(fieldLabel(QStringLiteral("Native Monitor Ratio"), screenCard));
    m_nativeMonitorRatio = new QComboBox(screenCard);
    m_nativeMonitorRatio->addItems({
        QStringLiteral("16:9"),
        QStringLiteral("21:9")
    });
    m_nativeMonitorRatio->setAccessibleName(QStringLiteral("Native monitor ratio"));
    screenLayout->addWidget(m_nativeMonitorRatio);
    screenLayout->addWidget(mutedText(
        QStringLiteral("The rendered aspect ratio may differ from the physical monitor ratio when using stretched or letterboxed resolutions."),
        screenCard
    ));
    screenLayout->addStretch();

    QVBoxLayout* opticsLayout = nullptr;
    auto* opticsCard = createSectionCard(
        QStringLiteral("INPUT GROUP 03"),
        QStringLiteral("Optics Sliders"),
        QStringLiteral("Set the current ADS values used by the 1x and 2.5x optic groups."),
        body,
        opticsLayout
    );

    opticsLayout->addWidget(createSliderField(
        QStringLiteral("1x Optics ADS"),
        QStringLiteral("Default NEXUS reference value: 58."),
        m_ads1xSlider,
        m_ads1xValue,
        1,
        200,
        58,
        opticsCard
    ));

    opticsLayout->addSpacing(6);
    opticsLayout->addWidget(createSliderField(
        QStringLiteral("2.5x Optics ADS"),
        QStringLiteral("Default NEXUS reference value: 100."),
        m_ads25xSlider,
        m_ads25xValue,
        1,
        200,
        100,
        opticsCard
    ));
    opticsLayout->addStretch();

    inputGrid->addWidget(baseCard, 0, 0);
    inputGrid->addWidget(screenCard, 1, 0);
    inputGrid->addWidget(opticsCard, 0, 1, 2, 1);
    bodyLayout->addLayout(inputGrid);

    auto* outputCard = new CardFrame(body, true);
    auto* outputLayout = new QVBoxLayout(outputCard);
    outputLayout->setContentsMargins(20, 18, 20, 20);
    outputLayout->setSpacing(14);

    auto* outputHeader = new QHBoxLayout();
    auto* outputText = new QVBoxLayout();
    outputText->setSpacing(4);
    auto* outputEyebrow = new QLabel(QStringLiteral("OUTPUT PANEL"), outputCard);
    outputEyebrow->setProperty("accent", true);
    outputEyebrow->setFont(NexusTheme::font(8, QFont::Bold));
    auto* outputTitle = new QLabel(QStringLiteral("Calculated Scale Factors"), outputCard);
    outputTitle->setFont(NexusTheme::font(14, QFont::Bold));
    outputText->addWidget(outputEyebrow);
    outputText->addWidget(outputTitle);
    outputText->addWidget(mutedText(
        QStringLiteral("Results are populated by the application's existing sensitivity conversion backend."),
        outputCard
    ));
    outputHeader->addLayout(outputText, 1);
    outputLayout->addLayout(outputHeader);

    auto* metrics = new QHBoxLayout();
    metrics->setSpacing(12);
    metrics->addWidget(createMetricCard(
        QStringLiteral("Horizontal (X) Scale Factor"),
        QStringLiteral("Applied to horizontal sensitivity scaling."),
        m_horizontalOutput,
        outputCard
    ), 1);
    metrics->addWidget(createMetricCard(
        QStringLiteral("Vertical (Y) Scale Factor"),
        QStringLiteral("Applied to vertical sensitivity scaling."),
        m_verticalOutput,
        outputCard
    ), 1);
    outputLayout->addLayout(metrics);

    m_statusLabel = new QLabel(
        QStringLiteral("Ready. Enter values and calculate."),
        outputCard
    );
    m_statusLabel->setProperty("muted", true);
    m_statusLabel->setFont(NexusTheme::font(9));
    m_statusLabel->setWordWrap(true);
    outputLayout->addWidget(m_statusLabel);

    auto* actions = new QHBoxLayout();
    actions->setSpacing(8);
    auto* resetButton = new QPushButton(QStringLiteral("Reset Defaults"), outputCard);
    resetButton->setCursor(Qt::PointingHandCursor);
    m_calculateButton = createAccentButton(QStringLiteral("Calculate Scale Factors"), outputCard);
    m_calculateButton->setIcon(NexusTheme::icon(QStringLiteral("target_32.png")));
    m_calculateButton->setCursor(Qt::PointingHandCursor);
    actions->addStretch();
    actions->addWidget(resetButton);
    actions->addWidget(m_calculateButton);
    outputLayout->addLayout(actions);

    bodyLayout->addWidget(outputCard);
    bodyLayout->addStretch();

    bindIntegerSlider(m_verticalFovSlider, m_verticalFovValue);
    bindIntegerSlider(m_ads1xSlider, m_ads1xValue);
    bindIntegerSlider(m_ads25xSlider, m_ads25xValue);

    connect(m_calculateButton, &QPushButton::clicked,
            this, &SensitivityFovConverterPage::requestConversion);
    connect(resetButton, &QPushButton::clicked,
            this, &SensitivityFovConverterPage::resetInputs);
}

QVariantMap SensitivityFovConverterPage::currentInputs() const {
    return {
        {QStringLiteral("in_game_sensitivity"), m_sensitivityInput->text().toDouble()},
        {QStringLiteral("mouse_multiplier"), m_multiplierInput->text().toDouble()},
        {QStringLiteral("vertical_fov"), m_verticalFovSlider->value()},
        {QStringLiteral("aspect_ratio"), m_aspectRatio->currentText()},
        {QStringLiteral("native_monitor_ratio"), m_nativeMonitorRatio->currentText()},
        {QStringLiteral("ads_1x"), m_ads1xSlider->value()},
        {QStringLiteral("ads_2_5x"), m_ads25xSlider->value()}
    };
}

void SensitivityFovConverterPage::setScaleFactors(
    double horizontalScale,
    double verticalScale
) {
    if (!std::isfinite(horizontalScale) || !std::isfinite(verticalScale)) {
        setCalculationError(QStringLiteral("The converter backend returned an invalid result."));
        return;
    }

    m_horizontalOutput->setText(QString::number(horizontalScale, 'f', 6));
    m_verticalOutput->setText(QString::number(verticalScale, 'f', 6));
    setStatus(QStringLiteral("Scale factors calculated successfully."), true, false);
}

void SensitivityFovConverterPage::setCalculationError(const QString& message) {
    clearResults();
    setStatus(
        message.trimmed().isEmpty()
            ? QStringLiteral("The conversion could not be completed.")
            : message,
        false,
        true
    );
}

void SensitivityFovConverterPage::clearResults() {
    m_horizontalOutput->setText(QStringLiteral("—"));
    m_verticalOutput->setText(QStringLiteral("—"));
}

void SensitivityFovConverterPage::requestConversion() {
    bool sensitivityOk = false;
    bool multiplierOk = false;
    const double sensitivity = m_sensitivityInput->text().toDouble(&sensitivityOk);
    const double multiplier = m_multiplierInput->text().toDouble(&multiplierOk);

    if (!sensitivityOk || sensitivity <= 0.0) {
        setCalculationError(QStringLiteral("Enter an in-game sensitivity greater than zero."));
        m_sensitivityInput->setFocus();
        return;
    }
    if (!multiplierOk || multiplier <= 0.0) {
        setCalculationError(QStringLiteral("Enter a mouse multiplier greater than zero."));
        m_multiplierInput->setFocus();
        return;
    }

    setStatus(QStringLiteral("Values sent to the NEXUS converter backend…"));
    Q_EMIT conversionRequested(currentInputs());
}

void SensitivityFovConverterPage::resetInputs() {
    m_sensitivityInput->setText(QStringLiteral("48"));
    m_multiplierInput->setText(QStringLiteral("0.002"));
    m_verticalFovSlider->setValue(86);
    m_aspectRatio->setCurrentText(QStringLiteral("16:9"));
    m_nativeMonitorRatio->setCurrentText(QStringLiteral("16:9"));
    m_ads1xSlider->setValue(58);
    m_ads25xSlider->setValue(100);
    clearResults();
    setStatus(QStringLiteral("Defaults restored. Enter values and calculate."));
}

void SensitivityFovConverterPage::setStatus(
    const QString& text,
    bool success,
    bool error
) {
    m_statusLabel->setText(text);
    m_statusLabel->setProperty("muted", !success && !error);
    m_statusLabel->setProperty("success", success);
    m_statusLabel->setStyleSheet(error ? QStringLiteral("color: #FF6B82;") : QString());
    m_statusLabel->style()->unpolish(m_statusLabel);
    m_statusLabel->style()->polish(m_statusLabel);
}

void SensitivityFovConverterPage::bindIntegerSlider(
    QSlider* slider,
    QLineEdit* valueBox
) {
    connect(slider, &QSlider::valueChanged, valueBox, [valueBox](int value) {
        valueBox->setText(QString::number(value));
    });

    connect(valueBox, &QLineEdit::editingFinished, slider, [slider, valueBox]() {
        bool ok = false;
        const int requested = valueBox->text().toInt(&ok);
        if (ok) {
            slider->setValue(qBound(slider->minimum(), requested, slider->maximum()));
        }
        valueBox->setText(QString::number(slider->value()));
    });
}
