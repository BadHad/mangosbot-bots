#include "botpch.h"
#include "../../playerbot.h"
#include "AttackersValue.h"

#include "../../ServerFacade.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"

using namespace ai;
using namespace MaNGOS;

list<ObjectGuid> AttackersValue::Calculate()
{
    set<Unit*> targets;

    AddAttackersOf(bot, targets);

    Group* group = bot->GetGroup();
    if (group)
        AddAttackersOf(group, targets);

    RemoveNonThreating(targets);

    list<ObjectGuid> result;
	for (set<Unit*>::iterator i = targets.begin(); i != targets.end(); i++)
		result.push_back((*i)->GetObjectGuid());

    if (bot->duel && bot->duel->opponent)
        result.push_back(bot->duel->opponent->GetObjectGuid());

	return result;
}

void AttackersValue::AddAttackersOf(Group* group, set<Unit*>& targets)
{
    Group::MemberSlotList const& groupSlot = group->GetMemberSlots();
    for (Group::member_citerator itr = groupSlot.begin(); itr != groupSlot.end(); itr++)
    {
        Player *member = sObjectMgr.GetPlayer(itr->guid);
        if (!member || !sServerFacade.IsAlive(member) || member == bot)
            continue;

        AddAttackersOf(member, targets);
    }
}

struct AddGuardiansHelper
{
    explicit AddGuardiansHelper(list<Unit*> &units) : units(units) {}
    void operator()(Unit* target) const
    {
        units.push_back(target);
    }

    list<Unit*> &units;
};

void AttackersValue::AddAttackersOf(Player* player, set<Unit*>& targets)
{
    if (player->IsBeingTeleported())
        return;

	list<Unit*> units;
	MaNGOS::AnyUnfriendlyUnitInObjectRangeCheck u_check(player, sPlayerbotAIConfig.sightDistance);
    MaNGOS::UnitListSearcher<MaNGOS::AnyUnfriendlyUnitInObjectRangeCheck> searcher(units, u_check);
    Cell::VisitAllObjects(player, searcher, sPlayerbotAIConfig.sightDistance);
	for (list<Unit*>::iterator i = units.begin(); i != units.end(); i++)
    {
		if (!player->GetGroup())
		{
			Unit* unit = *i;
#ifdef CMANGOS
			if (!unit->getThreatManager().getThreat(player))
#endif
#ifdef MANGOS
			if (!unit->GetThreatManager().getThreat(player))
#endif
				continue;
		}
        {
            Unit* unit = *i;
            targets.insert(unit);
            unit->CallForAllControlledUnits(AddGuardiansHelper(units), CONTROLLED_PET | CONTROLLED_GUARDIANS | CONTROLLED_CHARM | CONTROLLED_MINIPET | CONTROLLED_TOTEMS);
        }
    }
}

void AttackersValue::RemoveNonThreating(set<Unit*>& targets)
{
    for(set<Unit *>::iterator tIter = targets.begin(); tIter != targets.end();)
    {
        Unit* unit = *tIter;
        if (!IsValidTarget(unit, bot))
        {
            set<Unit *>::iterator tIter2 = tIter;
            ++tIter;
            targets.erase(tIter2);
        }
        else
            ++tIter;
    }
}

bool AttackersValue::IsPossibleTarget(Unit *attacker, Player *bot)
{
    Creature *c = dynamic_cast<Creature*>(attacker);
    return attacker &&
        attacker->IsInWorld() &&
        attacker->GetMapId() == bot->GetMapId() &&
        !sServerFacade.UnitIsDead(attacker) &&
        !attacker->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE) &&
        !attacker->HasStealthAura() &&
        !attacker->HasInvisibilityAura() &&
        !attacker->IsPolymorphed() &&
#ifdef CMANGOS
        !attacker->IsStunned() &&
#endif
#ifdef MANGOS
        !attacker->hasUnitState(UNIT_STAT_STUNNED) &&
#endif
        !sServerFacade.IsCharmed(attacker) &&
        !sServerFacade.IsFeared(attacker) &&
        !sServerFacade.IsInRoots(attacker) &&
        !sServerFacade.IsFriendlyTo(attacker, bot) &&
        bot->IsWithinDistInMap(attacker, sPlayerbotAIConfig.sightDistance) &&
        //!((attacker->getLevel() == 1 && !sServerFacade.IsHostileTo(attacker, bot)) && (!bot->GetGroup())) &&
        !(attacker->GetCreatureType() == CREATURE_TYPE_CRITTER) &&
		!(sPlayerbotAIConfig.IsInPvpProhibitedZone(attacker->GetAreaId()) && attacker->GetObjectGuid().IsPlayer() && attacker->GetObjectGuid().IsPet()) &&
        (!c || (
            !c->IsInEvadeMode() &&
            (!attacker->HasFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_TAPPED)
#ifndef MANGOSBOT_TWO
                || bot->IsTappedByMeOrMyGroup(c)
#endif
                )
            )
            );
}

bool AttackersValue::IsValidTarget(Unit *attacker, Player *bot)
{
    return IsPossibleTarget(attacker, bot) &&
        (sServerFacade.GetThreatManager(attacker).getCurrentVictim() || attacker->GetTargetGuid() || attacker->GetObjectGuid().IsPlayer() ||
            attacker->GetObjectGuid() == bot->GetPlayerbotAI()->GetAiObjectContext()->GetValue<ObjectGuid>("pull target")->Get());
}

bool PossibleAdsValue::Calculate()
{
    PlayerbotAI *ai = bot->GetPlayerbotAI();
    list<ObjectGuid> possible = ai->GetAiObjectContext()->GetValue<list<ObjectGuid> >("possible targets")->Get();
    list<ObjectGuid> attackers = ai->GetAiObjectContext()->GetValue<list<ObjectGuid> >("attackers")->Get();

    for (list<ObjectGuid>::iterator i = possible.begin(); i != possible.end(); ++i)
    {
        ObjectGuid guid = *i;
        if (find(attackers.begin(), attackers.end(), guid) != attackers.end()) continue;

        Unit* add = ai->GetUnit(guid);
        if (add && !add->GetTargetGuid() && !sServerFacade.GetThreatManager(add).getCurrentVictim() && sServerFacade.IsHostileTo(add, bot))
        {
            for (list<ObjectGuid>::iterator j = attackers.begin(); j != attackers.end(); ++j)
            {
                Unit* attacker = ai->GetUnit(*j);
                if (!attacker) continue;

                float dist = sServerFacade.GetDistance2d(attacker, add);
                if (sServerFacade.IsDistanceLessOrEqualThan(dist, sPlayerbotAIConfig.aoeRadius * 1.5f)) continue;
                if (sServerFacade.IsDistanceLessOrEqualThan(dist, sPlayerbotAIConfig.aggroDistance)) return true;
            }
        }
    }
    return false;
}
