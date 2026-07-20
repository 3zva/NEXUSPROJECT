#include "firebaseauthclient.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <utility>

FirebaseAuthClient::FirebaseAuthClient(QObject* parent)
    : QObject(parent) {
    qRegisterMetaType<AuthSession>("AuthSession");
}

bool FirebaseAuthClient::loadConfiguration(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    const auto document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        return false;
    }

    setApiKey(document.object().value(QStringLiteral("apiKey")).toString());
    return isConfigured();
}

void FirebaseAuthClient::setApiKey(const QString& apiKey) {
    m_apiKey = apiKey.trimmed();
}

QString FirebaseAuthClient::apiKey() const {
    return m_apiKey;
}

bool FirebaseAuthClient::isConfigured() const {
    return !m_apiKey.isEmpty()
        && !m_apiKey.contains(QStringLiteral("PASTE_YOUR"), Qt::CaseInsensitive);
}

void FirebaseAuthClient::createAccount(
    const QString& fullName,
    const QString& username,
    const QString& email,
    const QString& password
) {
    if (!isConfigured()) {
        Q_EMIT requestFailed(QStringLiteral(
            "Firebase is not configured. Copy config/firebase_config.example.json "
            "to config/firebase_config.json and add the Web API key."
        ));
        return;
    }

    QJsonObject payload{
        {QStringLiteral("email"), email.trimmed()},
        {QStringLiteral("password"), password},
        {QStringLiteral("returnSecureToken"), true},
    };

    postIdentity(
        QStringLiteral("signUp"),
        payload,
        [this, fullName, username](const QJsonObject& object) {
            auto session = sessionFromIdentityResponse(object);
            session.authenticated = true;
            session.displayName = fullName.trimmed();
            session.username = username.trimmed();
            updateDisplayName(session, fullName.trimmed(), username.trimmed());
        }
    );
}

void FirebaseAuthClient::signIn(const QString& email, const QString& password) {
    if (!isConfigured()) {
        Q_EMIT requestFailed(QStringLiteral(
            "Firebase is not configured. Add the Web API key to "
            "config/firebase_config.json."
        ));
        return;
    }

    const QJsonObject payload{
        {QStringLiteral("email"), email.trimmed()},
        {QStringLiteral("password"), password},
        {QStringLiteral("returnSecureToken"), true},
    };

    postIdentity(
        QStringLiteral("signInWithPassword"),
        payload,
        [this](const QJsonObject& object) {
            auto session = sessionFromIdentityResponse(object);
            session.authenticated = true;
            Q_EMIT signedIn(session);
        }
    );
}

void FirebaseAuthClient::sendVerificationEmail(const QString& idToken) {
    const QJsonObject payload{
        {QStringLiteral("requestType"), QStringLiteral("VERIFY_EMAIL")},
        {QStringLiteral("idToken"), idToken},
    };

    postIdentity(
        QStringLiteral("sendOobCode"),
        payload,
        [this](const QJsonObject&) { Q_EMIT verificationEmailSent(); }
    );
}

void FirebaseAuthClient::reloadUser(const QString& idToken) {
    const QJsonObject payload{{QStringLiteral("idToken"), idToken}};

    postIdentity(
        QStringLiteral("lookup"),
        payload,
        [this, idToken](const QJsonObject& object) {
            const auto users = object.value(QStringLiteral("users")).toArray();
            if (users.isEmpty() || !users.first().isObject()) {
                Q_EMIT requestFailed(QStringLiteral("Firebase returned no user record."));
                return;
            }

            const auto user = users.first().toObject();
            AuthSession session;
            session.authenticated = true;
            session.idToken = idToken;
            session.localId = user.value(QStringLiteral("localId")).toString();
            session.email = user.value(QStringLiteral("email")).toString();
            session.displayName = user.value(QStringLiteral("displayName")).toString();
            session.emailVerified = user.value(QStringLiteral("emailVerified")).toBool();
            Q_EMIT userReloaded(session);
        }
    );
}

void FirebaseAuthClient::sendPasswordReset(const QString& email) {
    const QJsonObject payload{
        {QStringLiteral("requestType"), QStringLiteral("PASSWORD_RESET")},
        {QStringLiteral("email"), email.trimmed()},
    };

    postIdentity(
        QStringLiteral("sendOobCode"),
        payload,
        [this](const QJsonObject&) { Q_EMIT passwordResetSent(); }
    );
}

void FirebaseAuthClient::refreshIdToken(const QString& refreshToken) {
    QJsonObject payload{
        {QStringLiteral("grant_type"), QStringLiteral("refresh_token")},
        {QStringLiteral("refresh_token"), refreshToken},
    };

    postSecureToken(
        payload,
        [this](const QJsonObject& object) {
            AuthSession session;
            session.authenticated = true;
            session.idToken = object.value(QStringLiteral("id_token")).toString();
            session.refreshToken = object.value(QStringLiteral("refresh_token")).toString();
            session.localId = object.value(QStringLiteral("user_id")).toString();
            Q_EMIT tokenRefreshed(session);
        }
    );
}

