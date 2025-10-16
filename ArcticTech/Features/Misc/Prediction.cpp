#include "Prediction.h"
#include "../../SDK/Interfaces.h"
#include "../../SDK/Globals.h"

#include "../RageBot/AnimationSystem.h"
#include "../RageBot/Exploits.h"

#include "../../Utils/Console.h"

void CPrediction::StartCommand(CBasePlayer* player, CUserCmd* cmd) {
	*Cheat.LocalPlayer->GetCurrentCommand() = cmd;
	Cheat.LocalPlayer->GetLastCommand() = *cmd;
	*predictionRandomSeed = cmd->random_seed;
	*predictionEntity = Cheat.LocalPlayer;
}

void CPrediction::RunPreThink(CBasePlayer* player) {
	if (!player->PhysicsRunThink(0))
		return;

	player->PreThink();
}

void CPrediction::RunThink(CBasePlayer* player) {
	//static auto SetNextThink = reinterpret_cast<void(__thiscall*)(int)>(Utils::PatternScan("client.dll", "55 8B EC 56 57 8B F9 8B B7 ? ? ? ? 8B C6"));
	static auto SetNextThink = reinterpret_cast<void(__thiscall*)(CBasePlayer*, int)>(Utils::PatternScan("client.dll", "55 8B EC 56 57 8B F9 8B B7 ? ? ? ? 8B C6"));
	int thinktick = player->m_nNextThinkTick();

	if (thinktick <= 0 || thinktick > player->m_nTickBase())
		return;

	//SetNextThink(0);
	SetNextThink(player, 0);

	player->Think();
}

void CPrediction::BackupData() {
	if (ctx.active_weapon) {
		if (ctx.active_weapon->IsGrenade())
			pre_prediction.m_fThrowTime = reinterpret_cast<CBaseGrenade*>(ctx.active_weapon)->m_flThrowTime();
		pre_prediction.m_flNextPrimaryAttack = ctx.active_weapon->m_flNextPrimaryAttack();

		weaponAccuracyPenality = ctx.active_weapon->m_fAccuracyPenalty();
		weaponRecoilIndex = ctx.active_weapon->m_flRecoilIndex();
	}

	pre_prediction.m_fFlags = Cheat.LocalPlayer->m_fFlags();
	pre_prediction.m_vecOrigin = Cheat.LocalPlayer->m_vecOrigin();
	pre_prediction.m_vecAbsOrigin = Cheat.LocalPlayer->GetAbsOrigin();
	pre_prediction.m_vecVelocity = Cheat.LocalPlayer->m_vecVelocity();
	pre_prediction.m_vecAbsVelocity = Cheat.LocalPlayer->m_vecAbsVelocity();
	pre_prediction.m_flDuckAmount = Cheat.LocalPlayer->m_flDuckAmount();
	pre_prediction.m_flDuckSpeed = Cheat.LocalPlayer->m_flDuckSpeed();
	pre_prediction.m_hGroundEntity = Cheat.LocalPlayer->m_hGroundEntity();
	pre_prediction.m_vecMins = Cheat.LocalPlayer->m_vecMins();
	pre_prediction.m_vecMaxs = Cheat.LocalPlayer->m_vecMaxs();

	flOldCurrentTime = GlobalVars->curtime;
	flOldFrameTime = GlobalVars->frametime;
	iOldTickcount = GlobalVars->tickcount;

	bOldIsFirstPrediction = Prediction->bIsFirstTimePredicted;
	bOldInPrediction = Prediction->bInPrediction;
}

void CPrediction::Update() {
	Prediction->Update(ClientState->m_nDeltaTick, ClientState->m_nDeltaTick > 0, ClientState->m_nLastCommandAck, ClientState->m_nLastOutgoingCommand + ClientState->m_nChokedCommands);
}

