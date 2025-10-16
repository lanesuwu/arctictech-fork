#include "Misc.h"
#include "../RageBot/Exploits.h"
#include "Prediction.h"
#include "../../Utils/Utils.h"
#include "../../SDK/Interfaces.h"
#include "../../SDK/Config.h"
#include "../../SDK/Globals.h"

void Miscellaneous::Clantag()
{
	static auto removed = false;

	if (config.misc.miscellaneous.clantag->get())
	{
		auto nci = EngineClient->GetNetChannelInfo();

		if (!nci)
			return;

		static auto time = -1;

		auto ticks = TIME_TO_TICKS(nci->GetAvgLatency(FLOW_OUTGOING)) + (float)GlobalVars->tickcount; //-V807
		auto intervals = 0.3f / GlobalVars->interval_per_tick;

		auto main_time = (int)(ticks / intervals) % 28;

		if (main_time != time && !ClientState->m_nChokedCommands)
		{
			auto tag = ("");

			switch (main_time)
			{
			case 0: tag = (""); break;
			case 1: tag = ("a"); break;
			case 2: tag = ("ar"); break;
			case 3: tag = ("arc");break;
			case 4: tag = ("arct");break;
			case 5: tag = ("arcti");break;
			case 6: tag = ("arctic");break;
			case 7: tag = ("arctict");break;
			case 8: tag = ("arcticte");break;
			case 9: tag = ("arctictec");break;
			case 10: tag = ("arctictech");break;
			case 11: tag = ("arctictech");break;
			case 12: tag = ("arctictech");break;
			case 13: tag = ("arctictech");break;
			case 14: tag = ("arctictec");break;
			case 15: tag = ("arcticte");break;
			case 16: tag = ("arctict");break;
			case 17: tag = ("arctic");break;
			case 18: tag = ("arcti");break;
			case 19: tag = ("arct");break;
			case 20: tag = ("arc");break;
			case 21: tag = ("ar");break;
			case 22: tag = ("a");break;
			case 23: tag = ("");break;
			}

			Utils::SetClantag(tag);
			time = main_time;
		}

		removed = false;
	}
}

void Miscellaneous::FastThrow() {
	static bool fast_throw_triggred = false;
	static int nLastButtons = 0;

	if (!ctx.active_weapon || !ctx.active_weapon->IsGrenade()) {
		fast_throw_triggred = false;
		return;
	}

	CBaseGrenade* grenade = reinterpret_cast<CBaseGrenade*>(ctx.active_weapon);

	if (!(ctx.cmd->buttons & (IN_ATTACK | IN_ATTACK2)) && (nLastButtons & (IN_ATTACK | IN_ATTACK2)) && grenade->m_bPinPulled())
		ctx.grenade_throw_tick = ctx.cmd->command_number;

	nLastButtons = ctx.cmd->buttons;

	if (!config.ragebot.aimbot.doubletap_options->get(3)) {
		fast_throw_triggred = false;
		return;
	}

	if (ctx.tickbase_shift > 0) {
		Exploits->LC_OverrideTickbase(ctx.tickbase_shift);

		float arm_time = max(Cheat.LocalPlayer->m_flNextAttack(), grenade->m_flNextPrimaryAttack());

		if (TICKS_TO_TIME(Cheat.LocalPlayer->m_nTickBase()) + TICKS_TO_TIME(ctx.tickbase_shift - 7) > arm_time)
			Exploits->LC_OverrideTickbase(7);

		if (grenade->m_flThrowTime() > 0.f)
			fast_throw_triggred = true;

		if (fast_throw_triggred)
			Exploits->LC_OverrideTickbase(0);
	}

	if (ctx.cmd->command_number == ctx.grenade_throw_tick + 8 && ctx.grenade_throw_tick != 0)
		ctx.switch_to_main_weapon = true;
}


