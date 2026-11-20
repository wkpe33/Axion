#include "udp_port_test.h"

#include <base/log.h>
#include <base/mem.h>
#include <base/secure.h>
#include <base/str.h>
#include <base/time.h>

#include <engine/config.h>
#include <engine/shared/config.h>
#include <engine/shared/masterserver.h>
#include <engine/shared/network.h>
#include <engine/shared/packer.h>
#include <engine/shared/protocol.h>

#include <algorithm>
#include <charconv>
#include <cstring>
#include <string_view>

using namespace std::chrono_literals;

double CUdpPortTest::SResult::Median() const
{
	if(!m_Received)
		return 0;
	auto aTimes = m_aTimes;
	std::sort(aTimes.begin(), aTimes.begin() + m_Received);
	return (aTimes[(m_Received - 1) / 2] + aTimes[m_Received / 2]) / 2;
}

double CUdpPortTest::SResult::Spread() const
{
	if(m_Received < 2)
		return 0;
	const auto [Min, Max] = std::minmax_element(m_aTimes.begin(), m_aTimes.begin() + m_Received);
	return *Max - *Min;
}

struct CUdpPortTest::SProbe : SSnapshot
{
	NETADDR m_BindAddress{};
	std::array<NETSOCKET, SEARCH_PORTS> m_aSockets{};
	std::chrono::nanoseconds m_NextSend{};
	std::chrono::nanoseconds m_SentAt{};
	size_t m_Next = 0;
	size_t m_Pending = 0;
	int m_Token = 0;
	bool m_Waiting = false;

	~SProbe() { Cancel(); }
	void Begin(std::vector<SResult> vResults, bool Search);
	void Cancel();
	void Step();
};

void CUdpPortTest::SProbe::Begin(std::vector<SResult> vResults, bool Search)
{
	m_Searching = Search;
	m_vResults = std::move(vResults);
	m_Next = 0;
	m_NextSend = time_get_nanoseconds();
	m_Running = true;
	m_Complete = false;
	for(size_t Index = 0; !Search && Index < m_vResults.size(); ++Index)
	{
		NETADDR Bind = m_BindAddress;
		Bind.type = m_Server.type;
		Bind.port = m_vResults[Index].m_Port;
		m_aSockets[Index] = net_udp_create(Bind);
		m_vResults[Index].m_BindFailed = !m_aSockets[Index];
	}
}

void CUdpPortTest::SProbe::Cancel()
{
	for(NETSOCKET &Socket : m_aSockets)
	{
		if(Socket)
			net_udp_close(Socket);
		Socket = nullptr;
	}
	m_Running = false;
	m_Searching = false;
	m_Waiting = false;
}

