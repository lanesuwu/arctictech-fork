#pragma once

#include "../../Utils/Utils.h"

struct CSGO_HudRadar {
	char pad_0000[284]; //0x0000
	Vector m_vecWorldOffset; //0x011C
	Vector2 m_vecImageOffset; //0x0128
	char pad_0130[4]; //0x0130
	float m_flRotation; //0x0134
	Vector m_vecWorldPosition; //0x0138
	float m_flCurTime; //0x0144
	char pad_0148[356]; //0x0148
	Vector m_vecLocalOrigin; //0x02AC
	Vector m_vecLocalAngles; //0x02B8
	char pad_02C4[404]; //0x02C4
};

class CHud {
public:
	int							m_iKeyBits;
	float						m_flMouseSensitivity;
	float						m_flMouseSensitivityFactor;
	float						m_flFOVSensitivityAdjust;

	void* FindHudElement(const char* name) {
		static auto func = reinterpret_cast<void* (__thiscall*)(void*, const char*)>(Utils::PatternScan("client.dll", "55 8B EC 53 8B 5D 08 56 57 8B F9 33 F6 39 77 28"));
		return func(this, name);
	}

	static CHud* Get() {
		return *reinterpret_cast<CHud**>(Utils::PatternScan("client.dll", "B9 ? ? ? ? E8 ? ? ? ? 85 C0 0F 84 ? ? ? ? 83 C0 ? 0F 84 ? ? ? ? 80 B8", 0x1));
	}
};