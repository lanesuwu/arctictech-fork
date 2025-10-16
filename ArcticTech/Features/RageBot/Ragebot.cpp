#include "Ragebot.h"
#include "AutoWall.h"
#include "../Misc/Prediction.h"
#include "../../SDK/Interfaces.h"
#include "../../SDK/Globals.h"
#include "../../Utils/Utils.h"
#include <algorithm>
#include "../Misc/AutoPeek.h"
#include "Exploits.h"
#include "../../Utils/Console.h"
#include "../ShotManager/ShotManager.h"
#include "../Visuals/Chams.h"
#include "../Visuals/ESP.h"
#include <array>
#include <limits>
#include <vector>
#include <queue>
#include <cmath>
#include <string>
#include <cfloat>

void CRagebot::CalcSpreadValues() {
	constexpr float two_pi = 2.f * PI_F;

	for (int i = 0; i < 100; i++) {
		Utils::RandomSeed(i);

		// uniform circle distribution
		float a = std::sqrt(Utils::RandomFloat(0.f, 1.f));
		float b = Utils::RandomFloat(0.f, two_pi);
		float c = std::sqrt(Utils::RandomFloat(0.f, 1.f));
		float d = Utils::RandomFloat(0.f, two_pi);

		float bcos = std::cos(b), bsin = std::sin(b);
		float dcos = std::cos(d), dsin = std::sin(d);

		spread_values[i].a = a;
		spread_values[i].bcos = bcos;
		spread_values[i].bsin = bsin;
		spread_values[i].c = c;
		spread_values[i].dcos = dcos;
		spread_values[i].dsin = dsin;
	}
}


weapon_settings_t CRagebot::GetWeaponSettings(int weaponId) {
	weapon_settings_t settings = config.ragebot.weapons.global;

	switch (weaponId) {
	case Scar20:
	case G3SG1:
		settings = config.ragebot.weapons.autosniper;
		break;
	case Ssg08:
		settings = config.ragebot.weapons.scout;
		break;
	case Awp:
		settings = config.ragebot.weapons.awp;
		break;
	case Deagle:
		settings = config.ragebot.weapons.deagle;
		break;
	case Revolver:
		settings = config.ragebot.weapons.revolver;
		break;
	case Fiveseven:
	case Glock:
	case Usp_s:
	case Tec9:
	case P250:
	case Hkp2000:
	case Elite:
		settings = config.ragebot.weapons.pistol;
		break;
	}

	return settings;
}

void CRagebot::UpdateUI(int idx) {
	if (!Cheat.InGame || !Cheat.LocalPlayer || !Cheat.LocalPlayer->IsAlive() || !ctx.active_weapon)
		return;

	int ui_id = 0;
	if (idx == -1)
		idx = ctx.active_weapon->m_iItemDefinitionIndex();

	switch (idx) {
	case Awp:
		ui_id = 1;
		break;
	case Scar20:
	case G3SG1:
		ui_id = 2;
		break;
	case Ssg08:
		ui_id = 3;
		break;
	case Deagle:
		ui_id = 4;
		break;
	case Revolver:
		ui_id = 5;
		break;
	case Fiveseven:
	case Glock:
	case Usp_s:
	case Tec9:
	case P250:
		ui_id = 6;
		break;
	}

	config.ragebot.selected_weapon->set(ui_id);
}

float CRagebot::CalcMinDamage(CBasePlayer* player) {
	int minimum_damage = settings.minimum_damage->get();
	if (config.ragebot.aimbot.minimum_damage_override_key->get())
		minimum_damage = settings.minimum_damage_override->get();

	if (minimum_damage >= 100) {
		return minimum_damage - 100 + player->m_iHealth();
	}
	else {
		return min(minimum_damage, player->m_iHealth() + 1);
	}
}

void CRagebot::AutoStop(bool predict) {
	if (!Cheat.LocalPlayer || !ctx.active_weapon)
		return;

	const int flags = Cheat.LocalPlayer->m_fFlags();

	// optional: skip if ducking on ground
	if ((ctx.cmd->buttons & IN_DUCK) && (flags & FL_ONGROUND))
		return;

	// no autostop in air unless enabled
	if (!(flags & FL_ONGROUND) && !settings.auto_stop->get(2))
		return;

	Vector velocity = Cheat.LocalPlayer->m_vecVelocity();
	float speed2D = velocity.Length2D();

	const float target_speed = ctx.active_weapon->MaxSpeed() * 0.2f;

	// already slow enough → scale input instead of braking hard
	if (speed2D <= target_speed + 1.f) {
		const float cmd_speed = std::hypot(ctx.cmd->forwardmove, ctx.cmd->sidemove);

		if (cmd_speed > 2.f) {
			float factor = target_speed / cmd_speed;
			ctx.cmd->forwardmove *= factor;
			ctx.cmd->sidemove *= factor;
		}
		return;
	}

	// convert velocity to angle
	QAngle vel_angle = Math::VectorAngles(velocity);

	QAngle view;
	EngineClient->GetViewAngles(view);

	// adjust yaw depending on prediction
	vel_angle.yaw = predict ? vel_angle.yaw : (view.yaw - vel_angle.yaw);
	vel_angle.Normalize();

	// get forward vector
	Vector forward;
	Math::AngleVectors(vel_angle, forward);
	forward.Normalize();

	// desired stop force
	float wish_speed = std::clamp(speed2D, 0.f, 450.f);

	if (!(flags & FL_ONGROUND)) {
		static ConVar* sv_airaccelerate = CVar->FindVar("sv_airaccelerate");
		if (sv_airaccelerate && GlobalVars->frametime > 0.f) {
			float air_factor = (speed2D * 0.936f) / (sv_airaccelerate->GetFloat() * GlobalVars->frametime);
			wish_speed = std::clamp(air_factor, 0.f, 450.f);
		}
	}

	Vector negated = forward * -wish_speed;

	ctx.cmd->forwardmove = negated.x;
	ctx.cmd->sidemove = negated.y;
}


float CRagebot::FastHitchance(LagRecord* target, float inaccuracy, int hitbox) {
	if (!target || !target->player || !ctx.active_weapon)
		return 0.f;

	if (inaccuracy < 0.f)
		inaccuracy = EnginePrediction->WeaponInaccuracy();

	if (inaccuracy <= FLT_EPSILON)
		return 1.f;

	// get studio model
	studiohdr_t* hdr = ModelInfoClient->GetStudioModel(target->player->GetModel());
	if (!hdr)
		return 0.f;

	mstudiohitboxset_t* set = hdr->GetHitboxSet(target->player->m_nHitboxSet());
	if (!set || hitbox >= set->numhitboxes)
		return 0.f;

	mstudiobbox_t* box = set->GetHitbox(hitbox);
	if (!box)
		return 0.f;

	// hitbox center in local space
	Vector center = (box->bbmin + box->bbmax) * 0.5f;
	Vector hitbox_pos;
	Math::VectorTransform(center, target->clamped_matrix[box->bone], &hitbox_pos);

	// real radius (capsule) or fallback
	float radius = box->flCapsuleRadius > 0.f
		? box->flCapsuleRadius
		: (box->bbmax - box->bbmin).Length() * 0.5f;

	// distance & chance
	float distance = (ctx.shoot_position - hitbox_pos).Length();
	if (distance <= 0.f)
		return 0.f;

	float chance = radius / (distance * inaccuracy);
	return std::clamp(chance, 0.f, 1.f);
}


