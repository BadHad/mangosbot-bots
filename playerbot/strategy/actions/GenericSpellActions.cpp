#include "botpch.h"
#include "../../playerbot.h"
#include "GenericActions.h"

using namespace ai;

bool CastSpellAction::Execute(Event event)
{
    if (spell == "mount")
    {
        if (!bot->IsInCombat())
        {
            if (!bot->IsMounted())
            {
                if (bot->IsDead())
                    return false;
                bool isOutdoor;
                uint16 areaFlag = bot->GetMap()->GetTerrain()->GetAreaFlag(bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ(), &isOutdoor);
                if (!isOutdoor)
                    return false;

                uint32 firstmount =
#ifdef MANGOSBOT_ZERO
                    40
#else
#ifdef MANGOSBOT_ONE
                    30
#else
                    20
#endif
#endif
                    ;

                uint32 secondmount =
#ifdef MANGOSBOT_ZERO
                    60
#else
#ifdef MANGOSBOT_ONE
                    60
#else
                    40
#endif
#endif
                    ;

                //Mounts
                if (bot->getLevel() >= secondmount && bot->GetTeamId() == TEAM_INDEX_ALLIANCE)
                {
                    return ai->CastSpell(23240, bot);
                }
                if (bot->getLevel() >= secondmount && bot->GetTeamId() == TEAM_INDEX_HORDE)
                {
                    return ai->CastSpell(23242, bot);
                }
                if (bot->getLevel() >= firstmount && bot->GetTeamId() == TEAM_INDEX_ALLIANCE)
                {
                    return ai->CastSpell(6899, bot);
                }
                if (bot->getLevel() >= firstmount && bot->GetTeamId() == TEAM_INDEX_HORDE)
                {
                    return ai->CastSpell(8395, bot);
                }
            }
        }
        return false;
    }

	return ai->CastSpell(spell, GetTarget());
}

bool CastSpellAction::isPossible()
{
    if (spell == "mount" && !bot->IsMounted() && !bot->IsInCombat())
        return true;
    if (spell == "mount" && bot->IsInCombat())
    {
        bot->Unmount();
        return false;
    }

	return ai->CanCastSpell(spell, GetTarget(), true);
}

bool CastSpellAction::isUseful()
{
    if (spell == "mount" && !bot->IsMounted() && !bot->IsInCombat())
        return true;
    if (spell == "mount" && bot->IsInCombat())
    {
        bot->Unmount();
        return false;
    }

    return GetTarget() && AI_VALUE2(bool, "spell cast useful", spell) && AI_VALUE2(float, "distance", GetTargetName()) <= range;
}

bool CastAuraSpellAction::isUseful()
{
    return CastSpellAction::isUseful() && !ai->HasAura(spell, GetTarget(), true);
}

bool CastEnchantItemAction::isPossible()
{
    if (!CastSpellAction::isPossible())
        return false;

    uint32 spellId = AI_VALUE2(uint32, "spell id", spell);
    return spellId && AI_VALUE2(Item*, "item for spell", spellId);
}

bool CastHealingSpellAction::isUseful()
{
    return CastAuraSpellAction::isUseful();
}

bool CastAoeHealSpellAction::isUseful()
{
    return CastSpellAction::isUseful();
}


Value<Unit*>* CurePartyMemberAction::GetTargetValue()
{
    return context->GetValue<Unit*>("party member to dispel", dispelType);
}

Value<Unit*>* BuffOnPartyAction::GetTargetValue()
{
    return context->GetValue<Unit*>("party member without aura", spell);
}
