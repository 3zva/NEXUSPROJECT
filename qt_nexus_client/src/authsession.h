#pragma once

#include <QString>

struct AuthSession {
    bool authenticated = false;
    QString email;
    QString displayName;
    QString username;
    QString idToken;
    QString refreshToken;
    QString localId;
    bool emailVerified = false;

    void clear() {
        authenticated = false;
        email.clear();
        displayName.clear();
        username.clear();
        idToken.clear();
        refreshToken.clear();
        localId.clear();
        emailVerified = false;
    }
};