float CRagebot::CalcHitchance(QAngle angles, LagRecord* target, int hitbox) {
	if (!target || !target->player || !ctx.active_weapon)
		return 0.f;

	// feet usually need faster check → so we fallback to FastHitchance
	if (hitbox == HITBOX_LEFT_FOOT || hitbox == HITBOX_RIGHT_FOOT)
		return FastHitchance(target, -1.f, 3.f);

	int damagegroup = HitboxToDamagegroup(hitbox);

	Vector forward, right, up;
	Math::AngleVectors(angles, forward, right, up);

	int hits = 0;
	constexpr int total_seeds = 100;

	const float weapon_inaccuracy = EnginePrediction->WeaponInaccuracy();
	const float weapon_spread = EnginePrediction->WeaponSpread();

	for (int i = 0; i < total_seeds; i++) {
		SpreadValues_t& s_val = spread_values[i];

		float a = s_val.a;
		float c = s_val.c;

		float inaccuracy = a * weapon_inaccuracy;
		float spread = c * weapon_spread;

		// special case for revolver (id 64)
		if (ctx.active_weapon->m_iItemDefinitionIndex() == 64) {
			a = 1.f - a * a;
			c = 1.f - c * c;
			inaccuracy = a * weapon_inaccuracy;
			spread = c * weapon_spread;
		}

		Vector direction = forward +
			right * (s_val.bcos * inaccuracy + s_val.dcos * spread) +
			up * (s_val.bsin * inaccuracy + s_val.dsin * spread);

		direction.Normalize();

		if (EngineTrace->RayIntersectPlayer(
			ctx.shoot_position,
			ctx.shoot_position + direction * 8192.f,
			target->player,
			target->clamped_matrix,
			damagegroup))
		{
			hits++;
		}
	}

	return (static_cast<float>(hits) / static_cast<float>(total_seeds));
}



bool CRagebot::CompareRecords(LagRecord* a, LagRecord* b) {
	if (!a || !b)
		return false;

	// position tolerance (~16 units squared = 4^2)
	const Vector vec_diff = a->m_vecOrigin - b->m_vecOrigin;
	if (vec_diff.LengthSqr() > 16.f)
		return false;

	// angle tolerance
	QAngle angle_diff = a->m_angEyeAngles - b->m_angEyeAngles;
	angle_diff.Normalize();

	if (std::fabs(angle_diff.yaw) > 90.f)
		return false;

	if (std::fabs(angle_diff.pitch) > 10.f)
		return false;

	// lag comp / tickbase consistency
	if (a->breaking_lag_comp != b->breaking_lag_comp)
		return false;

	if (a->shifting_tickbase != b->shifting_tickbase)
		return false;

	return true;
}


void CRagebot::SelectRecords(CBasePlayer* player, std::queue<LagRecord*>& target_records) {
	auto& records = LagCompensation->records(player->EntIndex());

	if (records.empty())
		return;

	if (Exploits->IsShifting() || !cvars.cl_lagcompensation->GetInt()) {
		LagRecord* record = &records.back();

		float latency = 0.f;
		if (INetChannelInfo* nci = EngineClient->GetNetChannelInfo()) {
			latency += nci->GetLatency(FLOW_INCOMING) + nci->GetLatency(FLOW_OUTGOING);

			int pred_ticks = TIME_TO_TICKS(latency);

			int next_update_tick = record->update_tick + record->m_nChokedTicks;

			if (GlobalVars->tickcount + pred_ticks >= next_update_tick)
				record = LagCompensation->ExtrapolateRecord(record, pred_ticks);
		}

		target_records.push(record);
		return;
	}

	bool ex_back = config.ragebot.aimbot.extended_backtrack->get() && !frametime_issues;

	LagRecord* last_valid_record = nullptr;
	for (int i = records.size() - 1; i >= 0; i--) {
		const auto record = &records[i];

		if (!LagCompensation->ValidRecord(record)) {
			WorldESP->AddDebugMessage(std::string(("CRageBot::SelectRecords")).append((" -> !LagCompensation->ValidRecord |")).append((" L") + std::to_string(__LINE__)));

			if (record->breaking_lag_comp || record->invalid)
				break;

			continue;
		}

		if (!target_records.empty() && CompareRecords(record, target_records.back()))
			continue;

		if (ex_back) {
			target_records.push(record);
			continue;
		}

		if (target_records.empty()) {
			target_records.push(record);
		} else {
			if (record->shooting)
				target_records.push(record);

			last_valid_record = record;
		}
	}

	if (!ex_back && last_valid_record && last_valid_record != &records.back()) {
		target_records.push(last_valid_record);
	}

	if (target_records.empty()) {
		// try to extrapolate the last record based on current latency
		auto last_record = &records.back();
		if (!last_record->shifting_tickbase && !last_record->invalid) {
			LagRecord* push_record = last_record;

			if (INetChannelInfo* nci = EngineClient->GetNetChannelInfo()) {
				float latency = 0.f;
				latency += nci->GetLatency(FLOW_INCOMING) + nci->GetLatency(FLOW_OUTGOING);

				int pred_ticks = TIME_TO_TICKS(latency);

				int next_update_tick = last_record->update_tick + last_record->m_nChokedTicks;

				// if by the time we account for latency we would be past the next update,
				// try extrapolating the record forward by pred_ticks
				if (GlobalVars->tickcount + pred_ticks >= next_update_tick) {
					LagRecord* extrapolated = LagCompensation->ExtrapolateRecord(last_record, pred_ticks);
					// fall back to the original record if extrapolation failed/null
					if (extrapolated)
						push_record = extrapolated;
				}
			}

			target_records.push(push_record);
		}
	}
}

void CRagebot::AddPoint(AimPoint_t&& point) {
	std::lock_guard<std::mutex> lock(scan_mutex);
	thread_work.emplace(point);
	selected_points++;
	scan_condition.notify_one();
}

