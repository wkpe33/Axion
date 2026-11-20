#include <base/dbg.h>
#include <base/mem.h>
#include <base/str.h>
#include <base/time.h>

#include <engine/client.h>
#include <engine/discord.h>

#if defined(CONF_DISCORD)
#include <discord_game_sdk.h>

typedef enum EDiscordResult(DISCORD_API *FDiscordCreate)(DiscordVersion, struct DiscordCreateParams *, struct IDiscordCore **);

#if defined(CONF_DISCORD_DYNAMIC)
#include <dlfcn.h>
static FDiscordCreate GetDiscordCreate()
{
	void *pSdk = dlopen("discord_game_sdk.so", RTLD_NOW);
	if(!pSdk)
	{
		return nullptr;
	}
	return (FDiscordCreate)dlsym(pSdk, "DiscordCreate");
}
#else
static FDiscordCreate GetDiscordCreate()
{
	return DiscordCreate;
}
#endif

class CDiscord : public IDiscord
{
	DiscordActivity m_Activity;
	bool m_UpdateActivity = false;
	int64_t m_LastActivityUpdate = 0;

	IDiscordCore *m_pCore = nullptr;
	IDiscordActivityManager *m_pActivityManager;

	FDiscordCreate m_pfnDiscordCreate;
	bool m_Enabled;

public:
	bool Init(FDiscordCreate pfnDiscordCreate)
	{
		m_pfnDiscordCreate = pfnDiscordCreate;
		m_Enabled = false;
		ClearGameInfo();
		return false;
	}
	bool InitDiscord()
	{
		if(m_pCore)
		{
			m_pCore->destroy(m_pCore);
			m_pCore = nullptr;
			m_pActivityManager = nullptr;
		}
		if(!m_Enabled)
			return false;

		DiscordCreateParams Params;
		DiscordCreateParamsSetDefault(&Params);
		Params.client_id = 1464646068699660451;
		Params.flags = EDiscordCreateFlags::DiscordCreateFlags_NoRequireDiscord;

		int Error = m_pfnDiscordCreate(DISCORD_VERSION, &Params, &m_pCore);

		if(Error != DiscordResult_Ok)
		{
			dbg_msg("discord", "error initializing discord instance, error=%d", Error);
			return true;
		}

		m_pActivityManager = m_pCore->get_activity_manager(m_pCore);

		m_UpdateActivity = true;
		return false;
	}

	void Update(bool Enabled) override
	{
		bool NeedsUpdate = m_Enabled != Enabled;
		m_Enabled = Enabled;

		if(NeedsUpdate)
			InitDiscord();

		if(m_pCore && m_Enabled)
		{
			// update every 5 seconds, rate limit is 5 updates per 20 seconds
			if(m_UpdateActivity && time_get() > m_LastActivityUpdate + time_freq() * 5)
			{
				m_UpdateActivity = false;
				m_LastActivityUpdate = time_get();

				m_pActivityManager->update_activity(m_pActivityManager, &m_Activity, nullptr, nullptr);
			}

			m_pCore->run_callbacks(m_pCore);
		}
	}

	void ClearGameInfo() override
	{
		mem_zero(&m_Activity, sizeof(DiscordActivity));

		str_copy(m_Activity.assets.large_image, "axion");
		str_copy(m_Activity.assets.large_text, "Axion");
		m_Activity.timestamps.start = time_timestamp();
		str_copy(m_Activity.details, "In the main menu");
		m_Activity.instance = false;

		m_UpdateActivity = true;
	}

	void SetGameInfo(const CServerInfo &ServerInfo, bool) override
	{
		mem_zero(&m_Activity, sizeof(DiscordActivity));

		str_copy(m_Activity.assets.large_image, "axion");
		str_copy(m_Activity.assets.large_text, "Axion");
		m_Activity.timestamps.start = time_timestamp();
		str_copy(m_Activity.name, "Online");
		m_Activity.instance = true;

		str_copy(m_Activity.details, "Playing Axion");
		str_copy(m_Activity.state, ServerInfo.m_aMap);

		m_UpdateActivity = true;
	}

	void UpdateServerInfo(const CServerInfo &ServerInfo) override
	{
		if(!m_Activity.instance)
			return;

		if(str_comp(m_Activity.state, ServerInfo.m_aMap) == 0)
			return;
		str_copy(m_Activity.state, ServerInfo.m_aMap);
		m_UpdateActivity = true;
	}

	void UpdatePlayerCount(int) override {}

	~CDiscord() override
	{
		if(m_pCore)
			m_pCore->destroy(m_pCore);
	}
};

static IDiscord *CreateDiscordImpl()
{
	FDiscordCreate pfnDiscordCreate = GetDiscordCreate();
	if(!pfnDiscordCreate)
	{
		return nullptr;
	}
	CDiscord *pDiscord = new CDiscord();
	if(pDiscord->Init(pfnDiscordCreate))
	{
		delete pDiscord;
		return nullptr;
	}
	return pDiscord;
}
#else
static IDiscord *CreateDiscordImpl()
{
	return nullptr;
}
#endif

class CDiscordStub : public IDiscord
{
	void Update(bool Enabled) override {}
	void ClearGameInfo() override {}
	void SetGameInfo(const CServerInfo &ServerInfo, bool Registered) override {}
	void UpdateServerInfo(const CServerInfo &ServerInfo) override {}
	void UpdatePlayerCount(int Count) override {}
};

IDiscord *CreateDiscord()
{
	IDiscord *pDiscord = CreateDiscordImpl();
	if(pDiscord)
	{
		return pDiscord;
	}
	return new CDiscordStub();
}
