#pragma once

#include <QList>
#include <QString>

struct OperatorRecord final {
    QString id;
    QString displayName;
    QString side;          // "attacker" or "defender"
    QString iconResource;  // relative to :/assets/
};

namespace OperatorCatalog {
[[nodiscard]] const QList<OperatorRecord>& all();
[[nodiscard]] QList<OperatorRecord> forSide(const QString& side);
[[nodiscard]] const OperatorRecord* findById(const QString& id);
[[nodiscard]] const OperatorRecord* findByDisplayName(const QString& displayName);
[[nodiscard]] QString resolveId(const QString& idOrDisplayName);
}