void CPrediction::Start(CUserCmd* cmd) {
	if (!MoveHelper)
		return;

	local_data[cmd->command_number % MULTIPLAYER_BACKUP].init(cmd);

	BackupData();
	StartCommand(Cheat.LocalPlayer, cmd);

	const float backup_velocity_modifier = Cheat.LocalPlayer->m_flVelocityModifier();

	GlobalVars->curtime = TICKS_TO_TIME(Cheat.LocalPlayer->m_nTickBase());
	GlobalVars->frametime = Prediction->bEnginePaused ? 0.f : GlobalVars->interval_per_tick;

	Prediction->bInPrediction = true;
	Prediction->bIsFirstTimePredicted = false;

	GameMovement->StartTrackPredictionErrors(Cheat.LocalPlayer);

	Cheat.LocalPlayer->UpdateButtonState(cmd->buttons);
	memset(&moveData, 0, sizeof(moveData));

	Prediction->CheckMovingGround(Cheat.LocalPlayer, GlobalVars->frametime);
	Prediction->SetLocalViewAngles(cmd->viewangles);

	//RunPreThink(Cheat.LocalPlayer);
	//RunThink(Cheat.LocalPlayer);

	MoveHelper->SetHost(Cheat.LocalPlayer);
	Prediction->SetupMove(Cheat.LocalPlayer, cmd, MoveHelper, &moveData);
	GameMovement->ProcessMovement(Cheat.LocalPlayer, &moveData);
	Prediction->FinishMove(Cheat.LocalPlayer, cmd, &moveData);

	//MoveHelper->ProcessImpacts();

	//Cheat.LocalPlayer->PostThink();

	Cheat.LocalPlayer->m_flVelocityModifier() = backup_velocity_modifier;

	GameMovement->FinishTrackPredictionErrors(Cheat.LocalPlayer);
	MoveHelper->SetHost(nullptr);

	if (ctx.active_weapon) {
		ctx.active_weapon->UpdateAccuracyPenality();
		weaponInaccuracy = ctx.active_weapon->GetInaccuracy();
		weaponSpread = ctx.active_weapon->GetSpread();
	}

	AnimationSystem->UpdatePredictionAnimation();
	ctx.shoot_position = Cheat.LocalPlayer->GetShootPosition();
}

void CPrediction::End() {
	if (!MoveHelper)
		return;

	GlobalVars->curtime = flOldCurrentTime;
	GlobalVars->frametime = flOldFrameTime;
	GlobalVars->tickcount = iOldTickcount;

	*Cheat.LocalPlayer->GetCurrentCommand() = nullptr;
	*predictionRandomSeed = -1;
	*predictionEntity = nullptr;

	if (ctx.active_weapon) {
		ctx.active_weapon->m_fAccuracyPenalty() = weaponAccuracyPenality;
		ctx.active_weapon->m_flRecoilIndex() = weaponRecoilIndex;
	}

	GameMovement->Reset();

	Prediction->bInPrediction = bOldInPrediction;
	Prediction->bIsFirstTimePredicted = bOldIsFirstPrediction;
}

void CPrediction::Repredict(CUserCmd* cmd, QAngle angles) {
	*Cheat.LocalPlayer->GetCurrentCommand() = cmd;
	Cheat.LocalPlayer->GetLastCommand() = *cmd;

	auto backup_angle = cmd->viewangles;
	cmd->viewangles = angles;

	Cheat.LocalPlayer->m_fFlags() = pre_prediction.m_fFlags;
	Cheat.LocalPlayer->m_vecOrigin() = pre_prediction.m_vecOrigin;
	Cheat.LocalPlayer->SetAbsOrigin(pre_prediction.m_vecAbsOrigin);
	Cheat.LocalPlayer->m_vecVelocity() = pre_prediction.m_vecVelocity;
	Cheat.LocalPlayer->m_vecAbsVelocity() = pre_prediction.m_vecAbsVelocity;
	Cheat.LocalPlayer->m_flDuckAmount() = pre_prediction.m_flDuckAmount;
	Cheat.LocalPlayer->m_flDuckSpeed() = pre_prediction.m_flDuckSpeed;
	Cheat.LocalPlayer->m_hGroundEntity() = pre_prediction.m_hGroundEntity;
	Cheat.LocalPlayer->m_vecMins() = pre_prediction.m_vecMins;
	Cheat.LocalPlayer->m_vecMaxs() = pre_prediction.m_vecMaxs;

	GameMovement->StartTrackPredictionErrors(Cheat.LocalPlayer);
	MoveHelper->SetHost(Cheat.LocalPlayer);
	Prediction->SetupMove(Cheat.LocalPlayer, cmd, MoveHelper, &moveData);
	GameMovement->ProcessMovement(Cheat.LocalPlayer, &moveData);
	Prediction->FinishMove(Cheat.LocalPlayer, cmd, &moveData);
	GameMovement->FinishTrackPredictionErrors(Cheat.LocalPlayer);
	MoveHelper->SetHost(nullptr);

	if (ctx.active_weapon) {
		ctx.active_weapon->UpdateAccuracyPenality();
		weaponInaccuracy = ctx.active_weapon->GetInaccuracy();
		weaponSpread = ctx.active_weapon->GetSpread();
	}

	AnimationSystem->UpdatePredictionAnimation();
	ctx.shoot_position = Cheat.LocalPlayer->GetShootPosition();

	cmd->viewangles = backup_angle;
}

