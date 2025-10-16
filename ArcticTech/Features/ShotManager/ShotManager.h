#pragma once

#include <vector>
#include <string>

#include "../../SDK/Misc/Vector.h"
#include "../../SDK/Misc/QAngle.h"


class CBasePlayer;
class IGameEvent;
struct LagRecord;

struct PlayerHurt_t {
	CBasePlayer* victim;
	int damagegroup = -1;
};

struct RegisteredShot_t {
	// client info
	Vector client_shoot_pos;
	Vector target_pos;
	QAngle client_angle;
	int command_number;
	int wanted_damage;
	int wanted_damagegroup;
	float hitchance = 0;
	int backtrack = 0;
	LagRecord* record;
	QAngle player_angle; // for building correct matrix
	bool safe_point = false;
	std::vector<Vector> client_impacts;

	// acked info
	Vector shoot_pos;
	Vector end_pos;
	QAngle angle;
	std::vector<Vector> impacts;
	int damage = 0;
	int	damagegroup = -1;
	Vector hit_point;
	int health = 0;

	bool unregistered = false;
	bool death = false;
	bool player_death = false;
	bool acked = false;
	bool recieved_events = false;
	std::string miss_reason;
};

class CBaseCombatWeapon;
class CShotManager {
	std::vector<RegisteredShot_t>	m_RegisteredShots;

	void LogMiss(RegisteredShot_t* shot);

public:
	void	DetectUnregisteredShots();
	void	ProcessManualShot();
	void	OnNetUpdate();
	bool	OnEvent(IGameEvent* event);
	void	AddShot(const Vector& shoot_pos, const Vector& target_pos, int damage, int damagegroup, float hitchance, bool safe, LagRecord* record, Vector impacts[], int total_impacts);
	void	Reset();
};

extern CShotManager* ShotManager;