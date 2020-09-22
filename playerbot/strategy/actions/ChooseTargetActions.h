#pragma once

#include "../Action.h"
#include "AttackAction.h"
#include "../../ServerFacade.h"

namespace ai
{
    class DpsAoeAction : public AttackAction
    {
    public:
        DpsAoeAction(PlayerbotAI* ai) : AttackAction(ai, "dps aoe") {}

        virtual string GetTargetName() { return "dps aoe target"; }
    };

    class DpsAssistAction : public AttackAction
    {
    public:
        DpsAssistAction(PlayerbotAI* ai) : AttackAction(ai, "dps assist") {}

        virtual string GetTargetName() { return "dps target"; }
    };

    class TankAssistAction : public AttackAction
    {
    public:
        TankAssistAction(PlayerbotAI* ai) : AttackAction(ai, "tank assist") {}
        virtual string GetTargetName() { return "tank target"; }
    };

    class AttackAnythingAction : public AttackAction
    {
    public:
        AttackAnythingAction(PlayerbotAI* ai) : AttackAction(ai, "attack anything") {}
        virtual string GetTargetName() { return "grind target"; }
        virtual bool isUseful() {
            return GetTarget() &&
            /*    (!AI_VALUE(list<ObjectGuid>, "nearest non bot players").empty() &&
                    AI_VALUE2(uint8, "health", "self target") > sPlayerbotAIConfig.mediumHealth &&
                    (!AI_VALUE2(uint8, "mana", "self target") || AI_VALUE2(uint8, "mana", "self target") > sPlayerbotAIConfig.mediumMana)
                ) || AI_VALUE2(bool, "combat", "self target")
*/
                  ((!AI_VALUE(list<ObjectGuid>, "nearest non bot players").empty() &&
                    (AI_VALUE2(uint8, "health", "self target") > sPlayerbotAIConfig.mediumHealth &&
                    (!AI_VALUE2(uint8, "mana", "self target") || AI_VALUE2(uint8, "mana", "self target") > sPlayerbotAIConfig.mediumMana))) || AI_VALUE2(bool, "combat", "self target"))
                ;
        }
        virtual bool isPossible()
        {
            return AttackAction::isPossible() && GetTarget();
        }

        virtual bool Execute(Event event)
        {
            bool result = AttackAction::Execute(event);
            if (result && GetTarget()) context->GetValue<ObjectGuid>("pull target")->Set(GetTarget()->GetObjectGuid());
            return result;
        }
    };

    class AttackLeastHpTargetAction : public AttackAction
    {
    public:
        AttackLeastHpTargetAction(PlayerbotAI* ai) : AttackAction(ai, "attack least hp target") {}
        virtual string GetTargetName() { return "least hp target"; }
    };

    class AttackEnemyPlayerAction : public AttackAction
    {
    public:
        AttackEnemyPlayerAction(PlayerbotAI* ai) : AttackAction(ai, "attack enemy player") {}
        virtual string GetTargetName() { return "enemy player target"; }
    };

    class AttackRtiTargetAction : public AttackAction
    {
    public:
        AttackRtiTargetAction(PlayerbotAI* ai) : AttackAction(ai, "attack rti target") {}
        virtual string GetTargetName() { return "rti target"; }
    };

    class DropTargetAction : public Action
    {
    public:
        DropTargetAction(PlayerbotAI* ai) : Action(ai, "drop target") {}

        virtual bool Execute(Event event)
        {
            Unit* target = context->GetValue<Unit*>("current target")->Get();
            ObjectGuid pullTarget = context->GetValue<ObjectGuid>("pull target")->Get();
            list<ObjectGuid> possible = ai->GetAiObjectContext()->GetValue<list<ObjectGuid> >("possible targets")->Get();
            if (pullTarget && find(possible.begin(), possible.end(), pullTarget) == possible.end())
            {
                context->GetValue<ObjectGuid>("pull target")->Set(ObjectGuid());
            }
            context->GetValue<Unit*>("current target")->Set(NULL);
            bot->SetSelectionGuid(ObjectGuid());
            ai->ChangeEngine(BOT_STATE_NON_COMBAT);
            ai->InterruptSpell();
            bot->AttackStop();
            Pet* pet = bot->GetPet();
            if (pet)
            {
#ifdef VMANGOS
                CreatureAI*
#endif
#ifdef CMANGOS
                    UnitAI*
#endif
                    creatureAI = ((Creature*)pet)->AI();
                if (creatureAI)
                {
#ifdef CMANGOS
                    creatureAI->SetReactState(REACT_PASSIVE);
#endif
#ifdef MANGOS
                    pet->GetCharmInfo()->SetReactState(REACT_PASSIVE);
                    pet->GetCharmInfo()->SetCommandState(COMMAND_FOLLOW);
#endif
                    pet->AttackStop();
                }
            }
            if (!urand(0, 25))
            {
                vector<uint32> sounds;
                if (target && sServerFacade.UnitIsDead(target))
                {
                    sounds.push_back(TEXTEMOTE_CHEER);
                    sounds.push_back(TEXTEMOTE_CONGRATULATE);
                }
                else
                {
                    sounds.push_back(304); // guard
                    sounds.push_back(325); // stay
                }
                if (!sounds.empty()) ai->PlaySound(sounds[urand(0, sounds.size() - 1)]);
            }
            return true;
        }
    };

}
