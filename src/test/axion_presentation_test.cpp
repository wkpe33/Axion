#include "test.h"

#include <engine/config.h>
#include <engine/console.h>
#include <engine/kernel.h>
#include <engine/shared/axion_cursor.h>
#include <engine/storage.h>

#include <game/client/components/axion_presentation.h>
#include <game/client/prediction/gameworld.h>

#include <gtest/gtest.h>

using namespace AxionPresentation;
class AxionPresentationTest : public testing::Test
{
	CConfig m_Saved = g_Config;

protected:
	void SetUp() override
	{
#define AXION_EFFECT(Name, Script, Label) g_Config.m_AxEffectMy##Name = g_Config.m_AxEffectOther##Name = g_Config.m_AxEffectShared##Name = 1;
#define AXION_SOUND(Name, Script, Id, Label) g_Config.m_AxSoundMy##Name = g_Config.m_AxSoundOther##Name = g_Config.m_AxSoundShared##Name = 1;
#include <engine/shared/axion_presentation_catalog.h>
#undef AXION_EFFECT
#undef AXION_SOUND
	}
	void TearDown() override { g_Config = m_Saved; }
};

TEST_F(AxionPresentationTest, EveryEffectIsIndependentForMainDummyRemoteAndUnknown)
{
	for(int i = 0; i < (int)EEffect::COUNT; ++i)
	{
		const auto Effect = (EEffect)i;
		EffectSetting(Effect, EOwner::MY) = 0;
		for(int Id : {7, 9, 42, -1, (int)MAX_CLIENTS})
		{
			COwnerScope Scope(Id);
			EXPECT_EQ(EffectEnabled(Effect, 7, 9), Id != 7 && Id != 9) << i;
			for(int j = 0; j < (int)EEffect::COUNT; ++j)
				if(j != i)
					EXPECT_TRUE(EffectEnabled((EEffect)j, 7, 9));
		}
		EffectSetting(Effect, EOwner::MY) = 1;
		EffectSetting(Effect, EOwner::OTHER) = 0;
		{
			COwnerScope Scope(42);
			EXPECT_FALSE(EffectEnabled(Effect, 7, 9));
		}
		{
			COwnerScope Scope(-1);
			EXPECT_TRUE(EffectEnabled(Effect, 7, 9));
		}
		EffectSetting(Effect, EOwner::OTHER) = 1;
		EffectSetting(Effect, EOwner::SHARED) = 0;
		{
			COwnerScope Scope(42);
			EXPECT_TRUE(EffectEnabled(Effect, 7, 9));
		}
		{
			COwnerScope Scope(-1);
			EXPECT_FALSE(EffectEnabled(Effect, 7, 9));
		}
		EffectSetting(Effect, EOwner::SHARED) = 1;
	}
	EXPECT_EQ(Owner(9, 7, -1), EOwner::OTHER); // disconnected dummy ID is not local
}

TEST_F(AxionPresentationTest, EveryGameplaySoundHasThreeIndependentFilters)
{
	int Count = 0;
	for(int Id = 0; Id < NUM_SOUNDS; ++Id)
	{
		if(!SoundSetting(Id, EOwner::MY))
			continue;
		++Count;
		for(auto Group : {EOwner::MY, EOwner::OTHER, EOwner::SHARED})
		{
			*SoundSetting(Id, Group) = 0;
			for(int OwnerId : {7, 9, 42, -1})
			{
				COwnerScope Scope(OwnerId);
				EXPECT_EQ(SoundEnabled(Id, 7, 9), Owner(OwnerId, 7, 9) != Group) << Id;
				for(int Other = 0; Other < NUM_SOUNDS; ++Other)
					if(Other != Id)
						EXPECT_TRUE(SoundEnabled(Other, 7, 9));
			}
			*SoundSetting(Id, Group) = 1;
		}
	}
	EXPECT_EQ(Count, SOUND_COUNT);
	EXPECT_EQ(SoundSetting(SOUND_MENU, EOwner::SHARED), nullptr);
	EXPECT_EQ(SoundSetting(SOUND_CHAT_CLIENT, EOwner::SHARED), nullptr);
	EXPECT_EQ(SoundSetting(SOUND_CHAT_SERVER, EOwner::SHARED), nullptr);
	EXPECT_EQ(SoundSetting(SOUND_CHAT_HIGHLIGHT, EOwner::SHARED), nullptr);
}

TEST_F(AxionPresentationTest, ScopeRestoresOwnerAndProjectileIdsAreNotGuessed)
{
	EXPECT_EQ(s_Owner, -1);
	{
		COwnerScope Main(7);
		{
			COwnerScope Remote(42);
			EXPECT_EQ(s_Owner, 42);
		}
		EXPECT_EQ(s_Owner, 7);
	}
	EXPECT_EQ(s_Owner, -1);

	CGameWorld::CPredictedEvent A(NETEVENTTYPE_SOUNDWORLD, vec2(1, 2), 1000, 50, SOUND_GUN_FIRE);
	A.m_PresentationOwner = 7;
	auto B = A;
	EXPECT_EQ(B.m_PresentationOwner, 7);
	CGameWorld::CPredictedEvent Unknown(NETEVENTTYPE_SOUNDWORLD, vec2(1, 2), 7, 50, SOUND_GUN_FIRE);
	EXPECT_EQ(Owner(Unknown.m_PresentationOwner, 7, 9), EOwner::SHARED);
	EXPECT_EQ(Unknown.m_Id, 7); // An entity ID equal to our client ID is still not ownership.
}

