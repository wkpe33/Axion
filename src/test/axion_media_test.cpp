#include <game/client/components/axion_media.h>
#include <gtest/gtest.h>
#include <chrono>
#include <thread>

TEST(AxionMedia, PollingAndDisableDoNotBlockTheRenderThread)
{
	CAxionMedia Media;
	// Exercise the real Windows provider without changing playback or volume.
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	const auto Start = std::chrono::steady_clock::now();
	for(int i = 0; i < 1000; ++i)
	{
		const auto Snapshot = Media.Snapshot();
		EXPECT_GE(Snapshot.m_PositionMs, 0);
		EXPECT_GE(Snapshot.m_DurationMs, 0);
		EXPECT_LE(Snapshot.m_Volume, 1.0f);
	}
	Media.SetEnabled(false);
	EXPECT_LT(std::chrono::steady_clock::now() - Start, std::chrono::seconds(1));
	Media.SetEnabled(true);
}
