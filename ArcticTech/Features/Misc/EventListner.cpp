#include "EventListner.h"

#include "../../SDK/Interfaces.h"
#include "../../SDK/Globals.h"
#include "../../SDK/Misc/fnv1a.h"
#include "../../Utils/Console.h"
#include "../RageBot/Exploits.h"
#include "../RageBot/AnimationSystem.h"
#include "../RageBot/LagCompensation.h"
#include "../RageBot/Ragebot.h"
#include "../Visuals/GrenadePrediction.h"
#include "../AntiAim/AntiAim.h"
#include "../Visuals/ESP.h"
#include "../Visuals/Chams.h"
#include "../ShotManager/ShotManager.h"
#include "../Lua/Bridge/Bridge.h"
#include "Misc.h"
#include "AutoPeek.h"

CEventListner* EventListner = new CEventListner;

static std::vector<const char*> s_RgisterEvents = {
	"player_hurt",
	"player_death",
	"player_spawned",
	"player_disconnect",
	"bomb_planted",
	"bomb_defused",
	"bomb_begindefuse",
	"bomb_beginplant",
	"bomb_abortplant",
	"bomb_abortdefuse",
	"bomb_exploded",
	"round_start",
	"round_end",
	"item_purchase",
	"round_freeze_end",
	"bullet_impact",
	"item_equip",
	"grenade_thrown"
};