void Miscellaneous::FastSwitch() {
	if (!ctx.switch_to_main_weapon)
		return;

	ctx.switch_to_main_weapon = false;
	CBaseCombatWeapon* best_weapon = nullptr;
	auto weapons = Cheat.LocalPlayer->m_hMyWeapons();
	int best_type = WEAPONTYPE_KNIFE;
	for (int i = 0; i < MAX_WEAPONS; i++) {
		auto weap = weapons[i];
		if (weap == INVALID_EHANDLE_INDEX)
			break;

		CBaseCombatWeapon* weapon = reinterpret_cast<CBaseCombatWeapon*>(EntityList->GetClientEntityFromHandle(weap));

		if (!weapon)
			continue;

		CCSWeaponData* weap_info = weapon->GetWeaponInfo();

		if (!weap_info)
			continue;

		if (weap_info->nWeaponType >= WEAPONTYPE_SUBMACHINEGUN && weap_info->nWeaponType <= WEAPONTYPE_MACHINEGUN) {
			best_weapon = weapon;
			best_type = weap_info->nWeaponType;
		}
		else if (weap_info->nWeaponType == WEAPONTYPE_PISTOL && best_type == WEAPONTYPE_KNIFE) {
			best_weapon = weapon;
			best_type = weap_info->nWeaponType;
		}
	}

	if (best_weapon)
		ctx.cmd->weaponselect = best_weapon->EntIndex();
}

void Miscellaneous::AutomaticGrenadeRelease() {
	static bool prev_release = false;
	static Vector on_release_move;
	static QAngle on_release_angle;

	if (ctx.should_release_grenade && ctx.active_weapon && ctx.active_weapon->IsGrenade()) {
		if (!prev_release) {
			on_release_move = Vector(ctx.cmd->sidemove, ctx.cmd->forwardmove);
			on_release_angle = ctx.grenade_release_angle;
		}

		ctx.cmd->buttons &= ~(IN_ATTACK | IN_ATTACK2);

		if (ctx.cmd->command_number <= ctx.grenade_throw_tick + 7) {
			ctx.cmd->sidemove = on_release_move.x;
			ctx.cmd->forwardmove = on_release_move.y;
			ctx.cmd->viewangles = on_release_angle;
		}
	}
	else if (!ctx.active_weapon || !ctx.active_weapon->IsGrenade()) {
		ctx.should_release_grenade = false;
	}

	prev_release = ctx.should_release_grenade;
}

static bool s_ShouldClearNotices = false;
void Miscellaneous::PreserveKillfeed() {
	if (!Cheat.InGame || !Cheat.LocalPlayer)
		return;

	static auto spawntime = 0.f;
	static auto status = false;
	static auto clear_deathnotices = reinterpret_cast<void(__thiscall*)(uintptr_t*)>(Utils::PatternScan("client.dll", "55 8B EC 83 EC 0C 53 56 8B 71 58"));

	auto set = false;
	if (spawntime != Cheat.LocalPlayer->m_flSpawnTime() || status != config.visuals.effects.preserve_killfeed->get())
	{
		set = true;
		status = config.visuals.effects.preserve_killfeed->get();
		spawntime = Cheat.LocalPlayer->m_flSpawnTime();
	}

	const auto hud_radar = CSGOHud->FindHudElement("CCSGO_HudRadar");
	const auto death_notice = reinterpret_cast<uintptr_t>(CSGOHud->FindHudElement("CCSGO_HudDeathNotice"));
	if (death_notice == 20)
		return;

	const auto notice_element = reinterpret_cast<uintptr_t*>(death_notice - 0x14);
	if (!death_notice || !notice_element)
		return;

	if (set) {
		const auto lifetime = reinterpret_cast<float*>(death_notice + 0x50);
		*lifetime = status ? FLT_MAX : 1.5f;
	}

	if (s_ShouldClearNotices) {
		s_ShouldClearNotices = false;
		clear_deathnotices(notice_element);
	}
}

void Miscellaneous::ClearKillfeed() {
	s_ShouldClearNotices = true;
}

void Miscellaneous::RadarAngles() {
	if (!Cheat.InGame)
		return;

	static const auto hud_radar = reinterpret_cast<CSGO_HudRadar*>(CSGOHud->FindHudElement("CCSGO_HudRadar"));

	if (!hud_radar)
		return;

	hud_radar->m_vecLocalAngles.y = 0.f;
}