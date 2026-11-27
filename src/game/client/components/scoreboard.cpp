/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "scoreboard.h"
#include "axion_hud_layout.h"

#include "axion_theme.h"

#include <base/net.h>
#include <engine/http.h>
#include <engine/external/json-parser/json.h>
#include <engine/storage.h>

#include <unordered_map>

#include <base/dbg.h>
#include <base/time.h>

#include <engine/console.h>
#include <engine/demo.h>
#include <engine/font_icons.h>
#include <engine/graphics.h>
#include <engine/shared/config.h>
#include <engine/textrender.h>

#include <generated/client_data7.h>
#include <generated/protocol.h>

#include <game/client/animstate.h>
#include <game/client/components/countryflags.h>
#include <game/client/components/motd.h>
#include <game/client/components/statboard.h>
#include <game/client/gameclient.h>
#include <game/client/ui.h>
#include <game/localization.h>

// Horizontal spacing of the scoreboard contents, both to its edges and between columns
static constexpr float MARGIN = 10.0f;

namespace
{
enum class EPlayerClient
{
	UNKNOWN,
	BEST_CLIENT,
	FAKE_BEST_CLIENT,
	NOVA,
};

struct CClientPresence
{
	const char *m_pUrl;
	EPlayerClient m_Type;
	std::shared_ptr<IHttpRequest> m_pRequest;
	std::unordered_map<std::string, EPlayerClient> m_Players;
};

CClientPresence s_aPresence[] = {
	{"https://150.241.70.188:8779/users.json", EPlayerClient::BEST_CLIENT},
	{"https://novaclient.137-131-212-78.sslip.io/users.json", EPlayerClient::NOVA},
};
NETADDR s_PresenceServer{};
int64_t s_NextPresenceRequest = 0;
IGraphics::CTextureHandle s_aClientIcons[3];

const char *JsonString(const json_value &Value)
{
	return Value.type == json_string ? Value.u.string.ptr : nullptr;
}

void ReadPresencePlayers(CClientPresence &Source, const json_value &Players)
{
	const auto Add = [&](const char *pName, const json_value &Player) {
		if(!pName || !*pName || str_length(pName) >= MAX_NAME_LENGTH)
			return;
		const auto &Fake = Player["fake"];
		const auto Type = Source.m_Type == EPlayerClient::BEST_CLIENT &&
			Fake.type == json_boolean && Fake.u.boolean ? EPlayerClient::FAKE_BEST_CLIENT : Source.m_Type;
		const auto Result = Source.m_Players.emplace(pName, Type);
		if(Type == EPlayerClient::FAKE_BEST_CLIENT)
			Result.first->second = Type;
	};
	if(Players.type == json_array)
	{
		for(unsigned i = 0; i < Players.u.array.length; ++i)
		{
			const auto &Player = *Players.u.array.values[i];
			Add(Player.type == json_string ? JsonString(Player) : JsonString(Player["name"]), Player);
		}
	}
	else if(Players.type == json_object)
	{
		if(const char *pName = JsonString(Players["name"]))
			Add(pName, Players);
		else
			for(unsigned i = 0; i < Players.u.object.length; ++i)
			{
				const auto &Entry = Players.u.object.values[i];
				const char *pName = JsonString(*Entry.value);
				if(Entry.value->type == json_object || Entry.value->type == json_string)
					Add(pName ? pName : Entry.name, *Entry.value);
			}
	}
}

void UpdateScoreboardPresence(IHttp *pHttp, NETADDR Server)
{
	Server.type &= ~NETTYPE_TW7;
	if(net_addr_comp(&Server, &s_PresenceServer) != 0)
	{
		for(auto &Source : s_aPresence)
		{
			if(Source.m_pRequest)
				Source.m_pRequest->Abort();
			Source.m_pRequest.reset();
			Source.m_Players.clear();
		}
		s_PresenceServer = Server;
		s_NextPresenceRequest = 0;
	}
	const auto Matches = [&](const char *pAddress) {
		NETADDR Address{};
		return pAddress && net_addr_from_str(&Address, pAddress) == 0 && net_addr_comp(&Address, &Server) == 0;
	};
	const auto Now = time_get();
	const bool Refresh = Now >= s_NextPresenceRequest;
	for(auto &Source : s_aPresence)
	{
		if(Source.m_pRequest && Source.m_pRequest->Done())
		{
			Source.m_Players.clear();
			if(Source.m_pRequest->State() == EHttpState::DONE)
			{
				if(json_value *pJson = Source.m_pRequest->ResultJson())
				{
					if(pJson->type == json_array)
					{
						for(unsigned i = 0; i < pJson->u.array.length; ++i)
						{
							const auto &Entry = *pJson->u.array.values[i];
							const char *pAddress = JsonString(Entry["server_address"]);
							if(!pAddress) pAddress = JsonString(Entry["address"]);
							if(!pAddress) pAddress = JsonString(Entry["server"]);
							if(Matches(pAddress))
								ReadPresencePlayers(Source, Entry["players"].type == json_none ? Entry : Entry["players"]);
						}
					}
					else if(pJson->type == json_object)
						for(unsigned i = 0; i < pJson->u.object.length; ++i)
						{
							const auto &Entry = pJson->u.object.values[i];
							if(Matches(Entry.name))
								ReadPresencePlayers(Source, *Entry.value);
						}
					json_value_free(pJson);
				}
			}
			Source.m_pRequest.reset();
		}
		if(Refresh && !Source.m_pRequest)
		{
			Source.m_pRequest = HttpGet(Source.m_pUrl);
			Source.m_pRequest->Timeout(CTimeout{5000, 10000, 100, 5});
			Source.m_pRequest->MaxResponseSize(1024 * 1024);
			Source.m_pRequest->LogProgress(HTTPLOG::FAILURE);
			pHttp->Run(Source.m_pRequest);
		}
	}
	if(Refresh)
		s_NextPresenceRequest = Now + 20 * time_freq();
}
}

CScoreboard::CScoreboard()
{
	CScoreboard::OnReset();
}

void CScoreboard::SetUiMousePos(vec2 Pos)
{
	const vec2 WindowSize = vec2(Graphics()->WindowWidth(), Graphics()->WindowHeight());
	const CUIRect *pScreen = Ui()->Screen();

	const vec2 UpdatedMousePos = Ui()->UpdatedMousePos();
	Pos = Pos / vec2(pScreen->w, pScreen->h) * WindowSize;
	Ui()->OnCursorMove(Pos.x - UpdatedMousePos.x, Pos.y - UpdatedMousePos.y);
}

void CScoreboard::LockMouse()
{
	Ui()->ClosePopupMenus();
	m_MouseUnlocked = false;
	SetUiMousePos(m_LastMousePos.value());
	m_LastMousePos = Ui()->MousePos();
}

void CScoreboard::ConKeyScoreboard(IConsole::IResult *pResult, void *pUserData)
{
	CScoreboard *pSelf = static_cast<CScoreboard *>(pUserData);

	pSelf->GameClient()->m_Spectator.OnRelease();
	pSelf->GameClient()->m_Emoticon.OnRelease();

	pSelf->m_Active = pResult->GetInteger(0) != 0;

	if(!pSelf->IsActive() && pSelf->m_MouseUnlocked)
	{
		pSelf->LockMouse();
	}
}

void CScoreboard::ConToggleScoreboardCursor(IConsole::IResult *pResult, void *pUserData)
{
	CScoreboard *pSelf = static_cast<CScoreboard *>(pUserData);

	if(!pSelf->IsActive() ||
		pSelf->GameClient()->m_Menus.IsActive() ||
		pSelf->GameClient()->m_Chat.IsActive() ||
		pSelf->Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		return;
	}

	pSelf->m_MouseUnlocked = !pSelf->m_MouseUnlocked;

	if(!pSelf->m_MouseUnlocked)
	{
		pSelf->Ui()->ClosePopupMenus();
	}

	vec2 OldMousePos = pSelf->Ui()->MousePos();

	if(pSelf->m_LastMousePos == std::nullopt)
	{
		pSelf->SetUiMousePos(pSelf->Ui()->Screen()->Center());
	}
	else
	{
		pSelf->SetUiMousePos(pSelf->m_LastMousePos.value());
	}

	// save pos, so moving the mouse in esc menu doesn't change the position
	pSelf->m_LastMousePos = OldMousePos;
}

void CScoreboard::OnConsoleInit()
{
	Console()->Register("+scoreboard", "", CFGFLAG_CLIENT, ConKeyScoreboard, this, "Show scoreboard");
	Console()->Register("toggle_scoreboard_cursor", "", CFGFLAG_CLIENT, ConToggleScoreboardCursor, this, "Toggle scoreboard cursor");
}

void CScoreboard::OnInit()
{
	m_DeadTeeTexture = Graphics()->LoadTexture("deadtee.png", IStorage::TYPE_ALL);
}

namespace
{
constexpr float PlayerPanelInset = 6.7f;
constexpr float PlayerHeaderHeight = 16;
constexpr float PlayerActionHeight = 18;
constexpr float PlayerActionGap = 3;

constexpr float PlayerPanelHeight(int Actions)
{
	return 2 * PlayerPanelInset + PlayerHeaderHeight + 4 + Actions * PlayerActionHeight + (Actions - 1) * PlayerActionGap;
}

bool s_ShowPlayerInfo = false;
float s_PlayerInfoProgress = 0;
const SPopupMenuProperties s_PlayerMenuStyle{IGraphics::CORNER_ALL, ColorRGBA(0, 0, 0, 0), ColorRGBA(0, 0, 0, 0)};

struct CAimPeak
{
	char m_aName[MAX_NAME_LENGTH]{};
	float m_Distance = 0;
};
CAimPeak s_aaAimPeaks[NUM_DUMMIES][MAX_CLIENTS];
}

void CScoreboard::OnNewSnapshot()
{
	auto &aPeaks = s_aaAimPeaks[g_Config.m_ClDummy];
	for(int Id = 0; Id < MAX_CLIENTS; ++Id)
	{
		auto &Peak = aPeaks[Id];
		const auto &Player = GameClient()->m_aClients[Id];
		if(!Player.m_Active || str_comp(Peak.m_aName, Player.m_aName) != 0)
		{
			Peak = {};
			if(!Player.m_Active)
				continue;
			str_copy(Peak.m_aName, Player.m_aName);
		}
		const auto &Char = GameClient()->m_Snap.m_aCharacters[Id];
		if(Char.m_Active && Char.m_HasExtendedDisplayInfo && !Player.m_Paused)
			Peak.m_Distance = std::max(Peak.m_Distance, length(vec2(Char.m_ExtendedData.m_TargetX, Char.m_ExtendedData.m_TargetY)));
	}
}

