#include "Resolver.h"

#include <algorithm>
#include <numeric>
#include <cmath>
#include "../../SDK/Interfaces.h"
#include "../../SDK/Misc/CBasePlayer.h"
#include "LagCompensation.h"
#include "../../SDK/Globals.h"
#include "AnimationSystem.h"
#include "../../Utils/Console.h"


CResolver* Resolver = new CResolver;

float CResolver::GetTime() {
    if (!Cheat.LocalPlayer || !Cheat.LocalPlayer->IsAlive())
        return GlobalVars->curtime;

    return TICKS_TO_TIME(Cheat.LocalPlayer->m_nTickBase());
}

// circular/weighted average yaw — more weight for recent records
float FindAvgYaw(const std::deque<LagRecord>& records) {
    const int minimumSamples = 3;
    if ((int)records.size() <= minimumSamples)
        return 0.f;

    float sin_sum = 0.f, cos_sum = 0.f;
    float weight_sum = 0.f;

    // last up-to-6 samples (skip the newest as it's often same-tick)
    int start = (int)records.size() - 2;
    int end = std::max<int>(0, static_cast<int>(records.size()) - 6);

    float weight = 1.0f;
    for (int i = start; i > end; --i) {
        const auto& record = records.at(i);
        float yaw = record.m_angEyeAngles.yaw;

        sin_sum += std::sinf(DEG2RAD(yaw)) * weight;
        cos_sum += std::cosf(DEG2RAD(yaw)) * weight;
        weight_sum += weight;
        weight *= 0.85f; // reduce weight for older samples
    }

    if (weight_sum <= 0.f)
        return 0.f;

    // atan2 of (sum_sin, sum_cos) is already the circular mean direction;
    // dividing by weight_sum is unnecessary for atan2, but keep numerically stable
    float avg_sin = sin_sum / weight_sum;
    float avg_cos = cos_sum / weight_sum;

    float ang = RAD2DEG(std::atan2f(avg_sin, avg_cos));
    return Math::AngleNormalize(ang);
}

// reset resolver data for a player or all
void CResolver::Reset(CBasePlayer* pl) {
    if (pl) {
        resolver_data[pl->EntIndex()].reset();
        return;
    }

    for (int i = 0; i < 64; ++i)
        resolver_data[i].reset();
}

R_PlayerState CResolver::DetectPlayerState(CBasePlayer* player, AnimationLayer* animlayers) {
    if (!player)
        return R_PlayerState::STANDING;

    if (!(player->m_fFlags() & FL_ONGROUND))
        return R_PlayerState::AIR;

    CCSGOPlayerAnimationState* animstate = player->GetAnimstate();
    if (!animstate)
        return R_PlayerState::STANDING;

    if (player->m_vecVelocity().Length2DSqr() > 256.f &&
        animstate->flWalkToRunTransition > 0.8f &&
        animlayers[ANIMATION_LAYER_MOVEMENT_MOVE].m_flPlaybackRate > 0.0001f)
        return R_PlayerState::MOVING;

    return R_PlayerState::STANDING;
}

// somewhat better anti-aim resolver + use mean and stddev of deltas to classify jitter/static
R_AntiAimType CResolver::DetectAntiAim(CBasePlayer* player, const std::deque<LagRecord>& records) {
    if (!player || records.size() < 8)
        return R_AntiAimType::NONE;

    std::vector<float> deltas;
    deltas.reserve(8);

    float prevYaw = player->m_angEyeAngles().yaw;
    // gather up to 8 deltas (recent)
    for (int i = (int)records.size() - 2; i >= 0 && (int)deltas.size() < 8; --i) {
        const auto& record = records.at(i);
        float yaw = record.m_angEyeAngles.yaw;
        float delta = std::fabs(Math::AngleDiff(yaw, prevYaw));
        deltas.push_back(delta);
        prevYaw = yaw;
    }

    if (deltas.empty())
        return R_AntiAimType::NONE;

    // compute mean and standard deviation
    float sum = std::accumulate(deltas.begin(), deltas.end(), 0.0f);
    float mean = sum / (float)deltas.size();

    float variance = 0.0f;
    for (float d : deltas) {
        float diff = d - mean;
        variance += diff * diff;
    }
    variance = variance / (float)deltas.size();
    float stddev = std::sqrt(variance);

    // Heuristics:
    // - If many deltas are large and stddev is high => JITTER
    // - If mean and stddev are small => STATIC
    int largeCount = (int)std::count_if(deltas.begin(), deltas.end(), [](float v) { return v > 28.0f; });
    if (largeCount > (int)(deltas.size() * 0.5f) || stddev > 18.0f)
        return R_AntiAimType::JITTER;

    if (mean < 6.0f && stddev < 6.0f)
        return R_AntiAimType::STATIC;

    return R_AntiAimType::UNKNOWN;
}

