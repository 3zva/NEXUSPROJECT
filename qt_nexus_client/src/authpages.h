#pragma once

#include <QStackedWidget>
#include <QString>
#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class QCheckBox;

class AuthFlowWidget final : public QWidget {
    Q_OBJECT

public:
    explicit AuthFlowWidget(QWidget* parent = nullptr);

    void showSignIn(const QString& prefillEmail = QString());
    void showCreateAccount();
    void showVerifyEmail(const QString& email);
    void showAccountCreated(const QString& email);
    void setBusy(bool busy, const QString& status = QString());
    void showError(const QString& message);
    void clearError();
    void setDemoMode(bool enabled);
    [[nodiscard]] QString verificationEmail() const;

Q_SIGNALS:
    void signInRequested(const QString& email, const QString& password);
    void createAccountRequested(
        const QString& fullName,
        const QString& username,
        const QString& email,
        const QString& password
    );
    void verificationCheckRequested();
    void resendVerificationRequested();
    void passwordResetRequested(const QString& email);

private:
    QWidget* buildSignInPage();
    QWidget* buildCreatePage();
    QWidget* buildVerifyPage();
    QWidget* buildCreatedPage();
    QWidget* buildBrandPanel(const QString& eyebrow, const QString& headline, const QString& body);
    QWidget* wrapRightPanel(QWidget* form);
    void validateAndSubmitCreate();

    QStackedWidget* m_stack = nullptr;
    QWidget* m_signInPage = nullptr;
    QWidget* m_createPage = nullptr;
    QWidget* m_verifyPage = nullptr;
    QWidget* m_createdPage = nullptr;

    QLineEdit* m_signInEmail = nullptr;
    QLineEdit* m_signInPassword = nullptr;
    QPushButton* m_signInButton = nullptr;
    QLabel* m_signInStatus = nullptr;

    QLineEdit* m_fullName = nullptr;
    QLineEdit* m_username = nullptr;
    QLineEdit* m_createEmail = nullptr;
    QLineEdit* m_createPassword = nullptr;
    QLineEdit* m_confirmPassword = nullptr;
    QCheckBox* m_terms = nullptr;
    QPushButton* m_createButton = nullptr;
    QLabel* m_createStatus = nullptr;

    QLabel* m_verifyEmailLabel = nullptr;
    QLabel* m_verifyStatus = nullptr;
    QPushButton* m_verifyButton = nullptr;
    QPushButton* m_resendButton = nullptr;

    QLabel* m_createdEmailLabel = nullptr;
    bool m_demoMode = false;
    QString m_verificationEmail;
};
