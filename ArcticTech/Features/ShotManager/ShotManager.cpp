#include "ShotManager.h"

#include "../../SDK/Interfaces.h"
#include "../../SDK/Globals.h"
#include "../../Utils/Console.h"

#include "../Misc/Prediction.h"
#include "../RageBot/AutoWall.h"
#include "../RageBot/LagCompensation.h"
#include "../Visuals/ESP.h"

#include "../Lua/Bridge/Bridge.h"


CShotManager* ShotManager = new CShotManager;

static const std::unordered_map<std::string_view, std::string> miss_color = {
	{"spread", "\aFFE59E"},
	{"prediction error", "\aFFE59E"},
	{"occlusion", "\aFFE59E"},
	{"correction", "\aEAFFA6"},
	{"lagcomp failure", "\aEAFFA6"},
	{"unknown", "\aEAFFA6"},
	{"death", "\aE6B1B1"},
	{"damage rejection", "\aE6B1B1"},
	{"unregistered shot", "\aE6B1B1"},
	{"player death", "\aFFFFFF"},
};

void CShotManager::LogMiss(RegisteredShot_t* shot) {
	std::string extra_info;
#ifdef RESOLVER_DEBUG
	std::string resolver;
	switch (shot->record->resolver_data.resolver_type) {
	case ResolverType::NONE:
		break;
	case ResolverType::FREESTAND:
		resolver = "freestand";
		break;
	case ResolverType::LOGIC:
		resolver = "logic";
		break;
	case ResolverType::ANIM:
		resolver = "anim";
		break;
	case ResolverType::BRUTEFORCE:
		resolver = "brute";
		break;
	case ResolverType::DEFAULT:
		resolver = "default";
		break;
	}

	if (!resolver.empty()) {
		extra_info += " | resolver: \aACCENT" + resolver + " " + std::to_string(static_cast<int>(shot->record->resolver_data.side * shot->record->resolver_data.max_desync_delta + 0.5f)) + "\xC2\xB0\a_MAIN_ | deltas: ";
		for (auto& layer : shot->record->resolver_data.layers)
			extra_info += std::format("{}({}) ", int(layer.delta * 1000.f), int(layer.desync));
		extra_info += " | choked: " + std::to_string(shot->record->m_nChokedTicks);
	}
#endif
	if (shot->record->shooting)
		extra_info += " | \aACCENTonshot\a_MAIN_";
	if (shot->record->breaking_lag_comp)
		extra_info += " | \aACCENTteleported\a_MAIN_";
	if (shot->safe_point)
		extra_info += " | \aACCENTsafe\a_MAIN_";

	Console->ArcticTag();
	Console->Print(std::format("missed \aACCENT{}\a_MAIN_'s \aACCENT{}\a_MAIN_ due to {}{}\a_MAIN_ (hc: \aACCENT{}%%\a_MAIN_ | bt: \aACCENT{}t\a_MAIN_{})\n",
		shot->record->player->GetName(),
		GetDamagegroupName(shot->wanted_damagegroup),
		miss_color.find(shot->miss_reason)->second,
		shot->miss_reason,
		static_cast<int>(shot->hitchance * 100),
		shot->backtrack,
		extra_info
	));
}

void CShotManager::DetectUnregisteredShots() {
	if (!Cheat.LocalPlayer || !Cheat.LocalPlayer->IsAlive())
		return;

	auto& pred_data = EnginePrediction->GetLocalData(ClientState->m_nCommandAck);

	if (pred_data.m_nSequence != ClientState->m_nCommandAck)
		return;

	auto weapon = reinterpret_cast<CBaseCombatWeapon*>(EntityList->GetClientEntityFromHandle(pred_data.m_hActiveWeapon));

	if (!weapon)
		return;

	float fLastShotTime = weapon->m_fLastShotTime();

	if (fLastShotTime + TICKS_TO_TIME(4.f) >= pred_data.m_fLastShotTime)
		return;

	for (auto it = m_RegisteredShots.rbegin(); it != m_RegisteredShots.rend(); it++) {
		if (it->acked)
			break;

		it->acked = true;
		it->unregistered = true;
		it->miss_reason = "unregistered shot";

		LogMiss(&*it);

		LUA_CALL_HOOK(LUA_AIM_ACK, &*it);

		break;
	}

	int next_cmd_nr = ClientState->m_nLastOutgoingCommand + ClientState->m_nChokedCommands + 1;
	for (int i = ClientState->m_nCommandAck; i < next_cmd_nr; i++)
		EnginePrediction->GetLocalData(i).m_fLastShotTime = fLastShotTime; // fix pred error

	auto weapon_info = weapon->GetWeaponInfo();

	if (!weapon_info)
		return;

	weapon->m_flNextPrimaryAttack() = fLastShotTime + weapon_info->flCycleTime;
}