void CScoreboard::OnReset()
{
	for(auto &aPeaks : s_aaAimPeaks)
		for(auto &Peak : aPeaks)
			Peak = {};
	m_Active = false;
	m_AltHeld = m_AltPreviousUnlocked = false;
	m_MouseUnlocked = false;
	m_LastMousePos = std::nullopt;
}

void CScoreboard::ResetTexts()
{
	for(CPlayerElement &Player : m_aPlayers)
	{
		Player.m_Score.Reset(TextRender());
		Player.m_ScoreMillis.Reset(TextRender());
		Player.m_Name.Reset(TextRender());
		Player.m_MuteMark.Reset(TextRender());
		Player.m_ReadyMark.Reset(TextRender());
		Player.m_Clan.Reset(TextRender());
		Player.m_Ping.Reset(TextRender());
	}
	m_TitleScore.Reset(TextRender());
	m_TitleScoreMillis.Reset(TextRender());
	m_HeadlineScore.Reset(TextRender());
	m_HeadlineName.Reset(TextRender());
	m_HeadlineClan.Reset(TextRender());
	m_HeadlinePing.Reset(TextRender());
}

void CScoreboard::OnShutdown()
{
	for(auto &Source : s_aPresence)
		if(Source.m_pRequest)
			Source.m_pRequest->Abort();
	for(auto &Icon : s_aClientIcons)
		Graphics()->UnloadTexture(&Icon);
	ResetTexts();
}

void CScoreboard::OnWindowResize()
{
	ResetTexts();
}

void CScoreboard::OnRelease()
{
	m_Active = false;

	if(m_MouseUnlocked)
	{
		LockMouse();
	}
}

bool CScoreboard::OnCursorMove(float x, float y, IInput::ECursorType CursorType)
{
	if(!IsActive() || !m_MouseUnlocked)
		return false;

	Ui()->ConvertMouseMove(&x, &y, CursorType);
	Ui()->OnCursorMove(x, y);

	return true;
}

bool CScoreboard::OnInput(const IInput::CEvent &Event)
{
	if((Event.m_Key == KEY_LALT || Event.m_Key == KEY_RALT) && IsActive() && !GameClient()->m_Menus.IsActive() && !GameClient()->m_Chat.IsActive())
	{
		if((Event.m_Flags & IInput::FLAG_PRESS) && !(Event.m_Flags & IInput::FLAG_REPEAT))
			ConToggleScoreboardCursor(nullptr, this);
		return true;
	}
	if(m_MouseUnlocked && Event.m_Key == KEY_ESCAPE && (Event.m_Flags & IInput::FLAG_PRESS))
	{
		LockMouse();
		return true;
	}

	return IsActive() && m_MouseUnlocked;
}

void CScoreboard::RenderTitle(CUIRect TitleLabel, int Team, const char *pTitle, float TitleFontSize)
{
	const bool IsMapTitle = !GameClient()->IsTeamPlay();
	if(IsMapTitle && m_MouseUnlocked && GameClient()->m_aMapDescription[0] != '\0')
	{
		const CUIRect Hitbox = ScoreboardHitbox(TitleLabel);
		const int ButtonResult = Ui()->DoButtonLogic(&m_MapTitleButtonId, 0, &Hitbox, BUTTONFLAG_RIGHT);
		if(ButtonResult != 0)
		{
			m_MapTitlePopupContext.m_pScoreboard = this;

			m_MapTitlePopupContext.m_FontSize = 12.0f;
			const float MaxWidth = 300.0f;
			const float Margin = 5.0f;
			const char *pDescription = GameClient()->m_aMapDescription;
			const float TextWidth = std::min(std::ceil(TextRender()->TextWidth(m_MapTitlePopupContext.m_FontSize, pDescription) + 0.5f), MaxWidth);
			float TextHeight = 0.0f;
			STextSizeProperties TextSizeProps{};
			TextSizeProps.m_pHeight = &TextHeight;
			TextRender()->TextWidth(m_MapTitlePopupContext.m_FontSize, pDescription, -1, TextWidth, 0, TextSizeProps);

			Ui()->DoPopupMenu(&m_MapTitlePopupContext, Ui()->MouseX(), Ui()->MouseY(), TextWidth + Margin * 2, TextHeight + Margin * 2, &m_MapTitlePopupContext, CMapTitlePopupContext::Render);
		}
		if(Ui()->HotItem() == &m_MapTitleButtonId)
		{
			TitleLabel.Draw(ColorRGBA(0.7f, 0.7f, 0.7f, 0.3f), IGraphics::CORNER_ALL, 5.0f);
		}
	}

	SLabelProperties Props;
	Props.m_MaxWidth = TitleLabel.w;
	Props.m_EllipsisAtEnd = true;
	Ui()->DoLabel(&TitleLabel, pTitle, TitleFontSize, Team == TEAM_RED ? TEXTALIGN_ML : TEXTALIGN_MR, Props);
}

void CScoreboard::RenderTitleScore(CUIRect ScoreLabel, int Team, float TitleFontSize)
{
	// map best
	char aScore[128] = "";
	const CNetObj_GameInfo *pGameInfoObj = GameClient()->m_Snap.m_pGameInfoObj;
	const bool TimeScore = GameClient()->m_GameInfo.m_TimeScore;
	const bool Race7 = Client()->IsSixup() && pGameInfoObj && pGameInfoObj->m_GameFlags & protocol7::GAMEFLAG_RACE;
	if(GameClient()->m_ReceivedDDNetPlayerFinishTimes || TimeScore || Race7)
	{
		if(GameClient()->m_MapBestTimeSeconds != FinishTime::UNSET)
		{
			Ui()->RenderTime(ScoreLabel,
				TitleFontSize,
				GameClient()->m_MapBestTimeSeconds,
				GameClient()->m_MapBestTimeSeconds == FinishTime::NOT_FINISHED_MILLIS,
				GameClient()->m_MapBestTimeMillis,
				GameClient()->m_ReceivedDDNetPlayerFinishTimesMillis,
				m_TitleScore, m_TitleScoreMillis, TextRender()->DefaultTextColor());
			return;
		}
	}
	else if(GameClient()->IsTeamPlay()) // normal score
	{
		const CNetObj_GameData *pGameDataObj = GameClient()->m_Snap.m_pGameDataObj;
		if(pGameDataObj)
		{
			str_format(aScore, sizeof(aScore), "%d", Team == TEAM_RED ? pGameDataObj->m_TeamscoreRed : pGameDataObj->m_TeamscoreBlue);
		}
	}
	else
	{
		if(GameClient()->m_Snap.m_SpecInfo.m_Active &&
			GameClient()->m_Snap.m_SpecInfo.m_SpectatorId != SPEC_FREEVIEW &&
			GameClient()->m_Snap.m_apPlayerInfos[GameClient()->m_Snap.m_SpecInfo.m_SpectatorId])
		{
			str_format(aScore, sizeof(aScore), "%d", GameClient()->m_Snap.m_apPlayerInfos[GameClient()->m_Snap.m_SpecInfo.m_SpectatorId]->m_Score);
		}
		else if(GameClient()->m_Snap.m_pLocalInfo)
		{
			str_format(aScore, sizeof(aScore), "%d", GameClient()->m_Snap.m_pLocalInfo->m_Score);
		}
	}

	const float ScoreTextWidth = aScore[0] != '\0' ? TextRender()->TextWidth(TitleFontSize, aScore, -1, -1.0f, 0) : 0.0f;
	if(ScoreTextWidth != 0.0f)
	{
		Ui()->DoLabel(&ScoreLabel, aScore, TitleFontSize, Team == TEAM_RED ? TEXTALIGN_MR : TEXTALIGN_ML);
	}
}

void CScoreboard::RenderTitleBar(CUIRect TitleBar, int Team, const char *pTitle)
{
	dbg_assert(Team == TEAM_RED || Team == TEAM_BLUE, "Team invalid");

	const float TitleFontSize = 20.0f;
	const float ScoreTextWidth = TextRender()->TextWidth(TitleFontSize, "00:00:00");
	const float TitleTextWidth = TextRender()->TextWidth(TitleFontSize, pTitle);

	TitleBar.VMargin(MARGIN, &TitleBar);
	CUIRect TitleLabel, ScoreLabel;
	if(Team == TEAM_RED)
	{
		TitleBar.VSplitRight(ScoreTextWidth, &TitleLabel, &ScoreLabel);
		TitleLabel.VSplitRight(5.0f, &TitleLabel, nullptr);
		TitleLabel.VSplitLeft(std::min(TitleTextWidth + 2.0f, TitleLabel.w), &TitleLabel, nullptr);
	}
	else
	{
		TitleBar.VSplitLeft(ScoreTextWidth, &ScoreLabel, &TitleLabel);
		TitleLabel.VSplitLeft(5.0f, nullptr, &TitleLabel);
		TitleLabel.VSplitRight(std::min(TitleTextWidth + 2.0f, TitleLabel.w), nullptr, &TitleLabel);
	}

	RenderTitle(TitleLabel, Team, pTitle, TitleFontSize);
	RenderTitleScore(ScoreLabel, Team, TitleFontSize);
}

void CScoreboard::RenderGoals(CUIRect Goals)
{
	Goals.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.5f), IGraphics::CORNER_ALL, 7.5f);
	Goals.VMargin(5.0f, &Goals);

	const float FontSize = 10.0f;
	const CNetObj_GameInfo *pGameInfoObj = GameClient()->m_Snap.m_pGameInfoObj;
	char aBuf[64];

	if(pGameInfoObj->m_ScoreLimit)
	{
		str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Score limit"), pGameInfoObj->m_ScoreLimit);
		Ui()->DoLabel(&Goals, aBuf, FontSize, TEXTALIGN_ML);
	}

	if(pGameInfoObj->m_TimeLimit)
	{
		str_format(aBuf, sizeof(aBuf), Localize("Time limit: %d min"), pGameInfoObj->m_TimeLimit);
		Ui()->DoLabel(&Goals, aBuf, FontSize, TEXTALIGN_MC);
	}

	if(pGameInfoObj->m_RoundNum && pGameInfoObj->m_RoundCurrent)
	{
		str_format(aBuf, sizeof(aBuf), Localize("Round %d/%d"), pGameInfoObj->m_RoundCurrent, pGameInfoObj->m_RoundNum);
		Ui()->DoLabel(&Goals, aBuf, FontSize, TEXTALIGN_MR);
	}
}

