#pragma once

#include <string>
#include "BaseElement.h"
#include "../../../settings/settings.h"
#include "../../../sdk/helpers/helper.h"
#include "../esp_utils.h"

class JaratedCondElement : public IBaseElement
{
public:
	bool ShouldDraw(CBaseEntity* pEnt, const ESP_Data&) const override
	{
		if (!pEnt->IsPlayer())
			return false;

		CTFPlayer* pPlayer = static_cast<CTFPlayer*>(pEnt);
		return pPlayer->InCond(TF_COND_URINE) && (Settings::ESP.fconditions & static_cast<int>(ESPConditionFlags::Jarated));
	}

	void Draw(Vec2& pos, CBaseEntity* ent, const ESP_Data& data, ESPContext&) const override
	{
		Color color = GetColor(ent, data);
		helper::draw::TextShadow(pos.x, pos.y, color, "Jarate");
	}

	Vec2 GetSize(const ESP_Data&) const override
	{
		int w, h;
		helper::draw::GetTextSize("Jarate", w, h);
		return Vec2(w, h);
	}

	ESP_ALIGNMENT GetAlignment() const override
	{
		return ESP_ALIGNMENT::RIGHT;
	}

	Color GetColor(CBaseEntity* pEnt, const ESP_Data &data) const override
	{
		return Color{255, 200, 0, 255};
	}
};