#include "authpages.h"
#include "nexuswidgets.h"
#include "theme.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSpacerItem>
#include <QVBoxLayout>

namespace {
QLabel* makeTitle(const QString& text, QWidget* parent) {
    auto* label = new QLabel(text, parent);
    label->setFont(NexusTheme::font(26, QFont::Bold));
    label->setWordWrap(true);
    return label;
}

QLabel* makeSubtitle(const QString& text, QWidget* parent) {
    auto* label = new QLabel(text, parent);
    label->setProperty("muted", true);
    label->setFont(NexusTheme::font(10));
    label->setWordWrap(true);
    return label;
}

QLabel* makeStatus(QWidget* parent) {
    auto* label = new QLabel(parent);
    label->setProperty("muted", true);
    label->setFont(NexusTheme::font(9));
    label->setWordWrap(true);
    label->setMinimumHeight(22);
    return label;
}

QLabel* makeFieldLabel(const QString& text, QWidget* parent) {
    auto* label = new QLabel(text, parent);
    label->setFont(NexusTheme::font(9, QFont::DemiBold));
    return label;
}

void setStatusLabel(QLabel* label, const QString& message, bool error = false) {
    label->setText(message);
    label->setStyleSheet(error
        ? QStringLiteral("color: #FF6B82;")
        : QStringLiteral("color: #98A2B7;"));
}

void setErrorLabel(QLabel* label, const QString& message) {
    setStatusLabel(label, message, !message.isEmpty());
}
}

AuthFlowWidget::AuthFlowWidget(QWidget* parent)
    : QWidget(parent) {
    setObjectName(QStringLiteral("appRoot"));
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_stack = new QStackedWidget(this);
    m_signInPage = buildSignInPage();
    m_createPage = buildCreatePage();
    m_verifyPage = buildVerifyPage();
    m_createdPage = buildCreatedPage();

    m_stack->addWidget(m_signInPage);
    m_stack->addWidget(m_createPage);
    m_stack->addWidget(m_verifyPage);
    m_stack->addWidget(m_createdPage);
    layout->addWidget(m_stack);

    showSignIn();
}

QWidget* AuthFlowWidget::buildBrandPanel(
    const QString& eyebrow,
    const QString& headline,
    const QString& body
) {
    auto* panel = new QFrame(this);
    panel->setObjectName(QStringLiteral("authBrandPanel"));
    panel->setStyleSheet(QStringLiteral(
        "QFrame#authBrandPanel { background: #0E1320; border-right: 1px solid #252D40; }"
    ));
    panel->setMinimumWidth(420);

    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(58, 34, 58, 54);
    layout->setSpacing(0);

    auto* brandRow = new QHBoxLayout();
    brandRow->setSpacing(12);
    auto* logo = new QLabel(panel);
    logo->setPixmap(NexusTheme::pixmap(QStringLiteral("nexus_logo_64.png"), 54, 54));
    auto* brand = new QLabel(QStringLiteral("NEXUS"), panel);
    brand->setFont(NexusTheme::font(16, QFont::Bold));
    brandRow->addWidget(logo);
    brandRow->addWidget(brand);
    brandRow->addStretch();
    layout->addLayout(brandRow);

    layout->addStretch(2);

    auto* eyebrowLabel = new QLabel(eyebrow.toUpper(), panel);
    eyebrowLabel->setProperty("accent", true);
    eyebrowLabel->setFont(NexusTheme::font(10, QFont::Bold));

    auto* headlineLabel = new QLabel(headline, panel);
    headlineLabel->setFont(NexusTheme::font(30, QFont::Bold));
    headlineLabel->setWordWrap(true);

    auto* bodyLabel = makeSubtitle(body, panel);
    bodyLabel->setFont(NexusTheme::font(13));

    layout->addWidget(eyebrowLabel);
    layout->addSpacing(30);
    layout->addWidget(headlineLabel);
    layout->addSpacing(22);
    layout->addWidget(bodyLabel);
    layout->addStretch(3);

    auto* step = new QLabel(QStringLiteral("●   SECURE NEXUS ACCESS"), panel);
    step->setStyleSheet(QStringLiteral("color: #4ED49A;"));
    step->setFont(NexusTheme::font(10, QFont::DemiBold));
    layout->addWidget(step);

    return panel;
}

