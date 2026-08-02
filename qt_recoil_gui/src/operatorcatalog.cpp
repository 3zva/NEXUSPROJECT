#include "operatorcatalog.h"

namespace OperatorCatalog {
const QList<OperatorRecord>& all() {
    static const QList<OperatorRecord> records{
        {QStringLiteral("striker"), QStringLiteral("Striker"), QStringLiteral("attacker"), QStringLiteral("operators/striker.png")},
        {QStringLiteral("sledge"), QStringLiteral("Sledge"), QStringLiteral("attacker"), QStringLiteral("operators/sledge.png")},
        {QStringLiteral("thatcher"), QStringLiteral("Thatcher"), QStringLiteral("attacker"), QStringLiteral("operators/thatcher.png")},
        {QStringLiteral("ash"), QStringLiteral("Ash"), QStringLiteral("attacker"), QStringLiteral("operators/ash.png")},
        {QStringLiteral("thermite"), QStringLiteral("Thermite"), QStringLiteral("attacker"), QStringLiteral("operators/thermite.png")},
        {QStringLiteral("twitch"), QStringLiteral("Twitch"), QStringLiteral("attacker"), QStringLiteral("operators/twitch.png")},
        {QStringLiteral("montagne"), QStringLiteral("Montagne"), QStringLiteral("attacker"), QStringLiteral("operators/montagne.png")},
        {QStringLiteral("glaz"), QStringLiteral("Glaz"), QStringLiteral("attacker"), QStringLiteral("operators/glaz.png")},
        {QStringLiteral("fuze"), QStringLiteral("Fuze"), QStringLiteral("attacker"), QStringLiteral("operators/fuze.png")},
        {QStringLiteral("blitz"), QStringLiteral("Blitz"), QStringLiteral("attacker"), QStringLiteral("operators/blitz.png")},
        {QStringLiteral("iq"), QStringLiteral("IQ"), QStringLiteral("attacker"), QStringLiteral("operators/iq.png")},
        {QStringLiteral("buck"), QStringLiteral("Buck"), QStringLiteral("attacker"), QStringLiteral("operators/buck.png")},
        {QStringLiteral("blackbeard"), QStringLiteral("Blackbeard"), QStringLiteral("attacker"), QStringLiteral("operators/blackbeard.png")},
        {QStringLiteral("capitao"), QStringLiteral("Capitão"), QStringLiteral("attacker"), QStringLiteral("operators/capitao.png")},
        {QStringLiteral("hibana"), QStringLiteral("Hibana"), QStringLiteral("attacker"), QStringLiteral("operators/hibana.png")},
        {QStringLiteral("jackal"), QStringLiteral("Jackal"), QStringLiteral("attacker"), QStringLiteral("operators/jackal.png")},
        {QStringLiteral("ying"), QStringLiteral("Ying"), QStringLiteral("attacker"), QStringLiteral("operators/ying.png")},
        {QStringLiteral("zofia"), QStringLiteral("Zofia"), QStringLiteral("attacker"), QStringLiteral("operators/zofia.png")},
        {QStringLiteral("dokkaebi"), QStringLiteral("Dokkaebi"), QStringLiteral("attacker"), QStringLiteral("operators/dokkaebi.png")},
        {QStringLiteral("lion"), QStringLiteral("Lion"), QStringLiteral("attacker"), QStringLiteral("operators/lion.png")},
        {QStringLiteral("finka"), QStringLiteral("Finka"), QStringLiteral("attacker"), QStringLiteral("operators/finka.png")},
        {QStringLiteral("maverick"), QStringLiteral("Maverick"), QStringLiteral("attacker"), QStringLiteral("operators/maverick.png")},
        {QStringLiteral("nomad"), QStringLiteral("Nomad"), QStringLiteral("attacker"), QStringLiteral("operators/nomad.png")},
        {QStringLiteral("gridlock"), QStringLiteral("Gridlock"), QStringLiteral("attacker"), QStringLiteral("operators/gridlock.png")},
        {QStringLiteral("nokk"), QStringLiteral("Nøkk"), QStringLiteral("attacker"), QStringLiteral("operators/nokk.png")},
        {QStringLiteral("amaru"), QStringLiteral("Amaru"), QStringLiteral("attacker"), QStringLiteral("operators/amaru.png")},
        {QStringLiteral("kali"), QStringLiteral("Kali"), QStringLiteral("attacker"), QStringLiteral("operators/kali.png")},
        {QStringLiteral("iana"), QStringLiteral("Iana"), QStringLiteral("attacker"), QStringLiteral("operators/iana.png")},
        {QStringLiteral("ace"), QStringLiteral("Ace"), QStringLiteral("attacker"), QStringLiteral("operators/ace.png")},
        {QStringLiteral("zero"), QStringLiteral("Zero"), QStringLiteral("attacker"), QStringLiteral("operators/zero.png")},
        {QStringLiteral("flores"), QStringLiteral("Flores"), QStringLiteral("attacker"), QStringLiteral("operators/flores.png")},
        {QStringLiteral("osa"), QStringLiteral("Osa"), QStringLiteral("attacker"), QStringLiteral("operators/osa.png")},
        {QStringLiteral("sens"), QStringLiteral("Sens"), QStringLiteral("attacker"), QStringLiteral("operators/sens.png")},
        {QStringLiteral("grim"), QStringLiteral("Grim"), QStringLiteral("attacker"), QStringLiteral("operators/grim.png")},
        {QStringLiteral("brava"), QStringLiteral("Brava"), QStringLiteral("attacker"), QStringLiteral("operators/brava.png")},
        {QStringLiteral("ram"), QStringLiteral("Ram"), QStringLiteral("attacker"), QStringLiteral("operators/ram.png")},
        {QStringLiteral("deimos"), QStringLiteral("Deimos"), QStringLiteral("attacker"), QStringLiteral("operators/deimos.png")},
        {QStringLiteral("rauora"), QStringLiteral("Rauora"), QStringLiteral("attacker"), QStringLiteral("operators/rauora.png")},
        {QStringLiteral("sentry"), QStringLiteral("Sentry"), QStringLiteral("defender"), QStringLiteral("operators/sentry.png")},
        {QStringLiteral("smoke"), QStringLiteral("Smoke"), QStringLiteral("defender"), QStringLiteral("operators/smoke.png")},
        {QStringLiteral("mute"), QStringLiteral("Mute"), QStringLiteral("defender"), QStringLiteral("operators/mute.png")},
        {QStringLiteral("castle"), QStringLiteral("Castle"), QStringLiteral("defender"), QStringLiteral("operators/castle.png")},
        {QStringLiteral("pulse"), QStringLiteral("Pulse"), QStringLiteral("defender"), QStringLiteral("operators/pulse.png")},
        {QStringLiteral("doc"), QStringLiteral("Doc"), QStringLiteral("defender"), QStringLiteral("operators/doc.png")},
        {QStringLiteral("rook"), QStringLiteral("Rook"), QStringLiteral("defender"), QStringLiteral("operators/rook.png")},
        {QStringLiteral("kapkan"), QStringLiteral("Kapkan"), QStringLiteral("defender"), QStringLiteral("operators/kapkan.png")},
        {QStringLiteral("tachanka"), QStringLiteral("Tachanka"), QStringLiteral("defender"), QStringLiteral("operators/tachanka.png")},
        {QStringLiteral("jager"), QStringLiteral("Jäger"), QStringLiteral("defender"), QStringLiteral("operators/jager.png")},
        {QStringLiteral("bandit"), QStringLiteral("Bandit"), QStringLiteral("defender"), QStringLiteral("operators/bandit.png")},
        {QStringLiteral("frost"), QStringLiteral("Frost"), QStringLiteral("defender"), QStringLiteral("operators/frost.png")},
        {QStringLiteral("valkyrie"), QStringLiteral("Valkyrie"), QStringLiteral("defender"), QStringLiteral("operators/valkyrie.png")},
        {QStringLiteral("caveira"), QStringLiteral("Caveira"), QStringLiteral("defender"), QStringLiteral("operators/caveira.png")},
        {QStringLiteral("echo"), QStringLiteral("Echo"), QStringLiteral("defender"), QStringLiteral("operators/echo.png")},
        {QStringLiteral("mira"), QStringLiteral("Mira"), QStringLiteral("defender"), QStringLiteral("operators/mira.png")},
        {QStringLiteral("lesion"), QStringLiteral("Lesion"), QStringLiteral("defender"), QStringLiteral("operators/lesion.png")},
        {QStringLiteral("ela"), QStringLiteral("Ela"), QStringLiteral("defender"), QStringLiteral("operators/ela.png")},
        {QStringLiteral("vigil"), QStringLiteral("Vigil"), QStringLiteral("defender"), QStringLiteral("operators/vigil.png")},
        {QStringLiteral("maestro"), QStringLiteral("Maestro"), QStringLiteral("defender"), QStringLiteral("operators/maestro.png")},
        {QStringLiteral("alibi"), QStringLiteral("Alibi"), QStringLiteral("defender"), QStringLiteral("operators/alibi.png")},
        {QStringLiteral("clash"), QStringLiteral("Clash"), QStringLiteral("defender"), QStringLiteral("operators/clash.png")},
        {QStringLiteral("kaid"), QStringLiteral("Kaid"), QStringLiteral("defender"), QStringLiteral("operators/kaid.png")},
        {QStringLiteral("mozzie"), QStringLiteral("Mozzie"), QStringLiteral("defender"), QStringLiteral("operators/mozzie.png")},
        {QStringLiteral("warden"), QStringLiteral("Warden"), QStringLiteral("defender"), QStringLiteral("operators/warden.png")},
        {QStringLiteral("goyo"), QStringLiteral("Goyo"), QStringLiteral("defender"), QStringLiteral("operators/goyo.png")},
        {QStringLiteral("wamai"), QStringLiteral("Wamai"), QStringLiteral("defender"), QStringLiteral("operators/wamai.png")},
        {QStringLiteral("oryx"), QStringLiteral("Oryx"), QStringLiteral("defender"), QStringLiteral("operators/oryx.png")},
        {QStringLiteral("melusi"), QStringLiteral("Melusi"), QStringLiteral("defender"), QStringLiteral("operators/melusi.png")},
        {QStringLiteral("aruni"), QStringLiteral("Aruni"), QStringLiteral("defender"), QStringLiteral("operators/aruni.png")},
        {QStringLiteral("thunderbird"), QStringLiteral("Thunderbird"), QStringLiteral("defender"), QStringLiteral("operators/thunderbird.png")},
        {QStringLiteral("thorn"), QStringLiteral("Thorn"), QStringLiteral("defender"), QStringLiteral("operators/thorn.png")},
        {QStringLiteral("azami"), QStringLiteral("Azami"), QStringLiteral("defender"), QStringLiteral("operators/azami.png")},
        {QStringLiteral("solis"), QStringLiteral("Solis"), QStringLiteral("defender"), QStringLiteral("operators/solis.png")},
        {QStringLiteral("fenrir"), QStringLiteral("Fenrir"), QStringLiteral("defender"), QStringLiteral("operators/fenrir.png")},
        {QStringLiteral("tubarao"), QStringLiteral("Tubarão"), QStringLiteral("defender"), QStringLiteral("operators/tubarao.png")},
        {QStringLiteral("skopos"), QStringLiteral("Skopós"), QStringLiteral("defender"), QStringLiteral("operators/skopos.png")},
        {QStringLiteral("denari"), QStringLiteral("Denari"), QStringLiteral("defender"), QStringLiteral("operators/denari.png")},
    };
    return records;
}

QList<OperatorRecord> forSide(const QString& side) {
    QList<OperatorRecord> result;
    const auto normalized = side.trimmed().toLower();
    for (const auto& record : all()) {
        if (record.side == normalized) {
            result.append(record);
        }
    }
    return result;
}

const OperatorRecord* findById(const QString& id) {
    const auto normalized = id.trimmed().toLower();
    for (const auto& record : all()) {
        if (record.id == normalized) {
            return &record;
        }
    }
    return nullptr;
}

const OperatorRecord* findByDisplayName(const QString& displayName) {
    const auto normalized = displayName.trimmed();
    for (const auto& record : all()) {
        if (record.displayName.compare(normalized, Qt::CaseInsensitive) == 0) {
            return &record;
        }
    }
    return nullptr;
}

QString resolveId(const QString& idOrDisplayName) {
    if (const auto* byId = findById(idOrDisplayName)) {
        return byId->id;
    }
    if (const auto* byName = findByDisplayName(idOrDisplayName)) {
        return byName->id;
    }
    return idOrDisplayName.trimmed().toLower();
}
}
