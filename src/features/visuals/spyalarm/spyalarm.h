#pragma once

#include "../../../sdk/classes/player.h"
#include "../../../sdk/interfaces/interfaces.h"
#include "../../../sdk/helpers/helper.h"
#include "../../../settings/settings.h"
#include "../../entitylist/entitylist.h"
#include "../../../sdk/defs.h"
#include <string>

namespace Misc
{
	// Runs ConVar-based misc features (no push, no engine sleep).
	// Call every tick from Post_CreateMove.
	// inline void RunConVars()
	// {
	// 	if (!interfaces::Cvar)
	// 		return;
 //
	// 	// No Push (tf_avoidteammates_pushaway)
	// 	static ConVar* tf_avoid = interfaces::Cvar->FindVar("tf_avoidteammates_pushaway");
	// 	if (tf_avoid)
	// 	{
	// 		int targetPush = Settings::Misc.no_push ? 0 : 1;
	// 		if (tf_avoid->GetInt() != targetPush)
	// 			tf_avoid->SetValue(targetPush);
	// 	}
 //
	// 	// No Engine Sleep (engine_no_focus_sleep)
	// 	static ConVar* engine_sleep = interfaces::Cvar->FindVar("engine_no_focus_sleep");
	// 	if (engine_sleep)
	// 	{
	// 		int targetSleep = Settings::Misc.no_engine_sleep ? 0 : 50;
	// 		if (engine_sleep->GetInt() != targetSleep)
	// 			engine_sleep->SetValue(targetSleep);
	// 	}
	// }

	// Draws an on-screen alarm when an enemy spy is within spy_alarm_range.
	inline void DrawSpyAlarm(CTFPlayer* pLocal)
	{
		if (!Settings::Misc.spy_alarm || !interfaces::GlobalVars || !interfaces::Engine || !interfaces::Surface)
			return;

		if (pLocal == nullptr || !pLocal->IsAlive())
			return;

		Vector localPos = pLocal->GetAbsOrigin();
		bool spyNearby = false;

		// Iterate through entities to find a nearby enemy spy
		for (const auto& entry : EntityList::GetEntities())
		{
			if (!(entry.flags & EntityFlags::IsPlayer))
				continue;

			if (!(entry.flags & EntityFlags::IsEnemy))
				continue;

			if (!(entry.flags & EntityFlags::IsAlive))
				continue;

			CTFPlayer* pEnemy = static_cast<CTFPlayer*>(entry.ptr);
			if (pEnemy == nullptr || pEnemy->m_iClass() != TF_CLASS_SPY)
				continue;

			float dist = (pEnemy->GetAbsOrigin() - localPos).Length();
			if (dist <= Settings::Misc.spy_alarm_range)
			{
				spyNearby = true;
				break;
			}
		}

		if (!spyNearby)
			return;

		// --- Visuals ---
		
		// Fixed colors (no pulse)
		Color warningColor = { 255, 50, 50, 255 };

		int sw, sh;
		interfaces::Engine->GetScreenSize(sw, sh);

		// Draw Text
		const std::string msg = "!!! SPY NEARBY !!!";
		int textw = 0, texth = 0;
		
		// Note: No font arg passed here because FontManager::SetFont handles it in the hook!
		helper::draw::GetTextSize(msg, textw, texth);

		int tx = (sw - textw) / 2;
		int ty = static_cast<int>(sh * 0.20f);
		helper::draw::TextShadow(tx, ty, warningColor, msg);

		// Draw Screen Border
		Color borderColor = { 255, 0, 0, 120 }; // Fixed alpha 
		constexpr int borderThickness = 4;

		interfaces::Surface->DrawSetColor(borderColor);
		interfaces::Surface->DrawFilledRect(0, 0, sw, borderThickness); // Top
		interfaces::Surface->DrawFilledRect(0, sh - borderThickness, sw, sh); // Bottom
		interfaces::Surface->DrawFilledRect(0, 0, borderThickness, sh); // Left
		interfaces::Surface->DrawFilledRect(sw - borderThickness, 0, sw, sh); // Right

		// --- Sound ---
		
		if (Settings::Misc.spy_alarm_sound)
		{
			static float lastSoundTime = 0.0f;
			
			// Play sound every 1.5 seconds if a spy is still nearby
			// Also checks if curtime < lastSoundTime to fix the sound breaking on server changes
			if (interfaces::GlobalVars->curtime - lastSoundTime > 1.5f || interfaces::GlobalVars->curtime < lastSoundTime)
			{
				interfaces::Surface->PlaySound("ui/spy_high_common.wav"); 
				lastSoundTime = interfaces::GlobalVars->curtime;
			}
		}
	}
}
