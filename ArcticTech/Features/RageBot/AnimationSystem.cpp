#include "AnimationSystem.h"
#include "LagCompensation.h"
#include "../AntiAim/AntiAim.h"
#include "Resolver.h"
#include "Exploits.h"
#include "../Lua/Bridge/Bridge.h"
#include "../Misc/Prediction.h"

void CAnimationSystem::CorrectLocalMatrix(matrix3x4_t* mat, int size) {
	Utils::MatrixMove(mat, size, sent_abs_origin, Cheat.LocalPlayer->GetAbsOrigin());
}

void FixLegMovement(AnimationLayer* server_layers) {
	if (config.antiaim.misc.leg_movement->get() != 1 && !(config.ragebot.aimbot.peek_assist->get() && config.ragebot.aimbot.peek_assist_keybind->get()))
		return;

	if (!(Cheat.LocalPlayer->m_fFlags() & FL_ONGROUND))
		return;

	if (Cheat.LocalPlayer->m_vecVelocity().LengthSqr() < 4096)
		return;

	CCSGOPlayerAnimationState* animstate = Cheat.LocalPlayer->GetAnimstate();

	float delta = Math::AngleDiff(ctx.cmd->viewangles.yaw, ctx.leg_slide_angle.yaw);
	float moveYaw = Math::AngleDiff(Math::VectorAngles(Cheat.LocalPlayer->m_vecVelocity()).yaw, ctx.cmd->viewangles.yaw);

	Cheat.LocalPlayer->m_flPoseParameter()[STRAFE_YAW] = Math::AngleToPositive(Math::AngleNormalize(moveYaw + delta)) / 360.f;

	AnimationLayer* layers = Cheat.LocalPlayer->GetAnimlayers();

	if (server_layers[ANIMATION_LAYER_MOVEMENT_STRAFECHANGE].m_flWeight > layers[ANIMATION_LAYER_MOVEMENT_STRAFECHANGE].m_flWeight)
		layers[ANIMATION_LAYER_MOVEMENT_STRAFECHANGE].m_flWeight = server_layers[ANIMATION_LAYER_MOVEMENT_STRAFECHANGE].m_flWeight;
}

void CAnimationSystem::UpdateLocalAnimations(CUserCmd* cmd) {
	if (local_anims.command_number == cmd->command_number)
		return;

	local_anims.command_number = cmd->command_number;

	CCSGOPlayerAnimationState* animstate = Cheat.LocalPlayer->GetAnimstate();
	AnimationLayer* animlayers = Cheat.LocalPlayer->GetAnimlayers();

	AnimationLayer animlayers_backup[13];
	float curtimeBackup = GlobalVars->curtime;
	float frametimeBackup = GlobalVars->frametime;
	memcpy(animlayers_backup, animlayers, sizeof(AnimationLayer) * 13);

	auto tb_info = Exploits->GetTickbaseInfo(cmd->command_number);

	GlobalVars->curtime = TICKS_TO_TIME(Cheat.LocalPlayer->m_nTickBase());

	LUA_CALL_HOOK(LUA_PRE_ANIMUPDATE, Cheat.LocalPlayer);

	bool bSendPacket = tb_info->sent;

	QAngle vangle = cmd->viewangles;
	if (bSendPacket && ctx.force_shot_angle) {
		vangle = ctx.shot_angles;
		ctx.force_shot_angle = false;
	}	

	animstate->flLastUpdateTime = local_anims.last_update;
	animstate->ForceUpdate();
	animstate->Update(vangle);
	animstate->bLanding = animlayers[ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB].m_flWeight > 0.f && animstate->bOnGround; // fuck valve broken code that sets bLanding to false

	if (bSendPacket && !Exploits->IsHidingShot(cmd) && animstate->nLastUpdateFrame == GlobalVars->framecount) {
		FixLegMovement(animlayers_backup);

		sent_abs_origin = Cheat.LocalPlayer->GetAbsOrigin();
		Cheat.LocalPlayer->SetAbsAngles(QAngle(0.f, animstate->flFootYaw));

		LUA_CALL_HOOK(LUA_POST_ANIMUPDATE, Cheat.LocalPlayer);

		QAngle backup_eye_angles = Cheat.LocalPlayer->m_angEyeAngles();
		QAngle backup_render_angles = Cheat.LocalPlayer->v_angle();
		Cheat.LocalPlayer->v_angle() = Cheat.LocalPlayer->m_angEyeAngles() = vangle;

		Cheat.LocalPlayer->m_BoneAccessor().m_ReadableBones = 0;
		Cheat.LocalPlayer->m_BoneAccessor().m_WritableBones = 0;

		Cheat.LocalPlayer->m_nOcclusionFrame() = 0;
		Cheat.LocalPlayer->m_nOcclusionFlags() = 0;

		BuildMatrix(Cheat.LocalPlayer, local_matrix, 128, BONE_USED_BY_ANYTHING, nullptr);
		Cheat.LocalPlayer->v_angle() = backup_render_angles;
		Cheat.LocalPlayer->m_angEyeAngles() = backup_eye_angles;
	}

	memcpy(local_anims.layers, animlayers, sizeof(AnimationLayer) * 13);

	local_anims.last_update = GlobalVars->curtime;

	GlobalVars->curtime = curtimeBackup;
	memcpy(animlayers, animlayers_backup, sizeof(AnimationLayer) * 13);
}

