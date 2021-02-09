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

#include "Conversation.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "DBCEnums.h"
#include "MapManager.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptedGossip.h"
#include "ScriptMgr.h"
#include "World.h"
#include "WorldSession.h"
#include "Chat.h"
#include "SharedDefines.h"


class playerscript_recruiter : public PlayerScript
{
public:
    playerscript_recruiter() : PlayerScript("playerscript_recruiter") {}

    void OnLevelChanged(Player* player, uint8 /*oldLevel*/) override
    {
        if (player->getLevel() != MAX_LEVEL)
            return;

        QueryResult result = LoginDatabase.PQuery("SELECT recruiter, recruiter_rewarded FROM account WHERE id = %u", player->GetSession()->GetAccountId());
        if (!result)
            return;

        Field* fields           = result->Fetch();
        uint32 recruiter        = fields[0].GetUInt32();
        bool recruiterRewarded  = fields[1].GetBool();

        if (recruiterRewarded)
            return;

        result = CharacterDatabase.PQuery("SELECT guid, NAME FROM characters WHERE account = %u ORDER BY totaltime DESC LIMIT 1", recruiter);
        if (!result)
            return;

        fields = result->Fetch();
        uint64 recruiterCharacterGUID = fields[0].GetUInt64();

        if (!recruiterCharacterGUID)
            return;

        result = LoginDatabase.PQuery("SELECT COUNT(*) FROM account WHERE recruiter = %u AND recruiter_rewarded = 1", recruiter);
        if (!result)
            return;

        fields = result->Fetch();
        uint32 recruiterRewardCount = fields[0].GetUInt32();
        uint32 rewardItem = 0;

        switch (++recruiterRewardCount)
        {
            case 1: rewardItem = 54860;     break; // X-53 Touring Rocket
            case 2: rewardItem = 37719;     break; // Swift Zhevra
            case 5: rewardItem = 106246;    break; // Emerald Hippogryph
            default: break;
        }

        if (rewardItem)
        {
            CharacterDatabase.PExecute("INSERT INTO character_shop (guid, type, itemId, itemCount) VALUES (" UI64FMTD ", 0, %u, 1)", recruiterCharacterGUID, rewardItem);
            LoginDatabase.PExecute("UPDATE account SET recruiter_rewarded = 1 WHERE id = %u", player->GetSession()->GetAccountId());
        }
    }
};

// TODO : this script is temp fix,
// remove it when legion start quests are properly fixed
// on HDB this is for lvl 45
class OnLegionArrival : public PlayerScript
{
public:
    OnLegionArrival() : PlayerScript("OnLegionArrival") { }

    enum
    {
        QUEST_THE_LEGION_RETURNS_A = 40519,
        QUEST_THE_LEGION_RETURNS_H = 43926,
        QUEST_BLINK_OF_AN_EYE = 44663,
        QUEST_KHADGARS_DISCOVERY = 44555,

        SPELL_LEGION_LAUNCH_PATCH_QUEST_LAUNCH = 258792,
        SPELL_MAGE_LEARN_GUARDIAN_HALL_TP = 204287,
        SPELL_WAR_LEARN_JUMP_TO_SKYHOLD = 192084,
        SPELL_DRUID_CLASS_HALL_TP = 204874,
        SPELL_CREATE_CLASS_HALL_ALLIANCE = 185506,
        SPELL_CREATE_CLASS_HALL_HORDE = 192191,

        CONVERSATION_KHADGAR_BLINK_OF_EYE = 3827,
    };

    void OnQuestStatusChange(Player* player, uint32 /*questId*/)
    {
        if ((player->IsInAlliance() && player->GetQuestStatus(QUEST_THE_LEGION_RETURNS_A) == QUEST_STATUS_REWARDED) || (player->IsInHorde() && player->GetQuestStatus(QUEST_THE_LEGION_RETURNS_H) == QUEST_STATUS_REWARDED))
        {
            if (player->GetQuestStatus(QUEST_BLINK_OF_AN_EYE) == QUEST_STATUS_NONE)
            {
                Conversation::CreateConversation(CONVERSATION_KHADGAR_BLINK_OF_EYE, player, player->GetPosition(), { player->GetGUID() });

                if (const Quest* quest = sObjectMgr->GetQuestTemplate(QUEST_BLINK_OF_AN_EYE))
                    player->AddQuest(quest, nullptr);
            }
        }
    }