void CPrediction::PatchAttackPacket(CUserCmd* cmd, bool restore)
{
	static bool m_bLastAttack = false;
	static bool m_bInvalidCycle = false;
	static float m_flLastCycle = 0.f;

	if (!Cheat.LocalPlayer)
		return;

	if (restore)
	{
		m_bLastAttack = cmd->weaponselect || (cmd->buttons & IN_ATTACK);
		m_flLastCycle = Cheat.LocalPlayer->m_flCycle();
	}
	else if (m_bLastAttack && !m_bInvalidCycle)
		m_bInvalidCycle = Cheat.LocalPlayer->m_flCycle() == 0.f && m_flLastCycle > 0.f;

	if (m_bInvalidCycle)
		Cheat.LocalPlayer->m_flCycle() = m_flLastCycle;
}

void CPrediction::StoreNetvars(int place) {
	CBasePlayer* local = Cheat.LocalPlayer;

	auto& nv = local_netvars[place % MULTIPLAYER_BACKUP];

	nv.command_number = place;
	nv.m_nTickBase = local->m_nTickBase();

	nv.m_aimPunchAngle = local->m_aimPunchAngle();
	nv.m_aimPunchAngleVel = local->m_aimPunchAngleVel();
	nv.m_flDuckAmount = local->m_flDuckAmount();
	nv.m_flFallVelocity = local->m_flFallVelocity();
	nv.m_vecViewOffset = local->m_vecViewOffset();
	nv.m_viewPunchAngle = local->m_viewPunchAngle();
	nv.m_vecVelocity = local->m_vecVelocity();
	nv.m_nTickBase = local->m_nTickBase();
	nv.m_flDuckSpeed = local->m_flDuckSpeed();
	nv.m_flVelocityModifier = local->m_flVelocityModifier();
	nv.m_flThirdpersonRecoil = local->m_flThirdpersonRecoil();
	nv.m_vecNetworkOrigin = local->m_vecNetworkOrigin();
	nv.filled = true;
}

void CPrediction::RestoreNetvars(int place) {
	CBasePlayer* local = Cheat.LocalPlayer;

	auto& nv = local_netvars[place % MULTIPLAYER_BACKUP];

	has_prediction_errors = false;

	if (!nv.filled || nv.command_number != place)
		return;

	auto aim_punch_vel_diff = nv.m_aimPunchAngleVel - local->m_aimPunchAngleVel();
	auto aim_punch_diff = nv.m_aimPunchAngle - local->m_aimPunchAngle();
	auto viewoffset_diff = nv.m_vecViewOffset - local->m_vecViewOffset();
	auto velocity_diff = nv.m_vecVelocity - local->m_vecVelocity();
	auto fall_vel_diff = nv.m_flFallVelocity - local->m_flFallVelocity();
	auto net_origin_diff = nv.m_vecNetworkOrigin - local->m_vecNetworkOrigin();

	if (std::abs(aim_punch_diff.pitch) <= 0.03125f && std::abs(aim_punch_diff.yaw) <= 0.03125f && std::abs(aim_punch_diff.roll) <= 0.03125f)
		local->m_aimPunchAngle() = nv.m_aimPunchAngle;
	else
		has_prediction_errors = true;

	if (std::abs(aim_punch_vel_diff.pitch) <= 0.03125f && std::abs(aim_punch_vel_diff.yaw) <= 0.03125f && std::abs(aim_punch_vel_diff.roll) <= 0.03125f)
		local->m_aimPunchAngleVel() = nv.m_aimPunchAngleVel;
	else
		has_prediction_errors = true;

	if (std::abs(viewoffset_diff.z) <= 0.065f)
		local->m_vecViewOffset() = nv.m_vecViewOffset;
	else
		has_prediction_errors = true;

	if (std::abs(fall_vel_diff) <= 0.5f)
		local->m_flFallVelocity() = nv.m_flFallVelocity;
	else
		has_prediction_errors = true;

	if (std::abs(velocity_diff.x) > 0.5f || std::abs(velocity_diff.y) > 0.5f || std::abs(velocity_diff.z) > 0.5f)
		has_prediction_errors = true;

	if (std::abs(net_origin_diff.x) > 0.0625f || std::abs(net_origin_diff.y) > 0.0625f || std::abs(net_origin_diff.z) > 0.0625f)
		has_prediction_errors = true;
}