float ValveAngleDiff(float destAngle, float srcAngle) {
    float delta = std::fmodf(destAngle - srcAngle, 360.0f);
    if (delta > 180.f) delta -= 360.f;
    if (delta < -180.f) delta += 360.f;
    return delta;
}

// setup anim layer attempt using provided delta (desync). More robust to invalid data.
void CResolver::SetupLayer(LagRecord* record, int idx, float delta) {
    if (!record || !record->player)
        return;

    CCSGOPlayerAnimationState* animstate = record->player->GetAnimstate();
    if (!animstate)
        return;

    QAngle angles = record->player->m_angEyeAngles();

    float newFootYaw = Math::AngleNormalize(angles.yaw + delta);
    animstate->flFootYaw = newFootYaw;

    Vector vel = record->player->m_vecVelocity();
    float flRawYawIdeal = 0.f;
    if (vel.Length2DSqr() > 1e-6f) {
        flRawYawIdeal = RAD2DEG(std::atan2f(-vel.y, -vel.x));
        if (flRawYawIdeal < 0.0f) flRawYawIdeal += 360.0f;
    }
    else {
        flRawYawIdeal = Math::AngleNormalize(angles.yaw);
    }

    animstate->flMoveYaw = Math::AngleNormalize(ValveAngleDiff(flRawYawIdeal, animstate->flFootYaw));

    if (record->prev_record && record->prev_record->animlayers)
        memcpy(record->player->GetAnimlayers(), record->prev_record->animlayers, sizeof(AnimationLayer) * 13);

    animstate->ForceUpdate();
    animstate->Update(angles);

    // IMPROVED STABILITY CALCULATION - use synced flPrimaryCycle
    float playback_diff = 0.0f;
    float cycle_diff = 0.0f;
    
    if (record->animlayers && record->player->GetAnimlayers()) {
        float storedPlayback = record->animlayers[ANIMATION_LAYER_MOVEMENT_MOVE].m_flPlaybackRate;
        float curPlayback = record->player->GetAnimlayers()[ANIMATION_LAYER_MOVEMENT_MOVE].m_flPlaybackRate;
        playback_diff = std::fabs(storedPlayback - curPlayback);
        
        // Use the synced flPrimaryCycle for better accuracy
        float storedCycle = record->prev_record ? animstate->flPrimaryCycle : 0.f;
        float curCycle = record->player->GetAnimlayers()[ANIMATION_LAYER_MOVEMENT_MOVE].m_flCycle;
        cycle_diff = std::fabs(curCycle - storedCycle);
    }

    ResolverLayer_t* resolver_layer = &record->resolver_data.layers[idx];
    resolver_layer->desync = delta;
    // Weight both playback rate and cycle differences
    resolver_layer->delta = playback_diff * 1000.0f + cycle_diff * 500.0f + 
                           std::fabs(Math::AngleDiff(animstate->flFootYaw, newFootYaw)) * 10.0f;

    CCSGOPlayerAnimationState* original = AnimationSystem->GetUnupdatedAnimstate(record->player->EntIndex());
    if (original)
        *animstate = *original;
}

void CResolver::SetupResolverLayers(CBasePlayer* player, LagRecord* record) {
    if (!player || !record)
        return;

    SetupLayer(record, 0, 0.f);
    SetupLayer(record, 1, record->resolver_data.max_desync_delta);
    SetupLayer(record, 2, -record->resolver_data.max_desync_delta);
    SetupLayer(record, 3, record->resolver_data.max_desync_delta * 0.6f);
    SetupLayer(record, 4, -record->resolver_data.max_desync_delta * 0.6f);
}

