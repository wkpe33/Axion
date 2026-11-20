#include <base/mem.h>
#include <base/net.h>
#include <base/secure.h>
#include <base/str.h>

#include <engine/client/udp_port_test.h>
#include <engine/shared/masterserver.h>
#include <engine/shared/network.h>

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <thread>

using namespace std::chrono_literals;

TEST(UdpPortTest, ProbesWhileClientIsNotUpdating)
{
	NETADDR Server{};
	ASSERT_EQ(net_addr_from_str(&Server, "127.0.0.1"), 0);
	NETSOCKET Socket = nullptr;
	for(int Attempt = 0; Attempt < 20 && !Socket; ++Attempt)
	{
		Server.port = 40000 + secure_rand_below(20000);
		Socket = net_udp_create(Server);
	}
	ASSERT_NE(Socket, nullptr);
	const std::unique_ptr<std::remove_pointer_t<NETSOCKET>, decltype(&net_udp_close)> pServer(Socket, net_udp_close);

	CUdpPortTest Test;
	ASSERT_TRUE(Test.Start(Server, {}, "0"));
	for(int Probe = 0; Probe < CUdpPortTest::NUM_PROBES; ++Probe)
	{
		ASSERT_EQ(net_socket_read_wait(Socket, 3s), 1);
		NETADDR From;
		unsigned char *pPacket;
		const int Size = net_udp_recv(Socket, &From, &pPacket);
		CNetPacketConstruct Request{};
		bool Sixup = false;
		ASSERT_GT(Size, 0);
		ASSERT_EQ(CNetBase::UnpackPacket(pPacket, Size, &Request, Sixup, false), 0);
		ASSERT_EQ(Request.m_DataSize, SERVERBROWSE_SIZE + 1);
		ASSERT_EQ(mem_comp(Request.m_aChunkData, SERVERBROWSE_GETINFO, SERVERBROWSE_SIZE), 0);
		char aReply[SERVERBROWSE_SIZE + 4];
		mem_copy(aReply, SERVERBROWSE_INFO, SERVERBROWSE_SIZE);
		str_format(aReply + SERVERBROWSE_SIZE, 4, "%u", static_cast<unsigned>(Request.m_aChunkData[SERVERBROWSE_SIZE]));
		CNetBase::SendPacketConnless(Socket, &From, aReply, SERVERBROWSE_SIZE + str_length(aReply + SERVERBROWSE_SIZE) + 1, false, nullptr);
	}

	const auto Deadline = std::chrono::steady_clock::now() + 3s;
	while(Test.Running() && std::chrono::steady_clock::now() < Deadline)
	{
		Test.Update();
		std::this_thread::sleep_for(5ms);
	}
	ASSERT_FALSE(Test.Running());
	ASSERT_EQ(Test.Results().size(), 1u);
	EXPECT_EQ(Test.Results()[0].m_Received, CUdpPortTest::NUM_PROBES);
}

TEST(UdpPortTest, CancelsSearchAndCanRestart)
{
	NETADDR Server{};
	ASSERT_EQ(net_addr_from_str(&Server, "127.0.0.1:9"), 0);
	CUdpPortTest Test;
	ASSERT_TRUE(Test.Start(Server, {}, "search"));
	Test.Cancel();
	const auto Deadline = std::chrono::steady_clock::now() + 3s;
	while(Test.Running() && std::chrono::steady_clock::now() < Deadline)
	{
		Test.Update();
		std::this_thread::sleep_for(5ms);
	}
	ASSERT_FALSE(Test.Running());
	EXPECT_EQ(Test.BestPort(), 0);
	ASSERT_TRUE(Test.Start(Server, {}, "0,40000,45000,50000"));
	Test.StopAndWait();
	EXPECT_FALSE(Test.Running());
}
