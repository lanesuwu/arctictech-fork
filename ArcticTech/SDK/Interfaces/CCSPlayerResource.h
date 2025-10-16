#pragma once

#include "../Misc/CBaseEntity.h"

class CCSPlayerResource : public CBaseEntity
{
public:
	NETVAR(m_bombsiteCenterA, Vector, "DT_CSPlayerResource", "m_bombsiteCenterA");
	NETVAR(m_bombsiteCenterB, Vector, "DT_CSPlayerResource", "m_bombsiteCenterB");
	NETVAR(m_iPlayerC4, int, "DT_CSPlayerResource", "m_iPlayerC4");
	PNETVAR(m_bAlive, bool, "DT_PlayerResource", "m_bAlive");
	PNETVAR(m_nPersonaDataPublicLevel, int, "DT_CSPlayerResource", "m_nPersonaDataPublicLevel");
};