// improved freestanding: sample multiple offsets and sum obstruction to pick safest side
void CResolver::DetectFreestand(CBasePlayer* player, LagRecord* record, const std::deque<LagRecord>& records) {
    if (!player || !record || records.size() < 16)
        return;

    // base eye pos for traces (account for duck)
    Vector eyePos = player->m_vecOrigin() + Vector(0.f, 0.f, 64.f - player->m_flDuckAmount() * 16.f);
    Vector toLocal = (Cheat.LocalPlayer->m_vecOrigin() - player->m_vecOrigin()).Normalized();

    // prefer average yaw from records if anti-aim not static (more stable)
    float notModifiedYaw = player->m_angEyeAngles().yaw;
    if (record->resolver_data.antiaim_type != R_AntiAimType::STATIC)
        notModifiedYaw = FindAvgYaw(records);

    // compute right vector from base yaw
    Vector right = Math::AngleVectors(QAngle(0.f, notModifiedYaw + 90.f, 0.f));

    // we will sample 5 offsets left/right at increasing radii and cast rays forward to count obstruction length
    const float radii[] = { 12.f, 20.f, 28.f, 36.f, 44.f };
    float leftScore = 0.f, rightScore = 0.f;

    CTraceFilterWorldAndPropsOnly filter;

    for (float r : radii) {
        Vector leftPos = eyePos - right * r;
        Vector rightPos = eyePos + right * r;

        Ray_t leftRay(leftPos, leftPos + toLocal * 150.f);
        Ray_t rightRay(rightPos, rightPos + toLocal * 150.f);
        CGameTrace leftTrace, rightTrace;

        EngineTrace->TraceRay(leftRay, MASK_SHOT_HULL | CONTENTS_GRATE, &filter, &leftTrace);
        EngineTrace->TraceRay(rightRay, MASK_SHOT_HULL | CONTENTS_GRATE, &filter, &rightTrace);

        // accumulate obstruction score: closer impact => more obstructed
        leftScore += (1.0f - leftTrace.fraction);
        rightScore += (1.0f - rightTrace.fraction);
    }

    // if both positions are fully free, no freestanding
    if (leftScore <= 0.001f && rightScore <= 0.001f) {
        record->resolver_data.side = 0;
        record->resolver_data.resolver_type = ResolverType::NONE;
        return;
    }

    // Choose side with *less* obstruction (safer to present body)
    record->resolver_data.side = (leftScore < rightScore) ? -1 : 1;
    record->resolver_data.resolver_type = ResolverType::FREESTAND;
}

void CResolver::Apply(LagRecord* record) {
    if (!record || record->resolver_data.side == 0)
        return;

    auto state = record->player->GetAnimstate();
    if (!state)
        return;

    float body_yaw = record->resolver_data.max_desync_delta * record->resolver_data.side;
    state->flMoveYaw = state->flFootYaw;
}

