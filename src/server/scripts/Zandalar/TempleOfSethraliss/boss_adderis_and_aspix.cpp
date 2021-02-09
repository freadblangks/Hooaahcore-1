#include "ScriptMgr.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "temple_of_sethraliss.h"

enum Events {
    EVENT_CONDUCTION = 1,
    EVENT_STATIC_SHOCK = 2,
    EVENT_GALE_FORCE = 3,
    EVENT_ARCING_BLADE = 4,
    EVENT_CYCLONE_STRIKE = 5,
    EVENT_PEARL_OF_THUNDER = 6,
    EVENT_ARC_DASH = 7,
    EVENT_LIGHTNING_SHIELD_ASPIX = 8,
    EVENT_CHECK_ENERGY = 9,
    EVENT_JOLT,
    EVENT_GUST
};

enum Actions {
    ACTION_SHIELD_ASPIX = 1,
    ACTION_SHIELD_ADDERIS = 2,
};

enum Spells {
    SPELL_LIGHTNING_SHIELD_ASPIX = 273411,
    SPELL_LIGHTNING_SHIELD_AURA = 263246,
    SPELL_STATIC_SHOCK = 263257,
    SPELL_JOLT = 263318,
    SPELL_CONDUCTION = 263371,
    SPELL_GUST = 263775,
    SPELL_CYCLONE_STRIKE = 261773,
    SPELL_ARCING_BLADE = 263234,
    SPELL_A_PEAR_OF_THUNDER = 263365
};

//133379
struct boss_aspix : public BossAI
{
    boss_aspix(Creature* creature) : BossAI(creature, DATA_ADDERIS_AND_ASPIX) { }

    void EnterCombat(Unit* who) override
    {
        BossAI::EnterCombat(who);
        events.ScheduleEvent(EVENT_JOLT, 3s);
        events.ScheduleEvent(EVENT_CONDUCTION, 8s);
        events.ScheduleEvent(EVENT_GUST, 13s);
        events.ScheduleEvent(EVENT_STATIC_SHOCK, 30s);
    }

    void Reset() override
    {
        me->CastSpell(nullptr, 274385);
        me->SetPower(POWER_ALL, 0);
        events.ScheduleEvent(EVENT_LIGHTNING_SHIELD_ASPIX, 1);
        if (Creature* adderis = me->FindNearestCreature(NPC_ADDERIS, 100, false))
        {
            adderis->Respawn(true);
            adderis->NearTeleportTo(adderis->GetHomePosition());
        }
        me->RemoveAura(SPELL_LIGHTNING_SHIELD_AURA);
        BossAI::Reset();
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
            case EVENT_CHECK_ENERGY:
                if (me->GetPower(POWER_ALL) == 100)
                {
                    me->CastSpell(me, SPELL_STATIC_SHOCK);
                    me->RemoveAura(SPELL_LIGHTNING_SHIELD_AURA);
                    if (Unit* adderis = me->FindNearestCreature(NPC_ADDERIS, 50, true))
                        adderis->GetAI()->DoAction(ACTION_SHIELD_ADDERIS);
                    break;
                }
                events.ScheduleEvent(EVENT_CHECK_ENERGY, 100ms);

            case EVENT_LIGHTNING_SHIELD_ASPIX:
                DoCast(SPELL_LIGHTNING_SHIELD_ASPIX);
                // triggered spells target X: 3334.772 Y: 3149.792 Z: 103.2884, X: 3330.362 Y: 3159.94 Z: 97.75124, X: 3332.996 Y: 3146.063 Z: 102.2195, X: 3346.23 Y: 3138.96 Z: 92.39861, X: 3338.792 Y: 3155.545 Z: 102.52,
                break;

            case EVENT_JOLT:
                DoCastRandom(SPELL_JOLT, 100.0f, false);
                events.Repeat(3s);

            case EVENT_CONDUCTION:
                DoCastRandom(SPELL_CONDUCTION, 100.0f, false);
                events.Repeat(15s);
                break;

            case EVENT_STATIC_SHOCK:
                me->SetPower(POWER_ALL, 100);
                if (me->GetPower(POWER_ALL) == 100)
                    DoCastAOE(SPELL_STATIC_SHOCK);
                break;

            case EVENT_GUST:
                DoCastRandom(SPELL_GUST, 100.0f, false);
                events.Repeat(6s);
                break;

            default:
                break;
            }
        }
        DoMeleeAttackIfReady();
    }

    void DoAction(int32 param) override
    {
        if (param == ACTION_SHIELD_ASPIX)
        {
            me->CastSpell(me, SPELL_LIGHTNING_SHIELD_ASPIX);
        }
    }
};

//133944
struct boss_adderis : public BossAI
{
    boss_adderis(Creature* creature) : BossAI(creature, DATA_ADDERIS_AND_ASPIX) { }

    void EnterCombat(Unit* who) override
    {
        BossAI::EnterCombat(who);
        events.ScheduleEvent(EVENT_ARCING_BLADE, 6s);
        events.ScheduleEvent(EVENT_CYCLONE_STRIKE, 8s);
        events.ScheduleEvent(EVENT_ARC_DASH, 18s);
    }

    void Reset() override
    {
        me->SetPower(POWER_ALL, 0);
        if (Creature* aspix = me->FindNearestCreature(NPC_ASPIX, 100, false))
        {
            aspix->Respawn(true);
            aspix->NearTeleportTo(aspix->GetHomePosition());
        }
        me->RemoveAura(SPELL_LIGHTNING_SHIELD_AURA);
        BossAI::Reset();
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        DoMeleeAttackIfReady();

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {

            case EVENT_CYCLONE_STRIKE:
                DoCastRandom(SPELL_CYCLONE_STRIKE, 60.f, false);
                events.Repeat(3s);
                break;

            case EVENT_ARCING_BLADE:
                 DoCastAOE(SPELL_ARCING_BLADE);
                 events.Repeat(15s);
                 break;

            case EVENT_ARC_DASH:
                 events.Repeat(20s);
                 break;

            case EVENT_PEARL_OF_THUNDER:
                 DoCastAOE(SPELL_A_PEAR_OF_THUNDER);
                 events.Repeat(25s);
                 break;
            }
        }
    }

    void JustDied(Unit* /*killer*/) override
    {
        if (auto* AdderisAspixDoor = me->FindNearestGameObject(GO_ADDERIS_ASPIX_EXIT, 100.0f))
            AdderisAspixDoor->SetGoState(GO_STATE_ACTIVE);
    }
};

//273411
class spell_lightning_shield : public AuraScript
{
    PrepareAuraScript(spell_lightning_shield);

    void OnRemove(AuraEffect const* /*&aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (Unit* caster = GetCaster())
            caster->AddAura(SPELL_LIGHTNING_SHIELD_AURA);
    }

    void Register() override
    {
        AfterEffectRemove += AuraEffectRemoveFn(spell_lightning_shield::OnRemove, EFFECT_1, SPELL_AURA_PERIODIC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
    }
};

void AddSC_boss_adderis_and_aspix()
{
    RegisterCreatureAI(boss_aspix);
    RegisterCreatureAI(boss_adderis);
    RegisterAuraScript(spell_lightning_shield);
}
