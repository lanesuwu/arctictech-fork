#include "CBaseCombatWeapon.h"
#include "CBasePlayer.h"
#include "../../SDK/Globals.h"
#include "../../Utils/Utils.h"
#include <string>
#include "../../Features/Misc/Prediction.h"
#include "../../Features/Visuals/WeaponIcons.h"
#include "../../Features/RageBot/Exploits.h"


std::string CCSWeaponData::GetName() {
	const wchar_t* name = Localize->FindSafe(szHudName);
	return Localize->utf16le_to_utf8(name);
}

CCSWeaponData* CBaseCombatWeapon::GetWeaponInfo() {
	if (!this)
		return nullptr;

	return WeaponSystem->GetWeaponData(m_iItemDefinitionIndex());
}

std::string CBaseCombatWeapon::GetName(CCSWeaponData* data) {
	if (!data)
		data = GetWeaponInfo();

	if (!data)
		return "";

	const wchar_t* name = Localize->FindSafe(data->szHudName);
	std::string s = Localize->utf16le_to_utf8(name);
	std::transform(s.begin(), s.end(), s.begin(), ::toupper);
	return s;
}

bool CBaseCombatWeapon::CanShoot(bool revolver_check) {
	CCSWeaponData* data = GetWeaponInfo();

	if (!data)
		return false;

	CBasePlayer* owner = (CBasePlayer*)EntityList->GetClientEntityFromHandle(m_hOwner());

	if (!owner)
		return false;

	if (GameRules()->IsFreezePeriod())
		return false;

	if (owner->m_fFlags() & FL_FROZEN)
		return false;

	if (data->iMaxClip1 > 0 && m_iClip() <= 0)
		return false;

	if (owner->m_iShotsFired() > 0 && !data->bFullAuto)
		return false;

	int tick_base = owner->m_nTickBase();
	if (owner == Cheat.LocalPlayer)
		tick_base = ctx.corrected_tickbase;

	const float cur_time = TICKS_TO_TIME(tick_base);

	if (revolver_check && m_iItemDefinitionIndex() == Revolver && m_flPostponeFireReadyTime() > TICKS_TO_TIME(owner->m_nTickBase()))
		return false;

	if (cur_time < m_flNextPrimaryAttack() || cur_time < owner->m_flNextAttack())
		return false;

	return true;
}

bool CBaseCombatWeapon::ThrowingGrenade() {
	if (!IsGrenade())
		return false;

	auto grenade = reinterpret_cast<CBaseGrenade*>(this);

	CBasePlayer* owner = (CBasePlayer*)EntityList->GetClientEntityFromHandle(m_hOwner());

	if (!owner)
		return false;

	float throw_time = grenade->m_flThrowTime();

	if (owner == Cheat.LocalPlayer && ctx.tickbase_shift > 0) {
		if (EnginePrediction->pre_prediction.m_fThrowTime > 0.f || throw_time > 0.f)
			return true;

		int throw_offset = ctx.cmd->command_number - ctx.grenade_throw_tick;

		if (throw_offset >= 0 && throw_offset <= 8)
			return true;
	}

	if (throw_time <= 0.f)
		return false;

	return throw_time < TICKS_TO_TIME(owner->m_nTickBase());
}

DXImage& CBaseCombatWeapon::GetIcon() {
	return WeaponIcons->GetIcon(m_iItemDefinitionIndex());
}