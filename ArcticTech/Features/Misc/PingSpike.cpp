#include "PingSpike.h"
#include "../../SDK/Interfaces.h"
#include "../../SDK/Config.h"
#include "../../SDK/Globals.h"


static CPingSpike s_PingSpike;
CPingSpike* PingSpike = &s_PingSpike;


void CPingSpike::OnPacketStart() {
	INetChannel* netchan = ClientState->m_NetChannel;

	if (netchan) {
		if (netchan->m_nInSequenceNr > lastincomingsequencenumber) {
			lastincomingsequencenumber = netchan->m_nInSequenceNr;

			sequences.emplace_front(netchan->m_iInReliableState, netchan->m_iOutReliableState, netchan->m_nInSequenceNr, GlobalVars->realtime);
		}

		if (sequences.size() > 2048)
			sequences.pop_back();
	}
}

void CPingSpike::LevelInit() {
    sequences.clear();
    lastincomingsequencenumber = 0;
}

void CPingSpike::OnSendDatagram() {
	if (!config.misc.miscellaneous.ping_spike->get() || !Cheat.InGame)
		return;

	float latency = config.misc.miscellaneous.ping_spike_amount->get() / 1000.f;
	auto netchan = ClientState->m_NetChannel;

	for (auto& seq : sequences) {
		if (GlobalVars->realtime - seq.time >= latency) {
			netchan->m_iInReliableState = seq.in_reliable;
			netchan->m_nInSequenceNr = seq.sequence;
			break;
		}
	}
}