    void OnLogin(Player* player, bool firstLogin) override
    {
        if (Map* map = sMapMgr->FindMap(0, 0))
            map->LoadGrid(-11099.8f, -2212.36f);

        if (Map* map = sMapMgr->FindMap(1669, 0))
        {
            map->LoadGrid(459.02f, 1450.02f);
            map->LoadGrid(4682.5f, 9851.57f);
            map->LoadGrid(-2624.08f, 8654.29f);
        }

    }

    void OnLevelChanged(Player* player, uint8 oldLevel) override
    {
        if (oldLevel < 45 && player->getLevel() >= 45)
        {
                HandleLegionArrival(player);
        }
    }

    void HandleLegionArrival(Player* player)
    {
        switch (player->getClass())
        {
        case CLASS_MAGE:
            player->CastSpell(player, SPELL_MAGE_LEARN_GUARDIAN_HALL_TP, true);
            break;
        case CLASS_WARRIOR:
            player->CastSpell(player, SPELL_WAR_LEARN_JUMP_TO_SKYHOLD, true);
            break;
        case CLASS_DRUID:
            player->CastSpell(player, SPELL_DRUID_CLASS_HALL_TP, true);
            break;
        case CLASS_HUNTER:
            player->m_taxi.SetTaximaskNode(1848); // Hunter Class Hall
            break;
        default:
            break;
        }

        ///QUEST_THE_LEGION_RETURNS
        player->CastSpell(player, SPELL_LEGION_LAUNCH_PATCH_QUEST_LAUNCH, true);

        if ((player->IsInAlliance() && player->GetQuestStatus(QUEST_THE_LEGION_RETURNS_A) == QUEST_STATUS_REWARDED) || (player->IsInHorde() && player->GetQuestStatus(QUEST_THE_LEGION_RETURNS_H) == QUEST_STATUS_REWARDED))
        {
            if (player->GetQuestStatus(QUEST_BLINK_OF_AN_EYE) == QUEST_STATUS_NONE)
            {
                Conversation::CreateConversation(CONVERSATION_KHADGAR_BLINK_OF_EYE, player, player->GetPosition(), { player->GetGUID() });

                if (const Quest* quest = sObjectMgr->GetQuestTemplate(QUEST_BLINK_OF_AN_EYE))
                    player->AddQuest(quest, nullptr);
            }
        }

        if (player->GetQuestStatus(QUEST_BLINK_OF_AN_EYE) == QUEST_STATUS_REWARDED)
            HandleGetAfterBlinkOfAnEye(player);
    }

    void OnQuestComplete(Player* player, Quest const* quest)
    {
        if (quest->GetQuestId() == QUEST_BLINK_OF_AN_EYE)
            HandleGetAfterBlinkOfAnEye(player);
    }

    void HandleGetAfterBlinkOfAnEye(Player* player)
    {
        if (player->GetQuestStatus(QUEST_KHADGARS_DISCOVERY) == QUEST_STATUS_NONE)
            if (const Quest* quest = sObjectMgr->GetQuestTemplate(QUEST_KHADGARS_DISCOVERY))
                player->AddQuest(quest, nullptr);

        if (uint32 artifact_knowledge = sWorld->getIntConfig(CONFIG_CURRENCY_START_ARTIFACT_KNOWLEDGE))
        {
            if (artifact_knowledge > 10)
                player->CompletedAchievement(10852);
            if (artifact_knowledge > 24)
                player->CompletedAchievement(10853);
        }

        player->CastSpell(player, player->IsInAlliance() ? SPELL_CREATE_CLASS_HALL_ALLIANCE : SPELL_CREATE_CLASS_HALL_HORDE, true);
        HandleGetFirstFollower(player);
    }