void CPrediction::RunCommand(CUserCmd* cmd) {
	auto& data = local_data[cmd->command_number % MULTIPLAYER_BACKUP];

	if (data.m_nSequence != cmd->command_number)
		return;

	data.m_nTickBase = Cheat.LocalPlayer->m_nTickBase();
	data.m_nButtons = cmd->buttons;
	data.m_flNextAttack = Cheat.LocalPlayer->m_flNextAttack();
	data.m_hActiveWeapon = Cheat.LocalPlayer->m_hActiveWeapon();

	auto weapon = reinterpret_cast<CBaseCombatWeapon*>(EntityList->GetClientEntityFromHandle(data.m_hActiveWeapon));

	if (!weapon)
		return;

	data.m_flNextPrimaryAttack = weapon->m_flNextPrimaryAttack();
	data.m_fLastShotTime = weapon->m_fLastShotTime();

	if (weapon->m_iItemDefinitionIndex() != Revolver)
		return;

	if (!(cmd->buttons & IN_ATTACK))
		return;

	int lowest_cmd = cmd->command_number - 149;
	int iter_cmd = cmd->command_number;
	int fire_cmd = 0;

	while (iter_cmd > lowest_cmd) {
		auto cmd_data = local_data[iter_cmd % MULTIPLAYER_BACKUP];
		fire_cmd = iter_cmd;

		if (!(cmd_data.m_nButtons & IN_ATTACK))
			break;

		float time = TICKS_TO_TIME(cmd_data.m_nTickBase);
		if (cmd_data.m_flNextPrimaryAttack > time || cmd_data.m_flNextAttack > time)
			break;

		iter_cmd--;
	}

	if (fire_cmd != 0 && cmd->command_number - fire_cmd >= 3)
		weapon->m_flPostponeFireReadyTime() = TICKS_TO_TIME(local_data[(fire_cmd + 3) % MULTIPLAYER_BACKUP].m_nTickBase) + 0.2f;

}

void CPrediction::NetUpdate() {
	if (!Cheat.InGame || !Cheat.LocalPlayer || !Cheat.LocalPlayer->IsAlive()) {
		antifakeduck_ticks = 0;
		return;
	}

	auto data = local_data[ClientState->m_nCommandAck % MULTIPLAYER_BACKUP];

	if (data.m_nSequence != ClientState->m_nCommandAck || data.m_nTickBase != Cheat.LocalPlayer->m_nTickBase())
		return;

	if (data.m_nButtons & IN_BULLRUSH && !(Cheat.LocalPlayer->m_nOldButtons() & IN_BULLRUSH)) {
		antifakeduck_ticks++;
		if (antifakeduck_ticks > 64)
			ctx.no_fakeduck = true;
	}
}

int CPrediction::PredictTickbase() {
	static int iTick = 0;

	if (ctx.cmd != nullptr)
	{
		static CUserCmd* pLastCmd = nullptr;

		// if command was not predicted - increment tickbase
		if (pLastCmd == nullptr || pLastCmd->hasbeenpredicted)
			iTick = Cheat.LocalPlayer->m_nTickBase();
		else
			iTick++;

		pLastCmd = ctx.cmd;
	}

	return iTick;
}

CPrediction* EnginePrediction = new CPrediction;