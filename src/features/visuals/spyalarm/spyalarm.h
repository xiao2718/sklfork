#pragma once

#include "../../../sdk/classes/player.h"
#include "../../../sdk/interfaces/interfaces.h"
#include "../../../sdk/helpers/helper.h"
#include "../../../settings/settings.h"
#include "../../entitylist/entitylist.h"
#include "../../../sdk/defs.h"
#include <cmath>
#include <string>

namespace Misc
{
	// Runs ConVar-based misc features (no push, no engine sleep).
	// Call every tick from Post_CreateMove.
	inline void RunConVars()
	{
		if (!interfaces::Cvar)
			return;

		// No Push (tf_avoidteammates_pushaway)
		static ConVar* tf_avoid = interfaces::Cvar->FindVar("tf_avoidteammates_pushaway");
		if (tf_avoid)
		{
			int targetPush = Settings::Misc.no_push ? 0 : 1;
			if (tf_avoid->GetInt() != targetPush)
				tf_avoid->SetValue(targetPush);
		}

		// No Engine Sleep (engine_no_focus_sleep)
		static ConVar* engine_sleep = interfaces::Cvar->FindVar("engine_no_focus_sleep");
		if (engine_sleep)
		{
			int targetSleep = Settings::Misc.no_engine_sleep ? 0 : 50;
			if (engine_sleep->GetInt() != targetSleep)
				engine_sleep->SetValue(targetSleep);
		}
	}

	// Draws a pulsing on-screen alarm when an enemy spy is within spy_alarm_range.
	// Must be called inside Surface->StartDrawing() / FinishDrawing().
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
		
		// Pulse alpha between 80 and 255 using an 8 Hz sine wave frequency
		float pulse = (sinf(interfaces::GlobalVars->curtime * 8.0f) + 1.0f) * 0.5f;
		int alpha = static_cast<int>(80 + pulse * 175);

		Color warningColor = { 255, 50, 50, alpha };

		int sw, sh;
		interfaces::Engine->GetScreenSize(sw, sh);

		// Draw Text
		FontManager::SetFont("esp font");
		const std::string msg = "!!! SPY NEARBY !!!";
		int textw = 0, texth = 0;
		helper::draw::GetTextSize(msg, textw, texth);

		int tx = (sw - textw) / 2;
		int ty = static_cast<int>(sh * 0.20f);
		helper::draw::TextShadow(tx, ty, warningColor, msg);

		// Draw Flashing Screen Border
		Color borderColor = { 255, 0, 0, static_cast<int>(pulse * 120) };
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
			if (interfaces::GlobalVars->curtime - lastSoundTime > 1.5f)
			{
				interfaces::Surface->PlaySound("ui/spy_high_common.wav"); 
				lastSoundTime = interfaces::GlobalVars->curtime;
			}
		}
	}
}