void CRagebot::GetMultipoints(LagRecord* record, int hitbox_id, float scale) {
	// basic sanity checks
	if (!record || !record->player)
		return;

	// clamp scale to avoid degenerate values or explosive offsets
	const float s = std::clamp(scale, 0.01f, 2.0f);

	// model / hitbox set checks
	studiohdr_t* studiomodel = ModelInfoClient->GetStudioModel(record->player->GetModel());
	if (!studiomodel)
		return;

	// guard hitbox set retrieval
	auto hitboxSet = studiomodel->GetHitboxSet(record->player->m_nHitboxSet());
	if (!hitboxSet)
		return;

	mstudiobbox_t* hitbox = hitboxSet->GetHitbox(hitbox_id);
	if (!hitbox)
		return;

	// explicitly skip foot / lower limb multipoints (these usually shouldn't use points like head/chest)
	switch (hitbox_id) {
	case HITBOX_LEFT_FOOT:
	case HITBOX_RIGHT_FOOT:
	case HITBOX_LEFT_CALF:
	case HITBOX_RIGHT_CALF:
	case HITBOX_LEFT_THIGH:
	case HITBOX_RIGHT_THIGH:
		return;
	default:
		break;
	}

	// validate bone index
	if (hitbox->bone < 0)
		return;

	// ensure we have a bone matrix for this bone
	// assumes record->clamped_matrix is sized correctly for bones; add length check if available
	matrix3x4_t boneMatrix = record->clamped_matrix[hitbox->bone];

	// transform AABB corners to world (or bone) space
	Vector mins, maxs;
	Math::VectorTransform(hitbox->bbmin, boneMatrix, &mins);
	Math::VectorTransform(hitbox->bbmax, boneMatrix, &maxs);

	// center & width calculations
	Vector center = (mins + maxs) * 0.5f;
	Vector sideDirection = center - mins;
	float sideLen = sideDirection.Length();
	// avoid divide by zero when normalizing; if zero, we won't use the normalized vector above.
	if (sideLen > 0.00001f)
		sideDirection /= sideLen;

	const float width = sideLen + hitbox->flCapsuleRadius;
	const float radius = hitbox->flCapsuleRadius;

	// local-space vertices relative to hitbox center (transform them with the bone matrix)
	// ordering: +Y, -Y, +Z, -Z  (local axes)
	std::array<Vector, 4> localVerts = {
		Vector(0.0f,  radius * s, 0.0f),
		Vector(0.0f, -radius * s, 0.0f),
		Vector(0.0f, 0.0f,  width * s),
		Vector(0.0f, 0.0f, -width * s)
	};

	// transform all local verts once
	std::array<Vector, 4> worldVerts;
	for (size_t i = 0; i < localVerts.size(); ++i)
		Math::VectorTransform(localVerts[i], boneMatrix, &worldVerts[i]);

	// helper for adding points (keeps call sites concise)
	auto add_point = [&](const Vector& pos, int hb) {
		AddPoint(AimPoint_t{ pos, hb, true, dont_shoot_next_points });
		};

	switch (hitbox_id) {
	case HITBOX_HEAD:
		// add side/top points
		for (const auto& v : worldVerts)
			add_point(v, hitbox_id);

		// extra top point if local player is looking down heavily and the head is above shoot position
		// (keeps the original logic but with clearer variable names)
		if (record->m_angEyeAngles.pitch > 85.f && center.z >= ctx.shoot_position.z - 16.f) {
			Vector top = Vector(center.x, center.y, center.z + width * s * 0.89f);
			add_point(top, hitbox_id);
		}
		break;

	case HITBOX_STOMACH:
	case HITBOX_PELVIS:
		// one low-side point
		add_point(worldVerts[1], hitbox_id);
		[[fallthrough]];

	case HITBOX_CHEST:
	case HITBOX_UPPER_CHEST:
		// front/back (Z +/-)
		add_point(worldVerts[2], hitbox_id);
		add_point(worldVerts[3], hitbox_id);
		break;

	default:
		// no multipoints for other hitboxes by default
		break;
	}
}

bool CRagebot::IsArmored(int hitbox) {
	switch (hitbox)
	{
	case HITBOX_LEFT_CALF:
	case HITBOX_LEFT_FOOT:
	case HITBOX_LEFT_THIGH:
	case HITBOX_RIGHT_THIGH:
	case HITBOX_RIGHT_FOOT:
	case HITBOX_RIGHT_CALF:
		return false;
	default:
		return true;
	}
}

void CRagebot::SelectPoints(LagRecord* record) {
	if (!record || !record->player)
		return;

	// cache commonly used pointers / values
	auto* player = record->player;
	auto* weaponInfo = ctx.weapon_info;
	if (!weaponInfo)
		return;

	// save previous global flag and restore at the end to avoid side-effects
	bool prev_dont_shoot = dont_shoot_next_points;

	for (int hitbox = 0; hitbox < HITBOX_MAX; ++hitbox) {
		// skip disabled hitboxes
		if (!hitbox_enabled(hitbox))
			continue;

		// if server enforces headshot-only, skip all non-head hitboxes
		if (cvars.mp_damage_headshot_only->GetInt() > 0 && hitbox != HITBOX_HEAD)
			continue;

		// if user forced body-aim, skip head
		if (hitbox == HITBOX_HEAD && config.ragebot.aimbot.force_body_aim->get())
			continue;

		// start with weapon base damage and scale it for the hitgroup
		float max_possible_damage = weaponInfo->iDamage;
		// scaleDamage may modify the damage according to armor/hitgroup multipliers
		player->ScaleDamage(HitboxToHitgroup(hitbox), weaponInfo, max_possible_damage);

		// minimum damage required to consider shooting (CalcMinDamage returns a float)
		float min_damage = CalcMinDamage(player);

		// if auto-stop is disabled and we cannot reach minimum damage, skip this hitbox
		// NOTE: settings.auto_stop->get(0) was used in original code; preserve semantics:
		if (!settings.auto_stop->get(0) && max_possible_damage < min_damage)
			continue;

		// decide whether we should *not* shoot subsequent multipoints for this hitbox
		bool should_not_shoot_next = (max_possible_damage < min_damage);

		// set the global/member flag used by GetMultipoints/AddPoint if your pipeline relies on it
		dont_shoot_next_points = should_not_shoot_next;

		// add the primary center point for this hitbox to the work queue
		{
			// compute center once
			Vector center = player->GetHitboxCenter(hitbox, record->clamped_matrix);

			// thread-safe push to worker queue
			std::lock_guard<std::mutex> lock(scan_mutex);
			thread_work.emplace(center, hitbox, false, should_not_shoot_next);
			++selected_points;
			scan_condition.notify_one();
		} // unlock ASAP

		// only generate multipoints if both enabled and we are not currently shifting exploits
		if (multipoints_enabled(hitbox) && !Exploits->IsShifting()) {
			const float scale = (hitbox == HITBOX_HEAD)
				? settings.head_point_scale->get() * 0.01f
				: settings.body_point_scale->get() * 0.01f;

			// GetMultipoints uses dont_shoot_next_points member (we set it above)
			GetMultipoints(record, hitbox, scale);
		}

		// reset member flag for the next hitbox iteration
		dont_shoot_next_points = prev_dont_shoot;
	}
}


