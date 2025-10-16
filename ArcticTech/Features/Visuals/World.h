#pragma once

class CViewSetup;

class CWorld {
	float m_flFOVOverriden = 0.f;
public:
	void Modulation();
	void Fog();
	void RemoveBlood();
	void SkyBox();
	void ProcessCamera(CViewSetup* view_setup);
	void OverrideSensitivity();
	void Smoke();
	void Crosshair();
	void SunDirection();
};

extern CWorld* World;