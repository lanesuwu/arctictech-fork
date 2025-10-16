#pragma once

#include "CBaseEntity.h"
#include "../Render.h"

#include <string>

class CCSWeaponData
{
public:
	std::byte pad0[0x14];			// 0x0000
	int iMaxClip1;					// 0x0014
	int iMaxClip2;					// 0x0018
	int iDefaultClip1;				// 0x001C
	int iDefaultClip2;				// 0x0020
	int iPrimaryMaxReserveAmmo;		// 0x0024
	int iSecondaryMaxReserveAmmo;	// 0x0028
	const char* szWorldModel;		// 0x002C
	const char* szViewModel;		// 0x0030
	const char* szDroppedModel;		// 0x0034
	std::byte pad1[0x50];			// 0x0038
	const char* szHudName;			// 0x0088
	const char* szWeaponName;		// 0x008C
	std::byte pad2[0x2];			// 0x0090
	bool bIsMeleeWeapon;			// 0x0092
	std::byte pad3[0x9];			// 0x0093
	float flWeaponWeight;			// 0x009C
	std::byte pad4[0x4];			// 0x00A0
	int iSlot;						// 0x00A4
	int iPosition;					// 0x00A8
	std::byte pad5[0x1C];			// 0x00AC
	int nWeaponType;				// 0x00C8
	std::byte pad6[0x4];			// 0x00CC
	int iWeaponPrice;				// 0x00D0
	int iKillAward;					// 0x00D4
	const char* szAnimationPrefix;	// 0x00D8
	float flCycleTime;				// 0x00DC
	float flCycleTimeAlt;			// 0x00E0
	std::byte pad8[0x8];			// 0x00E4
	bool bFullAuto;					// 0x00EC
	std::byte pad9[0x3];			// 0x00ED
	int iDamage;					// 0x00F0
	float flHeadShotMultiplier;		// 0x00F4
	float flArmorRatio;				// 0x00F8
	int iBullets;					// 0x00FC
	float flPenetration;			// 0x0100
	std::byte pad10[0x8];			// 0x0104
	float flRange;					// 0x010C
	float flRangeModifier;			// 0x0110
	float flThrowVelocity;			// 0x0114
	std::byte pad11[0xC];			// 0x0118
	bool bHasSilencer;				// 0x0124
	std::byte pad12[0xF];			// 0x0125
	float flMaxSpeed;			    // 0x0134
    float flMaxSpeedAlt;
	std::byte pad13[0x4];			// 0x013C
	float flSpread[2];				// 0x0140
	float flInaccuracyCrouch[2];	// 0x0148
	float flInaccuracyStand[2];		// 0x0150
    float _flInaccuracyUnknown;     // 0x0158
    float flInaccuracyJumpInitial;// 0x015C
	float flInaccuracyJump[2];		// 0x0160
	float flInaccuracyLand[2];		// 0x0168
	float flInaccuracyLadder[2];	// 0x0170
	float flInaccuracyFire[2];		// 0x0178
	float flInaccuracyMove[2];		// 0x0180
	float flInaccuracyReload;		// 0x0188
	int iRecoilSeed;				// 0x018C
	float flRecoilAngle[2];			// 0x0190
	float flRecoilAngleVariance[2];	// 0x0198
	float flRecoilMagnitude[2];		// 0x01A0
	float flRecoilMagnitudeVariance[2]; // 0x01A8
	int iSpreadSeed;				// 0x01B0

	std::string GetName();
};

enum WeaponId : short {
	None = 0,
	Deagle = 1,
	Elite,
	Fiveseven,
	Glock,
	Ak47 = 7,
	Aug,
	Awp,
	Famas,
	G3SG1,
	GalilAr = 13,
	M249,
	M4A1 = 16,
	Mac10,
	P90 = 19,
	ZoneRepulsor,
	Mp5sd = 23,
	Ump45,
	Xm1014,
	Bizon,
	Mag7,
	Negev,
	Sawedoff,
	Tec9,
	Taser,
	Hkp2000,
	Mp7,
	Mp9,
	Nova,
	P250,
	Shield,
	Scar20,
	Sg553,
	Ssg08,
	GoldenKnife,
	Knife,
	Flashbang = 43,
	HeGrenade,
	SmokeGrenade,
	Molotov,
	Decoy,
	IncGrenade,
	C4,
	Healthshot = 57,
	KnifeT = 59,
	M4a1_s,
	Usp_s,
	Cz75a = 63,
	Revolver,
	TaGrenade = 68,
	Axe = 75,
	Hammer,
	Spanner = 78,
	GhostKnife = 80,
	Firebomb,
	Diversion,
	FragGrenade,
	Snowball,
	BumpMine,
	Bayonet = 500,
	ClassicKnife = 503,
	Flip = 505,
	Gut,
	Karambit,
	M9Bayonet,
	Huntsman,
	Falchion = 512,
	Bowie = 514,
	Butterfly,
	Daggers,
	Paracord,
	SurvivalKnife,
	Ursus = 519,
	Navaja,
	NomadKnife,
	Stiletto = 522,
	Talon,
	SkeletonKnife = 525,
	NameTag = 1200,
	Sticker = 1209,
	MusicKit = 1314,
	SealedGraffiti = 1348,
	Graffiti = 1349,
	OperationHydraPass = 1352,
	BronzeOperationHydraCoin = 4353,
	Patch = 4609,
	Berlin2019SouvenirToken = 4628,
	GloveStuddedBrokenfang = 4725,
	Stockholm2021SouvenirToken = 4802,
	GloveStuddedBloodhound = 5027,
	GloveT,
	GloveCT,
	GloveSporty,
	GloveSlick,
	GloveLeatherWrap,
	GloveMotorcycle,
	GloveSpecialist,
	GloveHydra
};

