#pragma once

#include "authsession.h"

#include <QJsonObject>
#include <QMetaType>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <functional>

class FirebaseAuthClient final : public QObject {
    Q_OBJECT

public:
    explicit FirebaseAuthClient(QObject* parent = nullptr);

    bool loadConfiguration(const QString& filePath);
    void setApiKey(const QString& apiKey);
    [[nodiscard]] QString apiKey() const;
    [[nodiscard]] bool isConfigured() const;

    void createAccount(
        const QString& fullName,
        const QString& username,
        const QString& email,
        const QString& password
    );
    void signIn(const QString& email, const QString& password);
    void sendVerificationEmail(const QString& idToken);
    void reloadUser(const QString& idToken);
    void sendPasswordReset(const QString& email);
    void refreshIdToken(const QString& refreshToken);

Q_SIGNALS:
    void accountCreated(const AuthSession& session);
    void signedIn(const AuthSession& session);
    void verificationEmailSent();
    void userReloaded(const AuthSession& session);
    void passwordResetSent();
    void tokenRefreshed(const AuthSession& session);
    void requestFailed(const QString& message);

private:
    using JsonHandler = std::function<void(const QJsonObject&)>;

    void postIdentity(
        const QString& method,
        const QJsonObject& payload,
        JsonHandler success
    );
    void postSecureToken(
        const QJsonObject& payload,
        JsonHandler success
    );
    void updateDisplayName(
        const AuthSession& session,
        const QString& displayName,
        const QString& username
    );
    [[nodiscard]] static AuthSession sessionFromIdentityResponse(
        const QJsonObject& object
    );
    [[nodiscard]] static QString friendlyError(const QString& firebaseCode);

    QString m_apiKey;
    QNetworkAccessManager m_network;
};

Q_DECLARE_METATYPE(AuthSession)