void CRagebot::SelectBestPoint(ScannedTarget_t* target) {
	if (!target || !target->player) {
		return;
	}

	// initialize with very low damage so first valid point replaces them.
	const float NEG_INF = std::numeric_limits<float>::lowest();
	ScannedPoint_t best_body_point{};
	ScannedPoint_t best_head_point{};
	best_body_point.damage = NEG_INF;
	best_head_point.damage = NEG_INF;

	// choose the best body/head points according to priority and minimum damage thresholds
	for (const auto& point : target->points) {
		if (point.hitbox == HITBOX_HEAD) {
			// choose head point if:
			//  - we don't yet have a point >= minimum_damage (best damage < minimum)
			//  - or this point has higher priority than current best and itself exceeds min damage
			if (best_head_point.damage < target->minimum_damage ||
				(point.priority > best_head_point.priority && point.damage > target->minimum_damage)) {
				best_head_point = point;
			}
		}
		else {
			// non-head (body) hitboxes
			if (best_body_point.damage < target->minimum_damage ||
				(point.priority > best_body_point.priority && point.damage > target->minimum_damage)) {
				best_body_point = point;
			}
		}
	}

	// validate delayed_ticks index before use
	int idx = target->player->EntIndex();
	if (idx < 0 || idx >= static_cast<int>(delayed_ticks.size())) {
		target->best_point = (best_body_point.damage > target->minimum_damage) ? best_body_point : best_head_point;
		return;
	}

	int& delay = delayed_ticks[idx];
	const int delay_setting = settings.delay_shot->get(); // 0 disables
	const bool has_valid_body = best_body_point.damage > 1.0f && !best_body_point.dont_shoot;
	const bool has_valid_head = best_head_point.damage > 1.0f;

	// delay-shot logic: if both body & head have >1 damage and delay is enabled, postpone head until delay passes
	if (has_valid_body && has_valid_head && delay_setting > 0) {
		if (delay < delay_setting) {
			// mark head as temporarily invalid until delay reached
			best_head_point.damage = -1.0f;
		}
		// increment but clamp to avoid overflow from runaway increments
		delay = (std::min)(delay + 1, 1000000);
	}
	else {
		delay = 0;
	}

	// prefer head if it's a lethal or better option under certain conditions:
	// - if body damage is less than player's health AND head dmg > body dmg AND (player is currently shooting OR we are allowed to aim head if safe)
	if (best_body_point.damage < static_cast<float>(target->player->m_iHealth()) &&
		best_head_point.damage > best_body_point.damage &&
		(best_head_point.record && best_head_point.record->shooting || (settings.aim_head_if_safe->get() && best_head_point.safe_point))) {
		target->best_point = best_head_point;
		return;
	}

	if (target->player != last_target &&
		ctx.tickbase_shift == 0 &&
		config.ragebot.aimbot.doubletap->get() &&
		!config.ragebot.aimbot.force_teleport->get()) {

		if (best_body_point.damage > static_cast<float>(target->player->m_iHealth()) ||
			best_head_point.damage < static_cast<float>(target->player->m_iHealth())) {
			target->best_point = best_body_point;
			return;
		}

		target->best_point = best_head_point;
		return;
	}

	if (best_body_point.damage > target->minimum_damage) {
		target->best_point = best_body_point;
	}
	else {
		target->best_point = best_head_point;
	}
}

void CRagebot::CreateThreads() {
	for (int i = 0; i < GetRagebotThreads(); i++) {
		threads[i] = Utils::CreateSimpleThread(&CRagebot::ThreadScan, (void*)i);
		inited_threads++;
	}
}

void CRagebot::TerminateThreads() {
	remove_threads = true;
	scan_condition.notify_all();

	for (int i = 0; i < inited_threads; i++) {
		Utils::ThreadJoin(threads[i], 1000000);
		threads[i] = 0;
	}
	inited_threads = 0;
	remove_threads = false;
}

void CRagebot::ScanTargets() {
	scanned_targets.clear();

	for (int i = 0; i < ClientState->m_nMaxClients; i++) {
		CBasePlayer* player = reinterpret_cast<CBasePlayer*>(EntityList->GetClientEntity(i));

		if (!player || !player->IsAlive() || player->IsTeammate() || player->m_bDormant() || player->m_bGunGameImmunity())
			continue;

		if (frametime_issues && (i + ctx.cmd->command_number % 2) % 2 == 0)
			continue;

		ScanTarget(player);
	}
}

uintptr_t CRagebot::ThreadScan(int threadId) {
	while (true) {
		// wait until there's work or we're asked to exit.
		std::unique_lock<std::mutex> scan_lock(Ragebot->scan_mutex);
		Ragebot->scan_condition.wait(scan_lock, []() {
			return !Ragebot->thread_work.empty() || Ragebot->remove_threads;
			});

		// if we were asked to stop threads, exit loop.
		if (Ragebot->remove_threads) {
			// unlock (unique_lock will unlock on destruction) and exit
			break;
		}

		// pop one work item and snapshot current_record under the same lock to avoid races.
		AimPoint_t point = Ragebot->thread_work.front();
		Ragebot->thread_work.pop();

		// snapshot the record pointer so we use a consistent record for the processing below.
		LagRecord* record = Ragebot->current_record;
		// also snapshot result_target pointer (we'll still validate it later).
		ScannedTarget_t* result_target = Ragebot->result_target;

		// unlock the scan mutex as early as possible
		scan_lock.unlock();

		if (config.ragebot.aimbot.show_aimpoints->get()) {
			DebugOverlay->AddBoxOverlay(point.point,
				Vector(-1, -1, -1),
				Vector(1, 1, 1),
				QAngle(0, 0, 0),
				255, 255, 255, 200,
				GlobalVars->interval_per_tick * 2);
		}

		// simple validation: record and its player must be valid
		if (!record || !record->player) {
			// mark as completed
			{
				std::lock_guard<std::mutex> completed_lock(Ragebot->completed_mutex);
				++Ragebot->scanned_points;
				if (Ragebot->scanned_points >= Ragebot->selected_points)
					Ragebot->result_condition.notify_all();
			}
			continue;
		}

		CBasePlayer* target = record->player;

		// if force_safepoint is enabled, ensure the point is safe in both matrices.
		// NOTE: we snapshoted 'record' so we are using the same matrices that were present when we popped work.
		if (config.ragebot.aimbot.force_safepoint->get()) {
			bool opp_ok = EngineTrace->RayIntersectPlayer(ctx.shoot_position, point.point, target,
				record->opposite_matrix, HitboxToDamagegroup(point.hitbox));
			bool safe_ok = EngineTrace->RayIntersectPlayer(ctx.shoot_position, point.point, target,
				record->safe_matrix, HitboxToDamagegroup(point.hitbox));
			if (!(opp_ok && safe_ok)) {
				// not a safe point — mark as scanned and continue
				std::lock_guard<std::mutex> completed_lock(Ragebot->completed_mutex);
				++Ragebot->scanned_points;
				if (Ragebot->scanned_points >= Ragebot->selected_points)
					Ragebot->result_condition.notify_all();
				continue;
			}
		}

		// fire the bullet simulation
		FireBulletData_t bullet;
		if (AutoWall->FireBullet(Cheat.LocalPlayer, ctx.shoot_position, point.point, bullet, target)
			&& bullet.enterTrace.hitgroup == HitboxToHitgroup(point.hitbox)) {

			// build priority and flags
			int priority = point.multipoint ? 0 : 1;
			bool jitter_safe = false;

			// determine "jitter_safe" and raise priority if both traces are valid
			if (!config.ragebot.aimbot.force_safepoint->get()) {
				bool opp_ok = EngineTrace->RayIntersectPlayer(ctx.shoot_position, point.point, target,
					record->opposite_matrix, HitboxToDamagegroup(point.hitbox));
				bool safe_ok = EngineTrace->RayIntersectPlayer(ctx.shoot_position, point.point, target,
					record->safe_matrix, HitboxToDamagegroup(point.hitbox));
				if (opp_ok && safe_ok) {
					priority += 2;
					jitter_safe = true;
				}
			}

			// add more priority for chest/pelvis and other heuristics
			if (point.hitbox >= HITBOX_PELVIS && point.hitbox <= HITBOX_UPPER_CHEST)
				priority += 2;

			if (record->shooting)
				priority += 2;
			else if (record->m_angEyeAngles.pitch < 10.f) // aim at "defensive AA"
				priority += 1;

			if (GlobalVars->tickcount - record->update_tick < 12)
				priority += 1;

			// create scanned point
			ScannedPoint_t sc_point{};
			sc_point.damage = bullet.damage;
			sc_point.hitbox = point.hitbox;
			std::memcpy(sc_point.impacts, bullet.impacts, sizeof(Vector) * 5);
			sc_point.num_impacts = bullet.num_impacts;
			sc_point.point = point.point;
			sc_point.priority = priority;
			sc_point.record = record;
			sc_point.safe_point = jitter_safe;
			sc_point.dont_shoot = point.dont_shoot;

			// mark exploit charge if damage is significant
			if (bullet.damage > 5.f)
				Exploits->block_charge = true;

			// push result to target's point list under result_mutex
			{
				std::lock_guard<std::mutex> result_lock(Ragebot->result_mutex);
				if (result_target) // validate target pointer
					result_target->points.push_back(sc_point);
			}
		}

		// Mark work as completed
		{
			std::lock_guard<std::mutex> completed_lock(Ragebot->completed_mutex);
			++Ragebot->scanned_points;
			if (Ragebot->scanned_points >= Ragebot->selected_points)
				Ragebot->result_condition.notify_all();
		}
	}

	return 0;
}