    void HandleGetFirstFollower(Player* player)
    {
        if (player->GetQuestStatus(QUEST_BLINK_OF_AN_EYE) == QUEST_STATUS_COMPLETE || player->GetQuestStatus(QUEST_BLINK_OF_AN_EYE) == QUEST_STATUS_REWARDED)
        {
            switch (player->getClass())
            {
            case CLASS_WARRIOR:
                player->CastSpell(player, 198182, true);
                break;
            case CLASS_PALADIN:
                player->CastSpell(player, 181009, true);
                break;
            case CLASS_HUNTER:
                player->CastSpell(player, 203376, true);
                break;
            case CLASS_ROGUE:
                player->CastSpell(player, 235468, true);
                break;
            case CLASS_PRIEST:
                player->CastSpell(player, 219764, true);
                break;
            case CLASS_DEATH_KNIGHT:
                player->CastSpell(player, 191521, true);
                break;
            case CLASS_SHAMAN:
                player->CastSpell(player, 211710, true);
                break;
            case CLASS_MAGE:
                player->CastSpell(player, 217305, true);
                break;
            case CLASS_WARLOCK:
                player->CastSpell(player, 201163, true);
                break;
            case CLASS_MONK:
                player->CastSpell(player, 234265, true);
                break;
            case CLASS_DRUID:
                player->CastSpell(player, 210357, true);
                break;
            case CLASS_DEMON_HUNTER:
                player->CastSpell(player, player->IsInAlliance() ? 188249 : 215133, true);
                break;
            default:
                break;
            }
        }
        player->CastSpell(player, 164608, true);
    }
};

// TODO : this script is temp fix
// on HDB this is for lvl 45
class OnLegionArrival2 : public PlayerScript
{
public:
    OnLegionArrival2() : PlayerScript("OnLegionArrival2") { }

    enum
    {
        QUEST_UNITING_THE_ISLES = 43341,
        QUEST_ARMIES_OF_LEGIONFALL = 46730,
        QUEST_WHISPERS_OF_A_FRIGHTENED_WORLD = 46206,

        SPELL_LEGION_7_2_PATCH_QUEST_LAUNCH = 239624,
    };

    void OnLevelChanged(Player* player, uint8 oldLevel) override
    {
        if (oldLevel < 45 && player->getLevel() >= 45)
            Handle45Arrival(player);
    }

    void Handle45Arrival(Player* player)
    {
        if (player->GetQuestStatus(QUEST_UNITING_THE_ISLES) == QUEST_STATUS_NONE)
            if (const Quest* quest = sObjectMgr->GetQuestTemplate(QUEST_UNITING_THE_ISLES))
                player->AddQuest(quest, nullptr);
        if (player->GetQuestStatus(QUEST_ARMIES_OF_LEGIONFALL) == QUEST_STATUS_NONE)
            player->CastSpell(player, SPELL_LEGION_7_2_PATCH_QUEST_LAUNCH, true);
        //if (player->GetQuestStatus(QUEST_WHISPERS_OF_A_FRIGHTENED_WORLD) == QUEST_STATUS_NONE)
        //    player->CastSpell(player, SPELL_WHISPERS_OF_A_FRIGHTENED_WORLD_PUSH, true);
    }
};

// TODO : this script is temp fix,
// remove it when lordaeron battle is properly fixed
// on HDB this is for lvl 50
class OnBFAArrival : public PlayerScript
{
public:
    OnBFAArrival() : PlayerScript("OnBFAArrival") { }

    enum
    {
        QUEST_DYING_WORLD_A                 = 52946,
        QUEST_DYING_WORLD_H                 = 53028,

        SPELL_CREATE_WAR_CAMPAIGN_H         = 273381,
        SPELL_CREATE_WAR_CAMPAIGN_A         = 273382,

        CONVERSATION_MAGNI_DYING_WORLD      = 9316,
    };

    void OnLevelChanged(Player* player, uint8 oldLevel) override
    {
        if (oldLevel < 50 && player->getLevel() >= 50)
            HandleBFAStart(player);
    }

    void HandleBFAStart(Player* player)
    {
        if (player->GetQuestStatus(QUEST_DYING_WORLD_A) == QUEST_STATUS_NONE &&
            player->GetQuestStatus(QUEST_DYING_WORLD_H) == QUEST_STATUS_NONE)
        {
            player->CastSpell(player, player->IsInAlliance() ? SPELL_CREATE_WAR_CAMPAIGN_A : SPELL_CREATE_WAR_CAMPAIGN_H, true);

            Conversation::CreateConversation(CONVERSATION_MAGNI_DYING_WORLD, player, player->GetPosition(), { player->GetGUID() });

            if (const Quest* quest = sObjectMgr->GetQuestTemplate(player->IsInAlliance() ? QUEST_DYING_WORLD_A : QUEST_DYING_WORLD_H))
                player->AddQuest(quest, nullptr);
        }
    }
};

