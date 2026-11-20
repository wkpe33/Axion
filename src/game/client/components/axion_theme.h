#ifndef GAME_CLIENT_COMPONENTS_AXION_THEME_H
#define GAME_CLIENT_COMPONENTS_AXION_THEME_H
#include <base/color.h>

#include <engine/shared/config.h>

#include <algorithm>
#include <cmath>
struct SAxionPalette
{
	ColorRGBA m_Background, m_Border, m_Text;
};
inline SAxionPalette AxionThemePalette(int Theme)
{
	if(Theme == 1)
		return {{0.94f, 0.935f, 0.92f, 1}, {0.65f, 0.65f, 0.64f, 1}, {0.12f, 0.13f, 0.14f, 1}};
	return {{0.025f, 0.025f, 0.035f, 1}, {0.45f, 0.46f, 0.50f, 1}, {0.96f, 0.96f, 0.98f, 1}};
}
inline ColorRGBA AxionMixColor(ColorRGBA A, ColorRGBA B, float T)
{
	return {A.r + (B.r - A.r) * T, A.g + (B.g - A.g) * T, A.b + (B.b - A.b) * T, A.a + (B.a - A.a) * T};
}
inline float AxionTransition(float Delta, float Milliseconds = 150)
{
	return g_Config.m_AxReduceMotion || Milliseconds <= 0 ? 1.0f : 1.0f - std::exp(-std::clamp(Delta, 0.0f, 0.1f) * 3000.0f / Milliseconds);
}
inline void AxionMixPalette(SAxionPalette &Current, const SAxionPalette &Target, float T)
{
	Current.m_Background = AxionMixColor(Current.m_Background, Target.m_Background, T);
	Current.m_Border = AxionMixColor(Current.m_Border, Target.m_Border, T);
	Current.m_Text = AxionMixColor(Current.m_Text, Target.m_Text, T);
}
#endif
