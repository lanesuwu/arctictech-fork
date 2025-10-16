#include "../../SDK/Interfaces.h"
#include "../../SDK/Globals.h"
#include "Glow.h"

void Glow::Run() {
	if (!Cheat.LocalPlayer || !Cheat.InGame)
		return;

	for (int i = 0; i < GlowObjectManager->m_GlowObjects.Count(); i++) {
		GlowObjectDefinition_t& glowObject = GlowObjectManager->m_GlowObjects[i];

		if (!glowObject.m_pEntity || glowObject.m_pEntity->m_bDormant())
			continue;

		if (glowObject.m_nNextFreeSlot != -2)
			continue;

		int classId = glowObject.m_pEntity->GetClientClass()->m_ClassID;

		Color col;
		if (glowObject.m_pEntity->IsPlayer() && reinterpret_cast<CBasePlayer*>(glowObject.m_pEntity)->IsEnemy() && config.visuals.esp.glow->get())
			col = config.visuals.esp.glow_color->get();
		else if ((classId == C_BASE_CS_GRENADE || classId == C_BASE_CS_GRENADE_PROJECTILE || classId == C_MOLOTOV_PROJECTILE || classId == C_DECOY_PROJECTILE || classId == C_SMOKE_GRENADE_PROJECTILE) && config.visuals.other_esp.grenades->get())
			col = config.visuals.other_esp.grenade_predict_color->get();
		else if (classId == C_PLANTED_C4) {
			col = config.visuals.other_esp.bomb_color->get();
			col.alpha_modulatef(col.a * 0.8f);
		} else
			continue;

		glowObject.m_flGlowAlpha = col.a / 255.f;
		glowObject.m_flGlowAlphaMax = 1.f;
		glowObject.m_vGlowColor.x = col.r / 255.f;
		glowObject.m_vGlowColor.y = col.g / 255.f;
		glowObject.m_vGlowColor.z = col.b / 255.f;
		glowObject.m_nRenderStyle = 0;
		glowObject.m_bRenderWhenOccluded = true;
		//glowObject.m_bRenderWhenUnoccluded = false;
	}
}