void CAnimationSystem::UpdatePredictionAnimation() {
	CCSGOPlayerAnimationState* animstate = Cheat.LocalPlayer->GetAnimstate();

	float curtime_backup = GlobalVars->curtime;
	std::array<float, 24> poseparam_backup = Cheat.LocalPlayer->m_flPoseParameter();
	AnimationLayer animlayer_backup[13];

	memcpy(animlayer_backup, Cheat.LocalPlayer->GetAnimlayers(), sizeof(AnimationLayer) * 13);
	GlobalVars->curtime = TICKS_TO_TIME(Cheat.LocalPlayer->m_nTickBase());

	Cheat.LocalPlayer->m_flPoseParameter()[12] = (ctx.cmd->viewangles.pitch + 90.f) / 180.f;
	Cheat.LocalPlayer->SetAbsAngles(QAngle(0, animstate->flFootYaw));

	BuildMatrix(Cheat.LocalPlayer, prediction_matrix, 128, BONE_USED_BY_HITBOX, local_anims.layers);

	Cheat.LocalPlayer->m_flPoseParameter() = poseparam_backup;
	memcpy(Cheat.LocalPlayer->GetAnimlayers(), animlayer_backup, sizeof(AnimationLayer) * 13);
	GlobalVars->curtime = curtime_backup;
}

void CAnimationSystem::FrameStageNotify(EClientFrameStage stage) {
	switch (stage)
	{
	case FRAME_NET_UPDATE_POSTDATAUPDATE_END:
		for (int i = 0; i < ClientState->m_nMaxClients; ++i) {
			CBasePlayer* player = reinterpret_cast<CBasePlayer*>(EntityList->GetClientEntity(i));

			if (player && player != Cheat.LocalPlayer)
				DisableInterpolationFlags(player);
		}
		break;
	case FRAME_NET_UPDATE_END:
		break;
	case FRAME_RENDER_START:
		break;
	default:
		break;
	}
}

void CAnimationSystem::BuildMatrix(CBasePlayer* player, matrix3x4_t* boneToWorld, int maxBones, int mask, AnimationLayer* animlayers) {
	player->InvalidateBoneCache();

	if (animlayers != nullptr)
		memcpy(player->GetAnimlayers(), animlayers, sizeof(AnimationLayer) * 13);

	bool backupMaintainSequenceTransitions = player->m_bMaintainSequenceTransitions();
	int backupEffects = player->m_fEffects();
	int clientFlags = player->m_EntClientFlags();

	player->m_EntClientFlags() |= 2;
	player->m_fEffects() |= EF_NOINTERP; // Disable interp
	//player->m_bMaintainSequenceTransitions() = false; // uhhhh, idk

	hook_info.setup_bones = true;
	player->SetupBones(boneToWorld, maxBones, mask, GlobalVars->curtime);
	hook_info.setup_bones = false;

	player->m_fEffects() = backupEffects;
	//player->m_bMaintainSequenceTransitions() = backupMaintainSequenceTransitions;
	player->m_EntClientFlags() = clientFlags;
}

