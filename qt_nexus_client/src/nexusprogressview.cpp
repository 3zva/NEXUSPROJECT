#include "nexusprogressview.h"
#include "theme.h"

#include <QColor>
#include <QEasingCurve>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QVBoxLayout>
#include <algorithm>

NexusProgressView::NexusProgressView(
    NexusProgressMode mode,
    QWidget* parent
)
    : QWidget(parent),
      m_mode(mode) {
    setObjectName(QStringLiteral("nexusProgressView"));
    buildUi();
    applyModeText();
    reset();
}

void NexusProgressView::buildUi() {
    setStyleSheet(QStringLiteral(R"QSS(
        QWidget#nexusProgressView {
            background: #070A12;
        }
        QProgressBar#nexusProgressBar {
            min-height: 16px;
            max-height: 16px;
            border: 1px solid #252D40;
            border-radius: 8px;
            background: #0E1320;
            text-align: center;
            color: transparent;
        }
        QProgressBar#nexusProgressBar::chunk {
            border-radius: 7px;
            background: #765BFF;
        }
        QPushButton#progressActionButton {
            min-height: 42px;
        }
    )QSS"));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(34, 30, 34, 30);
    root->setSpacing(14);

    auto* brandRow = new QHBoxLayout();
    brandRow->setSpacing(12);

    m_logoLabel = new QLabel(this);
    m_logoLabel->setPixmap(
        NexusTheme::pixmap(QStringLiteral("nexus_logo_64.png"), 46, 46)
    );
    m_logoLabel->setFixedSize(46, 46);
    brandRow->addWidget(m_logoLabel, 0, Qt::AlignTop);

    auto* brandText = new QVBoxLayout();
    brandText->setSpacing(1);
    auto* nexus = new QLabel(QStringLiteral("NEXUS"), this);
    nexus->setFont(NexusTheme::font(17, QFont::Bold));
    m_eyebrowLabel = new QLabel(this);
    m_eyebrowLabel->setProperty("accent", true);
    m_eyebrowLabel->setFont(NexusTheme::font(8, QFont::DemiBold));
    brandText->addWidget(nexus);
    brandText->addWidget(m_eyebrowLabel);
    brandRow->addLayout(brandText);
    brandRow->addStretch();

    m_percentLabel = new QLabel(QStringLiteral("0%"), this);
    m_percentLabel->setProperty("accent", true);
    m_percentLabel->setFont(NexusTheme::font(18, QFont::Bold));
    brandRow->addWidget(m_percentLabel, 0, Qt::AlignTop);
    root->addLayout(brandRow);

    root->addSpacing(4);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setFont(NexusTheme::font(22, QFont::Bold));
    m_titleLabel->setWordWrap(true);
    root->addWidget(m_titleLabel);

    m_subtitleLabel = new QLabel(this);
    m_subtitleLabel->setProperty("muted", true);
    m_subtitleLabel->setWordWrap(true);
    m_subtitleLabel->setFont(NexusTheme::font(10));
    root->addWidget(m_subtitleLabel);

    root->addSpacing(8);

    auto* stageRow = new QHBoxLayout();
    m_stageLabel = new QLabel(this);
    m_stageLabel->setProperty("accent", true);
    m_stageLabel->setFont(NexusTheme::font(9, QFont::DemiBold));
    stageRow->addWidget(m_stageLabel);
    stageRow->addStretch();
    root->addLayout(stageRow);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setObjectName(QStringLiteral("nexusProgressBar"));
    m_progressBar->setRange(0, 100);
    m_progressBar->setTextVisible(false);
    root->addWidget(m_progressBar);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setFont(NexusTheme::font(10, QFont::DemiBold));
    m_statusLabel->setWordWrap(true);
    root->addWidget(m_statusLabel);

    m_detailLabel = new QLabel(this);
    m_detailLabel->setProperty("subtle", true);
    m_detailLabel->setFont(NexusTheme::font(9));
    m_detailLabel->setWordWrap(true);
    root->addWidget(m_detailLabel);

    root->addStretch();

    m_actionButton = new QPushButton(this);
    m_actionButton->setObjectName(QStringLiteral("progressActionButton"));
    m_actionButton->setCursor(Qt::PointingHandCursor);
    connect(m_actionButton, &QPushButton::clicked,
            this, &NexusProgressView::actionRequested);
    root->addWidget(m_actionButton);

    m_progressAnimation = new QPropertyAnimation(m_progressBar, "value", this);
    m_progressAnimation->setDuration(360);
    m_progressAnimation->setEasingCurve(QEasingCurve::OutCubic);

    connect(m_progressBar, &QProgressBar::valueChanged,
            this, &NexusProgressView::refreshPercentLabel);
}