TEST_F(AxionPresentationTest, ConfigRoundTripClampsAndPreservesUnrelatedValues)
{
	CTestInfo Info;
	Info.m_DeleteTestStorageFilesOnSuccess = true;
	auto Storage = Info.CreateTestStorage();
	ASSERT_NE(Storage, nullptr);
	std::unique_ptr<IKernel> Kernel(IKernel::Create());
	Kernel->RegisterInterface(Storage.get(), false);
	auto Console = CreateConsole(CFGFLAG_CLIENT);
	Kernel->RegisterInterface(Console.get(), false);
	std::unique_ptr<IConfigManager> Config(CreateConfigManager());
	Kernel->RegisterInterface(Config.get(), false);
	Console->Init();
	Config->Init();
	Console->StoreCommands(false);
	g_Config.m_ClSaveSettings = 1;
	g_Config.m_UiMousesens = 137;
#define AXION_EFFECT(Name, Script, Label) \
	Console->ExecuteLine("ax_effect_my_" #Script " -7", -1); \
	Console->ExecuteLine("ax_effect_other_" #Script " 9", -1); \
	Console->ExecuteLine("ax_effect_shared_" #Script " 0", -1);
#define AXION_SOUND(Name, Script, Id, Label) \
	Console->ExecuteLine("ax_sound_my_" #Script " -7", -1); \
	Console->ExecuteLine("ax_sound_other_" #Script " 9", -1); \
	Console->ExecuteLine("ax_sound_shared_" #Script " 0", -1);
#include <engine/shared/axion_presentation_catalog.h>
#undef AXION_EFFECT
#undef AXION_SOUND
	Console->ExecuteLine("ax_hud_invitation_x 1234; ax_hud_invitation_y 5678", -1);
	Console->ExecuteLine("ax_entities_background_preset MyPreset", -1);
	ASSERT_TRUE(Config->Save());
	SetUp();
	ASSERT_TRUE(Console->ExecuteFile("settings_ddnet.cfg", -1, true, IStorage::TYPE_SAVE));
#define AXION_EFFECT(Name, Script, Label) \
	EXPECT_EQ(g_Config.m_AxEffectMy##Name, 0); \
	EXPECT_EQ(g_Config.m_AxEffectOther##Name, 1); \
	EXPECT_EQ(g_Config.m_AxEffectShared##Name, 0); \
	EXPECT_EQ(DefaultConfig::AxEffectMy##Name, 1);
#define AXION_SOUND(Name, Script, Id, Label) \
	EXPECT_EQ(g_Config.m_AxSoundMy##Name, 0); \
	EXPECT_EQ(g_Config.m_AxSoundOther##Name, 1); \
	EXPECT_EQ(g_Config.m_AxSoundShared##Name, 0); \
	EXPECT_EQ(DefaultConfig::AxSoundMy##Name, 1);
#include <engine/shared/axion_presentation_catalog.h>
#undef AXION_EFFECT
#undef AXION_SOUND
	EXPECT_EQ(g_Config.m_UiMousesens, 137);
	EXPECT_STREQ(g_Config.m_AxEntitiesBackgroundPreset, "MyPreset");
	EXPECT_EQ(g_Config.m_AxHudInvitationX, 1234);
	EXPECT_EQ(g_Config.m_AxHudInvitationY, 5678);
}

TEST(AxionCursor, MenuAndHudHaveIdenticalPhysicalSpeedAtEveryWindowSize)
{
	for(vec2 Window : {vec2(1280, 720), vec2(1920, 1080), vec2(2560, 1440), vec2(3840, 2160)})
	{
		const vec2 Menu(Window.x / Window.y * 600, 600);
		const vec2 Hud = Menu / 2;
		for(float Sensitivity : {0.5f, 1.0f, 1.37f, 2.0f})
		{
			const vec2 Start = Window / 2;
			const vec2 End = Start + vec2(80, 40) * Sensitivity;
			const vec2 MenuDelta = (AxionCursorForCanvas(End, Window, Menu) - AxionCursorForCanvas(Start, Window, Menu)) * Window / Menu;
			const vec2 ChatDelta = (AxionCursorForCanvas(End, Window, Hud) - AxionCursorForCanvas(Start, Window, Hud)) * Window / Hud;
			EXPECT_NEAR(MenuDelta.x, ChatDelta.x, 0.001f);
			EXPECT_NEAR(MenuDelta.y, ChatDelta.y, 0.001f);
			EXPECT_NEAR(ChatDelta.x, 80 * Sensitivity, 0.001f);
		}
	}
	EXPECT_EQ(AxionCursorForCanvas(vec2(-50, 9999), vec2(1280, 720), vec2(640, 360)), vec2(0, 359.5f));
}
