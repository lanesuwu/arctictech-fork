#include "PingReducer.h"
#include "PingSpike.h"

#include "../../SDK/Interfaces.h"
#include "../../SDK/Globals.h"
#include "../../SDK/Hooks.h"


CPingReducer* PingReducer = new CPingReducer;

void CPingReducer::globals_t::read() {
	curtime = GlobalVars->curtime;
	frametime = GlobalVars->frametime;
	tickcount = GlobalVars->tickcount;
	cs_tickcount = ClientState->m_nOldTickCount;
}

void CPingReducer::globals_t::write() {
	GlobalVars->curtime = curtime;
	GlobalVars->frametime = frametime;
	GlobalVars->tickcount = tickcount;
	ClientState->m_nOldTickCount = cs_tickcount;
}

bool CPingReducer::AllowReadPackets() {
	if (!Cheat.InGame || !Cheat.LocalPlayer || !Cheat.LocalPlayer->IsAlive() || !config.misc.miscellaneous.ping_reducer->get())
		return true;

	shared_data.write();
	return false;
}

void CPingReducer::ReadPackets(bool final_tick) {
	if (!Cheat.InGame || !Cheat.LocalPlayer || !Cheat.LocalPlayer->IsAlive() || !config.misc.miscellaneous.ping_reducer->get())
		return;

	int backup_flags = Cheat.LocalPlayer->m_fFlags();
	int backup_tickbase = Cheat.LocalPlayer->m_nTickBase();
	Vector backup_abs_origin = Cheat.LocalPlayer->GetAbsOrigin();

	backup_data.read();

	oReadPackets(final_tick);
	shared_data.read();

	if (!Cheat.LocalPlayer)
		return;

	backup_data.write();

	PingSpike->OnPacketStart();

	Cheat.LocalPlayer->m_fFlags() = backup_flags;
	Cheat.LocalPlayer->m_nTickBase() = backup_tickbase;
	Cheat.LocalPlayer->SetAbsOrigin(backup_abs_origin);
}