void CAnimationSystem::DisableInterpolationFlags(CBasePlayer* player) {
	auto& var_mapping = player->m_VarMapping();

	for (int i = 0; i < var_mapping.m_nInterpolatedEntries; ++i)
		var_mapping.m_Entries[i].m_bNeedsToInterpolate = false;
}

void CAnimationSystem::UpdateAnimations(CBasePlayer* player, LagRecord* record, std::deque<LagRecord>& records) {
	CCSGOPlayerAnimationState* animstate = player->GetAnimstate();
	const int idx = player->EntIndex();

	if (!animstate || animstate->pEntity != player)
		return;

	record->player = player;
	record->m_vecAbsOrigin = player->m_vecOrigin();

	if (player->GetActiveWeapon())
		if (player->GetActiveWeapon()->m_fLastShotTime() <= player->m_flSimulationTime())
			if (player->GetActiveWeapon()->m_fLastShotTime() > player->m_flOldSimulationTime())
				record->shooting = true;

    QAngle& eye_angles = player->m_angEyeAngles();

    if (config.ragebot.aimbot.pitch_resolver->get() && (std::abs(eye_angles.pitch) > 90 || eye_angles.pitch < -88.f))
        eye_angles.pitch = 89.f;

	auto backupRealtime = GlobalVars->realtime;
	auto backupCurtime = GlobalVars->curtime;
	auto backupFrametime = GlobalVars->frametime;
	auto backupAbsFrametime = GlobalVars->absoluteframetime;
	auto backupInterp = GlobalVars->interpolation_amount;
	auto backupTickcount = GlobalVars->tickcount;
	auto backupFramecount = GlobalVars->framecount;
	auto backupAbsOrigin = player->GetAbsOrigin();
	auto backupAbsVelocity = player->m_vecAbsVelocity();
	auto backupLBY = player->m_flLowerBodyYawTarget();
	auto nOcclusionMask = player->m_nOcclusionFlags();
	auto nOcclusionFrame = player->m_nOcclusionFrame();
	auto backupAbsAngles = player->GetAbsAngles();
	auto backupiEFlags = player->m_iEFlags();

	GlobalVars->realtime = player->m_flSimulationTime();
	GlobalVars->curtime = player->m_flSimulationTime();
	GlobalVars->frametime = GlobalVars->interval_per_tick;
	GlobalVars->absoluteframetime = GlobalVars->interval_per_tick;
	GlobalVars->interpolation_amount = 0.f;
	GlobalVars->tickcount = TIME_TO_TICKS(player->m_flSimulationTime());
	GlobalVars->framecount = TIME_TO_TICKS(player->m_flSimulationTime());

	memcpy(record->animlayers, player->GetAnimlayers(), 13 * sizeof(AnimationLayer));

	if (record->prev_record) {
		animstate->flMoveWeight = record->prev_record->animlayers[ANIMATION_LAYER_MOVEMENT_MOVE].m_flWeight;
		animstate->flPrimaryCycle = record->prev_record->animlayers[ANIMATION_LAYER_MOVEMENT_MOVE].m_flCycle;
		animstate->flAccelerationWeight = record->prev_record->animlayers[ANIMATION_LAYER_MOVEMENT_STRAFECHANGE].m_flWeight;
		animstate->flDurationInAir = 0.f;

		float server_time_diff = record->m_flServerTime - record->prev_record->m_flServerTime;
		float sim_time_diff = record->m_flSimulationTime - record->prev_record->m_flSimulationTime;
		float time_diff = 0.f;

		if (sim_time_diff < 0.f || abs(sim_time_diff - server_time_diff) > TICKS_TO_TIME(4)) // shifting tickbase
			time_diff = server_time_diff;
		else
			time_diff = sim_time_diff;

		if (time_diff <= GlobalVars->interval_per_tick)
			time_diff = GlobalVars->interval_per_tick;

		record->m_nChokedTicks = std::clamp(TIME_TO_TICKS(time_diff) - 1, 0, 14);

		Vector origin_diff = player->m_vecOrigin() - record->prev_record->m_vecOrigin;

		player->m_vecVelocity() = origin_diff / time_diff;

		if (player->m_fFlags() & FL_ONGROUND) {
			player->m_vecVelocity().z = 0.f;

			float max_speed = 260.f;

			auto weapon = player->GetActiveWeapon();
			CCSWeaponData* weapon_info = nullptr;

			if (weapon)
				weapon_info = weapon->GetWeaponInfo();

			if (weapon_info)
				max_speed = player->m_bIsScoped() ? weapon_info->flMaxSpeedAlt : weapon_info->flMaxSpeed;

			float vel_length = player->m_vecVelocity().Length();
			if (vel_length > max_speed)
				player->m_vecVelocity() *= max_speed / vel_length;

			float anim_speed = 0.f;

			if (record->prev_record->m_fFlags & FL_ONGROUND
				&& record->animlayers[11].m_flWeight > 0.f
				&& record->animlayers[11].m_flWeight < 1.f
				&& record->animlayers[11].m_flPlaybackRate == record->prev_record->animlayers[11].m_flPlaybackRate) {
				auto anim_modifier = 0.35f * (1.f - record->animlayers[11].m_flWeight);
				if (anim_modifier > 0.f && anim_modifier < 1.f) {
					float vel_mod = anim_modifier + 0.55f;
					if (vel_mod < 0.9f || vel_length < vel_mod * max_speed)
						anim_speed = max_speed * vel_mod;
				}
			}

			if (anim_speed > 0.f)
				player->m_vecVelocity() *= anim_speed / player->m_vecVelocity().Length();

			if (record->animlayers[ANIMATION_LAYER_MOVEMENT_MOVE].m_flWeight <= 0.f)
				player->m_vecVelocity() = Vector(0, 0, 0);
		}
		else {
			float last_vel = record->prev_record->m_vecVelocity.LengthSqr();
			if (last_vel > (100.f * 100.f) && last_vel * 16.f < player->m_vecVelocity().LengthSqr()) // we teleported
				player->m_vecVelocity() *= 0.22f; // 3/14 or 2/14 should be more correct

			animstate->flDurationInAir = record->prev_record->animlayers[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL].m_flCycle / record->prev_record->animlayers[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL].m_flPlaybackRate;
		}
	}

	if (animstate->nLastUpdateFrame >= GlobalVars->framecount)
		animstate->nLastUpdateFrame = GlobalVars->framecount - 1;

	unupdated_animstate[idx] = *animstate;

	auto pose_params = player->m_flPoseParameter();

	player->SetAbsVelocity(player->m_vecVelocity());
	player->SetAbsOrigin(player->m_vecOrigin());

	player->m_BoneAccessor().m_ReadableBones = 0;
	player->m_BoneAccessor().m_WritableBones = 0;

	player->m_nOcclusionFrame() = 0;
	player->m_nOcclusionFlags() = 0;

	if (player->IsEnemy()) {
		Resolver->Run(player, record, records);

		*animstate = unupdated_animstate[idx];
		memcpy(player->GetAnimlayers(), record->animlayers, sizeof(AnimationLayer) * 13);

		Resolver->Apply(record);
		player->UpdateClientSideAnimation();
	}
	else {
		player->UpdateClientSideAnimation();
	}

	record->animlayers[12].m_flWeight = 0.f;

	hook_info.disable_clamp_bones = true;
	BuildMatrix(player, record->aim_matrix, 128, BONE_USED_BY_ANYTHING, record->animlayers);
	hook_info.disable_clamp_bones = false;

	interpolate_data_t* lerp_data = &interpolate_data[idx];
	lerp_data->net_origin = player->m_vecOrigin();

	memcpy(record->bone_matrix, record->aim_matrix, sizeof(matrix3x4_t) * 128);
	player->ClampBonesInBBox(record->bone_matrix, BONE_USED_BY_ANYTHING);
	memcpy(lerp_data->original_matrix, record->bone_matrix, sizeof(matrix3x4_t) * 128);

	record->bone_matrix_filled = true;

	if (player->IsEnemy()) {
		float deltaOriginal = Math::AngleDiff(animstate->flEyeYaw, animstate->flFootYaw);
		float eyeYawNew = Math::AngleNormalize(animstate->flEyeYaw + deltaOriginal);
		player->SetAbsAngles(QAngle(0, eyeYawNew, 0));
		player->m_flPoseParameter()[BODY_YAW] = 1.f - player->m_flPoseParameter()[BODY_YAW]; // opposite side

		hook_info.disable_clamp_bones = true;
		BuildMatrix(player, record->opposite_matrix, 128, BONE_USED_BY_HITBOX, record->animlayers);
		hook_info.disable_clamp_bones = false;

		if (config.ragebot.aimbot.show_aimpoints->get())
			DebugOverlay->AddBoxOverlay(player->GetHitboxCenter(HITBOX_HEAD, record->opposite_matrix), Vector(-1, -1, -1), Vector(1, 1, 1), QAngle(), 12, 255, 12, 160, 0.1f);
	}

	player->m_nOcclusionFrame() = nOcclusionFrame;
	player->m_nOcclusionFlags() = nOcclusionMask;
	player->m_iEFlags() = backupiEFlags;

	player->SetAbsOrigin(backupAbsOrigin);
	player->m_vecAbsVelocity() = backupAbsVelocity;

	GlobalVars->realtime = backupRealtime;
	GlobalVars->curtime = backupCurtime;
	GlobalVars->frametime = backupFrametime;
	GlobalVars->absoluteframetime = backupAbsFrametime;
	GlobalVars->interpolation_amount = backupInterp;
	GlobalVars->tickcount = backupTickcount;
	GlobalVars->framecount = backupFramecount;
	
	player->SetAbsAngles(backupAbsAngles);
	player->m_flLowerBodyYawTarget() = backupLBY;
	memcpy(player->GetAnimlayers(), record->animlayers, sizeof(AnimationLayer) * 13);
	memcpy(player->GetCachedBoneData().Base(), record->bone_matrix, sizeof(matrix3x4_t) * player->GetCachedBoneData().Count());
	player->m_flPoseParameter() = pose_params;
}

