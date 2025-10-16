#pragma once

#include <deque>

#include "../../SDK/Misc/CBasePlayer.h"

struct LagRecord;

enum class R_PlayerState {
	STANDING,
	MOVING,
	AIR
};

enum class R_AntiAimType {
	NONE,
	STATIC,
	JITTER,
	UNKNOWN,
};

enum class ResolverType {
	NONE,
	FREESTAND,
	LOGIC,
	ANIM,
	BRUTEFORCE,
	MEMORY,
	DEFAULT,
};

struct ResolverLayer_t {
	float desync = 0.f;
	float delta = 0.f;
};

#define RESOLVER_DEBUG 1

struct ResolverData_t {
	R_PlayerState player_state = R_PlayerState::STANDING;
	R_AntiAimType antiaim_type = R_AntiAimType::UNKNOWN;
	ResolverType resolver_type = ResolverType::NONE;

	ResolverLayer_t layers[5];

	float max_desync_delta = 0.f;
	int side = 0;
};

struct ResolverDataStatic_t {
	int brute_side = 0;
	float brute_time = 0.f;
	float last_resolved = 0.f;
	ResolverType res_type_last = ResolverType::NONE;
	int last_side = 0;
	int missed_shots = 0;

	void reset() {
		last_resolved = 0.f;
		brute_side = 0;
		brute_time = 0.f;
	}
};

class CResolver {
	ResolverDataStatic_t resolver_data[64];

	float GetTime();
public:
	CResolver() {
		for (int i = 0; i < 64; ++i) {
			resolver_data[i].reset();
		}
	}	

	void			Reset(CBasePlayer* pl = nullptr);

	R_PlayerState	DetectPlayerState(CBasePlayer* player, AnimationLayer* animlayers);
	R_AntiAimType	DetectAntiAim(CBasePlayer* player, const std::deque<LagRecord>& records);

	void			SetupLayer(LagRecord* record, int idx, float delta);
	void			SetupResolverLayers(CBasePlayer* player, LagRecord* record);

	void			DetectFreestand(CBasePlayer* player, LagRecord* record, const std::deque<LagRecord>& records);

	void			Apply(LagRecord* record);
	void			Run(CBasePlayer* player, LagRecord* record, std::deque<LagRecord>& records);

	void			OnMiss(CBasePlayer* player, LagRecord* record);
	void			OnHit(CBasePlayer* player, LagRecord* record);
};

extern CResolver* Resolver;