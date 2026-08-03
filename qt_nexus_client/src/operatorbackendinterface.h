#pragma once

#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QtGlobal>

// Connect the NEXUS GUI to the application's already-existing operator backend.
// This interface carries configuration only; it does not implement input logic.
class OperatorBackendInterface {
public:
    virtual ~OperatorBackendInterface() = default;

    virtual void updateOperatorSetting(
        const QString& operatorId,
        const QString& key,
        const QVariant& value
    ) = 0;

    virtual void saveOperatorRecord(
        const QString& operatorId,
        const QVariantMap& settings
    ) = 0;

    virtual void resetOperatorRecord(const QString& operatorId) = 0;

    // schema-v2 mapping: 0 off, 1 Weapon 1, 2 Weapon 2, 3 both weapons.
    virtual void setRapidFireSelection(
        const QString& operatorId,
        int rapidFireValue,
        bool enabled
    ) = 0;

    virtual void replaceAllOperatorSettings(
        const QVariantMap& allOperatorSettings
    ) = 0;

    // Optional NEXUS.6 hook. Existing backend implementations remain source
    // compatible because this method has a default no-op implementation.
    // converterInputs contains ads_1x and ads_2_5x from the shared converter.
    virtual void updateOperatorLoadout(
        const QString& operatorId,
        const QString& weaponSlot,
        const QString& selectedWeapon,
        const QVariantMap& attachments,
        const QVariantMap& converterInputs
    ) {
        Q_UNUSED(operatorId);
        Q_UNUSED(weaponSlot);
        Q_UNUSED(selectedWeapon);
        Q_UNUSED(attachments);
        Q_UNUSED(converterInputs);
    }
};