QWidget* AuthFlowWidget::wrapRightPanel(QWidget* form) {
    auto* outer = new QFrame(this);
    outer->setStyleSheet(QStringLiteral("background: #070A12;"));

    auto* outerLayout = new QVBoxLayout(outer);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    auto* scroll = new QScrollArea(outer);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setFrameShape(QFrame::NoFrame);

    auto* scrollContent = new QWidget(scroll);
    auto* centering = new QHBoxLayout(scrollContent);
    centering->setContentsMargins(46, 34, 46, 34);
    centering->addStretch();
    form->setMaximumWidth(560);
    form->setMinimumWidth(330);
    centering->addWidget(form, 1, Qt::AlignVCenter);
    centering->addStretch();

    scroll->setWidget(scrollContent);
    outerLayout->addWidget(scroll);
    return outer;
}

QWidget* AuthFlowWidget::buildSignInPage() {
    auto* page = new QWidget(this);
    auto* layout = new QHBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(buildBrandPanel(
        QStringLiteral("NEXUS ACCESS"),
        QStringLiteral("Welcome back.\nYour setup is waiting."),
        QStringLiteral("Sign in to restore your configuration and continue directly into installation.")
    ), 1);

    auto* form = new QWidget(page);
    auto* formLayout = new QVBoxLayout(form);
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setSpacing(10);

    auto* createLink = new QPushButton(QStringLiteral("Create an account  →"), form);
    createLink->setFlat(true);
    createLink->setStyleSheet(QStringLiteral(
        "QPushButton { color: #A898FF; border: none; background: transparent; text-align: left; padding: 0; }"
        "QPushButton:hover { color: #F7F9FF; }"
    ));
    connect(createLink, &QPushButton::clicked, this, &AuthFlowWidget::showCreateAccount);

    formLayout->addWidget(new QLabel(QStringLiteral("ACCOUNT ACCESS"), form));
    formLayout->addWidget(makeTitle(QStringLiteral("Sign in to NEXUS"), form));
    formLayout->addWidget(makeSubtitle(
        QStringLiteral("Enter the email and password connected to your NEXUS account."),
        form
    ));
    formLayout->addSpacing(10);
    formLayout->addWidget(makeFieldLabel(QStringLiteral("Email address"), form));
    m_signInEmail = new QLineEdit(form);
    m_signInEmail->setPlaceholderText(QStringLiteral("name@example.com"));
    formLayout->addWidget(m_signInEmail);
    formLayout->addWidget(makeFieldLabel(QStringLiteral("Password"), form));
    m_signInPassword = new QLineEdit(form);
    m_signInPassword->setEchoMode(QLineEdit::Password);
    m_signInPassword->setPlaceholderText(QStringLiteral("Your password"));
    formLayout->addWidget(m_signInPassword);

    auto* forgot = new QPushButton(QStringLiteral("Forgot password?"), form);
    forgot->setFlat(true);
    forgot->setStyleSheet(QStringLiteral(
        "QPushButton { color: #A898FF; border: none; background: transparent; text-align: right; padding: 0; }"
    ));
    connect(forgot, &QPushButton::clicked, this, [this]() {
        const auto email = m_signInEmail->text().trimmed();
        if (email.isEmpty()) {
            setErrorLabel(m_signInStatus, QStringLiteral("Enter your email address first."));
            return;
        }
        Q_EMIT passwordResetRequested(email);
    });
    formLayout->addWidget(forgot, 0, Qt::AlignRight);

    m_signInStatus = makeStatus(form);
    formLayout->addWidget(m_signInStatus);

    m_signInButton = createAccentButton(QStringLiteral("SIGN IN"), form);
    m_signInButton->setMinimumHeight(46);
    connect(m_signInButton, &QPushButton::clicked, this, [this]() {
        clearError();
        const auto email = m_signInEmail->text().trimmed();
        const auto password = m_signInPassword->text();
        if (email.isEmpty() || password.isEmpty()) {
            setErrorLabel(m_signInStatus, QStringLiteral("Enter both your email and password."));
            return;
        }
        Q_EMIT signInRequested(email, password);
    });
    connect(m_signInPassword, &QLineEdit::returnPressed, m_signInButton, &QPushButton::click);

    formLayout->addWidget(m_signInButton);
    formLayout->addSpacing(10);
    formLayout->addWidget(createLink);
    formLayout->addStretch();

    layout->addWidget(wrapRightPanel(form), 1);
    return page;
}

