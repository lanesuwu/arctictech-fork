#pragma once
#include "../../SDK/Interfaces.h"
#include "../../SDK/Misc/CBasePlayer.h"
#include "../../SDK/NetMessages.h"

class CSVCMsg_VoiceData;

struct ESPInfo_t {
	CBasePlayer*	player = nullptr;
	Vector			m_vecOrigin;
	Vector2			m_BoundingBox[2];
	int				m_nFakeDuckTicks = 0;
	int				m_nHealth;
	bool			m_bFakeDuck = 0;
	bool			m_bIsScoped = false;
	bool			m_bExploiting = false;
	bool			m_bBreakingLagComp = false;
	bool			m_bDormant = false;
	bool			m_bValid = false;
	bool			m_bHit = false;
	float			m_flAlpha = 0.f;
	float			m_flLastUpdateTime = 0.f;
	float			m_flLastSharedData = 0.f;
	int				m_iActiveWeapon;

	void reset() {
		player = nullptr;
		m_vecOrigin = Vector();
		m_nHealth = 0;
		m_flLastUpdateTime = 0.f;
		m_flAlpha = 0.f;
		m_bValid = false;
	}
};

enum SharedESPFlags {
	Shared_Scoped = (1 << 0),
	Shared_BreakLC = (1 << 1),
	Shared_Exploiting = (1 << 2),
	Shared_FakeDuck = (1 << 3),
};

struct VectorSmall {
	int16_t x = 0, y = 0, z = 0;

	VectorSmall() {}

	VectorSmall(Vector vec) {
		x = vec.x;
		y = vec.y;
		z = vec.z;
	}
};

struct SharedESP_t {
	int		m_iPlayer;
	short	m_ActiveWeapon;
	short	m_iHealth;
	VectorSmall	m_vecOrigin;
};

struct SharedEsp_Fatality
{
	uint16_t identifier;
	uint8_t user_id;
	uint8_t weapon_id;
	uint32_t server_tick;
	Vector pos;
};

static_assert(sizeof(SharedESP_t) == sizeof(SharedVoiceData_t));

class CWorldESP {
	struct DamageMarker_t {
		Vector position;
		float time = 0.f;
		int damage = 0;
	};

	struct Hitmarker_t {
		Vector position;
		float time = 0.f;
	};

	ESPInfo_t	esp_info[64];
	std::vector<DamageMarker_t> damage_markers;
	std::vector<Hitmarker_t> hit_markers;

public:
	ESPInfo_t&	GetESPInfo(int idx) { return esp_info[idx]; };
	void		ProcessSound(const SoundInfo_t& sound);
	void		RegisterCallback();
	void		AntiFatality();

	void		UpdatePlayer(int id);
	void		Draw();
	void		IconDisplay(CBasePlayer* pLocal, int Level);
	void		DrawPlayer(int id);
	void		DrawBox(const ESPInfo_t& info);
	void		DrawHealth(const ESPInfo_t& info);
	void		DrawName(const ESPInfo_t& info);
	void		DrawFlags(const ESPInfo_t& info);
	void		DrawWeapon(const ESPInfo_t& info);

	void		OtherESP();
	void		DrawGrenade(CBaseGrenade* nade, ClientClass* cl_class);
	void		DrawBomb(CBaseEntity* bomb, ClientClass* cl_class);

	void		AddHitmarker(const Vector& position);
	void		AddDamageMarker(const Vector& postion, int damage);
	void		RenderMarkers();
	void        SpinningStar(const ESPInfo_t& info);

	void		AddDebugMessage(std::string msg);
	void		RenderDebugMessages();

	std::vector<std::string> DebugMessages;
	std::vector<std::string> DebugMessagesSane;
};

extern CWorldESP* WorldESP;