void CRagebot::ScanTarget(CBasePlayer* target) {

	std::queue<LagRecord*> records;
	SelectRecords(target, records);

	if (records.empty()) {
		WorldESP->AddDebugMessage(std::string(("CRageBot::ScanTarget")).append((" -> Records Empty |")).append((" L") + std::to_string(__LINE__)));
		return;
	}

	float minimum_damage = CalcMinDamage(target);

	ScannedTarget_t* result = &scanned_targets.emplace_back();
	result->player = target;
	result->minimum_damage = minimum_damage;

	LagRecord* backup_record = LagCompensation->BackupData(target);

	while (records.size() > 0) {
		LagRecord* record = records.front();
		records.pop();

		LagCompensation->BacktrackEntity(record, false);
		record->BuildMatrix();

		memcpy(target->GetCachedBoneData().Base(), record->clamped_matrix, sizeof(matrix3x4_t) * target->GetCachedBoneData().Count());
		target->ForceBoneCache();

		current_record = record;
		result_target = result;
		selected_points = 0;
		scanned_points = 0;

		while (!thread_work.empty())
			thread_work.pop();

		SelectPoints(record);

		std::unique_lock<std::mutex> lock(completed_mutex);
		result_condition.wait(lock, [this]() {
			return scanned_points >= selected_points;
		});
	}

	LagCompensation->BacktrackEntity(backup_record);
	delete backup_record;

	SelectBestPoint(result);
	if (!result->best_point.record || result->best_point.damage < 2) {
		WorldESP->AddDebugMessage(std::string(("CRageBot::ScanTarget")).append((" -> !SelectBestPoint |")).append((" L") + std::to_string(__LINE__)));
		return;
	}

	result->angle = Math::VectorAngles_p(result->best_point.point - ctx.shoot_position);

	if (frametime_issues || Exploits->IsShifting()) { // fast hitchance approx
		result->hitchance = min(10.f / ((ctx.shoot_position - result->best_point.point).Length() * std::tan(EnginePrediction->WeaponInaccuracy())), 1.f);
	}
	else {
		result->hitchance = CalcHitchance(result->angle, result->best_point.record, result->best_point.hitbox);
	}

}

void CRagebot::RunPrediction(const QAngle& angle) {
	if (!Cheat.LocalPlayer || !ctx.cmd)
		return;

	// only predict if player is on ground and moving
	if (!(Cheat.LocalPlayer->m_fFlags() & FL_ONGROUND))
		return;

	static constexpr float kMinMoveSpeedSqr = 16.0f;
	if (Cheat.LocalPlayer->m_vecVelocity().LengthSqr() < kMinMoveSpeedSqr)
		return;

	// skip if duck input is held
	if (ctx.cmd->buttons & IN_DUCK)
		return;

	// save current player state
	pre_prediction.should_restore = true;
	pre_prediction.m_vecAbsOrigin = Cheat.LocalPlayer->GetAbsOrigin();
	pre_prediction.m_fFlags = Cheat.LocalPlayer->m_fFlags();
	pre_prediction.m_flDuckAmount = Cheat.LocalPlayer->m_flDuckAmount();
	pre_prediction.m_flDuckSpeed = Cheat.LocalPlayer->m_flDuckSpeed();
	pre_prediction.m_vecVelocity = Cheat.LocalPlayer->m_vecVelocity();
	pre_prediction.m_vecAbsVelocity = Cheat.LocalPlayer->m_vecAbsVelocity();
	pre_prediction.m_hGroundEntity = Cheat.LocalPlayer->m_hGroundEntity();
	pre_prediction.m_vecMins = Cheat.LocalPlayer->m_vecMins();
	pre_prediction.m_vecMaxs = Cheat.LocalPlayer->m_vecMaxs();

	// make a local copy of the current command for safe modification
	CUserCmd stop_cmd = *ctx.cmd;
	CUserCmd* backup_cmd = ctx.cmd;
	ctx.cmd = &stop_cmd;

	// perform autos-stop
	AutoStop(true);

	// run engine prediction using the passed angle
	EnginePrediction->Repredict(ctx.cmd, angle);

	// restore the original command pointer
	ctx.cmd = backup_cmd;
}



void CRagebot::RestorePrediction() {
	if (!pre_prediction.should_restore)
		return;

	Cheat.LocalPlayer->GetAbsOrigin() = pre_prediction.m_vecAbsOrigin;
	Cheat.LocalPlayer->m_fFlags() = pre_prediction.m_fFlags;
	Cheat.LocalPlayer->m_flDuckAmount() = pre_prediction.m_flDuckAmount;
	Cheat.LocalPlayer->m_flDuckSpeed() = pre_prediction.m_flDuckSpeed;
	Cheat.LocalPlayer->m_vecVelocity() = pre_prediction.m_vecVelocity;
	Cheat.LocalPlayer->m_vecAbsVelocity() = pre_prediction.m_vecAbsVelocity;
	Cheat.LocalPlayer->m_hGroundEntity() = pre_prediction.m_hGroundEntity;
	Cheat.LocalPlayer->m_vecMins() = pre_prediction.m_vecMins;
	Cheat.LocalPlayer->m_vecMaxs() = pre_prediction.m_vecMaxs;
}