void CScoreboard::RenderSpectators(CUIRect Spectators)
{
	Spectators.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.5f), IGraphics::CORNER_ALL, 7.5f);
	constexpr float SpectatorCut = 5.0f;
	Spectators.Margin(SpectatorCut, &Spectators);

	CTextCursor Cursor;
	Cursor.SetPosition(Spectators.TopLeft());
	Cursor.m_FontSize = 11.0f;
	Cursor.m_LineWidth = Spectators.w;
	Cursor.m_MaxLines = round_truncate(Spectators.h / Cursor.m_FontSize);

	int RemainingSpectators = 0;
	for(const CNetObj_PlayerInfo *pInfo : GameClient()->m_Snap.m_apInfoByName)
	{
		if(!pInfo || pInfo->m_Team != TEAM_SPECTATORS)
			continue;
		++RemainingSpectators;
	}

	TextRender()->TextEx(&Cursor, Localize("Spectators"));

	if(RemainingSpectators > 0)
	{
		TextRender()->TextEx(&Cursor, ": ");
	}

	bool CommaNeeded = false;
	for(const CNetObj_PlayerInfo *pInfo : GameClient()->m_Snap.m_apInfoByName)
	{
		if(!pInfo || pInfo->m_Team != TEAM_SPECTATORS)
			continue;

		if(CommaNeeded)
		{
			TextRender()->TextEx(&Cursor, ", ");
		}

		if(Cursor.m_LineCount == Cursor.m_MaxLines && RemainingSpectators >= 2)
		{
			// This is less expensive than checking with a separate invisible
			// text cursor though we waste some space at the end of the line.
			char aRemaining[64];
			str_format(aRemaining, sizeof(aRemaining), Localize("%d others…", "Spectators"), RemainingSpectators);
			TextRender()->TextEx(&Cursor, aRemaining);
			break;
		}

		CUIRect SpectatorRect, SpectatorRectLineBreak;
		float Margin = 1.0f;
		SpectatorRect.x = Cursor.m_X - Margin;
		SpectatorRect.y = Cursor.m_Y;

		if(g_Config.m_ClShowIds)
		{
			char aClientId[16];
			GameClient()->FormatClientId(pInfo->m_ClientId, aClientId, EClientIdFormat::NO_INDENT);
			TextRender()->TextEx(&Cursor, aClientId);
		}

		const CGameClient::CClientData &ClientData = GameClient()->m_aClients[pInfo->m_ClientId];
		{
			const char *pClanName = ClientData.m_aClan;
			if(pClanName[0] != '\0')
			{
				if(GameClient()->m_aLocalIds[g_Config.m_ClDummy] >= 0 && str_comp(pClanName, GameClient()->m_aClients[GameClient()->m_aLocalIds[g_Config.m_ClDummy]].m_aClan) == 0)
				{
					TextRender()->TextColor(color_cast<ColorRGBA>(ColorHSLA(g_Config.m_ClSameClanColor)));
				}
				else
				{
					TextRender()->TextColor(ColorRGBA(0.7f, 0.7f, 0.7f));
				}

				TextRender()->TextEx(&Cursor, pClanName);
				TextRender()->TextEx(&Cursor, " ");

				TextRender()->TextColor(TextRender()->DefaultTextColor());
			}
		}

		if(GameClient()->m_aClients[pInfo->m_ClientId].m_AuthLevel)
		{
			TextRender()->TextColor(color_cast<ColorRGBA>(ColorHSLA(g_Config.m_ClAuthedPlayerColor)));
		}

		TextRender()->TextEx(&Cursor, GameClient()->m_aClients[pInfo->m_ClientId].m_aName);
		TextRender()->TextColor(TextRender()->DefaultTextColor());

		CommaNeeded = true;
		--RemainingSpectators;

		bool LineBreakDetected = false;
		SpectatorRect.h = Cursor.m_FontSize;

		// detect line breaks
		if(Cursor.m_Y != SpectatorRect.y)
		{
			LineBreakDetected = true;
			SpectatorRectLineBreak.x = Spectators.x - SpectatorCut;
			SpectatorRectLineBreak.y = Cursor.m_Y;
			SpectatorRectLineBreak.h = Cursor.m_FontSize;
			SpectatorRectLineBreak.w = Cursor.m_X - Spectators.x + SpectatorCut + 2 * Margin;

			SpectatorRect.w = Spectators.x + Spectators.w + SpectatorCut - SpectatorRect.x;
		}
		else
		{
			SpectatorRect.w = Cursor.m_X - SpectatorRect.x + 2 * Margin;
		}

		if(m_MouseUnlocked && !GameClient()->m_AxionOverlay.CapturesScoreboardPointer())
		{
			const CUIRect Hitbox = ScoreboardHitbox(SpectatorRect);
			int ButtonResult = Ui()->DoButtonLogic(&m_aPlayers[pInfo->m_ClientId].m_PlayerButtonId, 0, &Hitbox, BUTTONFLAG_LEFT);

			if(LineBreakDetected && ButtonResult == 0)
			{
				const CUIRect SecondLineHitbox = ScoreboardHitbox(SpectatorRectLineBreak);
				ButtonResult = Ui()->DoButtonLogic(&m_aPlayers[pInfo->m_ClientId].m_SpectatorSecondLineButtonId, 0, &SecondLineHitbox, BUTTONFLAG_LEFT);
			}
			if(ButtonResult != 0)
			{
				s_ShowPlayerInfo = false;
				s_PlayerInfoProgress = 0;
				m_ScoreboardPopupContext.m_Animation = 0;
				m_ScoreboardPopupContext.m_Closing = false;
				std::fill(std::begin(m_ScoreboardPopupContext.m_aHover), std::end(m_ScoreboardPopupContext.m_aHover), 0.0f);
				m_ScoreboardPopupContext.m_Connection = g_Config.m_ClDummy;
				str_copy(m_ScoreboardPopupContext.m_aTargetName, GameClient()->m_aClients[pInfo->m_ClientId].m_aName);
				m_ScoreboardPopupContext.m_pScoreboard = this;
				m_ScoreboardPopupContext.m_ClientId = pInfo->m_ClientId;
				m_ScoreboardPopupContext.m_IsLocal = GameClient()->m_aLocalIds[0] == pInfo->m_ClientId ||
								     (Client()->DummyConnected() && GameClient()->m_aLocalIds[1] == pInfo->m_ClientId);
				m_ScoreboardPopupContext.m_IsSpectating = true;

				Ui()->DoPopupMenu(&m_ScoreboardPopupContext, Ui()->MouseX(), Ui()->MouseY(), 346.0f,
					PlayerPanelHeight(7) + 10.0f, &m_ScoreboardPopupContext, CScoreboardPopupContext::Render, s_PlayerMenuStyle);
			}

			if(Ui()->HotItem() == &m_aPlayers[pInfo->m_ClientId].m_PlayerButtonId ||
				Ui()->HotItem() == &m_aPlayers[pInfo->m_ClientId].m_SpectatorSecondLineButtonId ||
				(Ui()->IsPopupOpen(&m_ScoreboardPopupContext) && m_ScoreboardPopupContext.m_ClientId == pInfo->m_ClientId))
			{
				if(!LineBreakDetected)
				{
					SpectatorRect.Draw(TextRender()->DefaultTextSelectionColor(), IGraphics::CORNER_ALL, 2.5f);
				}
				else
				{
					SpectatorRect.Draw(TextRender()->DefaultTextSelectionColor(), IGraphics::CORNER_L, 2.5f);
					SpectatorRectLineBreak.Draw(TextRender()->DefaultTextSelectionColor(), IGraphics::CORNER_R, 2.5f);
				}
			}
		}
	}
}

