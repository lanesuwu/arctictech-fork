#pragma once
#include "Misc/CBasePlayer.h"
#include "Config.h"

class ConVar;

struct CheatState_t {
	CBasePlayer* LocalPlayer = nullptr;
	bool InGame = false;
	Vector2 ScreenSize;
	bool Unloaded = false;
};

struct HooksInfo_t {
	bool update_csa = false;
	bool setup_bones = false;
	bool console_log = false;
	bool disable_interpolation = false;
	bool in_draw_static_props = false;
	bool disable_clamp_bones = false;
	bool read_packets = false;
};

struct Ctx_t {
	bool active_app = false;
	bool console_visible = false;

	Vector camera_postion;
	CUserCmd* cmd = nullptr;
	bool send_packet = true;
	bool is_peeking = false;
	int tickbase_shift = 0;
	int shifted_last_tick = 0;
	Vector local_velocity;
	Vector last_local_velocity;
	bool update_nightmode = false;
	bool update_remove_blood = false;
	Vector local_sent_origin;
	bool breaking_lag_compensation = false;
	int should_buy = 0;
	QAngle leg_slide_angle;
	float last_shot_time = 0.f;
	bool planting_bomb = false;
	bool should_release_grenade = false;
	QAngle grenade_release_angle;
	bool no_fakeduck = false;

	QAngle sent_angles;
	QAngle shot_angles;
	bool force_shot_angle = false;

	CBaseCombatWeapon* active_weapon = nullptr;
	CCSWeaponData* weapon_info = nullptr;
	Vector shoot_position;
	bool fake_duck = false;
	int grenade_throw_tick = 0;
	bool switch_to_main_weapon = true;

	int corrected_tickbase = 0;

	std::vector<int> sent_commands;

	void reset() {
		sent_commands.clear();
		tickbase_shift = 0;
		local_velocity = Vector();
		last_local_velocity = Vector();
		breaking_lag_compensation = false;
		active_weapon = nullptr;
		weapon_info = nullptr;
		grenade_throw_tick = 0;
		fake_duck = 0;
		last_shot_time = 0.f;
		planting_bomb = false;
		no_fakeduck = false;
	}
};

struct CVars {
	ConVar* r_aspectratio;
	ConVar* mat_postprocessing_enable;
	ConVar* r_DrawSpecificStaticProp;
	ConVar* fog_override;
	ConVar* fog_color;
	ConVar* post_processing;
	ConVar* fog_start;
	ConVar* fog_end;
	ConVar* fog_maxdensity;
	ConVar* molotov_throw_detonate_time;
	ConVar* sv_cheats;
	ConVar* sv_gravity;
	ConVar* sv_jump_impulse;
	ConVar* sv_maxunlag;
	ConVar* weapon_recoil_scale;
	ConVar* cl_csm_shadows;
	ConVar* cl_foot_contact_shadows;
	ConVar* cl_lagcompensation;
	ConVar* cl_interp;
	ConVar* cl_interp_ratio;
	ConVar* weapon_debug_spread_show;
	ConVar* r_drawsprites;
	ConVar* zoom_sensitivity_ratio_mouse;
	ConVar* mp_damage_headshot_only;
	ConVar* mp_friendlyfire;
	ConVar* net_earliertempents;
	ConVar* cl_threaded_bone_setup;
	ConVar* dsp_slow_cpu;
	ConVar* r_eyemove;
	ConVar* r_eyegloss;
	ConVar* cl_pred_doresetlatch;
	ConVar* sv_clockcorrection_msecs;
	ConVar* sv_maxusrcmdprocessticks;
	ConVar* sv_infinite_ammo;
	ConVar* sensitivity;
	ConVar* default_fov;
	ConVar* ff_damage_reduction_grenade;
    ConVar* sv_air_max_wishspeed;
};

extern CheatState_t Cheat;
extern CVars cvars;
extern Ctx_t ctx;
extern HooksInfo_t hook_info;