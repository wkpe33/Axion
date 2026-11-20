#include "test.h"

#include <base/io.h>

#include <engine/console.h>
#include <engine/kernel.h>
#include <engine/shared/axion_texture_limits.h>
#include <engine/shared/config.h>
#include <engine/storage.h>

#include <game/client/components/axion_game_timer.h>

#include <gtest/gtest.h>

TEST(AxionConfig, ReadLegacyPrefixWithoutRegisteringLegacyCommands)
{
	CTestInfo Info;
	Info.m_DeleteTestStorageFilesOnSuccess = true;
	auto Storage = Info.CreateTestStorage();
	ASSERT_NE(Storage, nullptr);
	std::unique_ptr<IKernel> Kernel(IKernel::Create());
	Kernel->RegisterInterface(Storage.get(), false);
	auto Console = CreateConsole(CFGFLAG_CLIENT);
	Kernel->RegisterInterface(Console.get(), false);
	Console->Init();
	Console->StoreCommands(false);
	int Value = 0;
	Console->Register("ax_custom_aspect_ratio_num", "i[value]", CFGFLAG_CLIENT, [](IConsole::IResult *pResult, void *pData) { *static_cast<int *>(pData) = pResult->GetInteger(0); }, &Value, "test");
	ASSERT_EQ(Console->GetCommandInfo("bc_custom_aspect_ratio_num", CFGFLAG_CLIENT, false), nullptr);
	IOHANDLE File = Storage->OpenFile("legacy.cfg", IOFLAG_WRITE, IStorage::TYPE_SAVE);
	ASSERT_NE(File, nullptr);
	const char aConfig[] = "  bc_custom_aspect_ratio_num 175\n";
	io_write(File, aConfig, sizeof(aConfig) - 1);
	io_close(File);
	ASSERT_TRUE(Console->ExecuteFile("legacy.cfg", IConsole::CLIENT_ID_UNSPECIFIED, true, IStorage::TYPE_SAVE));
	EXPECT_EQ(Value, 175);
	Console->ExecuteLine("ax_custom_aspect_ratio_num 192", IConsole::CLIENT_ID_UNSPECIFIED);
	EXPECT_EQ(Value, 192);
}

TEST(AxionTextures, BoundsHdSkinsAndPreservesSpriteGrid)
{
	EXPECT_EQ(AxionBoundImage(8192, 4096, 512, 256, 8, 4), std::make_pair(512, 256));
	EXPECT_EQ(AxionBoundImage(1032, 516, 512, 256, 8, 4), std::make_pair(512, 256));
	EXPECT_EQ(AxionBoundImage(256, 128, 512, 256, 8, 4), std::make_pair(256, 128));
	EXPECT_EQ(AxionBoundImage(4096, 2048, 256, 256), std::make_pair(256, 128));
}

TEST(AxionTimer, RaceAndCountdownFollowGameTime)
{
	CNetObj_GameInfo Info{};
	Info.m_RoundStartTick = 500;
	EXPECT_EQ(AxionGameTimer(&Info, 3500, 50).m_Seconds, 60);
	Info.m_GameStateFlags = GAMESTATEFLAG_RACETIME;
	Info.m_WarmupTimer = -1000;
	EXPECT_EQ(AxionGameTimer(&Info, 3500, 50).m_Seconds, 50);
	Info.m_GameStateFlags = 0;
	Info.m_WarmupTimer = 0;
	Info.m_TimeLimit = 2;
	const auto Countdown = AxionGameTimer(&Info, 6250, 50);
	EXPECT_EQ(Countdown.m_Seconds, 5);
	EXPECT_TRUE(Countdown.m_Warning);
	EXPECT_TRUE(Countdown.m_Urgent);
	Info.m_GameStateFlags = GAMESTATEFLAG_GAMEOVER;
	EXPECT_EQ(AxionGameTimer(&Info, 6250, 50).m_Seconds, 0);
	EXPECT_FALSE(AxionGameTimer(nullptr, 0, 50).m_Valid);
}

#include <engine/shared/axion_background_presets.h>
#include <game/client/components/axion_background.h>

