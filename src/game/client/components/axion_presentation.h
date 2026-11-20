#ifndef GAME_CLIENT_COMPONENTS_AXION_PRESENTATION_H
#define GAME_CLIENT_COMPONENTS_AXION_PRESENTATION_H
#include <engine/shared/config.h>

#include <generated/protocol.h>
namespace AxionPresentation
{
	enum class EOwner
	{
		MY,
		OTHER,
		SHARED
	};
	inline EOwner Owner(int Id, int MainId, int DummyId)
	{
		if(Id < 0 || Id >= MAX_CLIENTS)
			return EOwner::SHARED;
		return Id == MainId || Id == DummyId ? EOwner::MY : EOwner::OTHER;
	}
	inline thread_local int s_Owner = -1;
	class COwnerScope
	{
		int m_Previous;

	public:
		explicit COwnerScope(int Id) :
			m_Previous(s_Owner) { s_Owner = Id; }
		~COwnerScope() { s_Owner = m_Previous; }
		COwnerScope(const COwnerScope &) = delete;
		COwnerScope &operator=(const COwnerScope &) = delete;
	};
	enum class EEffect
	{
#define AXION_EFFECT(Name, Script, Label) Name,
#define AXION_SOUND(Name, Script, Id, Label)
#include <engine/shared/axion_presentation_catalog.h>
#undef AXION_EFFECT
#undef AXION_SOUND
		COUNT
	};
	inline int &EffectSetting(EEffect Effect, EOwner Owner)
	{
		switch(Effect)
		{
#define AXION_EFFECT(Name, Script, Label) \
	case EEffect::Name: return Owner == EOwner::MY ? g_Config.m_AxEffectMy##Name : Owner == EOwner::OTHER ? g_Config.m_AxEffectOther##Name : \
														g_Config.m_AxEffectShared##Name;
#define AXION_SOUND(Name, Script, Id, Label)
#include <engine/shared/axion_presentation_catalog.h>
#undef AXION_EFFECT
#undef AXION_SOUND
		default: return g_Config.m_AxEffectSharedParticles;
		}
	}
	inline bool EffectEnabled(EEffect Effect, int MainId, int DummyId)
	{
		return EffectSetting(Effect, Owner(s_Owner, MainId, DummyId)) != 0;
	}
	inline constexpr int SOUND_COUNT = 0
#define AXION_EFFECT(Name, Script, Label)
#define AXION_SOUND(Name, Script, Id, Label) +1
#include <engine/shared/axion_presentation_catalog.h>
#undef AXION_EFFECT
#undef AXION_SOUND
		;
	inline int *SoundSetting(int Sound, EOwner Owner)
	{
		switch(Sound)
		{
#define AXION_EFFECT(Name, Script, Label)
#define AXION_SOUND(Name, Script, Id, Label) \
	case Id: return Owner == EOwner::MY ? &g_Config.m_AxSoundMy##Name : Owner == EOwner::OTHER ? &g_Config.m_AxSoundOther##Name : \
												     &g_Config.m_AxSoundShared##Name;
#include <engine/shared/axion_presentation_catalog.h>
#undef AXION_EFFECT
#undef AXION_SOUND
		default: return nullptr;
		}
	}
	inline bool SoundEnabled(int Sound, int MainId, int DummyId)
	{
		const int *pSetting = SoundSetting(Sound, Owner(s_Owner, MainId, DummyId));
		return !pSetting || *pSetting != 0;
	}
}
#endif
