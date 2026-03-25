#include "warp.h"
#include "../../settings/settings.h"
#include "../../imgui/imgui.h"
#include <cstdio>

namespace Warp
{
	int m_iStoredTicks = 0;
	WarpState m_iDesiredState = WarpState::WAITING;
	bool m_bShifting = false;
	bool m_bRecharging = false;
	int m_iShiftAmount = 0;
	bool m_bDoubleTap = false;

	void Reset()
	{
		m_iDesiredState = WarpState::WAITING;
		m_iStoredTicks = 0;
		m_bShifting = false;
		m_bRecharging = false;
		m_iShiftAmount = 0;
		m_bDoubleTap = false;
	}

	int GetMaxTicks()
	{
		static ConVar* sv_maxusrcmdprocessticks = interfaces::Cvar->FindVar("sv_maxusrcmdprocessticks");
		return sv_maxusrcmdprocessticks->GetInt();
	}

	void RunCreateMove(CTFPlayer* pLocal, CTFWeaponBase* pWeapon, CUserCmd* pCmd)
	{
		if (!Settings::AntiAim.warp_key->IsActive()
		&& !Settings::AntiAim.warp_dt_key->IsActive()
		&& !Settings::AntiAim.warp_recharge_key->IsActive())
			Warp::m_iDesiredState = WarpState::WAITING;

		if (!Settings::AntiAim.warp_key->IsEnabled())
			return;

		if (!interfaces::Engine->IsInGame() || !interfaces::Engine->IsConnected())
			return;

		if (interfaces::EngineVGui->IsGameUIVisible() || interfaces::EngineVGui->IsConsoleVisible())
			return;

		// this is some shitty ass work
		// and it doesn't work right either
		if (m_iDesiredState == WarpState::DT_MOVING)
		{
			const Vector vecVelocity = pLocal->GetVelocity();
			float flSpeed = vecVelocity.Length2D();

			if (flSpeed <= 20.0f)
			{
				m_iDesiredState = WarpState::DT;
				return;
			}

			Vector vecForward, vecRight;
			Math::AngleVectors(pCmd->viewangles, &vecForward, &vecRight);

			const float forwardVel = vecVelocity.Dot(vecForward);
			const float sideVel = vecVelocity.Dot(vecRight);

			// why ts looks like the user just stops pressing WASD?
			// just fucking kill me already
			pCmd->forwardmove = -forwardVel;
			pCmd->sidemove = -sideVel;
			pCmd->buttons &= ~IN_ATTACK;
			return;
		}

		if (Settings::AntiAim.warp_dt_key->IsActive() && Warp::m_iStoredTicks >= Warp::GetMaxTicks())
		{
			if (!IsValidWeapon(pWeapon))
				return;

			if (helper::localplayer::IsAttacking(pLocal, pWeapon, pCmd))
			{
				if (pLocal->GetVelocity().Length2D() > 0.0f)
				{
					m_iDesiredState = WarpState::DT_MOVING;
					pCmd->buttons &= ~IN_ATTACK;
					return;
				}
				else
				{
					m_iDesiredState = WarpState::DT;
					return;
				}
			}
		}

		if (Settings::AntiAim.warp_key->IsActive() && Warp::m_iStoredTicks > 0)
		{
			Warp::m_iDesiredState = WarpState::RUNNING;
			return;
		}

		if (Settings::AntiAim.warp_recharge_key->IsActive() && Warp::m_iStoredTicks < GetMaxTicks())
		{
			Warp::m_iDesiredState = WarpState::RECHARGING;
			return;
		}
	}

	void DrawContents()
	{
		ImVec2 pos = ImGui::GetCursorScreenPos();
		ImDrawList* draw = ImGui::GetWindowDrawList();
		constexpr int width = 100;
		constexpr int height = 20;

		float ratio = static_cast<float>(m_iStoredTicks)/GetMaxTicks();

		ImVec2 min = {pos.x, pos.y};
		ImVec2 max = {pos.x + (width * ratio), pos.y + height};

		if (m_iStoredTicks > 0)
			draw->AddRectFilled(min, max, IM_COL32(0, 119, 200, 255), 2.0f);

		char chrText[12];
		sprintf(chrText, "%i/%i", Warp::m_iStoredTicks, GetMaxTicks());
		ImVec2 textSize = ImGui::CalcTextSize(chrText);

		draw->AddText(ImVec2(pos.x + width/2.0f - textSize.x/2.0f, pos.y + height/2.0f - textSize.y/2.0f), IM_COL32(255, 255, 255, 255), chrText);
	}

	void RunWindow()
	{
		int flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
		if (!Settings::menu_open)
			flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration;

		if (ImGui::Begin("Stored Ticks", nullptr, flags))
		{
			DrawContents();
			ImGui::Dummy(ImVec2(100, 20));
		}

		ImGui::End();
	}

	bool IsValidWeapon(CTFWeaponBase* pWeapon)
	{
		return pWeapon->GetWeaponType() == EWeaponType::HITSCAN;
	}
}