void CEventListner::FireGameEvent(IGameEvent* event) {
	bool skip_hurt = ShotManager->OnEvent(event);

	int local_id = EngineClient->GetLocalPlayer();
	int user_id_pl = EngineClient->GetPlayerForUserID(event->GetInt("userid"));

	auto name_hash = FNV1a(event->GetName());

	switch (name_hash) {
		case FNV1a("player_hurt"): {
			CBasePlayer* victim = reinterpret_cast<CBasePlayer*>(EntityList->GetClientEntity(user_id_pl));

			if (EngineClient->GetPlayerForUserID(event->GetInt("attacker")) == local_id) {
				if (config.visuals.esp.hitsound->get())
					EngineClient->ExecuteClientCmd("play buttons/arena_switch_press_02.wav");

				if (!skip_hurt) {
					if (config.visuals.esp.damage_marker->get())
						WorldESP->AddDamageMarker(victim->m_vecOrigin() + Vector(0, 0, 80), event->GetInt("dmg_health"));
				}

				if (config.misc.miscellaneous.logs->get(0) && !skip_hurt) {
					std::string action = "hit";
					std::string weapon = event->GetString("weapon");
					if (weapon == "taser")
						action = "zeused";
					else if (weapon == "hegrenade")
						action = "naded";
					else if (weapon == "inferno")
						action = "mollied";
					else if (weapon == "knife")
						action = "knifed";
					std::string hitbox;
					int hb = event->GetInt("hitgroup");
					if (action == "hit" && hb != HITGROUP_GENERIC)
						hitbox = " in the \aACCENT" + GetHitgroupName(hb) + "\a_MAIN_";

					Console->ArcticTag();
					Console->Print(std::format("{} \aACCENT{}\a_MAIN_{} for \aACCENT{}\a_MAIN_ damage (\aACCENT{}\a_MAIN_ remaining)\n", action, victim->GetName(), hitbox, event->GetInt("dmg_health"), event->GetInt("health")));
				}
			}

			if (victim && victim->m_bDormant()) {
				victim->m_iHealth() = event->GetInt("health");
				WorldESP->GetESPInfo(user_id_pl).m_nHealth = victim->m_iHealth();
			}

			break;
		}
		case FNV1a("player_death"): {
			if (user_id_pl == local_id) {
				ctx.reset();
				Exploits->target_tickbase_shift = 0;
				ctx.tickbase_shift = 0;
				AutoPeek->Disable();
				ctx.no_fakeduck = false;
			}

			auto& esp_info = WorldESP->GetESPInfo(user_id_pl);

			Resolver->Reset((CBasePlayer*)EntityList->GetClientEntity(user_id_pl));
			AnimationSystem->InvalidateInterpolation(user_id_pl);
			LagCompensation->Invalidate(user_id_pl);
			esp_info.m_flLastUpdateTime = 0.f;
			esp_info.m_nHealth = 0;
			Ragebot->CalcSpreadValues(); // maybe we got bad values previously?

			break;
		}
		case FNV1a("round_start"): {
			LagCompensation->Reset();
			ctx.should_buy = true;
			ctx.planting_bomb = false;

			//Utils::ForceFullUpdate();

			AntiAim->ResetManual();
			Miscellaneous::ClearKillfeed();
			NadeWarning->Reset();

			for (int i = 0; i < ClientState->m_nMaxClients; i++)
				WorldESP->GetESPInfo(i).m_nHealth = 100;

			break;
		}
		case FNV1a("bullet_impact"): {
			if (user_id_pl == local_id && config.visuals.effects.server_impacts->get() && Cheat.LocalPlayer) {
				Vector pos = Vector(event->GetFloat("x"), event->GetFloat("y"), event->GetFloat("z"));
				Color col = config.visuals.effects.server_impacts_color->get();
				DebugOverlay->AddBox(pos, Vector(-1, -1, -1), Vector(1, 1, 1), col, config.visuals.effects.impacts_duration->get());
			}
			break;
		}
		case FNV1a("item_equip"): {
			CBasePlayer* player = reinterpret_cast<CBasePlayer*>(EntityList->GetClientEntity(user_id_pl));

			if (player && player->m_bDormant()) {
				auto& esp_info = WorldESP->GetESPInfo(user_id_pl);
				esp_info.m_iActiveWeapon = event->GetInt("defindex");
			}

			if (player == Cheat.LocalPlayer)
				Ragebot->UpdateUI(event->GetInt("defindex"));

			break;
		}
		case FNV1a("item_purchase"): {
			CBasePlayer* player = reinterpret_cast<CBasePlayer*>(EntityList->GetClientEntity(user_id_pl));
			std::string item = event->GetString("weapon");

			if (config.misc.miscellaneous.logs->get(2) && player) {
				Console->ArcticTag();
				Console->Print(std::format("\aACCENT{} \a_MAIN_bought\aACCENT {}\n", player->GetName(), item));
			}

			if (player && player->m_bDormant()) {

				if (item == "vest")
					player->m_ArmorValue() = 100;
				else if (item == "vesthelm")
					player->m_bHasHelmet() = true;
			}

			break;
		}
		case FNV1a("player_disconnect"): {
			Chams->RemoveShotChams(user_id_pl);
			LagCompensation->Reset(user_id_pl);
			WorldESP->GetESPInfo(user_id_pl).reset();
			AnimationSystem->InvalidateInterpolation(user_id_pl);
			break;
		}
		case FNV1a("bomb_beginplant"): {
			if (user_id_pl == local_id)
				ctx.planting_bomb = true;

			break;
		}
		case FNV1a("bomb_abortplant"): {
			if (user_id_pl == local_id)
				ctx.planting_bomb = false;

			break;
		}
		case FNV1a("bomb_planted"): {
			ctx.planting_bomb = false;
			break;
		}
		case FNV1a("grenade_thrown"): {
			NadeWarning->OnEvent(event);

			if (user_id_pl == local_id)
				ctx.switch_to_main_weapon = true;
		}
	}

	for (auto& func : Lua->hooks.getHooks(LUA_GAMEEVENTS))
		func.func(event);
}

int CEventListner::GetEventDebugID() {
	return 42;
}

void CEventListner::Register() {
	m_iDebugId = 42;

	for (auto name : s_RgisterEvents) {
		GameEventManager->AddListener(this, name, false);
	}
}

void CEventListner::Unregister() {
	GameEventManager->RemoveListener(this);
}