void CScoreboard::RenderScoreboard(CUIRect Scoreboard, int Team, int CountStart, int CountEnd, CScoreboardRenderState &State, int NumPlayersForSize)
{
	dbg_assert(Team == TEAM_RED || Team == TEAM_BLUE, "Team invalid");

	const CNetObj_GameInfo *pGameInfoObj = GameClient()->m_Snap.m_pGameInfoObj;
	const CNetObj_GameData *pGameDataObj = GameClient()->m_Snap.m_pGameDataObj;
	const bool TimeScore = GameClient()->m_GameInfo.m_TimeScore;
	const bool MillisecondScore = GameClient()->m_ReceivedDDNetPlayerFinishTimes;
	const bool TrueMilliseconds = GameClient()->m_ReceivedDDNetPlayerFinishTimesMillis;
	const int NumPlayers = NumPlayersForSize >= 0 ? NumPlayersForSize : (CountEnd - CountStart);
	const bool LowScoreboardWidth = Scoreboard.w < 350.0f;

	bool Race7 = Client()->IsSixup() && pGameInfoObj && pGameInfoObj->m_GameFlags & protocol7::GAMEFLAG_RACE;

	const bool UseTime = Race7 || TimeScore || MillisecondScore;

	// calculate measurements
	float LineHeight;
	float TeeSizeMod;
	float Spacing;
	float RoundRadius;
	float FontSize;
	if(NumPlayers <= 8)
	{
		LineHeight = 30.0f;
		TeeSizeMod = 0.5f;
		Spacing = 8.0f;
		RoundRadius = 5.0f;
		FontSize = 12.0f;
	}
	else if(NumPlayers <= 12)
	{
		LineHeight = 25.0f;
		TeeSizeMod = 0.45f;
		Spacing = 2.5f;
		RoundRadius = 5.0f;
		FontSize = 12.0f;
	}
	else if(NumPlayers <= 16)
	{
		LineHeight = 20.0f;
		TeeSizeMod = 0.4f;
		Spacing = 0.0f;
		RoundRadius = 2.5f;
		FontSize = 12.0f;
	}
	else if(NumPlayers <= 24)
	{
		LineHeight = 13.5f;
		TeeSizeMod = 0.3f;
		Spacing = 0.0f;
		RoundRadius = 2.5f;
		FontSize = 10.0f;
	}
	else if(NumPlayers <= 32)
	{
		LineHeight = 10.0f;
		TeeSizeMod = 0.2f;
		Spacing = 0.0f;
		RoundRadius = 2.5f;
		FontSize = 8.0f;
	}
	else if(LowScoreboardWidth && NumPlayers <= 48)
	{
		LineHeight = 7.5f;
		TeeSizeMod = 0.125f;
		Spacing = 0.0f;
		RoundRadius = 1.0f;
		FontSize = 7.0f;
	}
	else
	{
		LineHeight = 5.0f;
		TeeSizeMod = 0.1f;
		Spacing = 0.0f;
		RoundRadius = 1.0f;
		FontSize = 5.0f;
	}

	const float ScoreOffset = Scoreboard.x + MARGIN;
	const float ScoreLength = TextRender()->TextWidth(FontSize, UseTime ? "00:00:00" : "99999");
	const float TeeOffset = ScoreOffset + ScoreLength + MARGIN;
	const float TeeLength = 60.0f * TeeSizeMod;
	const float NameOffset = TeeOffset + TeeLength;
	const float CountryLength = (LineHeight - Spacing - TeeSizeMod * 5.0f) * 2.0f;
	const float PingLength = 27.5f;
	const float PingOffset = Scoreboard.x + Scoreboard.w - PingLength - MARGIN;
	const float CountryOffset = PingOffset - CountryLength;

	float NameLength = (LowScoreboardWidth ? 90.0f : 150.0f) - TeeLength;
	const float MinMiddleGap = 5.0f; // 2.5 before and after clan
	const float AvailableMiddle = CountryOffset - NameOffset;
	if(NameLength + MinMiddleGap > AvailableMiddle)
	{
		const float Shrinkable = AvailableMiddle - MinMiddleGap;
		NameLength = std::max(0.0f, Shrinkable * 0.7f);
	}

	const float ClanOffset = NameOffset + NameLength + 2.5f;
	const float ClanLength = std::max(0.0f, CountryOffset - ClanOffset - 2.5f);

	// render headlines
	const float HeadlineFontsize = 11.0f;
	CUIRect Headline;
	Scoreboard.HSplitTop(HeadlineFontsize * 2.0f, &Headline, &Scoreboard);
	const float HeadlineY = Headline.y + Headline.h / 2.0f - HeadlineFontsize / 2.0f;
	const ColorRGBA HeadlineColor = TextRender()->DefaultTextColor();
	m_HeadlineScore.Update(TextRender(), UseTime ? Localize("Time") : Localize("Score"), HeadlineFontsize);
	m_HeadlineName.Update(TextRender(), Localize("Name"), HeadlineFontsize);
	m_HeadlineClan.Update(TextRender(), Localize("Clan"), HeadlineFontsize);
	m_HeadlinePing.Update(TextRender(), Localize("Ping"), HeadlineFontsize);
	m_HeadlineScore.Render(TextRender(), vec2(ScoreOffset + ScoreLength - m_HeadlineScore.Width(), HeadlineY), HeadlineColor);
	m_HeadlineName.Render(TextRender(), vec2(NameOffset, HeadlineY), HeadlineColor);
	m_HeadlineClan.Render(TextRender(), vec2(ClanOffset + (ClanLength - m_HeadlineClan.Width()) / 2.0f, HeadlineY), HeadlineColor);
	m_HeadlinePing.Render(TextRender(), vec2(PingOffset + PingLength - m_HeadlinePing.Width(), HeadlineY), HeadlineColor);

	// render player entries
	int CountRendered = 0;
	int PrevDDTeam = -1;
	int &CurrentDDTeamSize = State.m_CurrentDDTeamSize;

	char aBuf[64];
	int MaxTeamSize = GameClient()->MaxTeamSize();

	for(int RenderDead = 0; RenderDead < 2; RenderDead++)
	{
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			// make sure that we render the correct team
			const CNetObj_PlayerInfo *pInfo = GameClient()->m_Snap.m_apInfoByDDTeamScore[i];
			if(!pInfo || pInfo->m_Team != Team)
				continue;
			bool IsDead = Client()->m_TranslationContext.m_aClients[pInfo->m_ClientId].m_PlayerFlags7 & protocol7::PLAYERFLAG_DEAD;
			if(!RenderDead && IsDead)
				continue;
			if(RenderDead && !IsDead)
				continue;
			if(CountRendered++ < CountStart)
				continue;

			int DDTeam = GameClient()->m_Teams.Team(pInfo->m_ClientId);
			int NextDDTeam = 0;

			ColorRGBA TextColor = TextRender()->DefaultTextColor();
			TextColor.a = RenderDead ? 0.5f : 1.0f;
			TextRender()->TextColor(TextColor);

			for(int j = i + 1; j < MAX_CLIENTS; j++)
			{
				const CNetObj_PlayerInfo *pInfoNext = GameClient()->m_Snap.m_apInfoByDDTeamScore[j];
				if(!pInfoNext || pInfoNext->m_Team != Team)
					continue;

				NextDDTeam = GameClient()->m_Teams.Team(pInfoNext->m_ClientId);
				break;
			}

			if(PrevDDTeam == -1)
			{
				for(int j = i - 1; j >= 0; j--)
				{
					const CNetObj_PlayerInfo *pInfoPrev = GameClient()->m_Snap.m_apInfoByDDTeamScore[j];
					if(!pInfoPrev || pInfoPrev->m_Team != Team)
						continue;

					PrevDDTeam = GameClient()->m_Teams.Team(pInfoPrev->m_ClientId);
					break;
				}
			}

			CUIRect RowAndSpacing, Row;
			Scoreboard.HSplitTop(LineHeight + Spacing, &RowAndSpacing, &Scoreboard);
			RowAndSpacing.HSplitTop(LineHeight, &Row, nullptr);

			// team background
			if(DDTeam != TEAM_FLOCK)
			{
				const ColorRGBA Color = GameClient()->GetDDTeamColor(DDTeam).WithAlpha(0.5f);
				int TeamRectCorners = 0;
				if(PrevDDTeam != DDTeam)
				{
					TeamRectCorners |= IGraphics::CORNER_T;
					State.m_TeamStartX = Row.x;
					State.m_TeamStartY = Row.y;
				}
				if(NextDDTeam != DDTeam)
					TeamRectCorners |= IGraphics::CORNER_B;
				// The team fill tiles adjacent rows, so it must stay inside each row.
				Graphics()->TextureClear();
				Graphics()->QuadsBegin();
				Graphics()->SetColor(Color);
				Graphics()->DrawRectExt(RowAndSpacing.x, RowAndSpacing.y, RowAndSpacing.w, RowAndSpacing.h, RoundRadius, TeamRectCorners);
				Graphics()->QuadsEnd();
				Graphics()->SetColor(1, 1, 1, 1);

				CurrentDDTeamSize++;

				if(NextDDTeam != DDTeam)
				{
					const float TeamFontSize = FontSize / 1.5f;

					if(NumPlayers > 8)
					{
						if(DDTeam == GameClient()->m_Teams.TeamSuper())
							str_copy(aBuf, Localize("Super"));
						else if(CurrentDDTeamSize <= 1)
							str_format(aBuf, sizeof(aBuf), "%d", DDTeam);
						else
							str_format(aBuf, sizeof(aBuf), Localize("%d\n(%d/%d)", "Team and size"), DDTeam, CurrentDDTeamSize, MaxTeamSize);
						TextRender()->Text(State.m_TeamStartX, std::max(State.m_TeamStartY + Row.h / 2.0f - TeamFontSize, State.m_TeamStartY + 1.5f /* padding top */), TeamFontSize, aBuf);
					}
					else
					{
						if(DDTeam == GameClient()->m_Teams.TeamSuper())
							str_copy(aBuf, Localize("Super"));
						else if(CurrentDDTeamSize > 1)
							str_format(aBuf, sizeof(aBuf), Localize("Team %d (%d/%d)"), DDTeam, CurrentDDTeamSize, MaxTeamSize);
						else
							str_format(aBuf, sizeof(aBuf), Localize("Team %d"), DDTeam);
						TextRender()->Text(Row.x + Row.w / 2.0f - TextRender()->TextWidth(TeamFontSize, aBuf) / 2.0f + 5.0f, Row.y + Row.h, TeamFontSize, aBuf);
					}

					CurrentDDTeamSize = 0;
				}
			}
			PrevDDTeam = DDTeam;

			// background so it's easy to find the local player or the followed one in spectator mode
			if((!GameClient()->m_Snap.m_SpecInfo.m_Active && pInfo->m_Local) ||
				(GameClient()->m_Snap.m_SpecInfo.m_SpectatorId == SPEC_FREEVIEW && pInfo->m_Local) ||
				(GameClient()->m_Snap.m_SpecInfo.m_Active && pInfo->m_ClientId == GameClient()->m_Snap.m_SpecInfo.m_SpectatorId))
			{
				Row.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.25f), IGraphics::CORNER_ALL, RoundRadius);
			}

			const CGameClient::CClientData &ClientData = GameClient()->m_aClients[pInfo->m_ClientId];
			CPlayerElement &Player = m_aPlayers[pInfo->m_ClientId];

			if(m_MouseUnlocked && !GameClient()->m_AxionOverlay.CapturesScoreboardPointer())
			{
				const CUIRect Hitbox = ScoreboardHitbox(Row);
				const int ButtonResult = Ui()->DoButtonLogic(&Player.m_PlayerButtonId, 0, &Hitbox, BUTTONFLAG_LEFT);
				if(ButtonResult != 0)
				{
					s_ShowPlayerInfo = false;
					s_PlayerInfoProgress = 0;
					m_ScoreboardPopupContext.m_Animation = 0;
					m_ScoreboardPopupContext.m_Closing = false;
					std::fill(std::begin(m_ScoreboardPopupContext.m_aHover), std::end(m_ScoreboardPopupContext.m_aHover), 0.0f);
					m_ScoreboardPopupContext.m_Connection = g_Config.m_ClDummy;
					str_copy(m_ScoreboardPopupContext.m_aTargetName, GameClient()->m_aClients[pInfo->m_ClientId].m_aName);
					m_ScoreboardPopupContext.m_pScoreboard = this;
					m_ScoreboardPopupContext.m_ClientId = pInfo->m_ClientId;
					m_ScoreboardPopupContext.m_IsLocal = GameClient()->m_aLocalIds[0] == pInfo->m_ClientId ||
									     (Client()->DummyConnected() && GameClient()->m_aLocalIds[1] == pInfo->m_ClientId);
					m_ScoreboardPopupContext.m_IsSpectating = false;

					Ui()->DoPopupMenu(&m_ScoreboardPopupContext, Ui()->MouseX(), Ui()->MouseY(), 346.0f,
						PlayerPanelHeight(7) + 10.0f, &m_ScoreboardPopupContext, CScoreboardPopupContext::Render, s_PlayerMenuStyle);
				}

				if(Ui()->HotItem() == &Player.m_PlayerButtonId ||
					(Ui()->IsPopupOpen(&m_ScoreboardPopupContext) && m_ScoreboardPopupContext.m_ClientId == pInfo->m_ClientId))
				{
					Row.Draw(ColorRGBA(0.7f, 0.7f, 0.7f, 0.7f), IGraphics::CORNER_ALL, RoundRadius);
				}
			}

			// score
			CUIRect ScorePosition;
			ScorePosition.x = ScoreOffset;
			ScorePosition.w = ScoreLength;
			ScorePosition.y = Row.y;
			ScorePosition.h = Row.h;

			if(Race7)
			{
				Ui()->RenderTime(ScorePosition, FontSize, pInfo->m_Score / 1000, pInfo->m_Score == protocol7::FinishTime::NOT_FINISHED, pInfo->m_Score % 1000, true,
					Player.m_Score, Player.m_ScoreMillis, TextColor);
			}
			else if(MillisecondScore)
			{
				Ui()->RenderTime(ScorePosition, FontSize, ClientData.m_FinishTimeSeconds, ClientData.m_FinishTimeSeconds == FinishTime::NOT_FINISHED_MILLIS, ClientData.m_FinishTimeMillis, TrueMilliseconds,
					Player.m_Score, Player.m_ScoreMillis, TextColor);
			}
			else if(TimeScore)
			{
				Ui()->RenderTime(ScorePosition, FontSize, pInfo->m_Score, pInfo->m_Score == FinishTime::NOT_FINISHED_TIMESCORE, -1, false,
					Player.m_Score, Player.m_ScoreMillis, TextColor);
			}
			else
			{
				str_format(aBuf, sizeof(aBuf), "%d", std::clamp(pInfo->m_Score, -999, 99999));
				Player.m_Score.Update(TextRender(), aBuf, FontSize);
				Player.m_Score.Render(TextRender(), vec2(ScoreOffset + ScoreLength - Player.m_Score.Width(), ScorePosition.y + (Row.h - FontSize) / 2.0f), TextColor);
			}

			// CTF flag
			if(pGameInfoObj && (pGameInfoObj->m_GameFlags & GAMEFLAG_FLAGS) &&
				pGameDataObj && (pGameDataObj->m_FlagCarrierRed == pInfo->m_ClientId || pGameDataObj->m_FlagCarrierBlue == pInfo->m_ClientId))
			{
				Graphics()->TextureSet(pGameDataObj->m_FlagCarrierBlue == pInfo->m_ClientId ? GameClient()->m_GameSkin.m_SpriteFlagBlue : GameClient()->m_GameSkin.m_SpriteFlagRed);
				Graphics()->QuadsBegin();
				Graphics()->QuadsSetSubset(1.0f, 0.0f, 0.0f, 1.0f);
				IGraphics::CQuadItem QuadItem(TeeOffset, Row.y - 2.5f - Spacing / 2.0f, Row.h / 2.0f, Row.h);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
				Graphics()->QuadsEnd();
			}

			// skin
			if(RenderDead)
			{
				Graphics()->TextureSet(m_DeadTeeTexture);
				Graphics()->QuadsBegin();
				if(GameClient()->IsTeamPlay())
				{
					Graphics()->SetColor(GameClient()->m_Skins7.GetTeamColor(true, 0, GameClient()->m_aClients[pInfo->m_ClientId].m_Team, protocol7::SKINPART_BODY));
				}
				CTeeRenderInfo TeeInfo = GameClient()->m_aClients[pInfo->m_ClientId].m_RenderInfo;
				TeeInfo.m_Size *= TeeSizeMod;
				IGraphics::CQuadItem QuadItem(TeeOffset, Row.y, TeeInfo.m_Size, TeeInfo.m_Size);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
				Graphics()->QuadsEnd();
			}
			else
			{
				CTeeRenderInfo TeeInfo = ClientData.m_RenderInfo;
				TeeInfo.m_Size *= TeeSizeMod;
				vec2 OffsetToMid;
				CRenderTools::GetRenderTeeOffsetToRenderedTee(CAnimState::GetIdle(), &TeeInfo, OffsetToMid);
				const vec2 TeeRenderPos = vec2(TeeOffset + TeeLength / 2, Row.y + Row.h / 2.0f + OffsetToMid.y);
				RenderTools()->RenderTee(CAnimState::GetIdle(), &TeeInfo, EMOTE_NORMAL, vec2(1.0f, 0.0f), TeeRenderPos);

				if(m_MouseUnlocked)
				{
					const CUIRect SkinRect = {TeeOffset, Row.y, TeeLength, Row.h};
					GameClient()->m_Tooltips.DoToolTip(&m_aPlayers[pInfo->m_ClientId].m_PlayerButtonId, &SkinRect, ClientData.m_aSkinName);
				}
			}

			const float TextY = Row.y + (Row.h - FontSize) / 2.0f;

			// name
			{
				if(g_Config.m_ClShowIds)
				{
					char aClientId[16];
					GameClient()->FormatClientId(pInfo->m_ClientId, aClientId, EClientIdFormat::INDENT_AUTO);
					str_copy(aBuf, aClientId);
					str_append(aBuf, ClientData.m_aName);
				}
				else
				{
					str_copy(aBuf, ClientData.m_aName);
				}
				ColorRGBA NameColor = TextColor;
				if(ClientData.m_AuthLevel)
				{
					NameColor = color_cast<ColorRGBA>(ColorHSLA(g_Config.m_ClAuthedPlayerColor));
				}
				// TClient
				if(pInfo->m_ClientId >= 0 && g_Config.m_TcWarList && g_Config.m_TcWarListScoreboard && GameClient()->m_WarList.GetAnyWar(pInfo->m_ClientId))
				{
					NameColor = GameClient()->m_WarList.GetNameplateColor(pInfo->m_ClientId);
				}

				float NameTextOffset = NameOffset;
				EPlayerClient ClientBadge = EPlayerClient::UNKNOWN;
				if(Client()->State() == IClient::STATE_ONLINE)
				{
					for(auto &Source : s_aPresence)
					{
						const auto Entry = Source.m_Players.find(ClientData.m_aName);
						if(Entry != Source.m_Players.end())
							ClientBadge = Entry->second;
					}
					if(ClientBadge != EPlayerClient::UNKNOWN)
						for(const auto &Other : GameClient()->m_aClients)
							if(&Other != &ClientData && Other.m_Active && str_comp(Other.m_aName, ClientData.m_aName) == 0)
							{
								ClientBadge = EPlayerClient::UNKNOWN;
								break;
							}
				}
				if(ClientBadge != EPlayerClient::UNKNOWN)
				{
					const float Size = FontSize * 0.9f;
					const bool Fake = ClientBadge == EPlayerClient::FAKE_BEST_CLIENT;
					const bool Nova = ClientBadge == EPlayerClient::NOVA;
					const CUIRect Badge = {NameTextOffset, Row.y + (Row.h - Size) / 2, Size, Size};
					auto &Icon = s_aClientIcons[Nova ? 2 : Fake ? 1 : 0];
					if(!Icon.IsValid())
						Icon = Graphics()->LoadTexture(Nova ? "axion/nc_icon.png" : Fake ? "axion/bc_fake_icon.png" : "axion/bc_icon.png", IStorage::TYPE_ALL);
					if(Icon.IsValid())
					{
						Graphics()->TextureSet(Icon);
						Graphics()->QuadsBegin();
						Graphics()->SetColor(1, 1, 1, 1);
						const IGraphics::CQuadItem Quad(Badge.x, Badge.y, Badge.w, Badge.h);
						Graphics()->QuadsDrawTL(&Quad, 1);
						Graphics()->QuadsEnd();
					}
					NameTextOffset += Badge.w + FontSize * 0.22f;
				}
				const bool Muted = pInfo->m_ClientId >= 0 && (GameClient()->m_aClients[pInfo->m_ClientId].m_Foe || GameClient()->m_aClients[pInfo->m_ClientId].m_ChatIgnore);
				if(Muted)
				{
					TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
					Player.m_MuteMark.Update(TextRender(), FontIcon::COMMENT_SLASH, FontSize);
					TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
					Player.m_MuteMark.Render(TextRender(), vec2(NameTextOffset, TextY), NameColor);
					NameTextOffset += Player.m_MuteMark.Width();
				}
				Player.m_Name.Update(TextRender(), aBuf, FontSize, std::max(NameLength - (NameTextOffset - NameOffset), 0.0f), TEXTFLAG_RENDER | TEXTFLAG_ELLIPSIS_AT_END);
				Player.m_Name.Render(TextRender(), vec2(NameTextOffset, TextY), NameColor);

				// ready / watching
				if(Client()->IsSixup() && Client()->m_TranslationContext.m_aClients[pInfo->m_ClientId].m_PlayerFlags7 & protocol7::PLAYERFLAG_READY)
				{
					Player.m_ReadyMark.Update(TextRender(), "✓", FontSize);
					Player.m_ReadyMark.Render(TextRender(), vec2(NameTextOffset + Player.m_Name.Width(), TextY), ColorRGBA(0.1f, 1.0f, 0.1f, TextColor.a));
				}
			}

			// clan
			{
				ColorRGBA ClanColor = TextColor;
				if(GameClient()->m_aLocalIds[g_Config.m_ClDummy] >= 0 && str_comp(ClientData.m_aClan, GameClient()->m_aClients[GameClient()->m_aLocalIds[g_Config.m_ClDummy]].m_aClan) == 0)
				{
					ClanColor = color_cast<ColorRGBA>(ColorHSLA(g_Config.m_ClSameClanColor));
				}
				// TClient
				if(pInfo->m_ClientId >= 0 && g_Config.m_TcWarList && g_Config.m_TcWarListScoreboard && GameClient()->m_WarList.GetAnyWar(pInfo->m_ClientId))
				{
					ClanColor = GameClient()->m_WarList.GetClanColor(pInfo->m_ClientId);
				}

				Player.m_Clan.Update(TextRender(), ClientData.m_aClan, FontSize, ClanLength, TEXTFLAG_RENDER | TEXTFLAG_ELLIPSIS_AT_END);
				Player.m_Clan.Render(TextRender(), vec2(ClanOffset + (ClanLength - std::min(Player.m_Clan.Width(), ClanLength)) / 2.0f, TextY), ClanColor);
			}

			// country flag
			GameClient()->m_CountryFlags.Render(ClientData.m_Country, ColorRGBA(1.0f, 1.0f, 1.0f, 0.5f),
				CountryOffset, Row.y + (Spacing + TeeSizeMod * 5.0f) / 2.0f, CountryLength, Row.h - Spacing - TeeSizeMod * 5.0f);

			// ping
			ColorRGBA PingColor = TextRender()->DefaultTextColor();
			if(g_Config.m_ClEnablePingColor)
			{
				PingColor = color_cast<ColorRGBA>(ColorHSLA((300.0f - std::clamp(pInfo->m_Latency, 0, 300)) / 1000.0f, 1.0f, 0.5f));
			}
			str_format(aBuf, sizeof(aBuf), "%d", std::clamp(pInfo->m_Latency, 0, 999));
			Player.m_Ping.Update(TextRender(), aBuf, FontSize);
			Player.m_Ping.Render(TextRender(), vec2(PingOffset + PingLength - Player.m_Ping.Width(), TextY), PingColor);
			TextRender()->TextColor(TextRender()->DefaultTextColor());

			if(CountRendered == CountEnd)
				break;
		}
		if(CountRendered == CountEnd)
			break;
	}
}

