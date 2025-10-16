#pragma once

#include "../../SDK/Misc/CBaseEntity.h"
#include "../../SDK/Misc/CBasePlayer.h"
#include "../../SDK/Misc/CBaseCombatWeapon.h"
#include <vector>
#include <unordered_map>
#include <queue>

class GrenadePrediction {
private:
	CNewParticleEffect* pMolotovParticle = nullptr;
	Vector lerped_velocity;

protected:
	int nTick = 0;
	int m_nLastUpdateTick;
	int nextThinkTick = 0.f;
	float flDetonateTime = 60.f;
	std::vector<Vector> pathPoints;
	std::vector<Vector> collisionPoints;
	int weaponId = 0;
	bool detonate;
	Vector vecVelocity;
	Vector vecOrigin;
	Vector vecDetonate;
	bool runningPrediction;
	float flThrowTime;
	float flExpireTime;
	int nCollisionGroup;
	CBasePlayer* owner;
	CBaseEntity* pLastHitEntity;
	std::vector<CBaseEntity*> broken_breakables;
	int nBouncesCount;

public:
	static void PrecacheParticles();

	void Start();
	void Draw();
	void Predict(const Vector& orign, const Vector& velocity, float throwTime, int offset);
	bool PhysicsSimulate();
	void Think();
	void TraceHull(Vector& src, Vector& end, uint32_t mask, void* ignore, int collisionGroup, CGameTrace& trace);
	void TraceLine(Vector& src, Vector& end, uint32_t mask, void* ignore, int collisionGroup, CGameTrace& trace);
	void ClearTrace(CGameTrace& trace);
	void PhysicsTraceEntity(Vector& vecSrc, Vector& vecDst, uint32_t nMask, CGameTrace& pTrace);
	void PhysicsPushEntity(Vector& vecPush, CGameTrace& pTrace);
	void PerformFlyCollisionResolution(CGameTrace& pTrace);
	int  CalcDamage(Vector pos, CBasePlayer* target, CBaseEntity* skip = nullptr);
};

struct GrenadeWarningData_t {
	std::vector<Vector> path;
	float flThrowTime = 0.f;
	float flLastUpdate = 0.f;
};

struct ThrownGrenade_t {
	unsigned int m_hThrower = 0;
	float flThrowTime = 0.f;
	int grenadeType = 0;
};

class GrenadeWarning : public GrenadePrediction {
	std::unordered_map<unsigned int, GrenadeWarningData_t> grenade_warnings;
	std::vector<ThrownGrenade_t> thrown_grenades;

public:
	std::queue<Beam_t*> beams_to_free;

	static void Setup();
	void RenderBeam(const std::vector<Vector> points, Color clr, int segments = 2);
	void Precache();
	void ClearBeams();

	void Warning(CBaseGrenade* entity, int weapId);
	void OnEvent(IGameEvent* event);
	void RenderPaths();

	void Reset() { grenade_warnings.clear(); };
};

extern GrenadeWarning* NadeWarning;

inline float CSGO_Armor(float flDamage, int ArmorValue) {
	float flArmorRatio = 0.5f;
	float flArmorBonus = 0.5f;
	if (ArmorValue > 0) {
		float flNew = flDamage * flArmorRatio;
		float flArmor = (flDamage - flNew) * flArmorBonus;

		if (flArmor > static_cast<float>(ArmorValue)) {
			flArmor = static_cast<float>(ArmorValue) * (1.f / flArmorBonus);
			flNew = flDamage - flArmor;
		}

		flDamage = flNew;
	}
	return flDamage;
}

float CalculateThrowYaw(const Vector& wish_dir, const Vector& vel, float throw_velocity, float throw_strength);
float CalculateThrowPitch(float pitch, const Vector& vel, float throw_velocity, float throw_strength);