void CUdpPortTest::SProbe::Step()
{
	if(!m_Running)
		return;
	const auto Now = time_get_nanoseconds();
	if(m_Waiting)
	{
		for(int Count = 0; Count < 8; ++Count)
		{
			NETADDR From;
			unsigned char *pData;
			const int Size = net_udp_recv(m_aSockets[m_Pending], &From, &pData);
			if(Size <= 0)
				break;
			if(From != m_Server)
				continue;
			// Inspect the connectionless payload without writing to the gameplay packet log.
			static constexpr int HEADER_SIZE = 6;
			const auto Flags = CNetBase::UnpackPacketFlags(pData, Size);
			if(!Flags || !(*Flags & NET_PACKETFLAG_CONNLESS) || (pData[0] & 3) == 1 ||
				Size <= HEADER_SIZE + SERVERBROWSE_SIZE)
				continue;
			const unsigned char *pPayload = pData + HEADER_SIZE;
			const bool Extended = mem_comp(pPayload, SERVERBROWSE_INFO_EXTENDED, SERVERBROWSE_SIZE) == 0;
			if(!Extended && mem_comp(pPayload, SERVERBROWSE_INFO, SERVERBROWSE_SIZE) != 0)
				continue;
			CUnpacker Unpacker;
			Unpacker.Reset(pPayload + SERVERBROWSE_SIZE, Size - HEADER_SIZE - SERVERBROWSE_SIZE);
			const char *pToken = Unpacker.GetString();
			int Token = -1;
			const char *pEnd = pToken + std::strlen(pToken);
			const auto Parsed = std::from_chars(pToken, pEnd, Token);
			if(Unpacker.Error() || Parsed.ec != std::errc{} || Parsed.ptr != pEnd || Token != (Extended ? m_Token : m_Token & 0xff) || Now - m_SentAt > 500ms)
				continue;
			SResult &Result = m_vResults[m_Pending];
			Result.m_aTimes[Result.m_Received++] = std::chrono::duration<double, std::milli>(Now - m_SentAt).count();
			m_Waiting = false;
			break;
		}
		if(Now >= m_SentAt + 500ms)
			m_Waiting = false;
	}
	if(m_Searching && !m_Waiting && m_aSockets[m_Pending])
	{
		net_udp_close(m_aSockets[m_Pending]);
		m_aSockets[m_Pending] = nullptr;
	}
	if(m_Waiting || Now < m_NextSend)
		return;

	for(size_t Offset = 0; Offset < m_vResults.size(); ++Offset)
	{
		const size_t Index = (m_Next + Offset) % m_vResults.size();
		SResult &Result = m_vResults[Index];
		if(Result.m_BindFailed || Result.m_Sent == (m_Searching ? 1 : NUM_PROBES))
			continue;
		if(m_Searching)
		{
			++m_SearchProgress;
			NETADDR Bind = m_BindAddress;
			Bind.type = m_Server.type;
			Bind.port = Result.m_Port;
			m_aSockets[Index] = net_udp_create(Bind);
			if(!m_aSockets[Index])
			{
				Result.m_BindFailed = true;
				continue;
			}
		}
		unsigned Token;
		secure_random_fill(&Token, sizeof(Token));
		m_Token = Token & 0xffffff;
		unsigned char aData[sizeof(SERVERBROWSE_GETINFO) + 1];
		mem_copy(aData, SERVERBROWSE_GETINFO, sizeof(SERVERBROWSE_GETINFO));
		aData[sizeof(SERVERBROWSE_GETINFO)] = m_Token & 0xff;
		unsigned char aExtra[NET_CONNLESS_EXTRA_SIZE] = {
			static_cast<unsigned char>(m_Token >> 16), static_cast<unsigned char>(m_Token >> 8)};
		m_SentAt = time_get_nanoseconds();
		CNetBase::SendPacketConnless(m_aSockets[Index], &m_Server, aData, sizeof(aData), true, aExtra);
		++Result.m_Sent;
		m_Pending = Index;
		m_Next = (Index + 1) % m_vResults.size();
		m_NextSend = m_SentAt + 550ms;
		m_Waiting = true;
		return;
	}
	if(m_Searching)
	{
		std::sort(m_vResults.begin(), m_vResults.end(), [](const SResult &Left, const SResult &Right) {
			if(Left.m_Received != Right.m_Received)
				return Left.m_Received > Right.m_Received;
			return Left.Median() < Right.Median();
		});
		std::vector<SResult> vFinalists;
		for(size_t Index = 0; Index < 3 && m_vResults[Index].m_Received; ++Index)
			vFinalists.push_back({.m_Port = m_vResults[Index].m_Port});
		Cancel();
		m_vResults.clear();
		if(!vFinalists.empty())
			Begin(std::move(vFinalists), false);
		else
			m_Complete = true;
		return;
	}
	Cancel();
	m_Complete = true;
}

CUdpPortTest::~CUdpPortTest()
{
	StopAndWait();
}

bool CUdpPortTest::Start(const NETADDR &Server, const NETADDR &BindAddress, const char *pPorts)
{
	if(Running() || !Server.port || (Server.type != NETTYPE_IPV4 && Server.type != NETTYPE_IPV6))
		return false;

	std::vector<SResult> vResults;
	std::string_view Ports(pPorts);
	const bool Search = Ports == "search";
	if(Search)
		Ports = {};
	while(!Ports.empty())
	{
		const size_t Separator = Ports.find_first_of(", \t");
		const auto Part = Ports.substr(0, Separator);
		if(!Part.empty())
		{
			int Port = 0;
			const auto [End, Error] = std::from_chars(Part.data(), Part.data() + Part.size(), Port);
			if(Error != std::errc{} || End != Part.data() + Part.size() || Port < 0 || Port > 65535)
				return false;
			if(std::none_of(vResults.begin(), vResults.end(), [Port](const SResult &Result) { return Result.m_Port == Port; }))
				vResults.push_back({.m_Port = Port});
			if(vResults.size() > MAX_PORTS)
				return false;
		}
		if(Separator == Ports.npos)
			break;
		Ports.remove_prefix(Separator + 1);
	}
	if(!Search && vResults.empty())
		return false;

	// A finished worker has already closed its sockets before publishing its result.
	if(m_Worker.joinable())
		m_Worker.join();

	m_State = {.m_Server = Server, .m_vResults = vResults, .m_Searching = Search, .m_Running = true};
	m_Worker = std::jthread([this, Server, BindAddress, Search, vResults = std::move(vResults)](std::stop_token Stop) mutable {
		SProbe Probe;
		Probe.m_Server = Server;
		Probe.m_BindAddress = BindAddress;
		if(Search)
		{
			const int First = 32768 + secure_rand_below(32768 - SEARCH_PORTS + 1);
			for(int Port = First; Port < First + SEARCH_PORTS; ++Port)
				vResults.push_back({.m_Port = Port});
		}
		if(!Stop.stop_requested())
			Probe.Begin(std::move(vResults), Search);

		auto NextPublish = time_get_nanoseconds();
		while(Probe.m_Running && !Stop.stop_requested())
		{
			Probe.Step();
			const auto Now = time_get_nanoseconds();
			if(Probe.m_Running && Now >= NextPublish)
			{
				const std::lock_guard Lock(m_Mutex);
				m_PendingSnapshot = static_cast<const SSnapshot &>(Probe);
				NextPublish = Now + 40ms;
			}
			if(Probe.m_Waiting)
				net_socket_read_wait(Probe.m_aSockets[Probe.m_Pending], 4ms);
			else if(Probe.m_Running)
				std::this_thread::sleep_for(4ms);
		}
		Probe.Cancel();
		if(Stop.stop_requested())
			Probe.m_Complete = false;
		const std::lock_guard Lock(m_Mutex);
		m_PendingSnapshot = std::move(static_cast<SSnapshot &>(Probe));
	});
	return true;
}