void CScoreboard::RenderRecordingNotification(float x)
{
	char aBuf[512] = "";

	const auto &&AppendRecorderInfo = [&](int Recorder, const char *pName) {
		if(GameClient()->DemoRecorder(Recorder)->IsRecording())
		{
			char aTime[32];
			str_time((int64_t)GameClient()->DemoRecorder(Recorder)->Length() * 100, ETimeFormat::HOURS, aTime, sizeof(aTime));
			str_append(aBuf, pName);
			str_append(aBuf, " ");
			str_append(aBuf, aTime);
			str_append(aBuf, "  ");
		}
	};

	AppendRecorderInfo(RECORDER_MANUAL, Localize("Manual"));
	AppendRecorderInfo(RECORDER_RACE, Localize("Race"));
	AppendRecorderInfo(RECORDER_AUTO, Localize("Auto"));
	AppendRecorderInfo(RECORDER_REPLAYS, Localize("Replay"));

	if(aBuf[0] == '\0')
		return;

	const float FontSize = 10.0f;

	CUIRect Rect = {x, 0.0f, TextRender()->TextWidth(FontSize, aBuf) + 30.0f, 25.0f};
	Rect.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.4f), IGraphics::CORNER_B, 7.5f);
	Rect.VSplitLeft(10.0f, nullptr, &Rect);
	Rect.VSplitRight(5.0f, &Rect, nullptr);

	CUIRect Circle;
	Rect.VSplitLeft(10.0f, &Circle, &Rect);
	Circle.HMargin((Circle.h - Circle.w) / 2.0f, &Circle);
	Circle.Draw(ColorRGBA(1.0f, 0.0f, 0.0f, 1.0f), IGraphics::CORNER_ALL, Circle.h / 2.0f);

	Rect.VSplitLeft(5.0f, nullptr, &Rect);
	Ui()->DoLabel(&Rect, aBuf, FontSize, TEXTALIGN_ML);
}