void CRagebot::Run() {
	// reset any previous prediction restore flag
	pre_prediction.should_restore = false;

	// basic sanity check
	if (!config.ragebot.aimbot.enabled->get() || !ctx.active_weapon || !Cheat.LocalPlayer)
		return;

	// handle revolver auto-open
	AutoRevolver();

	// if frozen or freeze period, bail
	if ((Cheat.LocalPlayer->m_fFlags() & FL_FROZEN) || GameRules()->IsFreezePeriod())
		return;

	// skip grenades
	if (ctx.active_weapon->IsGrenade())
		return;

	// cache frequently-used values
	const int weapon_id = ctx.active_weapon->m_iItemDefinitionIndex();
	const bool shifting = Exploits->IsShifting();

	// weapon-specific immediate actions (only when not shifting)
	if (!shifting) {
		if (weapon_id == Taser) {
			Zeusbot();
			return;
		}

		if (ctx.weapon_info && ctx.weapon_info->nWeaponType == WEAPONTYPE_KNIFE) {
			Knifebot();
			return;
		}

		// update frametime health check periodically
		if (GlobalVars->realtime - last_frametime_check > 5.0f) {
			last_frametime_check = GlobalVars->realtime;
			frametime_issues = EnginePrediction->frametime() > GlobalVars->interval_per_tick; // fps below tickrate
		}
	}

	// can't shoot? bail
	if (!ctx.active_weapon->ShootingWeapon())
		return;

	// load settings for current weapon
	settings = GetWeaponSettings(weapon_id);

	// dormant aimbot only when enabled and not shifting
	if (config.ragebot.aimbot.dormant_aim->get() && !shifting)
		DormantAimbot();

	// run local prediction if we're not shifting (use current view angles)
	if (!shifting) {
		QAngle vangle{};
		EngineClient->GetViewAngles(vangle);
		RunPrediction(vangle);
	}

	// disable interpolation for scanning, then restore
	hook_info.disable_interpolation = true;
	ScanTargets();
	hook_info.disable_interpolation = false;

	// best target tracker (initialize safely)
	ScannedTarget_t best_target{};
	best_target.player = nullptr;
	best_target.best_point.damage = -FLT_MAX; // ensure comparisons are valid
	bool should_autostop = false;

	// determine if local is considered on ground (both client and pred state)
	const bool local_client_on_ground = (Cheat.LocalPlayer->m_fFlags() & FL_ONGROUND) != 0;
	const bool local_pred_on_ground = (EnginePrediction->pre_prediction.m_fFlags & FL_ONGROUND) != 0;
	const bool local_on_ground = local_client_on_ground && local_pred_on_ground;

	// weapon mode (scoped or not)
	const int m_nWeaponMode = (Cheat.LocalPlayer->m_bIsScoped()) ? 1 : 0;

	// compute a dynamic jump-inaccuracy tangent if using dynamic autostop
	float min_jump_inaccuracy_tan = 0.0f;
	if (settings.auto_stop->get(2) && !local_on_ground) {
		// initial inaccuracy baseline (source field name preserved)
		float flInaccuracyJumpInitial = ctx.weapon_info ? ctx.weapon_info->_flInaccuracyUnknown : 0.0f;

		// use tuned speed factors (defensive: guard against negative values)
		float sv_jump_impulse = cvars.sv_jump_impulse->GetFloat();
		if (sv_jump_impulse < 0.0f) sv_jump_impulse = 0.0f;

		float fSqrtMaxJumpSpeed = std::sqrt(sv_jump_impulse);
		float local_vel_z = ctx.local_velocity.z;
		float fSqrtVerticalSpeed = std::sqrt(std::fabs(local_vel_z) * 0.7f);

		float flAirSpeedInaccuracy = Math::RemapVal(fSqrtVerticalSpeed,
			fSqrtMaxJumpSpeed * 0.25f,
			fSqrtMaxJumpSpeed,
			0.0f,
			flInaccuracyJumpInitial);

		// clamp flAirSpeedInaccuracy to [0, 2*flInaccuracyJumpInitial]
		if (flAirSpeedInaccuracy < 0.0f) flAirSpeedInaccuracy = 0.0f;
		const float maxAir = 2.0f * flInaccuracyJumpInitial;
		if (flAirSpeedInaccuracy > maxAir) flAirSpeedInaccuracy = maxAir;

		// combine stand/jump/in-air inaccuracies and compute tangent
		float stand_inacc = ctx.weapon_info ? ctx.weapon_info->flInaccuracyStand[m_nWeaponMode] : 0.0f;
		float jump_inacc = ctx.weapon_info ? ctx.weapon_info->flInaccuracyJump[m_nWeaponMode] : 0.0f;
		float total_inacc = stand_inacc + jump_inacc + flAirSpeedInaccuracy;
		min_jump_inaccuracy_tan = std::tan(total_inacc);
		// guard if tan becomes NaN/inf (use 0)
		if (!std::isfinite(min_jump_inaccuracy_tan)) min_jump_inaccuracy_tan = 0.0f;
	}

	// base hitchance (0..1)
	float hitchance = settings.hitchance->get() * 0.01f;

	// apply relaxed hitchance rules for certain weapons/conditions when strict_hitchance disabled
	if (!settings.strict_hitchance->get()) {
		bool reduce = false;

		if (weapon_id == Ssg08 && !local_on_ground)
			reduce = true;

		if ((weapon_id == Scar20 || weapon_id == G3SG1) && (GlobalVars->realtime - Exploits->last_teleport_time < TICKS_TO_TIME(16)))
			reduce = true;

		if (weapon_id == Awp) {
			auto animstate = Cheat.LocalPlayer->GetAnimstate();
			if (animstate && animstate->bLanding)
				reduce = true;
		}

		if (reduce)
			hitchance *= 0.8f;
	}

	// iterate all scanned targets and select best according to hitchance/damage logic
	for (const auto& target : scanned_targets) {
		// skip invalid scans
		if (target.best_point.damage <= 1.0f)
			continue;

		// compute a fast jump hitchance ceiling for dynamic autostop decisions
		float jump_max_hc = FastHitchance(target.best_point.record, min_jump_inaccuracy_tan);

		// if this is the previously targeted player and we recently shot them, prefer stability
		if (target.best_point.damage > target.minimum_damage &&
			(ctx.cmd->command_number - last_target_shot) < 256 &&
			target.player == last_target) {

			if (target.hitchance > hitchance) {
				best_target = target;
				should_autostop = true;
				break;
			}
			else if (local_on_ground || (settings.auto_stop->get(2) && jump_max_hc >= hitchance * 0.2f)) {
				should_autostop = true;
			}
		}

		// prefer this target if its hitchance is above threshold and it has better damage than current best
		float current_best_damage = best_target.best_point.damage;
		float target_min_cmp = (target.minimum_damage > current_best_damage) ? target.minimum_damage : current_best_damage;
		if (target.hitchance > hitchance && target.best_point.damage > target_min_cmp) {
			best_target = target;
		}

		// if target's damage exceeds minimum_damage, may want to autostop and/or autoscope
		if (target.best_point.damage > target.minimum_damage) {
			if (local_on_ground || (settings.auto_stop->get(2) && jump_max_hc >= hitchance * 0.2f))
				should_autostop = true;

			if (settings.auto_scope->get() &&
				!Cheat.LocalPlayer->m_bIsScoped() &&
				!Cheat.LocalPlayer->m_bResumeZoom() &&
				ctx.weapon_info && ctx.weapon_info->nWeaponType == WEAPONTYPE_SNIPER) {

				ctx.cmd->buttons |= IN_ATTACK2; // attempt to scope
			}
		}

		// if damage is significant, consider stopping and block exploit charge
		if (target.best_point.damage > 2.0f) {
			if (settings.auto_stop->get(0) &&
				(local_on_ground || (settings.auto_stop->get(2) && jump_max_hc >= hitchance)) &&
				target.best_point.damage > target.minimum_damage * 0.4f) {

				should_autostop = true;
			}

			Exploits->block_charge = local_on_ground;
		}
	}

	// can we shoot this tick?
	bool can_shoot_this_tick = ctx.active_weapon->CanShoot();

	// special revolver shot handling (force fire)
	if (best_target.player && ctx.active_weapon->m_iItemDefinitionIndex() == Revolver && ctx.active_weapon->CanShoot(false))
		ctx.cmd->buttons |= IN_ATTACK;

	// if we require autostop mode '1' and can't shoot (and not revolver), bail out
	if (settings.auto_stop->get(1) && !can_shoot_this_tick && ctx.active_weapon->m_iItemDefinitionIndex() != Revolver)
		return;

	// perform autostop if needed (and not mid-autopeek return)
	if (should_autostop && !AutoPeek->IsReturning())
		AutoStop();

	// if we just shifted exploits, bail — can't safely shoot
	if (Exploits->IsShifting())
		return;

	// final checks before firing
	if (!best_target.player || !can_shoot_this_tick)
		return;

	// ensure we won't restore prediction after shot
	pre_prediction.should_restore = false;

	// set tick to target simulation time + lerp so the shot hits the evaluated backtrack record
	ctx.cmd->tick_count = TIME_TO_TICKS(best_target.best_point.record->m_flSimulationTime + LagCompensation->GetLerpTime());

	// set viewangles to aim at point and compensate for punch (recoil)
	QAngle aim_ang = Math::VectorAngles_p(best_target.best_point.point - ctx.shoot_position);
	QAngle punch = Cheat.LocalPlayer->m_aimPunchAngle();
	float recoil_scale = cvars.weapon_recoil_scale->GetFloat();
	ctx.cmd->viewangles = aim_ang - punch * recoil_scale;

	// fire
	ctx.cmd->buttons |= IN_ATTACK;

	// exploit actions for doubletap/hideshots
	if (Exploits->GetExploitType() == CExploits::E_DoubleTap)
		Exploits->ForceTeleport();
	else if (Exploits->GetExploitType() == CExploits::E_HideShots)
		Exploits->HideShot();

	// if not fake_ducking, make sure a packet is sent
	if (!config.antiaim.misc.fake_duck->get())
		ctx.send_packet = true;

	// handle autopeek return and update last target info
	AutoPeek->Return();
	last_target = best_target.player;
	last_target_shot = ctx.cmd->command_number;

	// draw client impact visuals if enabled
	if (config.visuals.effects.client_impacts->get() && !config.misc.miscellaneous.gamesense_mode->get()) {
		Color face_col = config.visuals.effects.client_impacts_color->get();
		for (int i = 0; i < best_target.best_point.num_impacts; ++i)
			DebugOverlay->AddBox(best_target.best_point.impacts[i], Vector(-1, -1, -1), Vector(1, 1, 1), face_col, config.visuals.effects.impacts_duration->get());
	}

	// record the shot for history/analysis
	LagRecord* record = best_target.best_point.record;
	ShotManager->AddShot(ctx.shoot_position,
		best_target.best_point.point,
		best_target.best_point.damage,
		HitboxToDamagegroup(best_target.best_point.hitbox),
		best_target.hitchance,
		best_target.best_point.safe_point,
		record,
		best_target.best_point.impacts,
		best_target.best_point.num_impacts);

	if (config.visuals.chams.shot_chams->get())
		Chams->AddShotChams(record);

	// move accumulated debug messages into a sane bucket for rendering next tick
	if (!WorldESP->DebugMessages.empty())
		WorldESP->DebugMessagesSane = std::move(WorldESP->DebugMessages);

	WorldESP->DebugMessages.clear();
}


