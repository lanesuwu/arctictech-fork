#pragma once
#include "../../Utils/VitualFunction.h"
#include "IEngineTrace.h"

class CBaseEntity;
class CBasePlayer;
class IClientNetworkable;
class ClientClass;
class CCSWeaponData;

class IClientEntityList
{
public:
    virtual IClientNetworkable*   GetClientNetworkable(int entnum) = 0;
    virtual IClientNetworkable*   GetClientNetworkableFromHandle(unsigned long hEnt) = 0;
    virtual void*                 GetClientUnknownFromHandle(unsigned long hEnt) = 0;
    virtual CBaseEntity*          GetClientEntity(int entNum) = 0;
    virtual CBaseEntity*          GetClientEntityFromHandle(unsigned long hEnt) = 0;
    virtual int                   NumberOfEntities(bool bIncludeNonNetworkable) = 0;
    virtual int                   GetHighestEntityIndex(void) = 0;
    virtual void                  SetMaxEntities(int maxEnts) = 0;
    virtual int                   GetMaxEntities() = 0;

    CBasePlayer* GetLocalOrSpec();
};

class IWeaponSystem {
public:
    CCSWeaponData* GetWeaponData(short index) {
        return CallVFunction<CCSWeaponData* (__thiscall*)(IWeaponSystem*, short)>(this, 2)(this, index);
    }
};

void* UTIL_GetServerPlayer(int playerIndex);