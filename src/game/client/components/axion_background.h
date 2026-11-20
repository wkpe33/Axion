#ifndef GAME_CLIENT_COMPONENTS_AXION_BACKGROUND_H
#define GAME_CLIENT_COMPONENTS_AXION_BACKGROUND_H
#include <base/color.h>

#include <engine/graphics.h>
#include <engine/shared/axion_background_presets.h>
#include <engine/shared/config.h>

#include <game/client/ui_rect.h>
inline SAxionBackground AxionCurrentBackground()
{
	return {0, g_Config.m_ClBackgroundEntitiesColor, 128, 0};
}
inline void AxionApplyBackground(const SAxionBackground &Background)
{
	if(!Background.Valid())
		return;
	g_Config.m_ClBackgroundEntitiesColor = Background.m_Primary;
}
inline void AxionDrawBackground(IGraphics *, const CUIRect &Rect, const SAxionBackground &Background)
{
	Rect.Draw(color_cast<ColorRGBA>(ColorHSLA(Background.m_Primary)), 0, 0);
}
#endif
