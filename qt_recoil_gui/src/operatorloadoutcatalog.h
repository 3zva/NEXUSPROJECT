#pragma once

#include <QList>
#include <QString>
#include <QStringList>
#include <QVariantMap>

struct OperatorLoadoutRecord final {
    QString operatorId;
    QString side;
    QStringList primaryWeapons;
    QStringList secondaryWeapons;
};

struct AttachmentOption final {
    QString id;
    QString displayName;
    QString adsProfileKey; // "ads_1x" or "ads_2_5x" for optics.
    double magnification = 1.0;
};

namespace OperatorLoadoutCatalog {
[[nodiscard]] const QList<OperatorLoadoutRecord>& all();
[[nodiscard]] const OperatorLoadoutRecord* findByOperatorId(const QString& operatorId);
[[nodiscard]] QStringList weaponsFor(const QString& operatorId, const QString& weaponSlot);
[[nodiscard]] QString defaultWeapon(const QString& operatorId, const QString& weaponSlot);
[[nodiscard]] double weaponRpm(const QString& weaponName);
[[nodiscard]] double delaySecondsForWeapon(const QString& weaponName);
[[nodiscard]] double defaultDelaySeconds(const QString& operatorId, const QString& weaponSlot);

[[nodiscard]] const QList<AttachmentOption>& opticOptions();
[[nodiscard]] const QList<AttachmentOption>& barrelOptions();
[[nodiscard]] const QList<AttachmentOption>& gripOptions();
[[nodiscard]] const QList<AttachmentOption>& underbarrelOptions();
[[nodiscard]] const AttachmentOption* findOptic(const QString& opticId);
[[nodiscard]] QVariantMap defaultAttachments();
}