Vector CAnimationSystem::GetInterpolated(CBasePlayer* player) {
	return interpolate_data[player->EntIndex()].origin;
}

void CAnimationSystem::RunInterpolation() {
	if (!Cheat.InGame)
		return;

	for (int i = 0; i < 64; i++) {
		CBasePlayer* player = reinterpret_cast<CBasePlayer*>(EntityList->GetClientEntity(i));

		if (!player || player == Cheat.LocalPlayer || !player->IsAlive() || player->m_bDormant())
			continue;

		interpolate_data_t* data = &interpolate_data[i];

		if ((data->net_origin - data->origin).LengthSqr() > 8192) {
			data->origin = data->net_origin;
			data->valid = false;
			continue;
		}

		if (!data->valid)
			data->origin = data->net_origin;

		float lerp_amt = 24.f;

		if (Cheat.LocalPlayer && Cheat.LocalPlayer->IsAlive() && player->IsEnemy())
			lerp_amt = 34.f; // speed up interpolation

		data->valid = true;
		data->origin += (data->net_origin - data->origin) * std::clamp(GlobalVars->frametime * lerp_amt, 0.f, 0.8f);
	}
}

void CAnimationSystem::InterpolateModel(CBasePlayer* player, matrix3x4_t* matrix) {
	if (player == Cheat.LocalPlayer)
		return;

	interpolate_data_t* data = &interpolate_data[player->EntIndex()];

	if (!data->valid)
		return;

	Utils::MatrixMove(data->original_matrix, matrix, player->GetCachedBoneData().Count(), data->net_origin, data->origin);
}

void CAnimationSystem::ResetInterpolation() {
	for (int i = 0; i < 64; i++)
		interpolate_data[i].valid = false;
}

void CAnimationSystem::InvalidateInterpolation(int i) {
	interpolate_data[i].valid = false;
}

CAnimationSystem* AnimationSystem = new CAnimationSystem;