void NexusProgressView::setMode(NexusProgressMode mode) {
    m_mode = mode;
    applyModeText();
    reset();
}

void NexusProgressView::applyModeText() {
    if (m_mode == NexusProgressMode::GameLaunch) {
        m_eyebrowLabel->setText(QStringLiteral("LAUNCH READINESS"));
        m_titleLabel->setText(QStringLiteral("Connecting NEXUS to Rainbow Six Siege"));
        m_subtitleLabel->setText(QStringLiteral(
            "NEXUS is already running. This window appears only after the game process is detected."
        ));
        m_actionButton->setText(QStringLiteral("CANCEL"));
    } else {
        m_eyebrowLabel->setText(QStringLiteral("CLIENT UPDATE"));
        m_titleLabel->setText(QStringLiteral("Updating NEXUS"));
        m_subtitleLabel->setText(QStringLiteral(
            "Keep NEXUS open while the update is downloaded, verified, and installed."
        ));
        m_actionButton->setText(QStringLiteral("CANCEL UPDATE"));
    }
}

void NexusProgressView::reset() {
    m_progressAnimation->stop();
    m_progressBar->setValue(0);
    m_stageLabel->setText(m_mode == NexusProgressMode::GameLaunch
        ? QStringLiteral("WAITING")
        : QStringLiteral("PREPARING"));
    m_statusLabel->setText(m_mode == NexusProgressMode::GameLaunch
        ? QStringLiteral("Waiting for Rainbow Six Siege...")
        : QStringLiteral("Preparing update..."));
    m_detailLabel->clear();
    setStatusColor(NexusTheme::Text);
    setActionVisible(false);
    setActionEnabled(true);
}

void NexusProgressView::setTitle(const QString& title) {
    m_titleLabel->setText(title);
}

void NexusProgressView::setSubtitle(const QString& subtitle) {
    m_subtitleLabel->setText(subtitle);
}

void NexusProgressView::setStage(const QString& stage) {
    m_stageLabel->setText(stage.trimmed().toUpper());
}

void NexusProgressView::setStatus(const QString& status) {
    m_statusLabel->setText(status);
    setStatusColor(NexusTheme::Text);
}

void NexusProgressView::setDetail(const QString& detail) {
    m_detailLabel->setText(detail);
}

void NexusProgressView::setProgress(int percent, bool animate) {
    const int bounded = std::clamp(percent, 0, 100);
    if (!animate) {
        m_progressAnimation->stop();
        m_progressBar->setValue(bounded);
        return;
    }

    m_progressAnimation->stop();
    m_progressAnimation->setStartValue(m_progressBar->value());
    m_progressAnimation->setEndValue(bounded);
    m_progressAnimation->start();
}

void NexusProgressView::setComplete(const QString& message) {
    setStage(QStringLiteral("READY"));
    setProgress(100, true);
    m_statusLabel->setText(message);
    setStatusColor(NexusTheme::Success);
}

void NexusProgressView::setError(const QString& message) {
    m_statusLabel->setText(message);
    setStatusColor(NexusTheme::Danger);
}

void NexusProgressView::setActionVisible(bool visible) {
    m_actionButton->setVisible(visible);
}

void NexusProgressView::setActionText(const QString& text) {
    m_actionButton->setText(text);
}

void NexusProgressView::setActionEnabled(bool enabled) {
    m_actionButton->setEnabled(enabled);
}

int NexusProgressView::progress() const {
    return m_progressBar->value();
}

NexusProgressMode NexusProgressView::mode() const {
    return m_mode;
}

void NexusProgressView::refreshPercentLabel(int value) {
    m_percentLabel->setText(QString::number(value) + QStringLiteral("%"));
}

void NexusProgressView::setStatusColor(const QColor& color) {
    m_statusLabel->setStyleSheet(
        QStringLiteral("color: %1;").arg(color.name())
    );
}
