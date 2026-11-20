#include <engine/shared/axion_invitation.h>

#include <game/client/components/axion_theme.h>

#include <gtest/gtest.h>

TEST(AxionInvitation, OnlyExactStockServerMessageWithMatchingTeamIsAccepted)
{
	SAxionInvitation Invite;
	ASSERT_TRUE(AxionParseInvitation("'wkpe' invited you to team 44. Use /team 44 to join.", Invite, 64));
	EXPECT_EQ(Invite.m_Name, "wkpe");
	EXPECT_EQ(Invite.m_Team, 44);
	EXPECT_FALSE(AxionParseInvitation("wkpe invited you to team 44", Invite, 64));
	EXPECT_FALSE(AxionParseInvitation("'wkpe' invited you to team 44. Use /team 45 to join.", Invite, 64));
	EXPECT_FALSE(AxionParseInvitation("'wkpe' invited you to team 0. Use /team 0 to join.", Invite, 64));
	EXPECT_FALSE(AxionParseInvitation("'wkpe' invited you to team 64. Use /team 64 to join.", Invite, 64));
	EXPECT_FALSE(AxionParseInvitation("'wkpe' invited you to team 999999999999999999999999. Use /team 44 to join.", Invite, 64));
	EXPECT_FALSE(AxionParseInvitation("'wkpe' invited you to team 44. Use /team 44 to join. extra", Invite, 64));
	ASSERT_TRUE(AxionParseInvitation("'a'b' invited you to team 2. Use /team 2 to join.", Invite, 64));
	EXPECT_EQ(Invite.m_Name, "a'b");
}

TEST(AxionTheme, PaletteContrastAndReducedMotion)
{
	const CConfig Saved = g_Config;
	const auto Dark = AxionThemePalette(0);
	const auto Light = AxionThemePalette(1);
	EXPECT_LT(Dark.m_Background.r, 0.1f);
	EXPECT_GT(Dark.m_Text.r, 0.9f);
	EXPECT_GT(Light.m_Background.r, 0.9f);
	EXPECT_LT(Light.m_Text.r, 0.2f);
	g_Config.m_AxReduceMotion = 0;
	const float Full = AxionTransition(0.02f);
	const float Half = AxionTransition(0.01f);
	EXPECT_NEAR(Full, 1 - (1 - Half) * (1 - Half), 0.00001f);
	g_Config.m_AxReduceMotion = 1;
	EXPECT_FLOAT_EQ(AxionTransition(0.001f), 1);
	g_Config = Saved;
}
