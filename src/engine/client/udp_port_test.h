#ifndef ENGINE_CLIENT_UDP_PORT_TEST_H
#define ENGINE_CLIENT_UDP_PORT_TEST_H

#include <base/net.h>

#include <array>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>
#include <vector>

class IConfigManager;

class CUdpPortTest
{
public:
	static constexpr int NUM_PROBES = 6;
	static constexpr int MAX_PORTS = 6;
	static constexpr int SEARCH_PORTS = 64;

	struct SResult
	{
		int m_Port = 0;
		int m_Sent = 0;
		int m_Received = 0;
		bool m_BindFailed = false;
		std::array<double, NUM_PROBES> m_aTimes{};
		double Median() const;
		double Spread() const;
	};

	~CUdpPortTest();
	bool Start(const NETADDR &Server, const NETADDR &BindAddress, const char *pPorts);
	void Update();
	void Cancel();
	void StopAndWait();
	bool Searching() const { return m_State.m_Searching; }
	int SearchProgress() const { return m_State.m_SearchProgress; }
	bool Running() const { return m_State.m_Running; }
	const NETADDR &Server() const { return m_State.m_Server; }
	const std::vector<SResult> &Results() const { return m_State.m_vResults; }
	int BestPort() const;
	int PreferredPort(const NETADDR &Server) const;
	void SetPreferredPort(const NETADDR &Server, int Port);
	static void Save(IConfigManager *pConfigManager, void *pUser);

private:
	struct SSnapshot
	{
		NETADDR m_Server{};
		std::vector<SResult> m_vResults;
		int m_SearchProgress = 0;
		bool m_Searching = false;
		bool m_Running = false;
		bool m_Complete = false;
	};
	struct SProbe;

	SSnapshot m_State;
	std::mutex m_Mutex;
	std::optional<SSnapshot> m_PendingSnapshot;
	std::jthread m_Worker;
	std::vector<std::pair<NETADDR, int>> m_vPreferences;
};

#endif