CUIRect CScoreboard::ScoreboardHitbox(const CUIRect &Rect) const
{
	return {Rect.x * m_ScoreboardScale + m_ScoreboardOffset.x, Rect.y * m_ScoreboardScale + m_ScoreboardOffset.y,
		Rect.w * m_ScoreboardScale, Rect.h * m_ScoreboardScale};
}

void CScoreboard::OnRender()
{
	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return;

	if(Client()->State() == IClient::STATE_ONLINE && IsActive())
		UpdateScoreboardPresence(Http(), Client()->ServerAddress());

	const CUIRect Screen = *Ui()->Screen();
	const CNetObj_GameInfo *pGameInfoObj = GameClient()->m_Snap.m_pGameInfoObj;
	const bool Teams = GameClient()->IsTeamPlay();
	const auto &aTeamSize = GameClient()->m_Snap.m_aTeamSize;
	const int NumPlayers = Teams ? std::max(aTeamSize[TEAM_RED], aTeamSize[TEAM_BLUE]) : aTeamSize[TEAM_RED];
	const float ScoreboardSmallWidth = 385.0f;
	const float ScoreboardWidth = !Teams && NumPlayers <= 16 ? ScoreboardSmallWidth : 750.0f;
	const float TitleHeight = 30.0f;
	CUIRect Scoreboard = {(Screen.w - ScoreboardWidth) / 2.0f, 75.0f, ScoreboardWidth, 355.0f + TitleHeight};
	const CUIRect Bounds = {Scoreboard.x, Scoreboard.y, Scoreboard.w, Scoreboard.h + 105.0f};

	if(!IsActive())
	{
		// lock mouse if scoreboard was opened by being dead or game pause
		if(m_MouseUnlocked)
		{
			LockMouse();
		}
		return;
	}

	if(!GameClient()->m_Menus.IsActive() && !GameClient()->m_Chat.IsActive())
	{
		Ui()->StartCheck();
		Ui()->Update();
	}

	AxionHud::Record(AxionHud::SCOREBOARD, Bounds, Screen.w, Screen.h);

	// if the score board is active, then we should clear the motd message as well
	if(GameClient()->m_Motd.IsActive())
		GameClient()->m_Motd.Clear();

	Ui()->MapScreen();
	const auto UiProjection = Graphics()->GetScreen();
	const CUIRect Scaled = AxionHud::ScoreboardBase(Bounds, Screen.w, Screen.h);
	const vec2 Offset = AxionHud::Offset(AxionHud::SCOREBOARD, Screen.w, Screen.h);
	const vec2 Position(std::clamp(Scaled.x + Offset.x, 0.0f, std::max(0.0f, Screen.w - Scaled.w)),
		std::clamp(Scaled.y + Offset.y, 0.0f, std::max(0.0f, Screen.h - Scaled.h)));
	m_ScoreboardScale = AxionHud::ScoreboardScale(Bounds, Screen.w, Screen.h);
	m_ScoreboardOffset = Position - Bounds.TopLeft() * m_ScoreboardScale;
	Graphics()->MapScreen(CScreenRect(-m_ScoreboardOffset / m_ScoreboardScale, (vec2(Screen.w, Screen.h) - m_ScoreboardOffset) / m_ScoreboardScale));
	CScoreboardRenderState RenderState{};

	if(Teams)
	{
		const char *pRedTeamName = GetTeamName(TEAM_RED);
		const char *pBlueTeamName = GetTeamName(TEAM_BLUE);

		// Game over title
		const CNetObj_GameData *pGameDataObj = GameClient()->m_Snap.m_pGameDataObj;
		if((pGameInfoObj->m_GameStateFlags & GAMESTATEFLAG_GAMEOVER) && pGameDataObj)
		{
			char aTitle[256];
			if(pGameDataObj->m_TeamscoreRed > pGameDataObj->m_TeamscoreBlue)
			{
				TextRender()->TextColor(ColorRGBA(0.975f, 0.17f, 0.17f, 1.0f));
				if(pRedTeamName == nullptr)
				{
					str_copy(aTitle, Localize("Red team wins!"));
				}
				else
				{
					str_format(aTitle, sizeof(aTitle), Localize("%s wins!"), pRedTeamName);
				}
			}
			else if(pGameDataObj->m_TeamscoreBlue > pGameDataObj->m_TeamscoreRed)
			{
				TextRender()->TextColor(ColorRGBA(0.17f, 0.46f, 0.975f, 1.0f));
				if(pBlueTeamName == nullptr)
				{
					str_copy(aTitle, Localize("Blue team wins!"));
				}
				else
				{
					str_format(aTitle, sizeof(aTitle), Localize("%s wins!"), pBlueTeamName);
				}
			}
			else
			{
				TextRender()->TextColor(ColorRGBA(0.91f, 0.78f, 0.33f, 1.0f));
				str_copy(aTitle, Localize("Draw!"));
			}

			const float TitleFontSize = 36.0f;
			CUIRect GameOverTitle = {Scoreboard.x, Scoreboard.y - TitleFontSize - 6.0f, Scoreboard.w, TitleFontSize};
			Ui()->DoLabel(&GameOverTitle, aTitle, TitleFontSize, TEXTALIGN_MC);
			TextRender()->TextColor(TextRender()->DefaultTextColor());
		}

		CUIRect RedScoreboard, BlueScoreboard, RedTitle, BlueTitle;
		Scoreboard.VSplitMid(&RedScoreboard, &BlueScoreboard, 7.5f);
		RedScoreboard.HSplitTop(TitleHeight, &RedTitle, &RedScoreboard);
		BlueScoreboard.HSplitTop(TitleHeight, &BlueTitle, &BlueScoreboard);

		RedTitle.Draw(ColorRGBA(0.975f, 0.17f, 0.17f, 0.5f), IGraphics::CORNER_T, 7.5f);
		BlueTitle.Draw(ColorRGBA(0.17f, 0.46f, 0.975f, 0.5f), IGraphics::CORNER_T, 7.5f);
		RedScoreboard.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.5f), IGraphics::CORNER_B, 7.5f);
		BlueScoreboard.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.5f), IGraphics::CORNER_B, 7.5f);

		RenderTitleBar(RedTitle, TEAM_RED, pRedTeamName == nullptr ? Localize("Red team") : pRedTeamName);
		RenderTitleBar(BlueTitle, TEAM_BLUE, pBlueTeamName == nullptr ? Localize("Blue team") : pBlueTeamName);

		auto RenderTeamScoreboard = [&](CUIRect TeamScoreboard, int Team, int TeamSize) {
			if(TeamSize <= 64)
			{
				RenderScoreboard(TeamScoreboard, Team, 0, TeamSize, RenderState);
			}
			else
			{
				const int FirstColumnSize = 64;
				CUIRect LeftColumn, RightColumn;
				TeamScoreboard.VSplitMid(&LeftColumn, &RightColumn, 2.5f);

				RenderScoreboard(LeftColumn, Team, 0, FirstColumnSize, RenderState, FirstColumnSize);
				RenderScoreboard(RightColumn, Team, FirstColumnSize, TeamSize, RenderState, FirstColumnSize);
			}
		};

		RenderTeamScoreboard(RedScoreboard, TEAM_RED, aTeamSize[TEAM_RED]);
		RenderTeamScoreboard(BlueScoreboard, TEAM_BLUE, aTeamSize[TEAM_BLUE]);
	}
	else
	{
		Scoreboard.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.5f), IGraphics::CORNER_ALL, 7.5f);

		const char *pTitle;
		if(pGameInfoObj && (pGameInfoObj->m_GameStateFlags & GAMESTATEFLAG_GAMEOVER))
		{
			pTitle = Localize("Game over");
		}
		else
		{
			pTitle = GameClient()->Map()->BaseName();
		}

		CUIRect Title;
		Scoreboard.HSplitTop(TitleHeight, &Title, &Scoreboard);
		RenderTitleBar(Title, TEAM_GAME, pTitle);

		if(NumPlayers <= 16)
		{
			RenderScoreboard(Scoreboard, TEAM_GAME, 0, NumPlayers, RenderState);
		}
		else if(NumPlayers <= 64)
		{
			int PlayersPerSide;
			if(NumPlayers <= 24)
				PlayersPerSide = 12;
			else if(NumPlayers <= 32)
				PlayersPerSide = 16;
			else if(NumPlayers <= 48)
				PlayersPerSide = 24;
			else
				PlayersPerSide = 32;

			CUIRect LeftScoreboard, RightScoreboard;
			Scoreboard.VSplitMid(&LeftScoreboard, &RightScoreboard);
			RenderScoreboard(LeftScoreboard, TEAM_GAME, 0, PlayersPerSide, RenderState);
			RenderScoreboard(RightScoreboard, TEAM_GAME, PlayersPerSide, 2 * PlayersPerSide, RenderState);
		}
		else
		{
			const int NumColumns = 3;
			const int PlayersPerColumn = std::ceil(128.0f / NumColumns);
			CUIRect RemainingScoreboard = Scoreboard;
			for(int i = 0; i < NumColumns; ++i)
			{
				CUIRect Column;
				RemainingScoreboard.VSplitLeft(Scoreboard.w / NumColumns, &Column, &RemainingScoreboard);
				RenderScoreboard(Column, TEAM_GAME, i * PlayersPerColumn, (i + 1) * PlayersPerColumn, RenderState);
			}
		}
	}

	CUIRect Spectators = {(Screen.w - ScoreboardSmallWidth) / 2.0f, Scoreboard.y + Scoreboard.h + 5.0f, ScoreboardSmallWidth, 100.0f};
	if(pGameInfoObj && (pGameInfoObj->m_ScoreLimit || pGameInfoObj->m_TimeLimit || (pGameInfoObj->m_RoundNum && pGameInfoObj->m_RoundCurrent)))
	{
		CUIRect Goals;
		Spectators.HSplitTop(25.0f, &Goals, &Spectators);
		Spectators.HSplitTop(5.0f, nullptr, &Spectators);
		RenderGoals(Goals);
	}
	RenderSpectators(Spectators);

	Graphics()->MapScreen(UiProjection);
	RenderRecordingNotification((Screen.w / 7) * 4 + 10);

	if(!GameClient()->m_Menus.IsActive() && !GameClient()->m_Chat.IsActive())
	{
		Ui()->RenderPopupMenus();

		// AxionOverlay draws the shared cursor after the scoreboard/HUD.

		Ui()->FinishCheck();
	}
}