void CResolver::Run(CBasePlayer* player, LagRecord* record, std::deque<LagRecord>& records) {
    if (!player || !record)
        return;

    if (GameRules()->IsFreezePeriod() || (player->m_fFlags() & FL_FROZEN) || !Cheat.LocalPlayer->IsAlive() || record->shooting)
        return;

    record->resolver_data.max_desync_delta = player->GetMaxDesyncDelta();

    // if no choked ticks or defusing, do not resolve
    if (!record->m_nChokedTicks || player->m_bIsDefusing()) {
        record->resolver_data.side = 0;
        record->resolver_data.resolver_type = ResolverType::NONE;
        return;
    }

    // derive base state and anti-aim characteristics
    record->resolver_data.player_state = DetectPlayerState(player, record->animlayers);
    record->resolver_data.antiaim_type = DetectAntiAim(player, records);

    // prepare resolver layer simulations
    SetupResolverLayers(player, record);
    record->resolver_data.resolver_type = ResolverType::NONE;

    // choose the layer with smallest delta (most stable)
    float min_delta = FLT_MAX;
    int best_idx = -1;
    for (int i = 0; i < 5; ++i) {
        auto* layer = &record->resolver_data.layers[i];
        if (layer->desync == 0.f) continue;

        if (layer->delta < min_delta) {
            min_delta = layer->delta;
            best_idx = i;
        }
    }


    if (best_idx != -1 && min_delta < 10.f) {
        // map index -> side
        float desync = record->resolver_data.layers[best_idx].desync;
        record->resolver_data.side = (desync < 0.f) ? -1 : 1;
        record->resolver_data.resolver_type = ResolverType::ANIM;
    }
    else {
        record->resolver_data.resolver_type = ResolverType::NONE;
    }

    // If still ambiguous or not on ground/moving, try other heuristics
    float vel_sqr = player->m_vecVelocity().LengthSqr();
    bool on_ground = (player->m_fFlags() & FL_ONGROUND) != 0;
    bool low_move = vel_sqr < 256.f || record->animlayers[ANIMATION_LAYER_MOVEMENT_MOVE].m_flWeight <= 0.f;

    if (low_move || record->resolver_data.resolver_type == ResolverType::NONE || !on_ground) {
        // Jitter handling: if anti-aim shows jitter, use direction of last known average vs current
        if (record->resolver_data.antiaim_type == R_AntiAimType::JITTER && records.size() > 12) {
            float eyeYaw = player->m_angEyeAngles().yaw;
            float prevYaw = FindAvgYaw(records);
            float delta = Math::AngleDiff(eyeYaw, prevYaw);

            // if delta close to zero, no clear side; otherwise choose side based on sign but bias toward memory
            if (std::fabs(delta) < 5.0f) {
                // fallback to memory if available
                auto res_data = &resolver_data[player->EntIndex()];
                record->resolver_data.side = res_data->last_side != 0 ? res_data->last_side : -1;
                record->resolver_data.resolver_type = ResolverType::MEMORY;
            }
            else {
                record->resolver_data.side = (delta < 0.f) ? 1 : -1;
                record->resolver_data.resolver_type = ResolverType::LOGIC;
            }
        }
        else {
            // try freestanding detection
            DetectFreestand(player, record, records);
        }
    }

    // memory fallback and brute forcing
    auto res_data = &resolver_data[player->EntIndex()];
    float curtime = GetTime();

    if (record->resolver_data.resolver_type != ResolverType::NONE) {
        res_data->last_resolved = curtime;
        res_data->last_side = record->resolver_data.side;
        res_data->res_type_last = record->resolver_data.resolver_type;
    }
    else {
        // fallback to memory if we have recent data
        record->resolver_data.resolver_type = ResolverType::MEMORY;
        if (res_data->last_resolved + 2.5f > curtime) // prefer recent memory (2.5s)
            record->resolver_data.side = res_data->last_side;
        else
            record->resolver_data.side = -1; // default guess
    }

    // if we recently brute-forced due to misses, prefer brute side (unless logic decided otherwise)
    if (curtime - res_data->brute_time < 5.f && record->resolver_data.resolver_type != ResolverType::LOGIC) {
        record->resolver_data.side = res_data->brute_side;
        record->resolver_data.resolver_type = ResolverType::BRUTEFORCE;
    }

    // ensure side is never zero
    if (record->resolver_data.side == 0) {
        record->resolver_data.side = -1;
        record->resolver_data.resolver_type = ResolverType::DEFAULT;
    }

    Apply(record);
}

void CResolver::OnMiss(CBasePlayer* player, LagRecord* record) {
    if (!player || !record)
        return;

    auto bf_data = &resolver_data[player->EntIndex()];

    // cycle brute side on misses: alternate side to try both options
    if (bf_data->missed_shots == 0)
        bf_data->brute_side = (record->resolver_data.side == 0) ? -1 : -record->resolver_data.side;
    else
        bf_data->brute_side = -bf_data->brute_side; // flip each miss

    bf_data->brute_time = GetTime();
    bf_data->missed_shots++;
}

void CResolver::OnHit(CBasePlayer* player, LagRecord* record) {
    if (!player || !record)
        return;

    auto bf_data = &resolver_data[player->EntIndex()];

    bf_data->brute_side = record->resolver_data.side;
    bf_data->brute_time = GetTime();
    bf_data->missed_shots = 0;
}