void CShotManager::ProcessManualShot() {
	if (!config.visuals.effects.client_impacts->get())
		return;

	FireBulletData_t bullet;
	AutoWall->FireBullet(Cheat.LocalPlayer, ctx.shoot_position, ctx.shoot_position + Math::AngleVectors(ctx.cmd->viewangles) * 8192.f, bullet);

	Color col = config.visuals.effects.client_impacts_color->get();

	for (int i = 0; i < bullet.num_impacts; i++)
		DebugOverlay->AddBox(bullet.impacts[i], Vector(-1, -1, -1), Vector(1, 1, 1), col, config.visuals.effects.impacts_duration->get());
}

bool CShotManager::OnEvent(IGameEvent* event) {
	const std::string event_name(event->GetName());

	if (event_name == "player_hurt") {
		const int attacker = EngineClient->GetPlayerForUserID(event->GetInt("attacker"));
		CBasePlayer* player = reinterpret_cast<CBasePlayer*>(EntityList->GetClientEntity(EngineClient->GetPlayerForUserID(event->GetInt("userid"))));

		if (attacker != EngineClient->GetLocalPlayer())
			return false;

		RegisteredShot_t* shot = nullptr;

		for (auto it = m_RegisteredShots.rbegin(); it != m_RegisteredShots.rend(); it++) {
			if (it->acked)
				break;

			if (it->record->player == player)
				shot = &*it;
		}

		if (!shot)
			return false; // manual fire or miss

		shot->damage = event->GetInt("dmg_health");
		shot->damagegroup = HitgroupToDamagegroup(event->GetInt("hitgroup"));
		shot->health = event->GetInt("health");
		shot->recieved_events = true;

		return true;
	}
	else if (event_name == "bullet_impact") {
		const int userid = EngineClient->GetPlayerForUserID(event->GetInt("userid"));

		if (userid != EngineClient->GetLocalPlayer())
			return false;

		RegisteredShot_t* shot = nullptr;

		for (auto it = m_RegisteredShots.rbegin(); it != m_RegisteredShots.rend(); it++) {
			if (it->acked)
				break;

			shot = &*it;
		}

		if (!shot || shot->impacts.size() > 4)
			return false; // manual fire

		Vector point(event->GetFloat("x"), event->GetFloat("y"), event->GetFloat("z"));

		shot->impacts.push_back(point);
		shot->recieved_events = true;

		float min_distance = (shot->hit_point - shot->target_pos).LengthSqr();
		float cur_distance = (point - shot->target_pos).LengthSqr();

		if (cur_distance < min_distance)
			shot->hit_point = point;

		return false;
	}
	else if (event_name == "player_death") {
		const int userid = EngineClient->GetPlayerForUserID(event->GetInt("userid"));
		const int attacker = EngineClient->GetPlayerForUserID(event->GetInt("attacker"));

		if (userid == EngineClient->GetLocalPlayer()) {
			for (auto it = m_RegisteredShots.rbegin(); it != m_RegisteredShots.rend(); it++) {
				if (it->acked)
					break;

				if (it->recieved_events)
					continue;

				it->acked = true;
				it->recieved_events = true;
				it->unregistered = true;
				it->death = true;
				it->miss_reason = "death";

				LogMiss(&*it);

				LUA_CALL_HOOK(LUA_AIM_ACK, &*it);
			}

			return false;
		}

		for (auto it = m_RegisteredShots.rbegin(); it != m_RegisteredShots.rend(); it++) {
			if (it->acked)
				break;

			if (it->record->player->EntIndex() == userid)
				it->player_death = true;
		}
		return false;
	}

	return false;
}