TEST(AxionBackground, PresetsRoundTripAndExplicitMutations)
{
 CTestInfo Info; Info.m_DeleteTestStorageFilesOnSuccess=true;
 auto Storage=Info.CreateTestStorage(); ASSERT_NE(Storage,nullptr);
 CAxionBackgroundPresets Presets; Presets.Load(Storage.get());
 EXPECT_TRUE(Presets.Presets().empty());
 ASSERT_TRUE(Presets.Create(Storage.get(),"My gradient",{1,128,255,2}));
 EXPECT_FALSE(Presets.Create(Storage.get(),"MY GRADIENT",{0,128,128,0}));
 CAxionBackgroundPresets Reload; Reload.Load(Storage.get());
 ASSERT_EQ(Reload.Presets().size(),1u);
 EXPECT_EQ(Reload.Presets()[0].m_Background.m_Direction,2);
 ASSERT_TRUE(Reload.Update(Storage.get(),0,{1,123,456,1}));
 ASSERT_TRUE(Reload.Rename(Storage.get(),0,"Renamed"));
 Presets.Load(Storage.get()); EXPECT_EQ(Presets.Find("renamed"),0);
 EXPECT_EQ(Presets.Presets()[0].m_Background.m_Secondary,456u);
 EXPECT_FALSE(Presets.Create(Storage.get(),"   ",{0,128,128,0}));
 EXPECT_FALSE(Presets.Create(Storage.get(),"Invalid",{1,0x1000000,128,0}));
 ASSERT_TRUE(Presets.Delete(Storage.get(),0)); Reload.Load(Storage.get());
 EXPECT_TRUE(Reload.Presets().empty());
}
TEST(AxionBackground, MalformedFileCannotBeOverwritten)
{
 CTestInfo Info; Info.m_DeleteTestStorageFilesOnSuccess=true;
 auto Storage=Info.CreateTestStorage(); ASSERT_NE(Storage,nullptr);
 auto File=Storage->OpenFile(CAxionBackgroundPresets::FILE_NAME,IOFLAG_WRITE,IStorage::TYPE_SAVE);
 ASSERT_NE(File,nullptr); io_write(File,"broken",6); io_close(File);
 CAxionBackgroundPresets Presets; Presets.Load(Storage.get());
 ASSERT_TRUE(Presets.ReadOnly()); EXPECT_FALSE(Presets.Create(Storage.get(),"Safe",{0,128,128,0}));
 File=Storage->OpenFile(CAxionBackgroundPresets::FILE_NAME,IOFLAG_READ,IStorage::TYPE_SAVE);
 ASSERT_NE(File,nullptr); EXPECT_EQ(io_length(File),6); io_close(File);
}
TEST(AxionBackground, LegacyGradientUsesPrimaryColor)
{
 const auto Saved = g_Config.m_ClBackgroundEntitiesColor;
 AxionApplyBackground({1, 128, 255, 2});
 const auto Background = AxionCurrentBackground();
 EXPECT_EQ(Background.m_Mode, 0);
 EXPECT_EQ(Background.m_Primary, 128u);
 g_Config.m_ClBackgroundEntitiesColor = Saved;
}

TEST(AxionBackground, InvalidEntriesAndDuplicateNamesPreserveOriginal)
{
 CTestInfo Info; Info.m_DeleteTestStorageFilesOnSuccess=true;
 auto Storage=Info.CreateTestStorage(); ASSERT_NE(Storage,nullptr);
 const char Data[]=R"({"version":1,"presets":[42,{"name":"Valid","mode":1,"primary":128,"secondary":255,"direction":0},{"name":"VALID","mode":0,"primary":128,"secondary":128,"direction":0},{"name":"Bad","mode":1,"primary":-1,"secondary":128,"direction":0}]})";
 auto File=Storage->OpenFile(CAxionBackgroundPresets::FILE_NAME,IOFLAG_WRITE,IStorage::TYPE_SAVE);
 ASSERT_NE(File,nullptr); io_write(File,Data,sizeof(Data)-1); io_close(File);
 CAxionBackgroundPresets Presets; Presets.Load(Storage.get());
 ASSERT_EQ(Presets.Presets().size(),1u); EXPECT_TRUE(Presets.ReadOnly());
 EXPECT_FALSE(Presets.Delete(Storage.get(),0));
 File=Storage->OpenFile(CAxionBackgroundPresets::FILE_NAME,IOFLAG_READ,IStorage::TYPE_SAVE);
 ASSERT_NE(File,nullptr); EXPECT_EQ(io_length(File),sizeof(Data)-1); io_close(File);
}

#include <engine/shared/axion_chat_reply.h>
TEST(AxionChat, ReplyKeepsUnicodeDraftAndDoesNotDuplicatePrefix)
{
 const auto Reply=AxionChatReply("Игрок","привет 🌍","",512);
 ASSERT_TRUE(Reply); EXPECT_EQ(*Reply,"Игрок: привет 🌍");
 EXPECT_EQ(AxionChatReply("Игрок",*Reply,"Игрок: ",512),Reply);
 const auto Changed=AxionChatReply("Друг",*Reply,"Игрок: ",512);
 ASSERT_TRUE(Changed); EXPECT_EQ(*Changed,"Друг: привет 🌍");
}
TEST(AxionChat, ReplyDoesNotSilentlyDiscardFullOrEditedDraft)
{
 EXPECT_FALSE(AxionChatReply("Player","A full draft","",12));
 const auto Edited=AxionChatReply("New","edited: draft","Old: ",512);
 ASSERT_TRUE(Edited); EXPECT_EQ(*Edited,"New: edited: draft");
}
