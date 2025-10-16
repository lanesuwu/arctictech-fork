#pragma once

#include "../../Utils/Utils.h"

#include "../Misc/CBaseEntity.h"

class CGameRules {
	char pad__[0x20];
public:
	bool m_bFreezePeriod;
	bool m_bWarmupPeriod;
	float m_fWarmupPeriodEnd;
	float m_fWarmupPeriodStart;

	NETVAR(m_fRoundStartTime, float, "DT_CSGameRules", "m_fRoundStartTime")
	NETVAR(m_iRoundTime, int, "DT_CSGameRules", "m_iRoundTime")

	bool IsFreezePeriod() {
		return m_bFreezePeriod;
	}
};

CGameRules* GameRules();