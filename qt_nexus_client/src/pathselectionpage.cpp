#include "pathselectionpage.h"
#include "nexusprogressview.h"
#include "nexuswidgets.h"
#include "theme.h"

#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedLayout>
#include <QVBoxLayout>

PathSelectionPage::PathSelectionPage(QWidget* parent)
    : QWidget(parent) {
    m_stack = new QStackedLayout(this);
    m_stack->setContentsMargins(0, 0, 0, 0);

    auto* content = new QWidget(this);
    auto* root = new QVBoxLayout(content);
    root->setContentsMargins(NexusTheme::ContentPadding, NexusTheme::ContentPadding,
                            NexusTheme::ContentPadding, NexusTheme::ContentPadding);
    root->setSpacing(18);

    root->addWidget(new PageHeading(
        QStringLiteral("SELECT INSTALLATION PATH"),
        QStringLiteral("Choose the directory NEXUS should use before loading the client."),
        this
    ));

    auto* card = new CardFrame(this, true);
    card->setMaximumWidth(760);
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(24, 24, 24, 24);
    cardLayout->setSpacing(12);

    auto* icon = new QLabel(card);
    icon->setPixmap(NexusTheme::pixmap(QStringLiteral("folder_64.png"), 64, 64));
    cardLayout->addWidget(icon, 0, Qt::AlignLeft);

    auto* title = new QLabel(QStringLiteral("Select a path"), card);
    title->setFont(NexusTheme::font(18, QFont::Bold));
    cardLayout->addWidget(title);

    auto* body = new QLabel(
        QStringLiteral("Choose the target directory. The LOAD button becomes available once a valid path is selected."),
        card
    );
    body->setProperty("muted", true);
    body->setWordWrap(true);
    cardLayout->addWidget(body);

    auto* pathRow = new QHBoxLayout();
    pathRow->setSpacing(10);
    m_pathEdit = new QLineEdit(card);
    m_pathEdit->setPlaceholderText(QStringLiteral("Select an installation directory..."));
    auto* browse = new QPushButton(QStringLiteral("Browse"), card);
    browse->setIcon(NexusTheme::icon(QStringLiteral("folder_32.png")));
    browse->setCursor(Qt::PointingHandCursor);
    pathRow->addWidget(m_pathEdit, 1);
    pathRow->addWidget(browse);
    cardLayout->addLayout(pathRow);

    m_status = new QLabel(card);
    m_status->setProperty("muted", true);
    m_status->setWordWrap(true);
    cardLayout->addWidget(m_status);

    m_loadButton = createAccentButton(QStringLiteral("LOAD"), card);
    m_loadButton->setMinimumHeight(46);
    m_loadButton->setEnabled(false);
    cardLayout->addWidget(m_loadButton);

    root->addWidget(card, 0, Qt::AlignHCenter);
    root->addStretch();

    m_loadingView = new NexusProgressView(NexusProgressMode::GameLaunch, this);
    m_loadingView->setActionVisible(false);
    m_stack->addWidget(content);
    m_stack->addWidget(m_loadingView);
    m_stack->setCurrentWidget(content);

    connect(browse, &QPushButton::clicked, this, [this]() {
        const auto initial = m_pathEdit->text().isEmpty()
            ? QDir::homePath()
            : m_pathEdit->text();
        const auto selected = QFileDialog::getExistingDirectory(
            this,
            QStringLiteral("Select NEXUS installation path"),
            initial,
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
        );
        if (!selected.isEmpty()) {
            setSelectedPath(selected);
        }
    });

    connect(m_pathEdit, &QLineEdit::textChanged, this, [this](const QString& path) {
        m_loadButton->setEnabled(!path.trimmed().isEmpty());
        showStatus(QString());
    });

    connect(m_loadButton, &QPushButton::clicked, this, [this]() {
        const auto path = selectedPath();
        if (path.isEmpty()) {
            showStatus(QStringLiteral("Select an installation path first."), true);
            return;
        }
        Q_EMIT loadRequested(path);
    });
}

QString PathSelectionPage::selectedPath() const {
    return m_pathEdit->text().trimmed();
}

void PathSelectionPage::setSelectedPath(const QString& path) {
    m_pathEdit->setText(QDir::toNativeSeparators(path));
}

void PathSelectionPage::showStatus(const QString& message, bool error) {
    m_status->setText(message);
    m_status->setStyleSheet(error
        ? QStringLiteral("color: #FF6B82;")
        : QStringLiteral("color: #98A2B7;"));
}

void PathSelectionPage::beginLoading() {
    m_pathEdit->setEnabled(false);
    m_loadButton->setEnabled(false);
    m_loadingView->reset();
    m_loadingView->setTitle(QStringLiteral("Loading NEXUS"));
    m_loadingView->setSubtitle(QStringLiteral("Keep this window open while NEXUS prepares the client and waits for Rainbow Six Siege."));
    m_loadingView->setStage(QStringLiteral("STARTING"));
    m_loadingView->setStatus(QStringLiteral("Preparing client services..."));
    m_loadingView->setDetail(QStringLiteral("The application will open after readiness is complete."));
    m_loadingView->setProgress(12, false);
    m_loadingView->setActionVisible(false);
    m_stack->setCurrentWidget(m_loadingView);
}

void PathSelectionPage::setLoadingWaiting() {
    m_loadingView->setStage(QStringLiteral("WAITING"));
    m_loadingView->setStatus(QStringLiteral("Waiting for Rainbow Six Siege..."));
    m_loadingView->setDetail(QStringLiteral("NEXUS is ready and watching for the game process."));
    m_loadingView->setProgress(35, true);
}

void PathSelectionPage::setLoadingGameDetected(qint64 pid, const QString& executableName) {
    m_loadingView->setStage(QStringLiteral("GAME DETECTED"));
    m_loadingView->setStatus(QStringLiteral("Rainbow Six Siege detected."));
    m_loadingView->setDetail(QStringLiteral("%1 · PID %2").arg(executableName).arg(pid));
    m_loadingView->setProgress(72, true);
}

void PathSelectionPage::setLoadingClientReady() {
    m_loadingView->setStage(QStringLiteral("FINALIZING"));
    m_loadingView->setStatus(QStringLiteral("NEXUS client is ready. Finalizing launch..."));
    m_loadingView->setDetail(QStringLiteral("Applying saved settings and runtime configuration."));
    m_loadingView->setProgress(90, true);
}

void PathSelectionPage::finishLoading() {
    m_loadingView->setComplete(QStringLiteral("NEXUS and Rainbow Six Siege are ready."));
}

void PathSelectionPage::failLoading(const QString& message) {
    m_pathEdit->setEnabled(true);
    m_loadButton->setEnabled(!selectedPath().isEmpty());
    m_loadingView->setStage(QStringLiteral("LOAD FAILED"));
    m_loadingView->setError(message.isEmpty()
        ? QStringLiteral("NEXUS could not complete the load process.")
        : message);
    m_loadingView->setDetail(QStringLiteral("Return to the load page and try again."));
    m_loadingView->setActionVisible(false);
}

void PathSelectionPage::resetLoading() {
    m_pathEdit->setEnabled(true);
    m_loadButton->setEnabled(!selectedPath().isEmpty());
    m_loadingView->reset();
    m_stack->setCurrentIndex(0);
}