bool CScoreboard::IsActive() const
{
	// if statboard is active don't show scoreboard
	if(GameClient()->m_Statboard.IsActive())
		return false;

	if(m_Active)
		return true;

	const CNetObj_GameInfo *pGameInfoObj = GameClient()->m_Snap.m_pGameInfoObj;
	if(GameClient()->m_Snap.m_pLocalInfo && !GameClient()->m_Snap.m_SpecInfo.m_Active)
	{
		// we are not a spectator, check if we are dead and the game isn't paused
		if(!GameClient()->m_Snap.m_pLocalCharacter && g_Config.m_ClScoreboardOnDeath &&
			!(pGameInfoObj && pGameInfoObj->m_GameStateFlags & GAMESTATEFLAG_PAUSED))
			return true;
	}

	// if the game is over
	if(pGameInfoObj && pGameInfoObj->m_GameStateFlags & GAMESTATEFLAG_GAMEOVER)
		return true;

	return false;
}

const char *CScoreboard::GetTeamName(int Team) const
{
	dbg_assert(Team == TEAM_RED || Team == TEAM_BLUE, "Team invalid");

	int ClanPlayers = 0;
	const char *pClanName = nullptr;
	for(const CNetObj_PlayerInfo *pInfo : GameClient()->m_Snap.m_apInfoByScore)
	{
		if(!pInfo || pInfo->m_Team != Team)
			continue;

		if(!pClanName)
		{
			pClanName = GameClient()->m_aClients[pInfo->m_ClientId].m_aClan;
			ClanPlayers++;
		}
		else
		{
			if(str_comp(GameClient()->m_aClients[pInfo->m_ClientId].m_aClan, pClanName) == 0)
				ClanPlayers++;
			else
				return nullptr;
		}
	}

	if(ClanPlayers > 1 && pClanName[0] != '\0')
		return pClanName;
	else
		return nullptr;
}