void CUdpPortTest::Cancel()
{
	m_Worker.request_stop();
}

void CUdpPortTest::StopAndWait()
{
	Cancel();
	if(m_Worker.joinable())
		m_Worker.join();
	const std::lock_guard Lock(m_Mutex);
	m_PendingSnapshot.reset();
	m_State.m_Running = false;
	m_State.m_Searching = false;
	m_State.m_Complete = false;
}

void CUdpPortTest::Update()
{
	{
		const std::unique_lock Lock(m_Mutex, std::try_to_lock);
		if(!Lock || !m_PendingSnapshot)
			return;
		m_State = std::move(*m_PendingSnapshot);
		m_PendingSnapshot.reset();
	}
	if(m_Worker.get_stop_token().stop_requested())
		m_State.m_Complete = false;
	if(!m_State.m_Complete)
		return;
	const int Best = BestPort();
	for(const auto &Result : m_State.m_vResults)
		log_info("udp-test", "port=%d replies=%d/%d median=%.2fms spread=%.2fms bind_failed=%d", Result.m_Port, Result.m_Received, Result.m_Sent, Result.Median(), Result.Spread(), Result.m_BindFailed);
	if(!g_Config.m_AxionUdpAuto || !Best)
		return;

	const auto &Winner = *std::find_if(m_State.m_vResults.begin(), m_State.m_vResults.end(), [Best](const SResult &Result) { return Result.m_Port == Best; });
	for(const auto &Other : m_State.m_vResults)
	{
		if(Other.m_Port == Best || Other.m_Received < Winner.m_Received - 1)
			continue;
		if(Other.Median() - Winner.Median() < std::max(2.0, (Winner.Spread() + Other.Spread()) / 2.0))
			return;
	}
	SetPreferredPort(m_State.m_Server, Best);
}

int CUdpPortTest::BestPort() const
{
	if(!m_State.m_Complete)
		return 0;
	const SResult *pBest = nullptr;
	for(const auto &Result : m_State.m_vResults)
	{
		if(Result.m_Sent != NUM_PROBES || Result.m_Received < 4)
			continue;
		if(!pBest || Result.m_Received > pBest->m_Received ||
			(Result.m_Received == pBest->m_Received && Result.Median() < pBest->Median()))
			pBest = &Result;
	}
	return pBest ? pBest->m_Port : 0;
}

int CUdpPortTest::PreferredPort(const NETADDR &Server) const
{
	const auto It = std::find_if(m_vPreferences.begin(), m_vPreferences.end(), [&Server](const auto &Entry) { return Entry.first == Server; });
	return It == m_vPreferences.end() ? 0 : It->second;
}

void CUdpPortTest::SetPreferredPort(const NETADDR &Server, int Port)
{
	if(!Server.port || (Server.type != NETTYPE_IPV4 && Server.type != NETTYPE_IPV6) || (Port < 0 || Port > 65535))
		return;
	const auto It = std::find_if(m_vPreferences.begin(), m_vPreferences.end(), [&Server](const auto &Entry) { return Entry.first == Server; });
	if(It != m_vPreferences.end())
	{
		if(Port == 0)
			m_vPreferences.erase(It);
		else
			It->second = Port;
	}
	else if(Port != 0)
	{
		if(m_vPreferences.size() == 256)
			m_vPreferences.erase(m_vPreferences.begin());
		m_vPreferences.emplace_back(Server, Port);
	}
}

void CUdpPortTest::Save(IConfigManager *pConfigManager, void *pUser)
{
	for(const auto &[Server, Port] : static_cast<CUdpPortTest *>(pUser)->m_vPreferences)
	{
		char aAddress[NETADDR_MAXSTRSIZE];
		net_addr_str(&Server, aAddress, sizeof(aAddress), true);
		char aLine[128];
		str_format(aLine, sizeof(aLine), "axion_udp_prefer %s %d", aAddress, Port);
		pConfigManager->WriteLine(aLine);
	}
}
