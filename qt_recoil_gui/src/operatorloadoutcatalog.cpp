#include "operatorloadoutcatalog.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHash>

namespace {
QHash<QString, double> loadWeaponRpms() {
    QFile file(QStringLiteral(":/config/weapon_rpm_defaults.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    const auto document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        return {};
    }

    QHash<QString, double> result;
    const auto weapons = document.object().value(QStringLiteral("weapons")).toArray();
    result.reserve(weapons.size());
    for (const auto& value : weapons) {
        const auto object = value.toObject();
        const QString weapon = object.value(QStringLiteral("weapon"))
            .toString()
            .trimmed()
            .toUpper();
        const double rpm = object.value(QStringLiteral("rpm")).toDouble(0.0);
        if (!weapon.isEmpty() && rpm > 0.0) {
            result.insert(weapon, rpm);
        }
    }
    return result;
}

QList<OperatorLoadoutRecord> loadRecords() {
    QFile file(QStringLiteral(":/config/operator_loadouts.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    const auto document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        return {};
    }

    QList<OperatorLoadoutRecord> result;
    const auto operators = document.object().value(QStringLiteral("operators")).toArray();
    result.reserve(operators.size());
    for (const auto& value : operators) {
        const auto object = value.toObject();
        OperatorLoadoutRecord record;
        record.operatorId = object.value(QStringLiteral("operator_id")).toString().trimmed().toLower();
        record.side = object.value(QStringLiteral("side")).toString().trimmed().toLower();
        for (const auto& weapon : object.value(QStringLiteral("primary_weapons")).toArray()) {
            const QString name = weapon.toString().trimmed();
            if (!name.isEmpty()) {
                record.primaryWeapons.append(name);
            }
        }
        for (const auto& weapon : object.value(QStringLiteral("secondary_weapons")).toArray()) {
            const QString name = weapon.toString().trimmed();
            if (!name.isEmpty()) {
                record.secondaryWeapons.append(name);
            }
        }
        if (!record.operatorId.isEmpty()) {
            result.append(record);
        }
    }
    return result;
}

const QList<AttachmentOption>& basicOptions(const QString& category) {
    static const QList<AttachmentOption> barrel{
        {QStringLiteral("none"), QStringLiteral("None"), QString(), 0.0},
        {QStringLiteral("suppressor"), QStringLiteral("Suppressor"), QString(), 0.0},
        {QStringLiteral("flash_hider"), QStringLiteral("Flash Hider"), QString(), 0.0},
        {QStringLiteral("compensator"), QStringLiteral("Compensator"), QString(), 0.0},
        {QStringLiteral("muzzle_brake"), QStringLiteral("Muzzle Brake"), QString(), 0.0},
        {QStringLiteral("extended_barrel"), QStringLiteral("Extended Barrel"), QString(), 0.0},
    };
    static const QList<AttachmentOption> grip{
        {QStringLiteral("none"), QStringLiteral("None"), QString(), 0.0},
        {QStringLiteral("vertical_grip"), QStringLiteral("Vertical Grip"), QString(), 0.0},
        {QStringLiteral("horizontal_grip"), QStringLiteral("Horizontal Grip"), QString(), 0.0},
        {QStringLiteral("angled_grip"), QStringLiteral("Angled Grip"), QString(), 0.0},
    };
    static const QList<AttachmentOption> underbarrel{
        {QStringLiteral("none"), QStringLiteral("None"), QString(), 0.0},
        {QStringLiteral("laser"), QStringLiteral("Laser"), QString(), 0.0},
    };
    if (category == QStringLiteral("grip")) {
        return grip;
    }
    if (category == QStringLiteral("underbarrel")) {
        return underbarrel;
    }
    return barrel;
}
}

namespace OperatorLoadoutCatalog {
const QList<OperatorLoadoutRecord>& all() {
    static const QList<OperatorLoadoutRecord> records = loadRecords();
    return records;
}

const OperatorLoadoutRecord* findByOperatorId(const QString& operatorId) {
    const QString normalized = operatorId.trimmed().toLower();
    for (const auto& record : all()) {
        if (record.operatorId == normalized) {
            return &record;
        }
    }
    return nullptr;
}

QStringList weaponsFor(const QString& operatorId, const QString& weaponSlot) {
    const auto* record = findByOperatorId(operatorId);
    if (record == nullptr) {
        return {};
    }
    return weaponSlot.trimmed().toLower() == QStringLiteral("secondary")
        ? record->secondaryWeapons
        : record->primaryWeapons;
}

QString defaultWeapon(const QString& operatorId, const QString& weaponSlot) {
    const auto weapons = weaponsFor(operatorId, weaponSlot);
    return weapons.isEmpty() ? QString() : weapons.first();
}

double weaponRpm(const QString& weaponName) {
    static const QHash<QString, double> rpms = loadWeaponRpms();
    const QString normalized = weaponName.trimmed().toUpper();
    if (normalized.isEmpty()) {
        return 0.0;
    }
    return rpms.value(normalized, 0.0);
}

double delaySecondsForWeapon(const QString& weaponName) {
    const double rpm = weaponRpm(weaponName);
    // User-specified NEXUS default formula. Do not invert this expression.
    return rpm > 0.0 ? rpm / 60000.0 : 0.0;
}

double defaultDelaySeconds(const QString& operatorId, const QString& weaponSlot) {
    return delaySecondsForWeapon(defaultWeapon(operatorId, weaponSlot));
}

const QList<AttachmentOption>& opticOptions() {
    // These IDs intentionally collapse the selected optic into the two ADS
    // profiles currently exposed by the NEXUS converter. The existing backend
    // can replace/filter this list with exact operator+weapon availability.
    static const QList<AttachmentOption> options{
        {QStringLiteral("iron_1x"), QStringLiteral("Iron Sights / 1.0x"), QStringLiteral("ads_1x"), 1.0},
        {QStringLiteral("red_dot_1x"), QStringLiteral("Red Dot / 1.0x"), QStringLiteral("ads_1x"), 1.0},
        {QStringLiteral("holographic_1x"), QStringLiteral("Holographic / 1.0x"), QStringLiteral("ads_1x"), 1.0},
        {QStringLiteral("reflex_1x"), QStringLiteral("Reflex / 1.0x"), QStringLiteral("ads_1x"), 1.0},
        {QStringLiteral("magnified_2_5x"), QStringLiteral("Magnified Scope / 2.5x"), QStringLiteral("ads_2_5x"), 2.5},
    };
    return options;
}

const QList<AttachmentOption>& barrelOptions() {
    return basicOptions(QStringLiteral("barrel"));
}

const QList<AttachmentOption>& gripOptions() {
    return basicOptions(QStringLiteral("grip"));
}

const QList<AttachmentOption>& underbarrelOptions() {
    return basicOptions(QStringLiteral("underbarrel"));
}

const AttachmentOption* findOptic(const QString& opticId) {
    const QString normalized = opticId.trimmed().toLower();
    for (const auto& option : opticOptions()) {
        if (option.id == normalized) {
            return &option;
        }
    }
    return nullptr;
}

QVariantMap defaultAttachments() {
    return {
        {QStringLiteral("optic"), QStringLiteral("iron_1x")},
        {QStringLiteral("barrel"), QStringLiteral("none")},
        {QStringLiteral("grip"), QStringLiteral("none")},
        {QStringLiteral("underbarrel"), QStringLiteral("none")},
        {QStringLiteral("ads_profile_key"), QStringLiteral("ads_1x")},
        {QStringLiteral("optic_magnification"), 1.0},
    };
}
}
