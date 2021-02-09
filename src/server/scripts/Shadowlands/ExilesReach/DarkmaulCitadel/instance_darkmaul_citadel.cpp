/*
 * Copyright (C) 2021 Hooaahcore
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptMgr.h"
#include "Player.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "AreaTrigger.h"
#include "AreaTriggerAI.h"
#include "InstanceScript.h"
#include "CombatAI.h"
#include "LFGMgr.h"
#include "Group.h"
#include "darkmaul_citadel.h"

//2236
struct instance_darkmaul_citadel : public InstanceScript
{
    instance_darkmaul_citadel(InstanceMap* map) : InstanceScript(map) {  }

private:
    bool CombatAI;    
    Creature* GetGarrick() { return GetCreature(NPC_COMBAT_AI_GARRICK); }
    Creature* GetHenry() { return GetCreature(NPC_COMBAT_AI_HENRY); }   

    void Initialize() override
    {
        CombatAI = false;
    }

    void OnPlayerEnter(Player* player) override
    {
        if (!CombatAI)
            GenerateCombatAI(player);
    }

    void GenerateCombatAI(Player* player)
    {        
        CombatAI = true;
        switch (player->GetTeam())
        {
        case ALLIANCE:
            if (Creature* garrick = GetGarrick())
            {
                ObjectGuid garrickOwner = player->GetGUID();
                garrick->SetOwnerGUID(garrickOwner);
                garrick->SetFaction(player->getFaction());
                int32 attackPower = garrick->GetOwner()->GetTotalAttackPowerValue(BASE_ATTACK) * 3.0f;
                garrick->SetAttackPower(attackPower);
                garrick->GetMotionMaster()->MoveFollow(player, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
            }
            if (Creature* henry = GetHenry())
            {
                ObjectGuid henryOwner = player->GetGUID();
                henry->SetOwnerGUID(henryOwner);
                henry->SetFaction(henry->getFaction());
                int32 attackPower = henry->GetOwner()->GetTotalAttackPowerValue(BASE_ATTACK) * 3.0f;
                henry->SetAttackPower(attackPower);
                int32 spellPower = int32(100.0f * henry->GetOwner()->GetTotalSpellPowerValue(SPELL_SCHOOL_MASK_NORMAL, true));
                henry->GetMotionMaster()->MoveFollow(player, PET_FOLLOW_DIST, 6.28f);
            }
            break;
        case HORDE:
            break;
        }
    }
};

void AddSC_instance_darkmaul_citadel()
{
    RegisterInstanceScript(instance_darkmaul_citadel, 2236);
}