void FirebaseAuthClient::postIdentity(
    const QString& method,
    const QJsonObject& payload,
    JsonHandler success
) {
    if (!isConfigured()) {
        Q_EMIT requestFailed(QStringLiteral("Firebase Web API key is missing."));
        return;
    }

    QUrl url(QStringLiteral("https://identitytoolkit.googleapis.com/v1/accounts:") + method);
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("key"), m_apiKey);
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    auto* reply = m_network.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, success = std::move(success)]() {
        const auto data = reply->readAll();
        const auto document = QJsonDocument::fromJson(data);
        const auto object = document.object();
        const auto networkError = reply->error();
        reply->deleteLater();

        if (networkError != QNetworkReply::NoError || object.contains(QStringLiteral("error"))) {
            const auto code = object
                .value(QStringLiteral("error"))
                .toObject()
                .value(QStringLiteral("message"))
                .toString(reply->errorString());
            Q_EMIT requestFailed(friendlyError(code));
            return;
        }

        success(object);
    });
}

void FirebaseAuthClient::postSecureToken(
    const QJsonObject& payload,
    JsonHandler success
) {
    if (!isConfigured()) {
        Q_EMIT requestFailed(QStringLiteral("Firebase Web API key is missing."));
        return;
    }

    QUrl url(QStringLiteral("https://securetoken.googleapis.com/v1/token"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("key"), m_apiKey);
    url.setQuery(query);

    QUrlQuery form;
    for (auto iterator = payload.begin(); iterator != payload.end(); ++iterator) {
        form.addQueryItem(iterator.key(), iterator.value().toString());
    }

    QNetworkRequest request(url);
    request.setHeader(
        QNetworkRequest::ContentTypeHeader,
        QStringLiteral("application/x-www-form-urlencoded")
    );

    auto* reply = m_network.post(request, form.query(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, [this, reply, success = std::move(success)]() {
        const auto document = QJsonDocument::fromJson(reply->readAll());
        const auto object = document.object();
        const auto networkError = reply->error();
        reply->deleteLater();

        if (networkError != QNetworkReply::NoError || object.contains(QStringLiteral("error"))) {
            const auto code = object
                .value(QStringLiteral("error"))
                .toObject()
                .value(QStringLiteral("message"))
                .toString(QStringLiteral("TOKEN_REFRESH_FAILED"));
            Q_EMIT requestFailed(friendlyError(code));
            return;
        }

        success(object);
    });
}

void FirebaseAuthClient::updateDisplayName(
    const AuthSession& session,
    const QString& displayName,
    const QString& username
) {
    QJsonObject payload{
        {QStringLiteral("idToken"), session.idToken},
        {QStringLiteral("displayName"), displayName},
        {QStringLiteral("returnSecureToken"), true},
    };

    postIdentity(
        QStringLiteral("update"),
        payload,
        [this, session, displayName, username](const QJsonObject& object) mutable {
            auto updated = sessionFromIdentityResponse(object);
            updated.authenticated = true;
            updated.email = session.email;
            updated.localId = session.localId;
            updated.displayName = displayName;
            updated.username = username;
            Q_EMIT accountCreated(updated);
        }
    );
}

AuthSession FirebaseAuthClient::sessionFromIdentityResponse(const QJsonObject& object) {
    AuthSession session;
    session.email = object.value(QStringLiteral("email")).toString();
    session.displayName = object.value(QStringLiteral("displayName")).toString();
    session.idToken = object.value(QStringLiteral("idToken")).toString();
    session.refreshToken = object.value(QStringLiteral("refreshToken")).toString();
    session.localId = object.value(QStringLiteral("localId")).toString();
    return session;
}

QString FirebaseAuthClient::friendlyError(const QString& firebaseCode) {
    const auto code = firebaseCode.section(QLatin1Char(' '), 0, 0).trimmed();
    if (code == QStringLiteral("EMAIL_EXISTS")) {
        return QStringLiteral("An account already exists for that email address.");
    }
    if (code == QStringLiteral("INVALID_LOGIN_CREDENTIALS")
        || code == QStringLiteral("EMAIL_NOT_FOUND")
        || code == QStringLiteral("INVALID_PASSWORD")) {
        return QStringLiteral("The email or password is incorrect.");
    }
    if (code == QStringLiteral("USER_DISABLED")) {
        return QStringLiteral("This account has been disabled.");
    }
    if (code == QStringLiteral("WEAK_PASSWORD")) {
        return QStringLiteral("The password does not meet the Firebase password policy.");
    }
    if (code == QStringLiteral("OPERATION_NOT_ALLOWED")) {
        return QStringLiteral("Email/password sign-in is not enabled in Firebase Authentication.");
    }
    if (code == QStringLiteral("TOO_MANY_ATTEMPTS_TRY_LATER")) {
        return QStringLiteral("Too many attempts. Wait a moment and try again.");
    }
    if (code == QStringLiteral("INVALID_ID_TOKEN") || code == QStringLiteral("TOKEN_EXPIRED")) {
        return QStringLiteral("Your session expired. Please sign in again.");
    }
    if (code == QStringLiteral("NETWORK_ERROR")) {
        return QStringLiteral("Could not reach Firebase. Check your internet connection.");
    }
    return firebaseCode.isEmpty() ? QStringLiteral("Firebase authentication failed.") : firebaseCode;
}
