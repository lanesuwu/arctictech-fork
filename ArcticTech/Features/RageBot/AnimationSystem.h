#pragma once
#include <array>

#include "../../SDK/Globals.h"
#include "../../SDK/Interfaces.h"

struct LagRecord;

class CAnimationSystem {
	struct interpolate_data_t {
		Vector origin;
		Vector net_origin;
		matrix3x4_t original_matrix[128];
		bool valid = false;

		Vector backtrack_origin;
	};

	interpolate_data_t interpolate_data[64];
	CCSGOPlayerAnimationState unupdated_animstate[64]; // keep unupdated unimstates here, not in records

	matrix3x4_t local_matrix[128];
	matrix3x4_t prediction_matrix[128];
	AnimationLayer local_layers[13];
	CCSGOPlayerAnimationState* prediction_animstate = new CCSGOPlayerAnimationState;

	struct {
		QAngle vangle;
		QAngle abs_angles;
		AnimationLayer layers[13];
		std::array<float, 24> poseparams;
		float last_update = 0.f;

		int command_number = 0;
	} local_anims;

	Vector sent_abs_origin;
public:
	void	FrameStageNotify(EClientFrameStage stage);
	void	UpdateLocalAnimations(CUserCmd* cmd);
	void	OnFrameStart();

	matrix3x4_t* GetLocalBoneMatrix() { return local_matrix; };
	void	CorrectLocalMatrix(matrix3x4_t* mat, int size);
	Vector	GetLocalSentAbsOrigin() { return sent_abs_origin; };
	void	UpdatePredictionAnimation();
	CCSGOPlayerAnimationState* GetPredictionAnimstate() { return prediction_animstate; };
	matrix3x4_t* GetPredictionMatrix() { return prediction_matrix; };

	void	BuildMatrix(CBasePlayer* player, matrix3x4_t* boneToWorld, int maxBones, int mask, AnimationLayer* animlayers);
	void	DisableInterpolationFlags(CBasePlayer* player);
	void	UpdateAnimations(CBasePlayer* player, LagRecord* record, std::deque<LagRecord>& records);
	CCSGOPlayerAnimationState* GetUnupdatedAnimstate(int id) { return &unupdated_animstate[id]; };

	Vector	GetInterpolated(CBasePlayer* player);
	void	RunInterpolation();
	void	InterpolateModel(CBasePlayer* player, matrix3x4_t* matrix);
	void	ResetInterpolation();
	void	InvalidateInterpolation(int i);
	inline interpolate_data_t* GetInterpolateData(int idx) { return &interpolate_data[idx]; };
};

extern CAnimationSystem* AnimationSystem;