QWidget* AuthFlowWidget::buildCreatePage() {
    auto* page = new QWidget(this);
    auto* layout = new QHBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(buildBrandPanel(
        QStringLiteral("CREATE YOUR ACCESS"),
        QStringLiteral("One account.\nYour entire setup."),
        QStringLiteral("Create a secure account, verify your email, and continue directly into installation.")
    ), 1);

    auto* form = new QWidget(page);
    auto* formLayout = new QVBoxLayout(form);
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setSpacing(9);

    auto* back = new QPushButton(QStringLiteral("‹  Back to sign in"), form);
    back->setFlat(true);
    back->setStyleSheet(QStringLiteral(
        "QPushButton { color: #A898FF; border: none; background: transparent; text-align: left; padding: 0; }"
    ));
    connect(back, &QPushButton::clicked, this, [this]() { showSignIn(); });

    formLayout->addWidget(back, 0, Qt::AlignLeft);
    formLayout->addSpacing(4);
    auto* eyebrow = new QLabel(QStringLiteral("NEW ACCOUNT"), form);
    eyebrow->setProperty("accent", true);
    eyebrow->setFont(NexusTheme::font(9, QFont::Bold));
    formLayout->addWidget(eyebrow);
    formLayout->addWidget(makeTitle(QStringLiteral("Create your account"), form));
    formLayout->addWidget(makeSubtitle(
        QStringLiteral("Use accurate information so account recovery works correctly."),
        form
    ));
    formLayout->addSpacing(8);

    auto* namesGrid = new QGridLayout();
    namesGrid->setContentsMargins(0, 0, 0, 0);
    namesGrid->setHorizontalSpacing(12);
    namesGrid->setVerticalSpacing(6);
    namesGrid->addWidget(makeFieldLabel(QStringLiteral("Full name"), form), 0, 0);
    namesGrid->addWidget(makeFieldLabel(QStringLiteral("Username"), form), 0, 1);
    m_fullName = new QLineEdit(form);
    m_fullName->setPlaceholderText(QStringLiteral("Your name"));
    m_username = new QLineEdit(form);
    m_username->setPlaceholderText(QStringLiteral("Username"));
    namesGrid->addWidget(m_fullName, 1, 0);
    namesGrid->addWidget(m_username, 1, 1);
    namesGrid->setColumnStretch(0, 1);
    namesGrid->setColumnStretch(1, 1);
    formLayout->addLayout(namesGrid);

    formLayout->addWidget(makeFieldLabel(QStringLiteral("Email address"), form));
    m_createEmail = new QLineEdit(form);
    m_createEmail->setPlaceholderText(QStringLiteral("name@example.com"));
    formLayout->addWidget(m_createEmail);

    auto* passwordsGrid = new QGridLayout();
    passwordsGrid->setContentsMargins(0, 0, 0, 0);
    passwordsGrid->setHorizontalSpacing(12);
    passwordsGrid->setVerticalSpacing(6);
    passwordsGrid->addWidget(makeFieldLabel(QStringLiteral("Password"), form), 0, 0);
    passwordsGrid->addWidget(makeFieldLabel(QStringLiteral("Confirm password"), form), 0, 1);
    m_createPassword = new QLineEdit(form);
    m_createPassword->setEchoMode(QLineEdit::Password);
    m_confirmPassword = new QLineEdit(form);
    m_confirmPassword->setEchoMode(QLineEdit::Password);
    passwordsGrid->addWidget(m_createPassword, 1, 0);
    passwordsGrid->addWidget(m_confirmPassword, 1, 1);
    passwordsGrid->setColumnStretch(0, 1);
    passwordsGrid->setColumnStretch(1, 1);
    formLayout->addLayout(passwordsGrid);

    auto* requirements = new CardFrame(form, false);
    auto* requirementsLayout = new QGridLayout(requirements);
    requirementsLayout->setContentsMargins(14, 11, 14, 11);
    requirementsLayout->setHorizontalSpacing(18);
    requirementsLayout->setVerticalSpacing(6);
    const QStringList items{
        QStringLiteral("○ 8+ characters"),
        QStringLiteral("○ Uppercase letter"),
        QStringLiteral("○ Lowercase letter"),
        QStringLiteral("○ Number"),
    };
    for (int index = 0; index < items.size(); ++index) {
        auto* label = new QLabel(items[index], requirements);
        label->setProperty("muted", true);
        label->setFont(NexusTheme::font(9));
        requirementsLayout->addWidget(label, index / 2, index % 2);
    }
    formLayout->addWidget(requirements);

    m_terms = new QCheckBox(
        QStringLiteral("I agree to the Terms and Privacy Policy."),
        form
    );
    formLayout->addWidget(m_terms);

    auto* links = new QLabel(
        QStringLiteral("<a style='color:#A898FF' href='terms'>Terms</a>"
                       "   ·   "
                       "<a style='color:#A898FF' href='privacy'>Privacy Policy</a>"),
        form
    );
    links->setOpenExternalLinks(false);
    formLayout->addWidget(links);

    m_createStatus = makeStatus(form);
    formLayout->addWidget(m_createStatus);

    m_createButton = createAccentButton(QStringLiteral("Continue"), form);
    m_createButton->setMinimumHeight(46);
    connect(m_createButton, &QPushButton::clicked, this, &AuthFlowWidget::validateAndSubmitCreate);
    connect(m_confirmPassword, &QLineEdit::returnPressed, m_createButton, &QPushButton::click);
    formLayout->addWidget(m_createButton);
    formLayout->addStretch();

    layout->addWidget(wrapRightPanel(form), 1);
    return page;
}