void CRagebot::Zeusbot() {
	const float inaccuracy_tan = std::tan(EnginePrediction->WeaponInaccuracy());

	if (!ctx.active_weapon->CanShoot())
		return;

	for (int i = 0; i < ClientState->m_nMaxClients; i++) {
		CBasePlayer* player = reinterpret_cast<CBasePlayer*>(EntityList->GetClientEntity(i));
		auto& records = LagCompensation->records(i);

		if (!player || player->IsTeammate() || !player->IsAlive() || player->m_bDormant() || player->m_bGunGameImmunity())
			continue;

		LagRecord* backup_record = LagCompensation->BackupData(player);

		for (auto i = records.rbegin(); i != records.rend(); i = std::next(i)) {
			const auto record = &*i;
			if (!LagCompensation->ValidRecord(record)) {

				WorldESP->AddDebugMessage(std::string(("CRageBot::Knifebot")).append((" -> Invalid Record |")).append((" L") + std::to_string(__LINE__)));

				if (record->breaking_lag_comp)
					break;

				continue;
			}

			float distance = ((record->m_vecOrigin + (record->m_vecMaxs + record->m_vecMins) * 0.5f) - ctx.shoot_position).LengthSqr();

			if (distance > 155 * 155)
				continue;

			LagCompensation->BacktrackEntity(record, false);
			record->BuildMatrix();

			const Vector points[]{
				player->GetHitboxCenter(HITBOX_STOMACH, record->clamped_matrix),
				player->GetHitboxCenter(HITBOX_CHEST, record->clamped_matrix),
				player->GetHitboxCenter(HITBOX_UPPER_CHEST, record->clamped_matrix),
				player->GetHitboxCenter(HITBOX_LEFT_UPPER_ARM, record->clamped_matrix),
				player->GetHitboxCenter(HITBOX_RIGHT_UPPER_ARM, record->clamped_matrix)
			};

			for (const auto& point : points) {
				CGameTrace trace = EngineTrace->TraceRay(ctx.shoot_position, point, MASK_SHOT | CONTENTS_GRATE, Cheat.LocalPlayer);

				if (trace.hit_entity != player) {
					CGameTrace clip_trace;
					Ray_t ray(ctx.shoot_position, point);
					if (!EngineTrace->ClipRayToPlayer(ray, MASK_SHOT | CONTENTS_GRATE, player, &clip_trace))
						continue;

					if (clip_trace.fraction < trace.fraction)
						trace = clip_trace;
					else
						continue;
				}

				float hitchance = min(12.f / ((ctx.shoot_position - point).Length() * inaccuracy_tan), 1.f);

				if (hitchance < 0.7f)
					continue;

				WorldESP->AddDebugMessage(std::string(("CRageBot::ZeusBot")).append((" -> Hitchance Success |")).append((" L") + std::to_string(__LINE__)));

				if (config.visuals.effects.client_impacts->get()) {
					Color col = config.visuals.effects.client_impacts_color->get();
					DebugOverlay->AddBox(trace.endpos, Vector(-1, -1, -1), Vector(1, 1, 1), col, config.visuals.effects.impacts_duration->get());
				}

				ctx.cmd->viewangles = Math::VectorAngles(point - ctx.shoot_position);
				ctx.cmd->buttons |= IN_ATTACK;
				ctx.cmd->tick_count = TIME_TO_TICKS(record->m_flSimulationTime + LagCompensation->GetLerpTime());

				if (!config.antiaim.misc.fake_duck->get())
					ctx.send_packet = true;

				if (config.visuals.chams.shot_chams->get())
					Chams->AddShotChams(record);

				LagCompensation->BacktrackEntity(backup_record);
				delete backup_record;
				return;
			}
		}

		LagCompensation->BacktrackEntity(backup_record);
		delete backup_record;
	}
}