CUi::EPopupMenuFunctionResult CScoreboard::CScoreboardPopupContext::Render(void *pContext, CUIRect View, bool Active)
{
	auto *p = static_cast<CScoreboardPopupContext *>(pContext);
	auto *pScore = p->m_pScoreboard;
	auto *pGame = pScore->GameClient();
	auto *pUi = pScore->Ui();
	if(p->m_ClientId < 0 || p->m_ClientId >= MAX_CLIENTS || p->m_Connection != g_Config.m_ClDummy)
		return CUi::POPUP_CLOSE_CURRENT;
	auto &Player = pGame->m_aClients[p->m_ClientId];
	if(!Player.m_Active || str_comp(Player.m_aName, p->m_aTargetName) != 0)
		return CUi::POPUP_CLOSE_CURRENT;
	const float Delta = pScore->Client()->RenderFrameTime();
	p->m_Animation += ((p->m_Closing ? 0.0f : 1.0f) - p->m_Animation) * AxionTransition(Delta, p->m_Closing ? 80.0f : 140.0f);
	if(p->m_Closing && p->m_Animation < 0.02f)
		return CUi::POPUP_CLOSE_CURRENT;
	const float Alpha = p->m_Animation;
	CUIRect Info;
	View.VSplitRight(148, &View, &Info);
	View.VSplitRight(7, &View, nullptr);
	const int TargetTeam = pGame->m_Teams.Team(p->m_ClientId);
	const bool ShowJoin = TargetTeam > 0 && TargetTeam < TEAM_SUPER;
	View.h = PlayerPanelHeight(ShowJoin ? 7 : 6);
	const auto Panel = [](CUIRect &Rect, float Opacity) {
		Rect.Draw(ColorRGBA(0.38f, 0.38f, 0.38f, Opacity), IGraphics::CORNER_ALL, 6);
		Rect.Margin(0.7f, &Rect);
		Rect.Draw(ColorRGBA(0.065f, 0.065f, 0.07f, Opacity), IGraphics::CORNER_ALL, 5.3f);
		Rect.Margin(PlayerPanelInset - 0.7f, &Rect);
	};
	Panel(View, Alpha);
	CUIRect Header, Toggle;
	View.HSplitTop(PlayerHeaderHeight, &Header, &View);
	Header.VSplitRight(22, &Header, &Toggle);
	static CButtonContainer s_Toggle;
	if(Active && !p->m_Closing && pUi->DoButtonLogic(&s_Toggle, 0, &Toggle, BUTTONFLAG_LEFT))
		s_ShowPlayerInfo = !s_ShowPlayerInfo;
	for(int Line = 0; Line < 3; ++Line)
	{
		CUIRect Bar = Toggle;
		Bar.VMargin(4, &Bar);
		Bar.y += 3 + 4 * Line;
		Bar.h = 1.4f;
		Bar.Draw(ColorRGBA(1, 1, 1, Alpha), IGraphics::CORNER_ALL, 0.7f);
	}
	s_PlayerInfoProgress += ((s_ShowPlayerInfo ? 1.0f : 0.0f) - s_PlayerInfoProgress) * AxionTransition(Delta, 95.0f);
	if(s_PlayerInfoProgress > 0.01f)
	{
		const float InfoAlpha = Alpha * s_PlayerInfoProgress;
		Info.x += 4 * (1 - s_PlayerInfoProgress);
		Info.y += 3;
		Info.h = 2 * PlayerPanelInset + 18 + 2 * (21 + 3) + 9 + 20;
		const float BaseX = Info.x + 0.7f;
		const float TipX = View.x + View.w + PlayerPanelInset;
		const float CenterY = Toggle.y + Toggle.h * 0.5f;
		Panel(Info, InfoAlpha);
		auto *pGraphics = pScore->Graphics();
		pGraphics->TextureClear();
		pGraphics->QuadsBegin();
		const auto Triangle = [&](float Tip, float HalfHeight, ColorRGBA Color) {
			IGraphics::CFreeformItem Shape(Tip, CenterY, BaseX, CenterY - HalfHeight,
				Tip, CenterY, BaseX, CenterY + HalfHeight);
			pGraphics->SetColor(Color);
			pGraphics->QuadsDrawFreeform(&Shape, 1);
		};
		Triangle(TipX, 7, ColorRGBA(0.38f, 0.38f, 0.38f, InfoAlpha));
		Triangle(TipX + 1.1f, 6, ColorRGBA(0.065f, 0.065f, 0.07f, InfoAlpha));
		pGraphics->QuadsEnd();
		pScore->TextRender()->TextColor(1, 1, 1, InfoAlpha);
		CUIRect Row;
		Info.HSplitTop(18, &Row, &Info);
		pUi->DoLabel(&Row, Localize("Player Info"), 11, TEXTALIGN_ML);
		const bool Own = p->m_ClientId == pGame->m_aLocalIds[g_Config.m_ClDummy] && pScore->Client()->State() == IClient::STATE_ONLINE;
		const float Distance = Own ? static_cast<float>(g_Config.m_ClDyncam ? g_Config.m_ClDyncamMaxDistance : g_Config.m_ClMouseMaxDistance) :
			s_aaAimPeaks[g_Config.m_ClDummy][p->m_ClientId].m_Distance;
		const bool HasAim = Own || Distance > 0;
		char aValue[48];
		if(HasAim)
			str_format(aValue, sizeof(aValue), "%.0f", Distance);
		else
			str_copy(aValue, "-");
		const auto Field = [&](const char *pName, const char *pValue, bool Mouse) {
			Info.HSplitTop(21, &Row, &Info);
			Row.Draw(ColorRGBA(0.16f, 0.16f, 0.17f, InfoAlpha), IGraphics::CORNER_ALL, 3);
			Row.VMargin(4, &Row);
			CUIRect Icon;
			Row.VSplitLeft(15, &Icon, &Row);
			const float X = Icon.x + 6, Y = Icon.y + Icon.h / 2;
			const ColorRGBA Ink(1, 1, 1, InfoAlpha);
			const auto Stroke = [&](float Dx, float Dy, float W, float H, float Radius, ColorRGBA Color) {
				CUIRect Shape = Icon;
				Shape.x = X + Dx;
				Shape.y = Y + Dy;
				Shape.w = W;
				Shape.h = H;
				Shape.Draw(Color, IGraphics::CORNER_ALL, Radius);
			};
			const float H = Mouse ? 5 : 4;
			Stroke(-4, -H, 8, 2 * H, 4, Ink);
			Stroke(-3, 1 - H, 6, 2 * H - 2, 3, ColorRGBA(0.16f, 0.16f, 0.17f, InfoAlpha));
			if(Mouse)
				Stroke(-0.5f, -4, 1, 3, 0, Ink);
			else
			{
				for(float Side : {-1.0f, 1.0f})
				{
					Stroke(4 * Side - 1.5f, -0.5f, 3, 1, 0, Ink);
					Stroke(-0.5f, 4 * Side - 1.5f, 1, 3, 0, Ink);
				}
			}
			pUi->DoLabel(&Row, pName, 8, TEXTALIGN_ML);
			pUi->DoLabel(&Row, pValue, 8, TEXTALIGN_MR);
			Info.HSplitTop(3, nullptr, &Info);
		};
		Field(Own ? Localize("Max cursor distance") : Localize("Observed max distance"), aValue, false);
		if(Own)
			str_format(aValue, sizeof(aValue), "%d", g_Config.m_InpMousesens);
		else
			str_copy(aValue, Localize("Unknown"));
		Field(Localize("Sensitivity"), aValue, true);
		Info.HSplitTop(9, &Row, &Info);
		pUi->DoLabel(&Row, Localize("Remote sensitivity cannot be measured"), 6, TEXTALIGN_ML);
		Info.HSplitTop(20, &Row, &Info);
		static CButtonContainer s_ApplyDistance;
		const int Limit = g_Config.m_ClDyncam ? 2000 : 5000;
		if(pUi->DoButton_PopupMenu(&s_ApplyDistance, Localize("Copy cursor limit"), &Row, 8, TEXTALIGN_MC, 3, false,
			Active && s_ShowPlayerInfo && !p->m_Closing && HasAim && Distance <= Limit, ColorRGBA(0.22f, 0.23f, 0.25f, InfoAlpha)))
		{
			int &Setting = g_Config.m_ClDyncam ? g_Config.m_ClDyncamMaxDistance : g_Config.m_ClMouseMaxDistance;
			Setting = round_to_int(Distance);
			p->m_Closing = true;
		}
	}
	CUIRect Label = Header;
	pScore->TextRender()->TextColor(1, 1, 1, Alpha);
	SLabelProperties Props;
	Props.m_MaxWidth = Label.w;
	Props.m_EllipsisAtEnd = true;
	pUi->DoLabel(&Label, Player.m_aName, 11, TEXTALIGN_ML, Props);
	View.HSplitTop(4, nullptr, &View);
	const int Local = pGame->m_aLocalIds[g_Config.m_ClDummy];
	const int LocalTeam = Local >= 0 ? pGame->m_Teams.Team(Local) : 0;
	const bool Live = pScore->Client()->State() == IClient::STATE_ONLINE;
	const bool TeamServer = pGame->m_GameInfo.m_EntitiesDDRace;
	const bool Other = Local != p->m_ClientId;
	char aJoin[48];
	str_format(aJoin, sizeof(aJoin), "Join Team %d", TargetTeam);
	const char *apLabels[] = {"Invite to Team", aJoin, "Copy Nickname", "Copy Skin", Player.m_Friend ? "Remove Friend" : "Add Friend", "Spectate", Player.m_ChatIgnore ? "Unmute" : "Mute"};
	bool aEnabled[] = {Live && TeamServer && Other && LocalTeam > 0 && LocalTeam < TEAM_SUPER && LocalTeam != TargetTeam,
		Live && TeamServer && Other && TargetTeam > 0 && TargetTeam < TEAM_SUPER && TargetTeam != LocalTeam,
		true, Live, Other, Other && (pGame->m_Snap.m_SpecInfo.m_Active || TeamServer), Other};
	for(int Index = 0; Index < static_cast<int>(std::size(apLabels)); ++Index)
	{
		if(Index == 1 && !ShowJoin)
			continue;
		CUIRect Button;
		View.HSplitTop(PlayerActionHeight, &Button, &View);
		if(Index + 1 < static_cast<int>(std::size(apLabels)))
			View.HSplitTop(PlayerActionGap, nullptr, &View);
		p->m_aHover[Index] += ((Active && Button.Inside(pUi->MousePos()) ? 1.0f : 0.0f) - p->m_aHover[Index]) * AxionTransition(Delta);
		const float Shade = 0.12f + 0.08f * p->m_aHover[Index];
		const bool Click = pUi->DoButton_PopupMenu(&p->m_aActions[Index], Localize(apLabels[Index]), &Button, 10, TEXTALIGN_ML, 6, false, aEnabled[Index] && !p->m_Closing, ColorRGBA(Shade, Shade, Shade + 0.01f, Alpha));
		if(!Click)
			continue;
		char aCommand[128];
		switch(Index)
		{
		case 0:
			str_format(aCommand, sizeof(aCommand), "/invite %s", Player.m_aName);
			pGame->m_Chat.SendChat(0, aCommand);
			break;
		case 1:
			str_format(aCommand, sizeof(aCommand), "/team %d", TargetTeam);
			pGame->m_Chat.SendChat(0, aCommand);
			break;
		case 2: pScore->Input()->SetClipboardText(Player.m_aName); break;
		case 3:
			if(pScore->Client()->IsSixup())
			{
				const auto &Skin = Player.m_aSixup[g_Config.m_ClDummy];
				for(int Part = 0; Part < protocol7::NUM_SKINPARTS; ++Part)
				{
					str_copy(CSkins7::ms_apSkinVariables[g_Config.m_ClDummy][Part], Skin.m_aaSkinPartNames[Part], protocol7::MAX_SKIN_LENGTH);
					*CSkins7::ms_apUCCVariables[g_Config.m_ClDummy][Part] = Skin.m_aUseCustomColors[Part];
					*CSkins7::ms_apColorVariables[g_Config.m_ClDummy][Part] = Skin.m_aSkinPartColors[Part];
				}
			}
			else if(g_Config.m_ClDummy)
			{
				str_copy(g_Config.m_ClDummySkin, Player.m_aSkinName);
				g_Config.m_ClDummyUseCustomColor = Player.m_UseCustomColor;
				g_Config.m_ClDummyColorBody = Player.m_ColorBody;
				g_Config.m_ClDummyColorFeet = Player.m_ColorFeet;
			}
			else
			{
				str_copy(g_Config.m_ClPlayerSkin, Player.m_aSkinName);
				g_Config.m_ClPlayerUseCustomColor = Player.m_UseCustomColor;
				g_Config.m_ClPlayerColorBody = Player.m_ColorBody;
				g_Config.m_ClPlayerColorFeet = Player.m_ColorFeet;
			}
			if(g_Config.m_ClDummy)
				pGame->SendDummyInfo(false);
			else
				pGame->SendInfo(false);
			break;
		case 4:
			if(Player.m_Friend)
				pGame->Friends()->RemoveFriend(Player.m_aName, Player.m_aClan);
			else
				pGame->Friends()->AddFriend(Player.m_aName, Player.m_aClan);
			Player.m_Friend = !Player.m_Friend;
			break;
		case 5:
			if(pGame->m_Snap.m_SpecInfo.m_Active)
				pGame->m_Spectator.Spectate(p->m_ClientId);
			else
			{
				str_format(aCommand, sizeof(aCommand), "/spec %s", Player.m_aName);
				pGame->m_Chat.SendChat(0, aCommand);
			}
			break;
		case 6: Player.m_ChatIgnore ^= 1; break;
		}
		p->m_Closing = true;
	}
	pScore->TextRender()->TextColor(pScore->TextRender()->DefaultTextColor());
	return CUi::POPUP_KEEP_OPEN;
}

CUi::EPopupMenuFunctionResult CScoreboard::CMapTitlePopupContext::Render(void *pContext, CUIRect View, bool Active)
{
	CMapTitlePopupContext *pPopupContext = static_cast<CMapTitlePopupContext *>(pContext);
	CScoreboard *pScoreboard = pPopupContext->m_pScoreboard;

	pScoreboard->TextRender()->Text(View.x, View.y, pPopupContext->m_FontSize, pScoreboard->GameClient()->m_aMapDescription, View.w);

	return CUi::POPUP_KEEP_OPEN;
}
