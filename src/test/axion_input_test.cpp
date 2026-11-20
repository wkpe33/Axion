#include <engine/shared/axion_input.h>

#include <gtest/gtest.h>

class CAxionInputTest : public testing::Test
{
	CConfig m_Config = g_Config;

protected:
	void TearDown() override { g_Config = m_Config; }
};

TEST_F(CAxionInputTest, UsesModeSpecificSettings)
{
	g_Config.m_AxionInputMode = AxionInput::TCLIENT;
	g_Config.m_TcFastInput = 1;
	g_Config.m_TcFastInputAmount = 15;
	EXPECT_FLOAT_EQ(AxionInput::Offset(), 0.75f);

	g_Config.m_AxionInputMode = AxionInput::BEST;
	g_Config.m_BcBestInputAmount = 200;
	g_Config.m_BcBestInputSmoothing = 50;
	g_Config.m_BcBestInputLatencyComp = 20;
	g_Config.m_BcBestInputOthers = 0;
	EXPECT_FLOAT_EQ(AxionInput::Offset(), 1.8f);
	EXPECT_FLOAT_EQ(AxionInput::OthersOffset(true), 0.0f);
	g_Config.m_BcBestInputOthers = 1;
	EXPECT_FLOAT_EQ(AxionInput::OthersOffset(false), 1.8f);

	g_Config.m_AxionInputMode = AxionInput::SAIKO;
	g_Config.m_BcSaikoInputAmount = 175;
	g_Config.m_BcSaikoInputOthers = 1;
	EXPECT_FLOAT_EQ(AxionInput::OthersOffset(false), 1.75f);

	g_Config.m_AxionInputMode = AxionInput::DELTA;
	g_Config.m_BcDeltaInputAmount = 125;
	g_Config.m_BcDeltaInputOthers = 0;
	EXPECT_FLOAT_EQ(AxionInput::Offset(), 1.25f);
	EXPECT_FLOAT_EQ(AxionInput::OthersOffset(false), 0.0f);
}

TEST_F(CAxionInputTest, RequiresPreInputForCloudOthers)
{
	g_Config.m_AxionInputMode = AxionInput::CLOUD;
	g_Config.m_BcCloudInputAmount = 240;
	g_Config.m_BcCloudInputOthers = 1;
	EXPECT_TRUE(AxionInput::Cloud());
	EXPECT_FLOAT_EQ(AxionInput::Offset(), 2.4f);
	EXPECT_FLOAT_EQ(AxionInput::OthersOffset(false), 0.0f);
	EXPECT_FLOAT_EQ(AxionInput::OthersOffset(true), 1.0f);
	g_Config.m_BcCloudInputAmount = 0;
	EXPECT_FALSE(AxionInput::Enabled());
	EXPECT_FLOAT_EQ(AxionInput::OthersOffset(true), 0.0f);
}

TEST_F(CAxionInputTest, InterpolatesAndCarriesFractionalTicks)
{
	int Tick = 20;
	float Intra = 0.75f;
	AxionInput::ApplyOffset(1.5f, Tick, Intra);
	EXPECT_EQ(Tick, 22);
	EXPECT_FLOAT_EQ(Intra, 0.25f);

	g_Config.m_AxionInputMode = AxionInput::BEST;
	g_Config.m_BcBestInputAmount = 100;
	g_Config.m_BcBestInputInterpolation = 1;
	EXPECT_FLOAT_EQ(AxionInput::Interpolation(0.25f, 500.0f), 0.25f);
	g_Config.m_BcBestInputInterpolation = 2;
	EXPECT_FLOAT_EQ(AxionInput::Interpolation(0.25f, 500.0f), 0.15625f);
	g_Config.m_BcBestInputInterpolation = 3;
	EXPECT_FLOAT_EQ(AxionInput::Interpolation(0.25f, 500.0f), 0.203125f);
}