void CRagebot::Knifebot() {
	const float inaccuracy_tan = std::tan(EnginePrediction->WeaponInaccuracy());

	if (!ctx.active_weapon->CanShoot())
		return;

	for (int i = 0; i < ClientState->m_nMaxClients; i++) {
		CBasePlayer* player = reinterpret_cast<CBasePlayer*>(EntityList->GetClientEntity(i));
		auto& records = LagCompensation->records(i);

		if (!player || player->IsTeammate() || !player->IsAlive() || player->m_bDormant() || player->m_bGunGameImmunity())
			continue;

		LagRecord* backup_record = LagCompensation->BackupData(player);

		for (auto i = records.rbegin(); i != records.rend(); i = std::next(i)) {
			const auto record = &*i;
			if (!LagCompensation->ValidRecord(record)) {

				WorldESP->AddDebugMessage(std::string(("CRageBot::Knifebot")).append((" -> Invalid Record |")).append((" L") + std::to_string(__LINE__)));

				if (record->breaking_lag_comp)
					break;

				continue;
			}

			Vector target_position = record->m_vecOrigin;
			target_position.z = std::clamp(ctx.shoot_position.z, record->m_vecOrigin.z, record->m_vecOrigin.z + record->m_vecMaxs.z) - 0.01f; // shitty valve tracer is broken if start.z == end.z

			float distance = (target_position - ctx.shoot_position).LengthSqr();

			if (distance > 6400.f) { // out of range
				if (distance < 32768.f)
					Exploits->block_charge = true;
				continue;
			}

			Exploits->block_charge = true;

			Vector vTragetForward = Math::AngleVectors(QAngle(0, record->m_angEyeAngles.yaw));
			vTragetForward.z = 0.f;

			Vector vecLOS = (record->m_vecOrigin - Cheat.LocalPlayer->GetAbsOrigin());
			vecLOS.z = 0.f;
			vecLOS.Normalize();

			float flDot = vecLOS.Dot(vTragetForward);

			bool can_backstab = false;
			if (flDot > 0.475f)
				can_backstab = true;

			int health = player->m_iHealth();
			bool first_swing = ctx.active_weapon->m_flNextPrimaryAttack() + 0.4f < GlobalVars->curtime;

			int left_click_dmg = 25;
			int right_click_dmg = 65;

			if (can_backstab) {
				left_click_dmg = 90;
				right_click_dmg = 180;
			}
			else if (first_swing) {
				left_click_dmg = 40;
			}

			bool should_right_click = false;

			if (right_click_dmg >= health && left_click_dmg < health)
				should_right_click = true;

			float knife_range = should_right_click ? 32.f : 48.f;

			Vector vecDelta = (target_position - ctx.shoot_position).Normalized();
			Vector vecEnd = ctx.shoot_position + vecDelta * knife_range;

			LagCompensation->BacktrackEntity(record);

			CGameTrace trace = EngineTrace->TraceHull(ctx.shoot_position, vecEnd, Vector(-16, -16, -18), Vector(16, 16, 18), MASK_SOLID, Cheat.LocalPlayer);

			if (trace.hit_entity != player || trace.fraction == 1.f) {
				CGameTrace clipped_trace;
				if (EngineTrace->ClipRayToPlayer(Ray_t(ctx.shoot_position, vecEnd + vecDelta * 18.f), MASK_SHOT | CONTENTS_GRATE, player, &clipped_trace) && clipped_trace.fraction < trace.fraction)
					trace = clipped_trace;
			}

			if (trace.hit_entity != player || ((trace.endpos - ctx.shoot_position).LengthSqr() > (knife_range * knife_range)))
				continue;

			ctx.cmd->viewangles = Math::VectorAngles(target_position - ctx.shoot_position);
			ctx.cmd->buttons |= should_right_click ? IN_ATTACK2 : IN_ATTACK;
			ctx.cmd->tick_count = TIME_TO_TICKS(record->m_flSimulationTime + LagCompensation->GetLerpTime());

			if ((should_right_click ? right_click_dmg : left_click_dmg) < health && Exploits->GetExploitType() == CExploits::E_DoubleTap)
				Exploits->ForceTeleport();

			ctx.last_shot_time = GlobalVars->realtime + 0.5f; // prevent from charging

			if (config.visuals.chams.shot_chams->get()) {
				memcpy(record->clamped_matrix, record->bone_matrix, sizeof(matrix3x4_t) * 128);
				Chams->AddShotChams(record);
			}

			LagCompensation->BacktrackEntity(backup_record);
			delete backup_record;
			return;
		}

		LagCompensation->BacktrackEntity(backup_record);
		delete backup_record;
	}
}

void CRagebot::AutoRevolver() {
	if (ctx.cmd->buttons & IN_ATTACK)
		return;

	if (ctx.active_weapon->m_iItemDefinitionIndex() != Revolver)
		return;

	ctx.cmd->buttons &= ~IN_ATTACK2;

	static float next_cock_time = 0.f;
	float time = TICKS_TO_TIME(Cheat.LocalPlayer->m_nTickBase());

	if (ctx.active_weapon->m_flPostponeFireReadyTime() > time) {
		if (time > next_cock_time)
			ctx.cmd->buttons |= IN_ATTACK;
	}
	else {
		next_cock_time = time + 0.25f;
	}
}

void CRagebot::DormantAimbot() {

	const float inaccuracy_tan = std::tan(EnginePrediction->WeaponInaccuracy());

	if (!ctx.active_weapon->CanShoot() && settings.auto_stop->get(2)) {
		WorldESP->AddDebugMessage(std::string(("CRageBot::DormantAimbot")).append(" -> NoShoot |").append((" L") + std::to_string(__LINE__)));

		return;
	}

	for (int i = 0; i < ClientState->m_nMaxClients; i++) {
		CBasePlayer* player = reinterpret_cast<CBasePlayer*>(EntityList->GetClientEntity(i));

		if (!player || !player->m_bDormant() || !player->IsAlive() || player->IsTeammate())
			continue;

		if (EnginePrediction->curtime() - WorldESP->GetESPInfo(i).m_flLastUpdateTime > 5.f)
			continue;

		Vector shoot_target = player->m_vecOrigin() + Vector(0, 0, 36.013f);

		FireBulletData_t bullet;
		if (!AutoWall->FireBullet(Cheat.LocalPlayer, ctx.shoot_position, shoot_target, bullet) || bullet.damage < CalcMinDamage(player) || (bullet.enterTrace.hit_entity != nullptr && bullet.enterTrace.hit_entity->IsPlayer()))
			continue;

		float hc = min(7.f / ((ctx.shoot_position - shoot_target).Length() * inaccuracy_tan), 1.f);

		AutoStop();
		if (hc * 100.f < settings.hitchance->get() || !ctx.active_weapon->CanShoot()) {
			WorldESP->AddDebugMessage(std::string(("CRageBot::DormantAimbot")).append(" -> NoShoot |").append((" L") + std::to_string(__LINE__)));
			continue;
		}

		ctx.cmd->viewangles = Math::VectorAngles_p(shoot_target - ctx.shoot_position) - Cheat.LocalPlayer->m_aimPunchAngle() * cvars.weapon_recoil_scale->GetFloat();
		ctx.cmd->buttons |= IN_ATTACK;

		if (config.visuals.effects.client_impacts->get()) {
			for (const auto& impact : bullet.impacts)
				DebugOverlay->AddBoxOverlay(impact, Vector(-1, -1, -1), Vector(1, 1, 1), QAngle(),
					config.visuals.effects.client_impacts_color->get().r,
					config.visuals.effects.client_impacts_color->get().g,
					config.visuals.effects.client_impacts_color->get().b,
					config.visuals.effects.client_impacts_color->get().a,
					config.visuals.effects.impacts_duration->get());
		}

		if (Exploits->GetExploitType() == CExploits::E_DoubleTap)
			Exploits->ForceTeleport();
		else if (Exploits->GetExploitType() == CExploits::E_HideShots)
			Exploits->HideShot();
		if (!config.antiaim.misc.fake_duck->get())
			ctx.send_packet = true;

		AutoPeek->Return();

		break;
	}

}

CRagebot* Ragebot = new CRagebot;