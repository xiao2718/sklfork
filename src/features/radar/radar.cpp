#include "radar.h"

namespace Radar
{
	void DrawContents();
	void DrawHealthbar(ImDrawList* draw, ImVec2 pos, int health, int maxhealth, int iconSize);

	int m_iRange;
	float m_flRadius;

	void Init();

	// Call in EngineVGui->Paint
	void Run();

	float GetRadius();
	int GetRange();
	Vec2 WorldToRadar(const Vector& localPos, const Vector& enemyPos, float viewAnglesYaw);
};

void Radar::Run()
{
	int size = Settings::Radar.size;

	m_iRange = Settings::Radar.range;
	m_flRadius = size * 0.5f;

	if (size == 0)
		return;

	int flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
	if (!Settings::menu_open)
		flags |= ImGuiWindowFlags_NoMove;

	if (ImGui::Begin("Radar", nullptr, flags))
	{
		DrawContents();
		ImGui::Dummy(ImVec2(size, size));
	}

	ImGui::End();
}

Vec2 Radar::WorldToRadar(const Vector& localPos, const Vector& enemyPos, float viewAnglesYaw)
{
	Vec2 delta = {enemyPos.x - localPos.x, enemyPos.y - localPos.y};

	float yaw = DEG2RAD(-viewAnglesYaw);
	float cosYaw = std::cos(yaw);
	float sinYaw = std::sin(yaw);

	float rx = delta.x * cosYaw - delta.y * sinYaw;
	float ry = delta.y * cosYaw + delta.x * sinYaw;

	float dist = std::sqrt(rx * rx + ry * ry);
	if (dist > m_iRange)
	{
		float s = m_iRange / dist;
		rx *= s;
		ry *= s;
	}

	rx = (rx / m_iRange) * m_flRadius;
	ry = (ry / m_iRange) * m_flRadius;

	return { rx, ry };
}

float Radar::GetRadius()
{
	return m_flRadius;
}

int Radar::GetRange()
{
	return m_iRange;
}

// Draw a directional arrow (triangle) at `pos` pointing from `center` toward `pos`.
static void DrawSpyArrow(ImDrawList* draw, ImVec2 center, ImVec2 pos, float iconSize)
{
	float dx = pos.x - center.x;
	float dy = pos.y - center.y;
	float len = std::sqrt(dx * dx + dy * dy);
	if (len < 0.0001f) return;

	// Normalised direction from center to spy
	float nx = dx / len;
	float ny = dy / len;

	// Perpendicular
	float px = -ny;
	float py =  nx;

	float tipLen  = iconSize * 2.2f;
	float baseLen = iconSize * 1.0f;
	float baseHalf = iconSize * 0.9f;

	ImVec2 tip   = { pos.x + nx * tipLen,  pos.y + ny * tipLen };
	ImVec2 base1 = { pos.x + px * baseHalf - nx * baseLen, pos.y + py * baseHalf - ny * baseLen };
	ImVec2 base2 = { pos.x - px * baseHalf - nx * baseLen, pos.y - py * baseHalf - ny * baseLen };

	// Filled triangle
	draw->AddTriangleFilled(tip, base1, base2, IM_COL32(255, 160, 0, 230));
	// Outline for readability
	draw->AddTriangle(tip, base1, base2, IM_COL32(0, 0, 0, 180), 1.2f);
}

void Radar::DrawContents()
{
	int size = Settings::Radar.size;
	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 center = { pos.x + m_flRadius, pos.y + m_flRadius };
	ImDrawList* draw = ImGui::GetWindowDrawList();

	// Background circle
	draw->AddCircle(center, m_flRadius, IM_COL32(30, 30, 30, 180), 64, 1.0f);

	// Cross-hair lines
	draw->AddLine({ center.x, pos.y },          { center.x, pos.y + size }, IM_COL32(255, 255, 255, 40));
	draw->AddLine({ pos.x, center.y },          { pos.x + size, center.y }, IM_COL32(255, 255, 255, 40));

	if (EntityList::GetPlayerResources() == nullptr)
		return;

	if (!interfaces::Engine->IsInGame() || !interfaces::Engine->IsConnected())
		return;

	CTFPlayer* pLocal = helper::engine::GetLocalPlayer();
	if (pLocal == nullptr || !pLocal->IsAlive())
		return;

	Vector localPos = pLocal->GetAbsOrigin();
	Vector viewAngles; interfaces::Engine->GetViewAngles(viewAngles);
	float viewYaw = viewAngles.y - 90.0f;

	int iconSize = Settings::Radar.icon_size;

	for (const auto& entry : EntityList::GetEntities())
	{
		if (entry.ptr == EntityList::GetLocal())
			continue;

		// --- Buildings ---
		if (entry.flags & EntityFlags::IsBuilding)
		{
			if (!Settings::Radar.buildings)
				continue;

			// enemies_only: skip friendly buildings
			if (Settings::Radar.enemies_only && !(entry.flags & EntityFlags::IsEnemy))
				continue;
		}

		// --- Players ---
		if (entry.flags & EntityFlags::IsPlayer)
		{
			if (!Settings::Radar.players)
				continue;

			CTFPlayer* pPlayer = static_cast<CTFPlayer*>(entry.ptr);
			if (!pPlayer->IsAlive())
				continue;

			// enemies_only: skip teammates
			if (Settings::Radar.enemies_only && !(entry.flags & EntityFlags::IsEnemy))
				continue;
		}

		// --- Projectiles ---
		if (entry.flags & EntityFlags::IsProjectile)
		{
			if (!Settings::Radar.projectiles)
				continue;
		}

		Vec2 p = WorldToRadar(localPos, entry.ptr->GetAbsOrigin(), viewYaw);
		Color color = ESP_Utils::GetEntityColor(entry.ptr);

		ImVec2 screenPos = { center.x + p.x, center.y - p.y };

		// Detect enemy spy for special rendering
		bool isEnemySpy = false;
		if ((entry.flags & (EntityFlags::IsPlayer | EntityFlags::IsEnemy)) ==
		    (EntityFlags::IsPlayer | EntityFlags::IsEnemy))
		{
			CTFPlayer* pPlayer = static_cast<CTFPlayer*>(entry.ptr);
			if (pPlayer != nullptr && pPlayer->m_iClass() == TF_CLASS_SPY)
				isEnemySpy = true;
		}

		if (isEnemySpy && Settings::Misc.spy_alarm)
		{
			// Draw a faint line from the radar center toward the spy so it
			// is visible even when the spy is far away (dot is on edge)
			draw->AddLine(center, screenPos, IM_COL32(255, 140, 0, 90), 1.5f);

			// Draw the directional arrow at the spy's radar position
			DrawSpyArrow(draw, center, screenPos, static_cast<float>(iconSize));
		}
		else
		{
			draw->AddCircleFilled(screenPos, static_cast<float>(iconSize),
			                      IM_COL32(color.r(), color.g(), color.b(), color.a()));
		}
	}
}

void Radar::DrawHealthbar(ImDrawList* draw, ImVec2 pos, int health, int maxhealth, int iconSize)
{
	int half = static_cast<int>(iconSize);
	constexpr int barOffset = 3;

	draw->AddRectFilled(ImVec2(pos.x - half, pos.y + barOffset),
	                    ImVec2(pos.x + iconSize, pos.y + (barOffset * 2.0f)),
	                    IM_COL32(255, 255, 255, 255));
}

void Radar::Init()
{
	m_iRange = 0;
	m_flRadius = 0;
}