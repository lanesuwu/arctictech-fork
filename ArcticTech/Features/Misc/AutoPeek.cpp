#include "AutoPeek.h"
#include "Prediction.h"
#include "../../SDK/Globals.h"
#include "../../SDK/Render.h"

CAutoPeek* AutoPeek = new CAutoPeek;


void CAutoPeek::Draw() {
	if (!Cheat.LocalPlayer || !Cheat.LocalPlayer->IsAlive() || !Cheat.InGame)
		return;

	bool state = config.ragebot.aimbot.peek_assist->get() && config.ragebot.aimbot.peek_assist_keybind->get();

	if (prev_state != state) {
		if (state) {
			if (Cheat.LocalPlayer->m_fFlags() & FL_ONGROUND) {
				position = Cheat.LocalPlayer->m_vecOrigin();
			}
			else {
				CGameTrace trace = EngineTrace->TraceHull(Cheat.LocalPlayer->m_vecOrigin(), Cheat.LocalPlayer->m_vecOrigin() - Vector(0, 0, 128), Cheat.LocalPlayer->m_vecMins(), Cheat.LocalPlayer->m_vecMaxs(), CONTENTS_SOLID, Cheat.LocalPlayer);

				position = trace.endpos;
			}
		}

		prev_state = state;
	}

	if (state && anim < 1.f)
		anim = std::clamp(anim + GlobalVars->frametime * 4.f, 0.f, 1.f);
	else if (!state && anim > 0.f)
		anim = std::clamp(anim - GlobalVars->frametime * 4.f, 0.f, 1.f);

	if (anim > 0.f)
		Render->Circle3DGradient(position, 23.f, config.ragebot.aimbot.peek_assist_color->get().alpha_modulatef(anim));
}

void CAutoPeek::CreateMove() {
	if (!(config.ragebot.aimbot.peek_assist_keybind->get() && config.ragebot.aimbot.peek_assist->get()) || !ctx.active_weapon) {
		returning = false;
		return;
	}

	float distance_sqr = (Cheat.LocalPlayer->m_vecOrigin() - position).LengthSqr();

	if (ctx.cmd->buttons & IN_ATTACK && ctx.active_weapon->ShootingWeapon() && ctx.active_weapon->CanShoot()) {
		Return();
	} else if (distance_sqr < 16.f) {
		if (ctx.active_weapon->m_flNextPrimaryAttack() - 0.15f < TICKS_TO_TIME(Cheat.LocalPlayer->m_nTickBase()))
			returning = false;
	}

	if (!returning && (!config.ragebot.aimbot.peek_assist_on_release->get() || distance_sqr < 16.f || ctx.cmd->buttons & (IN_MOVELEFT | IN_MOVERIGHT | IN_FORWARD | IN_BACK)))
		return;

	if (returning) {
		block_buttons &= ctx.cmd->buttons; // if player will release buttons, they will not be blocked
		ctx.cmd->buttons &= ~block_buttons;

		if (block_buttons & IN_FORWARD && ctx.cmd->forwardmove > 0.f)
			ctx.cmd->forwardmove = 0.f;

		if (block_buttons & IN_BACK && ctx.cmd->forwardmove < 0.f)
			ctx.cmd->forwardmove = 0.f;

		if (block_buttons & IN_MOVELEFT && ctx.cmd->sidemove < 0.f)
			ctx.cmd->sidemove = 0.f;

		if (block_buttons & IN_MOVERIGHT && ctx.cmd->sidemove > 0.f)
			ctx.cmd->sidemove = 0.f;

		if (distance_sqr < 16.f) {
			if (block_buttons == 0)
				returning = false;
			return;
		}
	}

	QAngle ang = Utils::VectorToAngle(Cheat.LocalPlayer->m_vecOrigin(), position);
	QAngle vang;
	EngineClient->GetViewAngles(vang);
	ang.yaw = vang.yaw - ang.yaw;
	ang.Normalize();

	Vector dir;
	Utils::AngleVectors(ang, dir);
	dir *= 450.f;
	ctx.cmd->forwardmove = dir.x;
	ctx.cmd->sidemove = dir.y;
}

void CAutoPeek::Return() {
	block_buttons = ctx.cmd->buttons & (IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT);
	returning = true;
}