#include "pathselectionpage.h"
#include "nexuswidgets.h"
#include "theme.h"

#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

PathSelectionPage::PathSelectionPage(QWidget* parent)
    : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
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
