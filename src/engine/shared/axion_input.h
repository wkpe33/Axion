/* Adapted from BestClient inputs, Copyright 2026 BestProject Team.
 * See BESTCLIENT-LICENSE.txt.
 */
#ifndef ENGINE_SHARED_AXION_INPUT_H
#define ENGINE_SHARED_AXION_INPUT_H

#include <base/vmath.h>
#include <engine/shared/config.h>

#include <algorithm>
#include <cmath>

namespace AxionInput
{
enum EMode
{
	TCLIENT,
	BEST,
	CLOUD,
	SAIKO,
	DELTA,
	MEOW,
	ANCIENT,
	NEXT,
	SNOW,
	SALATIK,
	NOVA,
	JUKKI,
	LIGHT,
	AZURE,
};

inline bool Cloud()
{
	return g_Config.m_AxionInputMode == CLOUD && g_Config.m_BcCloudInputAmount > 0;
}

inline float Offset(bool Hook = false)
{
	switch(g_Config.m_AxionInputMode)
	{
	case BEST:
		return g_Config.m_BcBestInputAmount / 100.0f *
		       (1.0f - g_Config.m_BcBestInputSmoothing / 200.0f) *
		       (1.0f + g_Config.m_BcBestInputLatencyComp / 100.0f);
	case CLOUD:
		return g_Config.m_BcCloudInputAmount / 100.0f;
	case SAIKO:
		return g_Config.m_BcSaikoInputAmount / 100.0f;
	case DELTA:
		return g_Config.m_BcDeltaInputAmount / 100.0f;
	case MEOW:
		return g_Config.m_AxMeowInputAmount / 100.0f;
	case ANCIENT:
		return g_Config.m_AxAncientInputAmount / 20.0f;
	case NEXT:
		return g_Config.m_AxNextInputAmount / 100.0f;
	case SNOW:
		return Hook && g_Config.m_AxSnowInputHookEnabled && g_Config.m_AxSnowInputAmount > 0 ? g_Config.m_AxSnowInputHookAmount / 20.0f : g_Config.m_AxSnowInputAmount / 100.0f;
	case SALATIK:
		return g_Config.m_AxSalatikInputAmount / 100.0f *
		       (1.0f - g_Config.m_AxSalatikInputSmoothing / 200.0f) *
		       (1.0f + g_Config.m_AxSalatikInputLatencyComp / 100.0f);
	case NOVA:
		return g_Config.m_AxNovaInputAmount / 100.0f;
	case JUKKI:
		return g_Config.m_AxJukkiInputAmount / 1000.0f;
	case LIGHT:
		return Hook && g_Config.m_AxLightInputHookEnabled && g_Config.m_AxLightInputAmount > 0 ? g_Config.m_AxLightInputHookAmount / 20.0f : g_Config.m_AxLightInputAmount / 100.0f;
	case AZURE:
		return g_Config.m_AxAzureInputAmount / 100.0f;
	default:
		return g_Config.m_TcFastInput ? g_Config.m_TcFastInputAmount / 20.0f : 0.0f;
	}
}

inline bool Enabled()
{
	return g_Config.m_AxionInputMode == TCLIENT ? g_Config.m_TcFastInput != 0 : Offset() > 0.0f;
}

inline float OthersOffset(bool ReceivedPreInput)
{
	switch(g_Config.m_AxionInputMode)
	{
	case BEST:
		return g_Config.m_BcBestInputOthers ? Offset() : 0.0f;
	case CLOUD:
		return g_Config.m_BcCloudInputOthers && ReceivedPreInput && Cloud() ? 1.0f : 0.0f;
	case SAIKO:
		return g_Config.m_BcSaikoInputOthers ? Offset() : 0.0f;
	case DELTA:
		return g_Config.m_BcDeltaInputOthers ? Offset() : 0.0f;
	case MEOW:
		return g_Config.m_AxMeowInputOthers ? (Offset()) : 0.0f;
	case ANCIENT:
		return g_Config.m_AxAncientInputOthers ? (std::min(g_Config.m_AxAncientInputAmount, g_Config.m_AxAncientInputOthersAmount) / 20.0f) : 0.0f;
	case NEXT:
		return g_Config.m_AxNextInputOthers ? (Offset()) : 0.0f;
	case SNOW:
		return g_Config.m_AxSnowInputOthers ? (Offset()) : 0.0f;
	case SALATIK:
		return g_Config.m_AxSalatikInputOthers ? (Offset()) : 0.0f;
	case NOVA:
		return g_Config.m_AxNovaInputOthers ? (Offset()) : 0.0f;
	case JUKKI:
		return g_Config.m_AxJukkiInputOthers ? (Offset() > 0.0f ? 1.0f : 0.0f) : 0.0f;
	case LIGHT:
		return g_Config.m_AxLightInputOthers ? (Offset()) : 0.0f;
	case AZURE:
		return g_Config.m_AxAzureInputOthers ? (Offset()) : 0.0f;
	default:
		return g_Config.m_TcFastInputOthers ? Offset() : 0.0f;
	}
}

inline void ApplyOffset(float OffsetTicks, int &Tick, float &Intra)
{
	const float Shifted = Intra + OffsetTicks;
	const int Whole = static_cast<int>(Shifted);
	Tick += Whole;
	Intra = Shifted - Whole;
}

inline float Interpolation(float Fraction, float Distance)
{
	const bool Salatik = g_Config.m_AxionInputMode == SALATIK;
	if((g_Config.m_AxionInputMode != BEST && !Salatik) || Offset() <= 0.0f)
		return Fraction;
	const float Linear = std::clamp(Fraction, 0.0f, 1.0f);
	const float Cubic = Linear * Linear * (3.0f - 2.0f * Linear);
	switch(Salatik ? g_Config.m_AxSalatikInputInterpolation : g_Config.m_BcBestInputInterpolation)
	{
	case 2: return Cubic;
	case 3: return mix(Linear, Cubic, std::clamp(Distance / 1000.0f, 0.0f, 1.0f));
	default: return Linear;
	}
}
} // namespace AxionInput

#endif