QWidget* AuthFlowWidget::buildVerifyPage() {
    auto* page = new QWidget(this);
    auto* layout = new QHBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(buildBrandPanel(
        QStringLiteral("VERIFY YOUR ACCOUNT"),
        QStringLiteral("One quick check.\nThen you are in."),
        QStringLiteral("Firebase sends a secure verification link to your email address.")
    ), 1);

    auto* form = new QWidget(page);
    auto* formLayout = new QVBoxLayout(form);
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setSpacing(12);

    auto* back = new QPushButton(QStringLiteral("‹  Change account details"), form);
    back->setFlat(true);
    back->setStyleSheet(QStringLiteral(
        "QPushButton { color: #A898FF; border: none; background: transparent; text-align: left; padding: 0; }"
    ));
    connect(back, &QPushButton::clicked, this, &AuthFlowWidget::showCreateAccount);

    formLayout->addWidget(back, 0, Qt::AlignLeft);
    formLayout->addWidget(makeTitle(QStringLiteral("Check your email"), form));
    formLayout->addWidget(makeSubtitle(
        QStringLiteral("Click the Firebase verification link, return to NEXUS, then confirm below."),
        form
    ));

    m_verifyEmailLabel = new QLabel(form);
    m_verifyEmailLabel->setProperty("accent", true);
    m_verifyEmailLabel->setFont(NexusTheme::font(11, QFont::DemiBold));
    m_verifyEmailLabel->setWordWrap(true);
    formLayout->addWidget(m_verifyEmailLabel);

    auto* note = new CardFrame(form, true);
    auto* noteLayout = new QVBoxLayout(note);
    noteLayout->setContentsMargins(16, 14, 16, 14);
    auto* noteTitle = new QLabel(QStringLiteral("VERIFICATION LINK SENT"), note);
    noteTitle->setFont(NexusTheme::font(10, QFont::Bold));
    auto* noteBody = makeSubtitle(
        QStringLiteral("The normal Firebase email verification flow uses a link, not a six-digit code."),
        note
    );
    noteLayout->addWidget(noteTitle);
    noteLayout->addWidget(noteBody);
    formLayout->addWidget(note);

    m_verifyStatus = makeStatus(form);
    formLayout->addWidget(m_verifyStatus);

    m_verifyButton = createAccentButton(QStringLiteral("I'VE VERIFIED MY EMAIL"), form);
    m_verifyButton->setMinimumHeight(46);
    connect(m_verifyButton, &QPushButton::clicked, this, &AuthFlowWidget::verificationCheckRequested);
    formLayout->addWidget(m_verifyButton);

    m_resendButton = new QPushButton(QStringLiteral("Resend verification link"), form);
    connect(m_resendButton, &QPushButton::clicked, this, &AuthFlowWidget::resendVerificationRequested);
    formLayout->addWidget(m_resendButton);
    formLayout->addStretch();

    layout->addWidget(wrapRightPanel(form), 1);
    return page;
}

QWidget* AuthFlowWidget::buildCreatedPage() {
    auto* page = new QWidget(this);
    auto* layout = new QHBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(buildBrandPanel(
        QStringLiteral("ACCOUNT READY"),
        QStringLiteral("NEXUS is ready.\nContinue your setup."),
        QStringLiteral("Your account has been created and the email address has been verified.")
    ), 1);

    auto* form = new QWidget(page);
    auto* formLayout = new QVBoxLayout(form);
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setSpacing(14);

    auto* logo = new QLabel(form);
    logo->setPixmap(NexusTheme::pixmap(QStringLiteral("check_64.png"), 72, 72));
    formLayout->addWidget(logo, 0, Qt::AlignLeft);
    formLayout->addWidget(makeTitle(QStringLiteral("Account created"), form));
    formLayout->addWidget(makeSubtitle(
        QStringLiteral("You can now sign in and continue to the installation path screen."),
        form
    ));
    m_createdEmailLabel = new QLabel(form);
    m_createdEmailLabel->setProperty("accent", true);
    m_createdEmailLabel->setFont(NexusTheme::font(11, QFont::DemiBold));
    formLayout->addWidget(m_createdEmailLabel);

    auto* continueButton = createAccentButton(QStringLiteral("CONTINUE TO SIGN IN"), form);
    continueButton->setMinimumHeight(46);
    connect(continueButton, &QPushButton::clicked, this, [this]() {
        showSignIn(m_verificationEmail);
    });
    formLayout->addWidget(continueButton);
    formLayout->addStretch();

    layout->addWidget(wrapRightPanel(form), 1);
    return page;
}