// TODO : this script is temp fix,
// remove it when bfa starting is properly fixed
// on HDB this is for lvl 50
class OnBFAArrival2 : public PlayerScript
{
public:
    OnBFAArrival2() : PlayerScript("OnBFAArrival2") { }

    int QUEST_WOLFSOFFENSIVE_A = 56031;
    int QUEST_WARCHIEFSORDER_H = 56030;

    void OnLevelChanged(Player* player, uint8 oldLevel) override
    {
        if (oldLevel < 50 && player->getLevel() >= 50)
            if (const Quest* quest = sObjectMgr->GetQuestTemplate(player->IsInAlliance() ? QUEST_WOLFSOFFENSIVE_A : QUEST_WARCHIEFSORDER_H))
                player->AddQuest(quest, nullptr);
    }
};

/* Worgen Running Wild spell */
class WorgenRunningWild : public PlayerScript
{
public:
    WorgenRunningWild() : PlayerScript("WorgenRunningWild") { }

    void OnLevelChanged(Player* player, uint8 /*oldLevel*/) override
    {
        if (player->getRace() == RACE_WORGEN && player->getLevel() == 20)
            player->CastSpell(player, SPELL_RUNNING_WILD_LEARN);
    }
};

/* Dual Wield fix */
class DualWieldFix : public PlayerScript
{
public:
    DualWieldFix() : PlayerScript("DualWieldFix") { }

    void OnLogin(Player* player, bool /* firstLogin */) override
    {
        if ((player->getClass() == CLASS_WARRIOR     ) ||
            (player->getClass() == CLASS_HUNTER      ) ||
            (player->getClass() == CLASS_ROGUE       ) ||
            (player->getClass() == CLASS_DEATH_KNIGHT) ||
            (player->getClass() == CLASS_MONK        ) ||
            (player->getClass() == CLASS_DEMON_HUNTER) )
            {
                if (!player->HasSpell(674))
                    player->LearnSpell(674, false, 0, true);
            }
    }

    void OnLevelChanged(Player* player, uint8 /*oldLevel*/) override
    {
        if ((player->getClass() == CLASS_WARRIOR     ) ||
            (player->getClass() == CLASS_HUNTER      ) ||
            (player->getClass() == CLASS_ROGUE       ) ||
            (player->getClass() == CLASS_DEATH_KNIGHT) ||
            (player->getClass() == CLASS_MONK        ) ||
            (player->getClass() == CLASS_DEMON_HUNTER) )
            {
                if (!player->HasSpell(674))
                    player->LearnSpell(674, false, 0, true);
            }
    }
};

/* tempfix for "you cant speak that language" */
class chat_temp_fix : public PlayerScript
{
public:
    chat_temp_fix() : PlayerScript("chat_temp_fix") { }

    void OnLogin(Player* player, bool /* firstLogin */) override
    {
        if ((player->GetTeam() == ALLIANCE) && (!player->HasSpell(668)))
        {
            player->LearnSpell(668, false, true);
        }
        else if ((player->GetTeam() == HORDE) && (!player->HasSpell(669)))
        {
            player->LearnSpell(669, false, true);
        }
    }
};

class HooaahcoreWelcome : public PlayerScript
{
public:
    HooaahcoreWelcome() : PlayerScript("HooaahcoreWelcome") { }

    void OnLogin(Player* player, bool) override
    {
        ChatHandler(player->GetSession()).SendSysMessage("HooaahCore official build");
    }
};

void AddSC_custom_player_script()
{
    RegisterPlayerScript(playerscript_recruiter);
    RegisterPlayerScript(OnLegionArrival);          // TEMP FIX! Quest 40519 and 43926 - "legion returns". remove it when legion start quests are properly fixed
    RegisterPlayerScript(OnLegionArrival2);         // TEMP FIX! Quest 43341 - "uniting the isles".
    RegisterPlayerScript(OnBFAArrival);             // TEMP FIX! remove it when lordaeron battle is properly fixed.
    RegisterPlayerScript(OnBFAArrival2);            // TEMP FIX! remove it when bfa starting is properly fixed.
    RegisterPlayerScript(WorgenRunningWild);
    RegisterPlayerScript(DualWieldFix);
    RegisterPlayerScript(chat_temp_fix);
    RegisterPlayerScript(HooaahcoreWelcome);
}