enum EWeaponType : int
{
	WEAPONTYPE_KNIFE = 0,
	WEAPONTYPE_PISTOL = 1,
	WEAPONTYPE_SUBMACHINEGUN = 2,
	WEAPONTYPE_RIFLE = 3,
	WEAPONTYPE_SHOTGUN = 4,
	WEAPONTYPE_SNIPER = 5,
	WEAPONTYPE_MACHINEGUN = 6,
	WEAPONTYPE_C4 = 7,
	WEAPONTYPE_PLACEHOLDER = 8,
	WEAPONTYPE_GRENADE = 9,
	WEAPONTYPE_HEALTHSHOT = 11,
	WEAPONTYPE_FISTS = 12,
	WEAPONTYPE_BREACHCHARGE = 13,
	WEAPONTYPE_BUMPMINE = 14,
	WEAPONTYPE_TABLET = 15,
	WEAPONTYPE_MELEE = 16
};

class CBaseCombatWeapon : public CBaseEntity {
public:
	NETVAR(m_flNextPrimaryAttack, float, "DT_BaseCombatWeapon", "m_flNextPrimaryAttack")
    NETVAR(m_flNextSecondaryAttack, float, "DT_BaseCombatWeapon", "m_flNextPrimaryAttack")
    NETVAR(m_flRecoilIndex, float, "DT_WeaponCSBase", "m_flRecoilIndex")
	OFFSET(m_iItemDefinitionIndex, short, 0x2FBA)
    NETVAR(m_hOwner, unsigned long, "DT_BaseCombatWeapon", "m_hOwner")
    NETVAR(m_iClip, unsigned long, "DT_BaseCombatWeapon", "m_iClip1")
    NETVAR(m_fAccuracyPenalty, float, "CWeaponCSBase", "m_fAccuracyPenalty")
    NETVAR(m_hWeaponWorldModel, unsigned long, "DT_BaseCombatWeapon", "m_hWeaponWorldModel")
    NETVAR(m_fLastShotTime, float, "DT_WeaponCSBase", "m_fLastShotTime")
    NETVAR(m_flPostponeFireReadyTime, float, "DT_WeaponCSBase", "m_flPostponeFireReadyTime")
	NETVAR(m_zoomLevel, int, "DT_WeaponCSBaseGun", "m_zoomLevel")

    inline bool IsGrenade() {
        if (!this)
            return false;

        return GetWeaponInfo()->nWeaponType == WEAPONTYPE_GRENADE;
    }

    inline void UpdateAccuracyPenality() {
        CallVFunction<void(__thiscall*)(CBaseCombatWeapon*)>(this, 484)(this);
    }

    inline float GetInaccuracy() {
		return CallVFunction<float(__thiscall*)(CBaseCombatWeapon*)>(this, 483)(this);
    }

    inline float GetSpread() {
        return CallVFunction<float(__thiscall*)(CBaseCombatWeapon*)>(this, 453)(this);
    }

    inline float MaxSpeed() {
        return CallVFunction<float(__thiscall*)(CBaseCombatWeapon*)>(this, 442)(this);
    }

    inline bool ShootingWeapon() {
		auto weapon_info = GetWeaponInfo();
		if (!weapon_info)
			return false;
		int weapon_type = weapon_info->nWeaponType;

		switch (weapon_type) {
		case WEAPONTYPE_PISTOL:
		case WEAPONTYPE_SUBMACHINEGUN:
		case WEAPONTYPE_RIFLE:
		case WEAPONTYPE_SHOTGUN:
		case WEAPONTYPE_SNIPER:
		case WEAPONTYPE_MACHINEGUN:
			return true;
		default:
			return false;
		}
    }

	inline bool IsKnife() {
		auto winfo = GetWeaponInfo();
		if (!winfo)
			return false;
		return winfo->nWeaponType == WEAPONTYPE_KNIFE;
	}

	CCSWeaponData* GetWeaponInfo();
	std::string GetName(CCSWeaponData* custom_data = nullptr);
    bool CanShoot(bool revolver_check = true);
	bool ThrowingGrenade();
	DXImage& GetIcon();
};

class CC4 : public CBaseCombatWeapon {
public:
	NETVAR(m_bStartedArming, bool, "DT_WeaponC4", "m_bStartedArming")
	NETVAR(m_fArmedTime, float, "DT_WeaponC4", "m_fArmedTime")
};