void AuthFlowWidget::showSignIn(const QString& prefillEmail) {
    clearError();
    if (!prefillEmail.isEmpty()) {
        m_signInEmail->setText(prefillEmail);
    }
    m_signInPassword->clear();
    m_stack->setCurrentWidget(m_signInPage);
}

void AuthFlowWidget::showCreateAccount() {
    clearError();
    m_stack->setCurrentWidget(m_createPage);
}

void AuthFlowWidget::showVerifyEmail(const QString& email) {
    clearError();
    m_verificationEmail = email;
    m_verifyEmailLabel->setText(email);
    m_stack->setCurrentWidget(m_verifyPage);
}

void AuthFlowWidget::showAccountCreated(const QString& email) {
    clearError();
    m_verificationEmail = email;
    m_createdEmailLabel->setText(email);
    m_stack->setCurrentWidget(m_createdPage);
}

void AuthFlowWidget::setBusy(bool busy, const QString& status) {
    m_signInButton->setDisabled(busy);
    m_createButton->setDisabled(busy);
    m_verifyButton->setDisabled(busy);
    m_resendButton->setDisabled(busy);

    if (!status.isEmpty()) {
        if (m_stack->currentWidget() == m_signInPage) {
            setStatusLabel(m_signInStatus, status);
        } else if (m_stack->currentWidget() == m_createPage) {
            setStatusLabel(m_createStatus, status);
        } else if (m_stack->currentWidget() == m_verifyPage) {
            setStatusLabel(m_verifyStatus, status);
        }
    }
}

void AuthFlowWidget::showError(const QString& message) {
    setBusy(false);
    if (m_stack->currentWidget() == m_signInPage) {
        setErrorLabel(m_signInStatus, message);
    } else if (m_stack->currentWidget() == m_createPage) {
        setErrorLabel(m_createStatus, message);
    } else if (m_stack->currentWidget() == m_verifyPage) {
        setErrorLabel(m_verifyStatus, message);
    }
}

void AuthFlowWidget::clearError() {
    setErrorLabel(m_signInStatus, QString());
    setErrorLabel(m_createStatus, QString());
    setErrorLabel(m_verifyStatus, QString());
}

void AuthFlowWidget::setDemoMode(bool enabled) {
    m_demoMode = enabled;
    if (enabled) {
        setStatusLabel(
            m_signInStatus,
            QStringLiteral("Demo mode: use demo@nexus.local with password demo.")
        );
    }
}

QString AuthFlowWidget::verificationEmail() const {
    return m_verificationEmail;
}

void AuthFlowWidget::validateAndSubmitCreate() {
    clearError();
    const auto fullName = m_fullName->text().trimmed();
    const auto username = m_username->text().trimmed();
    const auto email = m_createEmail->text().trimmed();
    const auto password = m_createPassword->text();
    const auto confirmation = m_confirmPassword->text();

    if (fullName.isEmpty() || username.isEmpty() || email.isEmpty()
        || password.isEmpty() || confirmation.isEmpty()) {
        setErrorLabel(m_createStatus, QStringLiteral("Complete every account field."));
        return;
    }
    if (!email.contains(QLatin1Char('@'))) {
        setErrorLabel(m_createStatus, QStringLiteral("Enter a valid email address."));
        return;
    }
    if (password != confirmation) {
        setErrorLabel(m_createStatus, QStringLiteral("The passwords do not match."));
        return;
    }
    if (password.size() < 8
        || !password.contains(QRegularExpression(QStringLiteral("[A-Z]")))
        || !password.contains(QRegularExpression(QStringLiteral("[a-z]")))
        || !password.contains(QRegularExpression(QStringLiteral("[0-9]")))) {
        setErrorLabel(
            m_createStatus,
            QStringLiteral("Use 8+ characters with uppercase, lowercase, and a number.")
        );
        return;
    }
    if (!m_terms->isChecked()) {
        setErrorLabel(m_createStatus, QStringLiteral("Accept the Terms and Privacy Policy to continue."));
        return;
    }

    Q_EMIT createAccountRequested(fullName, username, email, password);
}
