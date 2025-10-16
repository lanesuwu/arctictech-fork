#include "CBaseEntity.h"
#include "../../Utils/VitualFunction.h"
#include "../../Utils/Utils.h"
#include "../Interfaces.h"

bool CBaseEntity::IsPlayer() {
	return CallVFunction<bool(__thiscall*)(CBaseEntity*)>(this, 158)(this);
}

bool CBaseEntity::IsWeapon() {
	return CallVFunction<bool(__thiscall*)(CBaseEntity*)>(this, 166)(this);
}

CBaseEntity* CBaseEntity::m_pMoveParent() {
	static auto _m_pMoveParent = *(uintptr_t*)Utils::PatternScan("client.dll", "66 90 8B 86 ? ? ? ? 83 F8 FF 74 1A 0F B7 C8 C1 E1 04", 0x4);

	return EntityList->GetClientEntityFromHandle(*(unsigned int*)(this + _m_pMoveParent));
}

ClientClass* CBaseEntity::GetClientClass() {
	IClientNetworkable* clientNetworkable = EntityList->GetClientNetworkable(EntIndex());

	if (!clientNetworkable)
		return nullptr;

	return clientNetworkable->GetClientClass();
}

IClientNetworkable* CBaseEntity::GetClientNetworkable() {
	return EntityList->GetClientNetworkable(EntIndex());
}

IClientUnknown* CBaseEntity::GetClientUnknown() {
	return reinterpret_cast<IClientUnknown*>(this);
}

ICollideable* CBaseEntity::GetCollideable() {
	auto unk = GetClientUnknown();
	if (!unk)
		return nullptr;

	return unk->GetCollideable();
}

CBaseEntity* CBaseEntity::GetShadowParent() {
	auto renderable = GetClientRenderable();

	if (!renderable)
		return nullptr;

	auto unknown = renderable->GetIClientUnknown();

	if (!unknown)
		return nullptr;

	return unknown->GetBaseEntity();
}

const model_t* CBaseEntity::GetModel() {
	void* pClientRenderable = (void*)(this + 0x4);
	typedef model_t* (__thiscall* fnGetModel)(void*);
	return CallVFunction<fnGetModel>(pClientRenderable, 8)(pClientRenderable);
}

Vector CBaseEntity::GetAbsOrigin() {
    return *reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(this) + 0xA0);

	//if (!this)
	//	return Vector();

	//return *CallVFunction<Vector*(__thiscall*)(CBaseEntity*)>(this, 10)(this);
}

QAngle CBaseEntity::GetAbsAngles() {
    return *reinterpret_cast<QAngle*>(reinterpret_cast<uintptr_t>(this) + 0xC4);

	//if (!this)
	//	return QAngle();

	//return *CallVFunction<QAngle*(__thiscall*)(CBaseEntity*)>(this, 11)(this);
}

Vector CBaseEntity::GetWorldPosition() {
	return m_vecOrigin() + (m_vecMins() + m_vecMaxs()) * 0.5f;
}

bool CBaseEntity::IsBreakable() {
	static auto _isBreakalbe = (bool(__thiscall*)(CBaseEntity*))Utils::PatternScan("client.dll", "55 8B EC 51 56 8B F1 85 F6 74 68");

	auto v1 = GetClientClass();
	if (!v1)
		return false;

	auto v2 = v1->m_ClassID;
	if (v2 == C_BASE_BUTTON || v2 == C_PHYSICS_PROP)
		return false;

	auto v3 = (int)v1->m_pNetworkName;
	if (*(DWORD*)v3 == 0x65724243 && *(DWORD*)(v3 + 7) == 0x53656C62) // intact window
		return true;

	if (*(DWORD*)v3 == 0x73614243 && *(DWORD*)(v3 + 7) == 0x79746974) // broken window
		return true;

	return _isBreakalbe(this);
}

datamap_t* CBaseEntity::GetPredDescMap() {
	return CallVFunction<datamap_t* (__thiscall*)(CBaseEntity*)>(this, 17)(this);
}

void CBaseEntity::SetAbsAngles(QAngle angles) {
	static auto setAbsAngles = (void(__thiscall*)(CBaseEntity*, const QAngle*))Utils::PatternScan("client.dll", "55 8B EC 83 E4 F8 83 EC 64 53 56 57 8B F1 E8");

	setAbsAngles(this, &angles);
}

void CBaseEntity::SetAbsOrigin(Vector origin) {
	static auto setAbsAngles = (void(__thiscall*)(CBaseEntity*, const Vector*))Utils::PatternScan("client.dll", "55 8B EC 83 E4 F8 51 53 56 57 8B F1 E8");

	setAbsAngles(this, &origin);
}

void CBaseEntity::SetCollisionBounds(Vector mins, Vector maxs) {
	static auto setCollisionBounds = reinterpret_cast<void(__thiscall*)(ICollideable*, const Vector*, const Vector*)>(Utils::PatternScan("client.dll", "53 8B DC 83 EC 08 83 E4 F8 83 C4 04 55 8B 6B 04 89 6C 24 04 8B EC 83 EC 18 56 57 8B 7B"));

	auto collidable = GetCollideable();

	if (!collidable)
		return;

	setCollisionBounds(collidable, &mins, &maxs);
}

CBasePlayer* CBaseGrenade::GetThrower() {
	CBasePlayer* result = reinterpret_cast<CBasePlayer*>(EntityList->GetClientEntityFromHandle(m_hThrower()));

	if (!result)
		result = reinterpret_cast<CBasePlayer*>(EntityList->GetClientEntityFromHandle(m_hOwnerEntity()));

	return result;
}