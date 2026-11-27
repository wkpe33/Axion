#ifndef GAME_CLIENT_AXION_UI_H
#define GAME_CLIENT_AXION_UI_H

#include <base/color.h>
#include <engine/graphics.h>
#include <game/client/ui_rect.h>

#include <algorithm>
#include <cmath>
#include <chrono>

namespace AxionUi
{
constexpr ColorRGBA Gray(float Shade, float Alpha = 1.0f)
{
	return ColorRGBA(Shade, Shade, Shade, Alpha);
}
constexpr auto Background = Gray(8 / 255.0f);
constexpr auto Panel = Gray(21 / 255.0f);
constexpr auto Button = Gray(28 / 255.0f);
constexpr auto Hover = Gray(41 / 255.0f);
constexpr auto Active = Gray(53 / 255.0f);
constexpr auto Border = Gray(48 / 255.0f);
constexpr auto Muted = Gray(160 / 255.0f);
constexpr float Radius = 6.0f;
inline std::chrono::nanoseconds StartupFinished{};

inline ColorRGBA Blend(ColorRGBA From, ColorRGBA To, float Amount)
{
	return ColorRGBA(std::lerp(From.r, To.r, Amount), std::lerp(From.g, To.g, Amount),
		std::lerp(From.b, To.b, Amount), std::lerp(From.a, To.a, Amount));
}

inline void Surface(CUIRect Rect, ColorRGBA Color, int Corners = IGraphics::CORNER_ALL, float Rounding = Radius)
{
	if(Rect.w <= 1 || Rect.h <= 1)
		return;
	Rounding = std::clamp(Rounding, 0.0f, std::min(Rect.w, Rect.h) / 2);
	Rect.Draw(Border.WithAlpha(Color.a), Corners, Rounding);
	Rect.Margin(0.55f, &Rect);
	Rect.Draw(Color, Corners, std::max(0.0f, Rounding - 0.55f));
}

inline float Animate(float Value, float Target, float Delta, bool Enabled)
{
	return Enabled ? Target + (Value - Target) * std::exp(-std::clamp(Delta, 0.0f, 0.1f) * 16.0f) : Target;
}

inline CUIRect LogoRect(CUIRect Screen, float VerticalCenter)
{
	const float Width = std::min(Screen.h * 0.65f, Screen.w * 0.58f);
	const float Height = Width / 3.0f;
	return {Screen.x + (Screen.w - Width) / 2, Screen.y + Screen.h * VerticalCenter - Height / 2, Width, Height};
}

inline void Logo(IGraphics *pGraphics, IGraphics::CTextureHandle Texture, CUIRect Screen, float VerticalCenter, float Alpha, bool Compact = false)
{
	if(!Texture.IsValid())
		return;
	CUIRect Rect = LogoRect(Screen, VerticalCenter);
	if(Compact)
	{
		const float Width = std::min(Screen.w - 8.0f, (Screen.h - 3.0f) * 3.0f);
		Rect = {Screen.x + (Screen.w - Width) * 0.5f, Screen.y + (Screen.h - Width / 3.0f) * 0.5f, Width, Width / 3.0f};
	}
	pGraphics->WrapClamp();
	pGraphics->TextureSet(Texture);
	pGraphics->QuadsBegin();
	pGraphics->SetColor(1, 1, 1, Alpha);
	IGraphics::CQuadItem Quad(Rect.x, Rect.y, Rect.w, Rect.h);
	pGraphics->QuadsDrawTL(&Quad, 1);
	pGraphics->QuadsEnd();
	pGraphics->SetColor(1, 1, 1, 1);
	pGraphics->WrapNormal();
}
}
#endif