void CShotManager::OnNetUpdate() {
	if (!Cheat.InGame || !Cheat.LocalPlayer)
		return;

	int cmd_nr = ctx.cmd ? ctx.cmd->command_number : ClientState->m_nLastOutgoingCommand;

	DetectUnregisteredShots();

	for (auto it = m_RegisteredShots.rbegin(); it != m_RegisteredShots.rend(); it++) {
		if (it->acked)
			break;

		if (!it->recieved_events || it->impacts.empty()) { // haven't received events yet or unregistered
			if (it->player_death) {
				it->acked = true;
				it->miss_reason = "player death";

				LogMiss(&*it);

				LUA_CALL_HOOK(LUA_AIM_ACK, &*it);
			}

			if (cmd_nr - it->command_number > 64) {
				it->acked = true;
				it->unregistered = true;
				it->miss_reason = "unregistered shot";
			}

			continue;
		}

		RegisteredShot_t* shot = &*it;

		shot->acked = true;
		shot->end_pos = shot->impacts.back();

		Vector direction;
		
		const int total_impacts = shot->impacts.size();
		if (total_impacts > 1) { // we can correct get correct shoot pos & angle
			direction = shot->impacts[total_impacts - 1] - shot->impacts[total_impacts - 2]; // impacts should be sorted by distance

			shot->angle = Math::VectorAngles(direction);
			shot->shoot_pos = EngineTrace->ClosestPoint(shot->impacts[total_impacts - 1], shot->impacts[total_impacts - 2], shot->client_shoot_pos); // correct shoot pos by trace
		}
		else {
			shot->angle = Math::VectorAngles(shot->impacts.back() - shot->client_shoot_pos);
			shot->shoot_pos = shot->client_shoot_pos;

			direction = (shot->end_pos - shot->shoot_pos).Normalized();
		}

		CBasePlayer* player = shot->record->player;
		LagRecord* backup_record = &LagCompensation->records(player->EntIndex()).back(); // just updated player, so latest record is correct

		LagCompensation->BacktrackEntity(shot->record);

		QAngle backup_angle = player->m_angEyeAngles();

		player->m_angEyeAngles() = shot->player_angle;
		shot->record->BuildMatrix();
		player->m_angEyeAngles() = backup_angle;

		memcpy(player->GetCachedBoneData().Base(), shot->record->clamped_matrix, player->GetCachedBoneData().Count() * sizeof(matrix3x4_t));

		if (shot->damage > 0) {
			Resolver->OnHit(player, shot->record);

			if (config.visuals.esp.hitmarker->get())
				WorldESP->AddHitmarker(shot->hit_point);

			if (config.visuals.esp.damage_marker->get())
				WorldESP->AddDamageMarker(shot->hit_point + Vector(0, 0, 10), shot->damage);

			if (shot->wanted_damagegroup != shot->damagegroup) {
				CGameTrace trace;
				Ray_t ray(shot->shoot_pos, shot->shoot_pos + direction * 8192);

				if (!EngineTrace->ClipRayToPlayer(ray, MASK_SHOT_HULL | CONTENTS_GRATE, player, &trace) || trace.hit_entity != player) {
					shot->miss_reason = "correction";
				}
				else {
					if (HitgroupToDamagegroup(trace.hitgroup) == shot->wanted_damagegroup) {
						shot->miss_reason = "correction";
						Resolver->OnMiss(player, shot->record);
					}
					else {
						if ((shot->shoot_pos - shot->client_shoot_pos).LengthSqr() > 1.f)
							shot->miss_reason = "prediction error";
						else
							shot->miss_reason = "spread";
					}
				}
			}

			std::string hitbox = "\aACCENT" + GetDamagegroupName(shot->damagegroup) + "\a_MAIN_";
			if (shot->damagegroup != shot->wanted_damagegroup)
				hitbox += "(\aACCENT" + GetDamagegroupName(shot->wanted_damagegroup) + "\a_MAIN_)";

			std::string damage = std::format("\aACCENT{}\a_MAIN_", shot->damage);
			if (shot->wanted_damage != shot->damage)
				damage += std::format("(\aACCENT{}\a_MAIN_)", shot->wanted_damage);

			std::string extra_info; 
#ifdef RESOLVER_DEBUG
				std::string resolver;
			switch (shot->record->resolver_data.resolver_type) {
			case ResolverType::NONE:
				break;
			case ResolverType::FREESTAND:
				resolver = "freestand";
				break;
			case ResolverType::LOGIC:
				resolver = "logic";
				break;
			case ResolverType::ANIM:
				resolver = "anim";
				break;
			case ResolverType::BRUTEFORCE:
				resolver = "brute";
				break;
			case ResolverType::DEFAULT:
				resolver = "default";
				break;
			case ResolverType::MEMORY:
				resolver = "memory";
				break;
			}


			if (!resolver.empty()) {
				extra_info += " | resolver: \aACCENT" + resolver + " " + std::to_string(static_cast<int>(shot->record->resolver_data.side * shot->record->resolver_data.max_desync_delta + 0.5f)) + "\xC2\xB0\a_MAIN_ | deltas: ";
				for (auto& layer : shot->record->resolver_data.layers)
					extra_info += std::format("{}({}) ", int(layer.delta * 1000.f), int(layer.desync));
			}
#endif

			if (shot->record->breaking_lag_comp)
				extra_info += " | \aACCENTteleported\a_MAIN_";
			if (shot->safe_point)
				extra_info += " | \aACCENTsafe\a_MAIN_";
			if (shot->record->shooting)
				extra_info += " | \aACCENTonshot\a_MAIN_";
			if (!shot->miss_reason.empty())
				extra_info += " | mismatch: " + miss_color.find(shot->miss_reason)->second + shot->miss_reason + "\a_MAIN_";

			Console->ArcticTag();
			Console->Print(std::format("hit \aACCENT{} \a_MAIN_in the {} for {} (hc: \aACCENT{}%%\a_MAIN_ | bt: \aACCENT{}t\a_MAIN_{})\n", 
				player->GetName(), 
				hitbox, 
				damage,
				static_cast<int>(shot->hitchance * 100),
				shot->backtrack,
				extra_info
			));
		}
		else {
			CGameTrace trace;
			Ray_t ray(shot->shoot_pos, shot->shoot_pos + direction * 8192);

			if (shot->shoot_pos.DistTo(shot->end_pos) + 10.f < shot->client_shoot_pos.DistTo(shot->target_pos)) {
				it->miss_reason = "occlusion";
			}
			else if (!EngineTrace->ClipRayToPlayer(ray, MASK_SHOT_HULL | CONTENTS_GRATE, player, &trace) || trace.hit_entity != player) {
				if ((shot->shoot_pos - shot->client_shoot_pos).LengthSqr() > 1.f || shot->hitchance == 1.f)
					it->miss_reason = "prediction error";
				else
					it->miss_reason = "spread";
			}
			else {
				Vector target_hit_point = shot->client_impacts[0];
				if (shot->client_impacts.size() > 1) {
					for (int i = 1; i < shot->client_impacts.size(); i++) {
						if ((shot->client_impacts[i] - shot->target_pos).LengthSqr() < (target_hit_point - shot->target_pos).LengthSqr())
							target_hit_point = shot->target_pos;
					}
				}

				if ((shot->hit_point - target_hit_point).LengthSqr() < 64.f && shot->impacts.size() > 1) {
					it->miss_reason = "damage rejection";
				}
				else {
					auto& records = LagCompensation->records(shot->record->player->EntIndex());
					bool break_lag_comp = false;

					
					if (cvars.cl_lagcompensation->GetInt() == 0 && (records.back().m_vecOrigin - it->record->m_vecOrigin).LengthSqr() > 16.f) {
						it->miss_reason = "prediction error";

					} else {
						for (auto it = records.rbegin(); it != records.rend(); it++) {
							if (it->m_flSimulationTime < shot->record->m_flSimulationTime)
								break;

							if (it->breaking_lag_comp) {
								break_lag_comp = true;
								break;
							}
						}

						if (break_lag_comp) {
							it->miss_reason = "lagcomp failure";
						} else if (shot->safe_point) {
							it->miss_reason = "unknown";
						} else {
							Resolver->OnMiss(player, shot->record);
							it->miss_reason = "correction";
						}
					}
				}
			}

			LogMiss(shot);
		}

		LagCompensation->BacktrackEntity(backup_record);

		LUA_CALL_HOOK(LUA_AIM_ACK, shot);
	}
}

void CShotManager::AddShot(const Vector& shoot_pos, const Vector& target_pos, int damage, int damagegroup, float hitchance, bool safe, LagRecord* record, Vector impacts[], int total_impacts) {
	RegisteredShot_t* shot = &m_RegisteredShots.emplace_back();

	shot->client_shoot_pos = shoot_pos;
	shot->target_pos = target_pos;
	shot->client_angle = Math::VectorAngles(target_pos - shoot_pos);
	shot->command_number = ctx.cmd->command_number;
	shot->hitchance = hitchance;
	shot->backtrack = GlobalVars->tickcount - record->update_tick;
	shot->record = record;
	shot->player_angle = record->player->m_angEyeAngles();
	shot->wanted_damage = damage;
	shot->wanted_damagegroup = damagegroup;
	shot->safe_point = safe;

	for (int i = 0; i < total_impacts; i++)
		shot->client_impacts.push_back(impacts[i]);

	LUA_CALL_HOOK(LUA_AIM_SHOT, shot);

	while (m_RegisteredShots.size() > 8)
		m_RegisteredShots.erase(m_RegisteredShots.begin());
}

void CShotManager::Reset() {
	m_RegisteredShots.clear();
}