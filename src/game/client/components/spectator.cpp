/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include "spectator.h"

#include "camera.h"

#include <base/net.h>
#include <engine/graphics.h>
#include <engine/shared/config.h>
#include <engine/textrender.h>

#include <generated/protocol.h>

#include <game/client/animstate.h>
#include <game/client/gameclient.h>
#include <game/mapitems.h>
#include <game/localization.h>

#include <limits>

bool CSpectator::CanChangeSpectatorId()
{
	// don't change SpectatorId when not spectating
	if(!GameClient()->m_Snap.m_SpecInfo.m_Active)
		return false;

	// stop follow mode from changing SpectatorId
	if(Client()->State() == IClient::STATE_DEMOPLAYBACK && GameClient()->m_DemoSpecId == SPEC_FOLLOW)
		return false;

	return true;
}

void CSpectator::SpectateNext(bool Reverse)
{
	int CurIndex = -1;
	const CNetObj_PlayerInfo **paPlayerInfos = GameClient()->m_Snap.m_apInfoByDDTeamName;

	// m_SpectatorId may be uninitialized if m_Active is false
	if(GameClient()->m_Snap.m_SpecInfo.m_Active)
	{
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(paPlayerInfos[i] && paPlayerInfos[i]->m_ClientId == GameClient()->m_Snap.m_SpecInfo.m_SpectatorId)
			{
				CurIndex = i;
				break;
			}
		}
	}

	int Start;
	if(CurIndex != -1)
	{
		if(Reverse)
			Start = CurIndex - 1;
		else
			Start = CurIndex + 1;
	}
	else
	{
		if(Reverse)
			Start = -1;
		else
			Start = 0;
	}

	int Increment = Reverse ? -1 : 1;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		int PlayerIndex = (Start + i * Increment) % MAX_CLIENTS;
		// % in C++ takes the sign of the dividend, not divisor
		if(PlayerIndex < 0)
			PlayerIndex += MAX_CLIENTS;

		const CNetObj_PlayerInfo *pPlayerInfo = paPlayerInfos[PlayerIndex];
		if(pPlayerInfo && pPlayerInfo->m_Team != TEAM_SPECTATORS)
		{
			Spectate(pPlayerInfo->m_ClientId);
			break;
		}
	}
}

void CSpectator::ConKeySpectator(IConsole::IResult *pResult, void *pUserData)
{
	CSpectator *pSelf = (CSpectator *)pUserData;

	if(pSelf->GameClient()->m_Scoreboard.IsActive())
		return;

	if(pSelf->GameClient()->m_Snap.m_SpecInfo.m_Active || pSelf->Client()->State() == IClient::STATE_DEMOPLAYBACK)
		pSelf->m_Active = pResult->GetInteger(0) != 0;
	else
		pSelf->m_Active = false;
}

void CSpectator::ConSpectate(IConsole::IResult *pResult, void *pUserData)
{
	CSpectator *pSelf = (CSpectator *)pUserData;
	if(!pSelf->CanChangeSpectatorId())
		return;

	pSelf->Spectate(pResult->GetInteger(0));
}

void CSpectator::ConSpectateNext(IConsole::IResult *pResult, void *pUserData)
{
	CSpectator *pSelf = (CSpectator *)pUserData;
	if(!pSelf->CanChangeSpectatorId())
		return;

	pSelf->SpectateNext(false);
}

void CSpectator::ConSpectatePrevious(IConsole::IResult *pResult, void *pUserData)
{
	CSpectator *pSelf = (CSpectator *)pUserData;
	if(!pSelf->CanChangeSpectatorId())
		return;

	pSelf->SpectateNext(true);
}

void CSpectator::ConSpectateClosest(IConsole::IResult *pResult, void *pUserData)
{
	CSpectator *pSelf = (CSpectator *)pUserData;
	pSelf->SpectateClosest();
}

void CSpectator::ConMultiView(IConsole::IResult *pResult, void *pUserData)
{
	CSpectator *pSelf = (CSpectator *)pUserData;
	int Input = pResult->GetInteger(0);

	if(Input == -1)
		std::fill(std::begin(pSelf->GameClient()->m_aMultiViewId), std::end(pSelf->GameClient()->m_aMultiViewId), false); // remove everyone from multiview
	else if(Input < MAX_CLIENTS && Input >= 0)
		pSelf->GameClient()->m_aMultiViewId[Input] = !pSelf->GameClient()->m_aMultiViewId[Input]; // activate or deactivate one player from multiview
}

CSpectator::CSpectator()
{
	m_SelectorMouse = vec2(0.0f, 0.0f);
	CSpectator::OnReset();
}

void CSpectator::OnConsoleInit()
{
	ConfigManager()->RegisterCallback([](IConfigManager *pConfig, void *pUserData) {
		const auto *pSelf = static_cast<CSpectator *>(pUserData);
		for(const auto &Record : pSelf->m_vCheatRecords)
		{
			char aName[128], aServer[192], aLine[400];
			char *pName = aName, *pServer = aServer;
			str_escape(&pName, Record.m_aName, aName + sizeof(aName));
			str_escape(&pServer, Record.m_aServer, aServer + sizeof(aServer));
			str_format(aLine, sizeof(aLine), "ax_cheat_record \"%s\" \"%s\" %d %d %d %d %d %d %d",
				aName, aServer, Record.m_Score, Record.m_Aimbot, Record.m_Wall, Record.m_Avoid, Record.m_AimReturns, Record.m_WallHooks, Record.m_HookAssist);
			pConfig->WriteLine(aLine);
		}
	}, this);
	Console()->Register("ax_cheat_record", "s[name] s[server] i[score] i[aim] i[wall] i[avoid] i[returns] ?i[wall-hooks] ?i[hook-assist]",
		CFGFLAG_CLIENT, [](IConsole::IResult *pResult, void *pUserData) {
			auto *pSelf = static_cast<CSpectator *>(pUserData);
			if(pSelf->m_vCheatRecords.size() >= 512)
				return;
			auto &Record = pSelf->m_vCheatRecords.emplace_back();
			str_copy(Record.m_aName, pResult->GetString(0));
			str_copy(Record.m_aServer, pResult->GetString(1));
			Record.m_Score = std::clamp(pResult->GetInteger(2), 0, 95);
			Record.m_Aimbot = std::max(0, pResult->GetInteger(3));
			Record.m_Wall = std::max(0, pResult->GetInteger(4));
			Record.m_Avoid = std::max(0, pResult->GetInteger(5));
			Record.m_AimReturns = std::max(0, pResult->GetInteger(6));
			Record.m_WallHooks = pResult->NumArguments() > 7 ? std::max(0, pResult->GetInteger(7)) : 0;
			Record.m_HookAssist = pResult->NumArguments() > 8 ? std::max(0, pResult->GetInteger(8)) : 0;
		}, this, "Restore a local CheatCheck history entry");

	Console()->Register("ax_cheat_check", "?i[client-id]", CFGFLAG_CLIENT, [](IConsole::IResult *pResult, void *pUserData) {
		auto *pSelf = static_cast<CSpectator *>(pUserData);
		pSelf->StartCheatCheck(pResult->NumArguments() ? pResult->GetInteger(0) : pSelf->GameClient()->m_Snap.m_SpecInfo.m_SpectatorId);
	}, this, "Analyze a player or the current spectator target (use -1 to stop)");
	Console()->Register("ax_ghost_spectate", "", CFGFLAG_CLIENT, [](IConsole::IResult *, void *pUserData) {
		static_cast<CSpectator *>(pUserData)->ToggleGhostSpectate();
	}, this, "Toggle the local spectator camera and suspend controls");
	Console()->Register("+spectate", "", CFGFLAG_CLIENT, ConKeySpectator, this, "Open spectator mode selector");
	Console()->Register("spectate", "i[spectator-id]", CFGFLAG_CLIENT, ConSpectate, this, "Switch spectator mode");
	Console()->Register("spectate_next", "", CFGFLAG_CLIENT, ConSpectateNext, this, "Spectate the next player");
	Console()->Register("spectate_previous", "", CFGFLAG_CLIENT, ConSpectatePrevious, this, "Spectate the previous player");
	Console()->Register("spectate_closest", "", CFGFLAG_CLIENT, ConSpectateClosest, this, "Spectate the closest player");
	Console()->Register("spectate_multiview", "i[id]", CFGFLAG_CLIENT, ConMultiView, this, "Add/remove Client-IDs to spectate them exclusively (-1 to reset)");
}

bool CSpectator::OnCursorMove(float x, float y, IInput::ECursorType CursorType)
{
	if(!m_Active)
		return false;

	Ui()->ConvertMouseMove(&x, &y, CursorType);
	m_SelectorMouse += vec2(x, y);
	return true;
}

bool CSpectator::OnInput(const IInput::CEvent &Event)
{
	if(IsActive() && Event.m_Flags & IInput::FLAG_PRESS && Event.m_Key == KEY_ESCAPE)
	{
		OnRelease();
		return true;
	}

	if(g_Config.m_ClSpectatorMouseclicks)
	{
		if(GameClient()->m_Snap.m_SpecInfo.m_Active && !IsActive() && !GameClient()->m_MultiViewActivated &&
			!Ui()->IsPopupOpen() && !GameClient()->m_GameConsole.IsActive() && !GameClient()->m_Menus.IsActive() &&
			!GameClient()->m_Chat.IsActive() && !GameClient()->m_Scoreboard.CursorActive())
		{
			if(Event.m_Flags & IInput::FLAG_PRESS && Event.m_Key == KEY_MOUSE_1)
			{
				if(GameClient()->m_Snap.m_SpecInfo.m_SpectatorId != SPEC_FREEVIEW)
					Spectate(SPEC_FREEVIEW);
				else
					SpectateClosest();
				return true;
			}
		}
	}

	if(GameClient()->m_Camera.SpectatingPlayer() && GameClient()->m_Camera.CanUseAutoSpecCamera())
	{
		if(Event.m_Flags & IInput::FLAG_PRESS && Event.m_Key == KEY_MOUSE_2)
		{
			GameClient()->m_Camera.ResetAutoSpecCamera();
			return true;
		}
	}

	return false;
}

void CSpectator::OnRelease()
{
	m_WasActive = false;
	m_Active = false;
	m_SelectedSpectatorId = NO_SELECTION;
	m_MultiViewActivateDelay = 0.0f;
}

void CSpectator::OnRender()
{
	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return;

	RenderCheatCheck();

	if(!GameClient()->m_MultiViewActivated && m_MultiViewActivateDelay != 0.0f)
	{
		if(m_MultiViewActivateDelay <= Client()->LocalTime())
		{
			m_MultiViewActivateDelay = 0.0f;
			GameClient()->m_MultiViewActivated = true;
		}
	}

	if(!m_Active)
	{
		// closing the spectator menu
		if(m_WasActive)
		{
			if(m_SelectedSpectatorId != NO_SELECTION)
			{
				if(m_SelectedSpectatorId == MULTI_VIEW)
					GameClient()->m_MultiViewActivated = true;
				else if(m_SelectedSpectatorId == SPEC_FREEVIEW || m_SelectedSpectatorId == SPEC_FOLLOW)
					GameClient()->m_MultiViewActivated = false;

				if(!GameClient()->m_MultiViewActivated)
					Spectate(m_SelectedSpectatorId);

				if(GameClient()->m_MultiViewActivated && m_SelectedSpectatorId != MULTI_VIEW && GameClient()->m_Teams.Team(m_SelectedSpectatorId) != GameClient()->m_MultiViewTeam)
				{
					GameClient()->ResetMultiView();
					Spectate(m_SelectedSpectatorId);
					m_MultiViewActivateDelay = Client()->LocalTime() + 0.3f;
				}
			}
			m_WasActive = false;
		}
		return;
	}

	if(!GameClient()->m_Snap.m_SpecInfo.m_Active && Client()->State() != IClient::STATE_DEMOPLAYBACK)
	{
		m_Active = false;
		m_WasActive = false;
		return;
	}

	m_WasActive = true;
	m_SelectedSpectatorId = NO_SELECTION;

	// draw background
	float Width = 400 * 3.0f * Graphics()->ScreenAspect();
	float Height = 400 * 3.0f;
	float ObjWidth = 300.0f;
	float FontSize = 20.0f;
	float BigFontSize = 20.0f;
	float StartY = -190.0f;
	float LineHeight = 60.0f;
	float TeeSizeMod = 1.0f;
	float RoundRadius = 30.0f;
	bool MultiViewSelected = false;
	int TotalPlayers = 0;
	int PerLine = 8;
	float BoxMove = -10.0f;
	float BoxOffset = 0.0f;

	const auto &apPlayerInfos = GameClient()->m_Snap.m_apInfoByDDTeamName;
	for(const auto &pInfo : apPlayerInfos)
	{
		if(!pInfo || pInfo->m_Team == TEAM_SPECTATORS)
			continue;

		++TotalPlayers;
	}

	if(TotalPlayers > 96)
	{
		FontSize = 15.0f;
		LineHeight = 15.0f;
		TeeSizeMod = 0.3f;
		PerLine = 32;
		RoundRadius = 5.0f;
		BoxMove = 3.0f;
		BoxOffset = 6.0f;
	}
	else if(TotalPlayers > 64)
	{
		FontSize = 16.0f;
		LineHeight = 19.0f;
		TeeSizeMod = 0.45f;
		PerLine = 24;
		RoundRadius = 6.0f;
		BoxMove = 3.0f;
		BoxOffset = 6.0f;
	}
	else if(TotalPlayers > 32)
	{
		FontSize = 18.0f;
		LineHeight = 30.0f;
		TeeSizeMod = 0.7f;
		PerLine = 16;
		RoundRadius = 10.0f;
		BoxMove = 3.0f;
		BoxOffset = 6.0f;
	}
	if(TotalPlayers > 16)
	{
		ObjWidth = 600.0f;
	}

	const vec2 ScreenSize = vec2(Width, Height);
	const vec2 ScreenCenter = ScreenSize / 2.0f;
	CUIRect SpectatorRect = {Width / 2.0f - ObjWidth, Height / 2.0f - 300.0f, ObjWidth * 2.0f, 600.0f};
	CUIRect SpectatorMouseRect;
	SpectatorRect.Margin(20.0f, &SpectatorMouseRect);

	const bool WasTouchPressed = m_TouchState.m_AnyPressed;
	Ui()->UpdateTouchState(m_TouchState);
	if(m_TouchState.m_AnyPressed)
	{
		const vec2 TouchPos = (m_TouchState.m_PrimaryPosition - vec2(0.5f, 0.5f)) * ScreenSize;
		if(SpectatorMouseRect.Inside(ScreenCenter + TouchPos))
		{
			m_SelectorMouse = TouchPos;
		}
	}
	else if(WasTouchPressed)
	{
		const vec2 TouchPos = (m_TouchState.m_PrimaryPosition - vec2(0.5f, 0.5f)) * ScreenSize;
		if(!SpectatorRect.Inside(ScreenCenter + TouchPos))
		{
			OnRelease();
			return;
		}
	}

	Graphics()->MapScreenToSize(Width, Height);

	SpectatorRect.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.3f), IGraphics::CORNER_ALL, 20.0f);

	// clamp mouse position to selector area
	m_SelectorMouse.x = std::clamp(m_SelectorMouse.x, -(ObjWidth - 20.0f), ObjWidth - 20.0f);
	m_SelectorMouse.y = std::clamp(m_SelectorMouse.y, -280.0f, 280.0f);

	const bool MousePressed = Input()->KeyPress(KEY_MOUSE_1) || m_TouchState.m_PrimaryPressed;

	// draw selections
	if((Client()->State() == IClient::STATE_DEMOPLAYBACK && GameClient()->m_DemoSpecId == SPEC_FREEVIEW) ||
		(Client()->State() != IClient::STATE_DEMOPLAYBACK && GameClient()->m_Snap.m_SpecInfo.m_SpectatorId == SPEC_FREEVIEW))
	{
		Graphics()->DrawRect(Width / 2.0f - (ObjWidth - 20.0f), Height / 2.0f - 280.0f, ((ObjWidth * 2.0f) / 3.0f) - 40.0f, 60.0f, ColorRGBA(1.0f, 1.0f, 1.0f, 0.25f), IGraphics::CORNER_ALL, 20.0f);
	}

	if(GameClient()->m_MultiViewActivated)
	{
		Graphics()->DrawRect(Width / 2.0f - (ObjWidth - 20.0f) + (ObjWidth * 2.0f / 3.0f), Height / 2.0f - 280.0f, ((ObjWidth * 2.0f) / 3.0f) - 40.0f, 60.0f, ColorRGBA(1.0f, 1.0f, 1.0f, 0.25f), IGraphics::CORNER_ALL, 20.0f);
	}

	if(Client()->State() == IClient::STATE_DEMOPLAYBACK && GameClient()->m_Snap.m_LocalClientId >= 0 && GameClient()->m_DemoSpecId == SPEC_FOLLOW)
	{
		Graphics()->DrawRect(Width / 2.0f - (ObjWidth - 20.0f) + (ObjWidth * 2.0f * 2.0f / 3.0f), Height / 2.0f - 280.0f, ((ObjWidth * 2.0f) / 3.0f) - 40.0f, 60.0f, ColorRGBA(1.0f, 1.0f, 1.0f, 0.25f), IGraphics::CORNER_ALL, 20.0f);
	}

	bool FreeViewSelected = false;
	if(m_SelectorMouse.x >= -(ObjWidth - 20.0f) && m_SelectorMouse.x <= -(ObjWidth - 20.0f) + ((ObjWidth * 2.0f) / 3.0f) - 40.0f &&
		m_SelectorMouse.y >= -280.0f && m_SelectorMouse.y <= -220.0f)
	{
		m_SelectedSpectatorId = SPEC_FREEVIEW;
		FreeViewSelected = true;
		if(MousePressed)
		{
			GameClient()->m_MultiViewActivated = false;
			Spectate(m_SelectedSpectatorId);
		}
	}
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, FreeViewSelected ? 1.0f : 0.5f);
	TextRender()->Text(Width / 2.0f - (ObjWidth - 40.0f), Height / 2.0f - 280.f + (60.f - BigFontSize) / 2.f, BigFontSize, Localize("Free-View"), -1.0f);

	if(m_SelectorMouse.x >= -(ObjWidth - 20.0f) + (ObjWidth * 2.0f / 3.0f) && m_SelectorMouse.x <= -(ObjWidth - 20.0f) + (ObjWidth * 2.0f / 3.0f) + ((ObjWidth * 2.0f) / 3.0f) - 40.0f &&
		m_SelectorMouse.y >= -280.0f && m_SelectorMouse.y <= -220.0f)
	{
		m_SelectedSpectatorId = MULTI_VIEW;
		MultiViewSelected = true;
		if(MousePressed)
		{
			GameClient()->m_MultiViewActivated = true;
		}
	}
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, MultiViewSelected ? 1.0f : 0.5f);
	TextRender()->Text(Width / 2.0f - (ObjWidth - 40.0f) + (ObjWidth * 2.0f / 3.0f), Height / 2.0f - 280.f + (60.f - BigFontSize) / 2.f, BigFontSize, Localize("Multi-View"), -1.0f);

	if(Client()->State() == IClient::STATE_DEMOPLAYBACK && GameClient()->m_Snap.m_LocalClientId >= 0)
	{
		bool FollowSelected = false;
		if(m_SelectorMouse.x >= -(ObjWidth - 20.0f) + (ObjWidth * 2.0f * 2.0f / 3.0f) && m_SelectorMouse.x <= -(ObjWidth - 20.0f) + (ObjWidth * 2.0f * 2.0f / 3.0f) + ((ObjWidth * 2.0f) / 3.0f) - 40.0f &&
			m_SelectorMouse.y >= -280.0f && m_SelectorMouse.y <= -220.0f)
		{
			m_SelectedSpectatorId = SPEC_FOLLOW;
			FollowSelected = true;
			if(MousePressed)
			{
				GameClient()->m_MultiViewActivated = false;
				Spectate(m_SelectedSpectatorId);
			}
		}
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, FollowSelected ? 1.0f : 0.5f);
		TextRender()->Text(Width / 2.0f - (ObjWidth - 40.0f) + (ObjWidth * 2.0f * 2.0f / 3.0f), Height / 2.0f - 280.0f + (60.f - BigFontSize) / 2.f, BigFontSize, Localize("Follow"), -1.0f);
	}

	float x = -(ObjWidth - 35.0f), y = StartY;

	int OldDDTeam = -1;

	for(int i = 0, Count = 0; i < MAX_CLIENTS; ++i)
	{
		if(!apPlayerInfos[i] || apPlayerInfos[i]->m_Team == TEAM_SPECTATORS)
			continue;

		++Count;

		if(Count == PerLine + 1 || (Count > PerLine + 1 && (Count - 1) % PerLine == 0))
		{
			x += 290.0f;
			y = StartY;
		}

		const CNetObj_PlayerInfo *pInfo = apPlayerInfos[i];
		int DDTeam = GameClient()->m_Teams.Team(pInfo->m_ClientId);
		int NextDDTeam = 0;

		for(int j = i + 1; j < MAX_CLIENTS; j++)
		{
			const CNetObj_PlayerInfo *pInfo2 = apPlayerInfos[j];

			if(!pInfo2 || pInfo2->m_Team == TEAM_SPECTATORS)
				continue;

			NextDDTeam = GameClient()->m_Teams.Team(pInfo2->m_ClientId);
			break;
		}

		if(OldDDTeam == -1)
		{
			for(int j = i - 1; j >= 0; j--)
			{
				const CNetObj_PlayerInfo *pInfo2 = apPlayerInfos[j];

				if(!pInfo2 || pInfo2->m_Team == TEAM_SPECTATORS)
					continue;

				OldDDTeam = GameClient()->m_Teams.Team(pInfo2->m_ClientId);
				break;
			}
		}

		if(DDTeam != TEAM_FLOCK)
		{
			const ColorRGBA Color = GameClient()->GetDDTeamColor(DDTeam).WithAlpha(0.5f);
			int Corners = 0;
			if(OldDDTeam != DDTeam)
				Corners |= IGraphics::CORNER_TL | IGraphics::CORNER_TR;
			if(NextDDTeam != DDTeam)
				Corners |= IGraphics::CORNER_BL | IGraphics::CORNER_BR;
			Graphics()->DrawRect(Width / 2.0f + x - 10.0f + BoxOffset, Height / 2.0f + y + BoxMove, 270.0f - BoxOffset, LineHeight, Color, Corners, RoundRadius);
		}

		OldDDTeam = DDTeam;

		if((Client()->State() == IClient::STATE_DEMOPLAYBACK && GameClient()->m_DemoSpecId == apPlayerInfos[i]->m_ClientId) || (Client()->State() != IClient::STATE_DEMOPLAYBACK && GameClient()->m_Snap.m_SpecInfo.m_SpectatorId == apPlayerInfos[i]->m_ClientId))
		{
			Graphics()->DrawRect(Width / 2.0f + x - 10.0f + BoxOffset, Height / 2.0f + y + BoxMove, 270.0f - BoxOffset, LineHeight, ColorRGBA(1.0f, 1.0f, 1.0f, 0.25f), IGraphics::CORNER_ALL, RoundRadius);
		}

		bool PlayerSelected = false;
		if(m_SelectorMouse.x >= x - 10.0f && m_SelectorMouse.x < x + 260.0f &&
			m_SelectorMouse.y >= y - (LineHeight / 6.0f) && m_SelectorMouse.y < y + (LineHeight * 5.0f / 6.0f))
		{
			m_SelectedSpectatorId = apPlayerInfos[i]->m_ClientId;
			PlayerSelected = true;
			if(MousePressed)
			{
				if(GameClient()->m_MultiViewActivated)
				{
					if(GameClient()->m_MultiViewTeam == DDTeam)
					{
						GameClient()->m_aMultiViewId[m_SelectedSpectatorId] = !GameClient()->m_aMultiViewId[m_SelectedSpectatorId];
						if(!GameClient()->m_aMultiViewId[GameClient()->m_Snap.m_SpecInfo.m_SpectatorId])
						{
							int NewClientId = GameClient()->FindFirstMultiViewId();
							if(NewClientId < MAX_CLIENTS && NewClientId >= 0)
							{
								GameClient()->CleanMultiViewId(NewClientId);
								GameClient()->m_aMultiViewId[NewClientId] = true;
								Spectate(NewClientId);
							}
						}
					}
					else
					{
						GameClient()->ResetMultiView();
						Spectate(m_SelectedSpectatorId);
						m_MultiViewActivateDelay = Client()->LocalTime() + 0.3f;
					}
				}
				else
				{
					Spectate(m_SelectedSpectatorId);
				}
			}
		}
		float TeeAlpha;
		if(Client()->State() == IClient::STATE_DEMOPLAYBACK &&
			!GameClient()->m_Snap.m_aCharacters[apPlayerInfos[i]->m_ClientId].m_Active)
		{
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.25f);
			TeeAlpha = 0.5f;
		}
		else
		{
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, PlayerSelected ? 1.0f : 0.5f);
			TeeAlpha = 1.0f;
		}
		CTextCursor NameCursor;
		NameCursor.SetPosition(vec2(Width / 2.0f + x + 50.0f, Height / 2.0f + y + BoxMove + (LineHeight - FontSize) / 2.f));
		NameCursor.m_FontSize = FontSize;
		NameCursor.m_Flags |= TEXTFLAG_ELLIPSIS_AT_END;
		NameCursor.m_LineWidth = 180.0f;
		if(g_Config.m_ClShowIds)
		{
			char aClientId[16];
			GameClient()->FormatClientId(apPlayerInfos[i]->m_ClientId, aClientId, EClientIdFormat::INDENT_AUTO);
			TextRender()->TextEx(&NameCursor, aClientId);
		}

		// TClient
		if(pInfo && pInfo->m_ClientId > 0 && g_Config.m_TcWarList && g_Config.m_TcWarListSpectate && GameClient()->m_WarList.GetAnyWar(pInfo->m_ClientId))
		{
			TextRender()->TextColor(GameClient()->m_WarList.GetPriorityColor(pInfo->m_ClientId));
		}

		TextRender()->TextEx(&NameCursor, GameClient()->m_aClients[apPlayerInfos[i]->m_ClientId].m_aName);

		if(GameClient()->m_MultiViewActivated)
		{
			if(GameClient()->m_aMultiViewId[apPlayerInfos[i]->m_ClientId])
			{
				TextRender()->TextColor(0.1f, 1.0f, 0.1f, PlayerSelected ? 1.0f : 0.5f);
				TextRender()->Text(Width / 2.0f + x + 50.0f + 180.0f, Height / 2.0f + y + BoxMove + (LineHeight - FontSize) / 2.f, FontSize - 3, "⬤", 220.0f);
			}
			else if(GameClient()->m_MultiViewTeam == DDTeam)
			{
				TextRender()->TextColor(1.0f, 0.1f, 0.1f, PlayerSelected ? 1.0f : 0.5f);
				TextRender()->Text(Width / 2.0f + x + 50.0f + 180.0f, Height / 2.0f + y + BoxMove + (LineHeight - FontSize) / 2.f, FontSize - 3, "◯", 220.0f);
			}
		}

		// flag
		if(GameClient()->m_Snap.m_pGameInfoObj && (GameClient()->m_Snap.m_pGameInfoObj->m_GameFlags & GAMEFLAG_FLAGS) &&
			GameClient()->m_Snap.m_pGameDataObj && (GameClient()->m_Snap.m_pGameDataObj->m_FlagCarrierRed == apPlayerInfos[i]->m_ClientId || GameClient()->m_Snap.m_pGameDataObj->m_FlagCarrierBlue == apPlayerInfos[i]->m_ClientId))
		{
			if(GameClient()->m_Snap.m_pGameDataObj->m_FlagCarrierBlue == apPlayerInfos[i]->m_ClientId)
				Graphics()->TextureSet(GameClient()->m_GameSkin.m_SpriteFlagBlue);
			else
				Graphics()->TextureSet(GameClient()->m_GameSkin.m_SpriteFlagRed);

			Graphics()->QuadsBegin();
			Graphics()->QuadsSetSubset(1, 0, 0, 1);

			float Size = LineHeight;
			IGraphics::CQuadItem QuadItem(Width / 2.0f + x - LineHeight / 5.0f, Height / 2.0f + y - LineHeight / 3.0f, Size / 2.0f, Size);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();
		}

		CTeeRenderInfo TeeInfo = GameClient()->m_aClients[apPlayerInfos[i]->m_ClientId].m_RenderInfo;
		TeeInfo.m_Size *= TeeSizeMod;

		const CAnimState *pIdleState = CAnimState::GetIdle();
		vec2 OffsetToMid;
		CRenderTools::GetRenderTeeOffsetToRenderedTee(pIdleState, &TeeInfo, OffsetToMid);
		vec2 TeeRenderPos(Width / 2.0f + x + 20.0f, Height / 2.0f + y + BoxMove + LineHeight / 2.0f + OffsetToMid.y);

		RenderTools()->RenderTee(pIdleState, &TeeInfo, EMOTE_NORMAL, vec2(1.0f, 0.0f), TeeRenderPos, TeeAlpha);

		if(GameClient()->m_aClients[apPlayerInfos[i]->m_ClientId].m_Friend)
		{
			TextRender()->TextColor(color_cast<ColorRGBA>(ColorHSLA(g_Config.m_ClMessageFriendColor)));
			TextRender()->Text(Width / 2.0f + x - TeeInfo.m_Size / 2.0f, Height / 2.0f + y + BoxMove + (LineHeight - FontSize) / 2.f, FontSize, "♥", 220.0f);
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
		}

		y += LineHeight;
	}
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

	RenderTools()->RenderCursor(ScreenCenter + m_SelectorMouse, 48.0f);
}

void CSpectator::OnReset()
{
	for(auto &Check : m_aCheatChecks)
		Check = {};
	m_CheatHudClient = m_CheatGhostClient = -1;
	m_GhostConnection = -1;
	m_GhostCameraApplied = false;
	OnRelease();
}

bool CSpectator::GhostSpectating() const
{
	return m_GhostConnection == g_Config.m_ClDummy && Client()->State() == IClient::STATE_ONLINE;
}

void CSpectator::StopGhostSpectate()
{
	if(m_GhostCameraApplied)
	{
		auto &Spec = GameClient()->m_Snap.m_SpecInfo;
		Spec = {};
		Spec.m_SpectatorId = SPEC_FREEVIEW;
		Spec.m_Zoom = 1.0f;
	}
	m_GhostCameraApplied = false;
	m_GhostConnection = -1;
	m_CheatGhostClient = -1;
	OnRelease();
	m_WasActive = false;
	GameClient()->ResetMultiView();
	GameClient()->m_Camera.SetZoom(m_ZoomBeforeGhost, g_Config.m_ClSmoothZoomTime, true);
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "axion", "Ghost spectate disabled.");
}

void CSpectator::ToggleGhostSpectate()
{
	auto &Snap = GameClient()->m_Snap;
	if(m_GhostConnection >= 0)
	{
		StopGhostSpectate();
		return;
	}
	if(Client()->State() != IClient::STATE_ONLINE || Snap.m_SpecInfo.m_Active || !Snap.m_pLocalCharacter ||
		GameClient()->m_FastPractice.Enabled())
		return;

	m_GhostConnection = g_Config.m_ClDummy;
	m_GhostTarget = SPEC_FREEVIEW;
	auto &Camera = GameClient()->m_Camera;
	m_ZoomBeforeGhost = Camera.m_UserZoomTarget;
	UpdateGhostSpectate();
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "axion", "Ghost spectate enabled.");
}

void CSpectator::OnNewSnapshot()
{
	// The new snapshot owns this state until we apply the local camera again.
	m_GhostCameraApplied = false;
	UpdateGhostSpectate();
	for(auto &Check : m_aCheatChecks)
		if(Check.m_Enabled)
			SampleCheatCheck(Check);
}

void CSpectator::UpdateCheatRecord(const SCheatCheck &Check)
{
	char aServer[96];
	net_addr_str(&Client()->ServerAddress(), aServer, sizeof(aServer), true);
	auto It = std::find_if(m_vCheatRecords.begin(), m_vCheatRecords.end(), [&](const SCheatRecord &Record) {
		return str_comp(Record.m_aName, Check.m_aName) == 0 && str_comp(Record.m_aServer, aServer) == 0;
	});
	if(It == m_vCheatRecords.end())
	{
		if(m_vCheatRecords.size() >= 512)
			m_vCheatRecords.erase(m_vCheatRecords.begin());
		m_vCheatRecords.emplace_back();
		It = std::prev(m_vCheatRecords.end());
		str_copy(It->m_aName, Check.m_aName);
		str_copy(It->m_aServer, aServer);
	}
	It->m_Score = std::max(It->m_Score, round_to_int(Check.m_Score));
	It->m_Aimbot = std::max(It->m_Aimbot, Check.m_AimbotEvents);
	It->m_Wall = std::max(It->m_Wall, Check.m_WallEvents);
	It->m_WallHooks = std::max(It->m_WallHooks, Check.m_WallHooks);
	It->m_HookAssist = std::max(It->m_HookAssist, Check.m_HookAssistEvents);
	It->m_Avoid = std::max(It->m_Avoid, Check.m_AvoidEvents);
	It->m_AimReturns = std::max(It->m_AimReturns, Check.m_AimReturns);
}

void CSpectator::DeleteCheatRecord(int Index)
{
	if(Index < 0 || Index >= (int)m_vCheatRecords.size())
		return;
	const auto Record = m_vCheatRecords[Index];
	m_vCheatRecords.erase(m_vCheatRecords.begin() + Index);
	if(Client()->State() == IClient::STATE_ONLINE)
	{
		char aServer[96];
		net_addr_str(&Client()->ServerAddress(), aServer, sizeof(aServer), true);
		if(str_comp(Record.m_aServer, aServer) == 0)
		{
			for(auto &Check : m_aCheatChecks)
			{
				if(Check.m_ClientId >= 0 && str_comp(Check.m_aName, Record.m_aName) == 0)
				{
					StopCheatCheck(Check.m_ClientId);
					Check = {};
				}
			}
		}
	}
}

void CSpectator::StopCheatCheck(int ClientId)
{
	if(ClientId < 0 || ClientId >= MAX_CLIENTS)
		return;
	auto &Check = m_aCheatChecks[ClientId];
	Check.m_Enabled = false;
	Check.m_LastTick = Check.m_AimDeadline = Check.m_PulseTick = Check.m_HookRiskTick = -1;
	Check.m_CountWallForCurrentHook = true;
	Check.m_ClusterCount = Check.m_TeleportCount = 0;
	if(m_CheatGhostClient == ClientId && GhostSpectating())
		StopGhostSpectate();
	if(m_CheatHudClient == ClientId)
		m_CheatHudClient = -1;
}

void CSpectator::StartCheatCheck(int ClientId)
{
	if(ClientId == -1)
	{
		StopCheatCheck(m_CheatHudClient);
		return;
	}
	if(Client()->State() != IClient::STATE_ONLINE || ClientId < 0 || ClientId >= MAX_CLIENTS ||
		!GameClient()->m_aClients[ClientId].m_Active)
		return;
	auto &Check = m_aCheatChecks[ClientId];
	if(Check.m_Enabled)
	{
		StopCheatCheck(ClientId);
		return;
	}
	const auto *pName = GameClient()->m_aClients[ClientId].m_aName;
	if(str_comp(Check.m_aName, pName) != 0)
		Check = {};
	Check.m_ClientId = ClientId;
	Check.m_Connection = g_Config.m_ClDummy;
	Check.m_Enabled = true;
	m_CheatHudClient = ClientId;
	str_copy(Check.m_aName, pName);
	UpdateCheatRecord(Check);
	if(!GameClient()->m_Snap.m_SpecInfo.m_Active && GameClient()->m_Snap.m_aCharacters[ClientId].m_Active)
	{
		ToggleGhostSpectate();
		if(GhostSpectating())
			m_CheatGhostClient = ClientId;
	}
	if(GameClient()->m_Snap.m_SpecInfo.m_Active)
	{
		Spectate(ClientId);
		if(m_CheatGhostClient >= 0)
			m_CheatGhostClient = ClientId;
	}
}

void CSpectator::SampleCheatCheck(SCheatCheck &Check)
{
	const auto &Player = GameClient()->m_aClients[Check.m_ClientId];
	const auto &Snap = GameClient()->m_Snap;
	const auto &Character = Snap.m_aCharacters[Check.m_ClientId];
	if(Check.m_Connection != g_Config.m_ClDummy || (Player.m_Active && str_comp(Check.m_aName, Player.m_aName) != 0))
	{
		StopCheatCheck(Check.m_ClientId);
		return;
	}
	if(!Snap.m_apPlayerInfos[Check.m_ClientId])
	{
		StopCheatCheck(Check.m_ClientId);
		return;
	}
	if(!Character.m_Active || Player.m_Paused || GameClient()->IsWorldPaused())
	{
		Check.m_LastTick = -1;
		Check.m_AimDeadline = Check.m_PulseTick = Check.m_HookRiskTick = -1;
		Check.m_CountWallForCurrentHook = true;
		return;
	}

	const int Tick = Client()->GameTick(g_Config.m_ClDummy);
	const int Ticks = Tick - Check.m_LastTick;
	if(Ticks == 0)
		return;
	const int Direction = Character.m_Cur.m_Direction;
	const int Attack = Character.m_Cur.m_AttackTick;
	const vec2 Aim = Character.m_HasExtendedDisplayInfo ?
		vec2(Character.m_ExtendedData.m_TargetX, Character.m_ExtendedData.m_TargetY) : vec2(0.0f, 0.0f);

	const vec2 Position(Character.m_Cur.m_X, Character.m_Cur.m_Y);
	const int Hook = Character.m_Cur.m_HookState;
	const int Hooked = Character.m_Cur.m_HookedPlayer;
	const auto Aligned = [](vec2 A, vec2 B, float Cosine) {
		const float Product = length(A) * length(B);
		return Product > 0.0f && dot(A, B) >= Cosine * Product;
	};
	if(Check.m_LastTick >= 0 && Ticks > 0 && Ticks <= 3)
	{
		const int TickSpeed = Client()->GameTickSpeed();
		const int OldEvents = Check.m_AimReturns + Check.m_AvoidEvents + Check.m_WallHooks + Check.m_HookAssistEvents + Check.m_WallEvents;
		if(Hook == HOOK_GRABBED && Hooked < 0 && Check.m_LastHookState != HOOK_GRABBED)
			++Check.m_WallHooks;
		const auto IsDanger = [&](vec2 Pos) {
			const int Index = Collision()->GetPureMapIndex(Pos);
			const auto Hazard = [](int Tile) {
				return Tile == TILE_FREEZE || Tile == TILE_DFREEZE || Tile == TILE_LFREEZE || Tile == TILE_DEATH;
			};
			return Hazard(Collision()->GetTileIndex(Index)) || Hazard(Collision()->GetFrontTileIndex(Index));
		};
		const auto &Tuning = GameClient()->m_aTuning[g_Config.m_ClDummy];
		const vec2 Velocity = vec2(Character.m_Cur.m_VelX, Character.m_Cur.m_VelY) / 256.0f;
		const auto ProjectDanger = [&](bool HoldHook, int InputDirection, int HoldTicks = 8) {
			vec2 Pos = Position, Vel = Velocity;
			for(int Ahead = 1; Ahead <= 8; ++Ahead)
			{
				const bool Grounded = Collision()->IsOnGround(Pos, CCharacterCore::PhysicalSize());
				const float MaxSpeed = Grounded ? Tuning.m_GroundControlSpeed : Tuning.m_AirControlSpeed;
				const float Acceleration = Grounded ? Tuning.m_GroundControlAccel : Tuning.m_AirControlAccel;
				const float Friction = Grounded ? Tuning.m_GroundFriction : Tuning.m_AirFriction;
				Vel.y += Tuning.m_Gravity;
				if(InputDirection < 0)
					Vel.x = SaturatedAdd(-MaxSpeed, MaxSpeed, Vel.x, -Acceleration);
				else if(InputDirection > 0)
					Vel.x = SaturatedAdd(-MaxSpeed, MaxSpeed, Vel.x, Acceleration);
				else
					Vel.x *= Friction;
				const vec2 ToHook = Check.m_LastHookPosition - Pos;
				if(HoldHook && Ahead <= HoldTicks && length(ToHook) > 46.0f)
				{
					vec2 Pull = normalize(ToHook) * static_cast<float>(Tuning.m_HookDragAccel);
					if(Pull.y > 0.0f)
						Pull.y *= 0.3f;
					Pull.x *= Pull.x * InputDirection > 0.0f ? 0.95f : 0.75f;
					const float Speed = length(Vel + Pull);
					if(Speed < Tuning.m_HookDragSpeed || Speed < length(Vel))
						Vel += Pull;
				}
				Vel = ClampVel(Collision()->GetMoveRestrictions(Pos), Vel);
				const float Ramp = VelocityRamp(length(Vel) * 50.0f,
					Tuning.m_VelrampStart, Tuning.m_VelrampRange, Tuning.m_VelrampCurvature);
				const vec2 Previous = Pos;
				Vel.x *= Ramp;
				Collision()->MoveBox(&Pos, &Vel, CCharacterCore::PhysicalSizeVec2(),
					vec2(Tuning.m_GroundElasticityX, Tuning.m_GroundElasticityY));
				Vel.x /= std::max(Ramp, 0.0001f);
				for(int Quarter = 1; Quarter <= 4; ++Quarter)
					if(IsDanger(mix(Previous, Pos, Quarter / 4.0f)))
						return Ahead;
			}
			return 0;
		};
		if(Check.m_HookRiskTick >= 0)
		{
			if(IsDanger(Position) || Character.m_ExtendedData.m_FreezeEnd != 0 || Hook >= HOOK_FLYING ||
				Direction != Check.m_LastDirection || Character.m_Cur.m_Jumped != Check.m_LastJumped)
				Check.m_HookRiskTick = -1;
			else if(Tick >= Check.m_HookRiskTick)
			{
				if(++Check.m_HookAssistEvents >= 3)
					Check.m_Score += 8.0f;
				Check.m_HookRiskTick = -1;
			}
		}
		if(Check.m_LastHookState == HOOK_GRABBED && Check.m_LastHookedPlayer < 0 && Hook == HOOK_IDLE &&
			Character.m_ExtendedData.m_FreezeEnd == 0 && !IsDanger(Position) &&
			Direction == Check.m_LastDirection && Character.m_Cur.m_Jumped == Check.m_LastJumped)
		{
			const int WithHook = ProjectDanger(true, Direction);
			if(WithHook > 0 && ProjectDanger(false, Direction) == 0)
			{
				int LatestSafeHold = 0;
				while(LatestSafeHold < WithHook && ProjectDanger(true, Direction, LatestSafeHold + 1) == 0)
					++LatestSafeHold;
				// Count releases close to the simulated boundary, not ordinary early exits.
				if(LatestSafeHold <= 4)
					Check.m_HookRiskTick = Tick + WithHook;
			}
		}
		Check.m_SampledTicks = std::min(Check.m_SampledTicks + Ticks, 5 * TickSpeed);
		const bool ValidAim = length(Aim) >= 16.0f && length(Check.m_LastAim) >= 16.0f;
		if(ValidAim)
			Check.m_ValidAimTicks = std::min(Check.m_ValidAimTicks + Ticks, 5 * TickSpeed);

		if(Hook < HOOK_FLYING || Check.m_LastHookState < HOOK_FLYING)
			Check.m_CountWallForCurrentHook = true;
		const bool LaunchingHook = Hook >= HOOK_FLYING && Check.m_LastHookState < HOOK_FLYING;
		if(ValidAim && Check.m_CountWallForCurrentHook && LaunchingHook && Hooked < 0 && Aim != Check.m_LastAim)
		{
			const float HookLength = std::clamp(static_cast<float>(Tuning.m_HookLength), 0.0f, 2000.0f);
			constexpr float HookOffset = CCharacterCore::PhysicalSize() * 1.5f;
			const vec2 PreviousDirection = normalize(Check.m_LastAim);
			const vec2 ActualDirection(Character.m_Cur.m_HookDx, Character.m_Cur.m_HookDy);
			const auto HitsWall = [&](vec2 Ray) {
				int Tele = 0;
				return Collision()->IntersectLineTeleHook(Position + Ray * HookOffset,
					Position + Ray * HookLength, nullptr, nullptr, &Tele) == TILE_SOLID;
			};
			if(HookLength > HookOffset && length(ActualDirection) > 0.0f && !HitsWall(PreviousDirection))
			{
				const float Before = std::atan2(PreviousDirection.y, PreviousDirection.x);
				const float Actual = std::atan2(ActualDirection.y, ActualDirection.x);
				const float Shift = std::abs(std::remainder(Actual - Before, 2.0f * pi));
				// Search only the observed correction range. A nearer valid ray rules out this match.
				constexpr float Step = pi / 180.0f;
				const int MaxSteps = std::min(45, static_cast<int>(std::ceil(Shift / Step)) + 1);
				bool FoundNearest = false;
				bool MatchesNearest = false;
				for(int Index = 1; Index <= MaxSteps && !FoundNearest; ++Index)
				{
					for(int Side : {1, -1})
					{
						const float Angle = Before + Index * Side * Step;
						if(!HitsWall(vec2(std::cos(Angle), std::sin(Angle))))
							continue;
						FoundNearest = true;
						MatchesNearest |= std::abs(std::remainder(Angle - Actual, 2.0f * pi)) <= 1.5f * Step;
					}
				}
				if(MatchesNearest)
				{
					Check.m_CountWallForCurrentHook = false;
					if(++Check.m_WallEvents >= 3 && Check.m_WallEvents <= 5)
						Check.m_Score += 3.0f;
				}
			}
		}
		if(!ValidAim || Tick > Check.m_AimDeadline)
			Check.m_AimDeadline = -1;
		if(Check.m_AimDeadline >= 0 && Hooked >= 0 && Hooked < MAX_CLIENTS &&
			Hook > 0 && Snap.m_aCharacters[Hooked].m_Active && Tick <= Check.m_AimDeadline - 6)
		{
			const auto &Target = Snap.m_aCharacters[Hooked].m_Cur;
			if(Aligned(Check.m_SnapAim, vec2(Target.m_X, Target.m_Y) - Position, 0.9660f))
				Check.m_AimKind = 1;
		}
		if(Check.m_AimDeadline >= 0 && Aligned(Aim, Check.m_AnchorAim, 0.9781f))
		{
			++Check.m_AimReturns;
			if(Check.m_AimKind == 1)
			{
				if(++Check.m_AimbotEvents > 1)
					Check.m_Score += 12.0f;
			}
			Check.m_AimDeadline = -1;
		}
		else if(ValidAim && Check.m_AimDeadline < 0 &&
			!Aligned(Aim, Check.m_LastAim, 0.9063f))
		{
			int Kind = 0;
			if(Hooked >= 0 && Hooked < MAX_CLIENTS && Hook > 0 &&
				Snap.m_aCharacters[Hooked].m_Active)
			{
				const auto &Target = Snap.m_aCharacters[Hooked].m_Cur;
				if(Aligned(Aim, vec2(Target.m_X, Target.m_Y) - Position, 0.9848f))
					Kind = 1;
			}
			if(Kind == 0 && Attack > Check.m_LastAttack && Attack >= Check.m_LastTick && Attack <= Tick)
			{
				for(int Id = 0; Id < MAX_CLIENTS; ++Id)
				{
					if(Id == Check.m_ClientId || !Snap.m_aCharacters[Id].m_Active)
						continue;
					const auto &Target = Snap.m_aCharacters[Id].m_Cur;
					const vec2 TargetPos(Target.m_X, Target.m_Y);
					if(distance(Position, TargetPos) <= 800.0f && Aligned(Aim, TargetPos - Position, 0.9962f) &&
						!Collision()->IntersectLine(Position, TargetPos, nullptr, nullptr))
					{
						Kind = 1;
						break;
					}
				}
			}
			// Unclassified turns use the stricter angular threshold.
			if(Kind || !Aligned(Aim, Check.m_LastAim, 0.5736f))
			{
				Check.m_AimKind = Kind;
				Check.m_AnchorAim = Check.m_LastAim;
				Check.m_SnapAim = Aim;
				Check.m_AimDeadline = Tick + 12;
			}
		}

		if(Check.m_PulseTick >= 0 && (Direction != Check.m_LastDirection || Tick - Check.m_PulseTick > 5))
		{
			if(Direction == 0 && Tick - Check.m_PulseTick <= 5 && Attack == Check.m_LastAttack)
			{
				if(Check.m_ClusterTile != Check.m_PulseTile || Tick - Check.m_ClusterTick > 30 * TickSpeed)
				{
					Check.m_ClusterTile = Check.m_PulseTile;
					Check.m_ClusterCount = 0;
				}
				Check.m_ClusterTick = Tick;
				if(++Check.m_ClusterCount >= 3)
				{
					++Check.m_AvoidEvents;
					Check.m_Score += 16.0f;
					Check.m_ClusterCount = 0;
				}
			}
			Check.m_PulseTick = -1;
		}
		if(Direction != 0 && Check.m_LastDirection == 0 && Attack == Check.m_LastAttack)
		{
			const auto Hazard = [](int Tile) {
				return Tile == TILE_FREEZE || Tile == TILE_DFREEZE || Tile == TILE_LFREEZE || Tile == TILE_DEATH;
			};
			int HazardIndex = -1;
			for(int Y = -1; Y <= 1 && HazardIndex < 0; ++Y)
			{
				for(int X = -1; X <= 1; ++X)
				{
					const int Candidate = Collision()->GetPureMapIndex(Position + vec2(X * 32.0f, Y * 32.0f));
					if((Hook == 0 && (Hazard(Collision()->GetTileIndex(Candidate)) || Hazard(Collision()->GetFrontTileIndex(Candidate)))) ||
						Collision()->IsTeleport(Candidate) || Collision()->IsEvilTeleport(Candidate))
					{
						HazardIndex = Candidate;
						break;
					}
				}
			}
			if(HazardIndex >= 0 && ProjectDanger(false, Check.m_LastDirection) > 0 && ProjectDanger(false, Direction) == 0)
			{
				Check.m_PulseTick = Tick;
				Check.m_PulseTile = HazardIndex;
			}
		}
		// A blocked approach can be a release or an immediate reversal while still moving toward the portal.
		if(Check.m_LastDirection != 0 && Direction != Check.m_LastDirection &&
			Character.m_Cur.m_VelX * Check.m_LastDirection > 0 &&
			(Check.m_LastTeleportCorrection < 0 || Tick - Check.m_LastTeleportCorrection >= TickSpeed / 4))
		{
			int Portal = -1;
			for(int X = 1; X <= 3 && Portal < 0; ++X)
			{
				for(int Y = -2; Y <= 2; ++Y)
				{
					const vec2 Probe = Position + vec2(X * Check.m_LastDirection * 32.0f, Y * 32.0f);
					const int Candidate = Collision()->GetPureMapIndex(Probe);
					if(Collision()->IsTeleport(Candidate) || Collision()->IsEvilTeleport(Candidate))
					{
						Portal = Candidate;
						break;
					}
				}
			}
			if(Portal >= 0)
			{
				const int Width = Collision()->GetWidth();
				const int PrevPortal = Check.m_TeleportCluster;
				if(PrevPortal < 0 || Tick - Check.m_LastTeleportCorrection > 30 * TickSpeed ||
					absolute(Portal % Width - PrevPortal % Width) > 2 || absolute(Portal / Width - PrevPortal / Width) > 2)
				{
					Check.m_TeleportCluster = Portal;
					Check.m_TeleportCount = 0;
				}
				Check.m_LastTeleportCorrection = Tick;
				if(++Check.m_TeleportCount >= 3)
				{
					++Check.m_AvoidEvents;
					Check.m_Score += 16.0f;
					Check.m_TeleportCount = 0;
				}
			}
		}
		Check.m_Score = std::min(95.0f, Check.m_Score);
		if(Check.m_AimReturns + Check.m_AvoidEvents + Check.m_WallHooks + Check.m_HookAssistEvents + Check.m_WallEvents != OldEvents)
			UpdateCheatRecord(Check);
	}
	else
	{
		Check.m_AimDeadline = Check.m_PulseTick = Check.m_HookRiskTick = -1;
		Check.m_CountWallForCurrentHook = true;
	}

	Check.m_LastTick = Tick;
	Check.m_LastDirection = Direction;
	Check.m_LastJumped = Character.m_Cur.m_Jumped;
	Check.m_LastAttack = Attack;
	Check.m_LastHookState = Hook;
	Check.m_LastHookedPlayer = Hooked;
	Check.m_LastHookPosition = vec2(Character.m_Cur.m_HookX, Character.m_Cur.m_HookY);
	Check.m_LastAim = Aim;
}

void CSpectator::RenderCheatCheck()
{
	if(m_CheatHudClient < 0 || GameClient()->m_Menus.IsActive() || GameClient()->m_Scoreboard.IsActive() ||
		GameClient()->m_Chat.IsActive() || GameClient()->m_AxionOverlay.Editing())
		return;
	const auto &Check = m_aCheatChecks[m_CheatHudClient];
	const auto OldScreen = Graphics()->GetScreen();
	const float Width = 300.0f * Graphics()->ScreenAspect();
	Graphics()->MapScreenToSize(Width, 300.0f);
	CUIRect Panel{8.0f, 38.0f, 160.0f, 41.0f};
	Panel.Draw(ColorRGBA(0.05f, 0.05f, 0.06f, 0.9f), IGraphics::CORNER_ALL, 3.0f);
	char aTitle[96];
	str_format(aTitle, sizeof(aTitle), "Cheat checker: %s", Check.m_aName);
	TextRender()->Text(Panel.x + 4, Panel.y + 3, 6.0f, aTitle, Panel.w - 8);
	char aStatus[96];
	if(Check.m_LastTick < 0)
		str_copy(aStatus, "Waiting for player data");
	else if(Check.m_SampledTicks < 5 * Client()->GameTickSpeed())
		str_copy(aStatus, "Collecting samples");
	else
		str_format(aStatus, sizeof(aStatus), "Suspicion: %.0f%% (heuristic)", Check.m_Score);
	TextRender()->Text(Panel.x + 4, Panel.y + 12, 6.0f, aStatus);
	str_format(aStatus, sizeof(aStatus), "Aim %d (%d returns)   Wall %d/%d   Hook %d   Freeze %d", Check.m_AimbotEvents, Check.m_AimReturns, Check.m_WallEvents, Check.m_WallHooks, Check.m_HookAssistEvents, Check.m_AvoidEvents);
	TextRender()->Text(Panel.x + 4, Panel.y + 21, 5.5f, aStatus);
	if(Check.m_ValidAimTicks == 0)
		TextRender()->Text(Panel.x + 4, Panel.y + 29, 5.5f, "Aim data unavailable");
	Graphics()->MapScreen(OldScreen);
}

void CSpectator::UpdateGhostSpectate()
{
	if(m_GhostConnection < 0)
		return;
	auto &Snap = GameClient()->m_Snap;
	auto &Spec = Snap.m_SpecInfo;
	if(!GhostSpectating() || (Snap.m_pLocalInfo && Snap.m_pLocalInfo->m_Team == TEAM_SPECTATORS) || (Spec.m_Active && !m_GhostCameraApplied))
	{
		StopGhostSpectate();
		return;
	}
	if(m_GhostTarget >= 0 && Snap.m_apPlayerInfos[m_GhostTarget] &&
		Snap.m_apPlayerInfos[m_GhostTarget]->m_Team == TEAM_SPECTATORS)
	{
		GameClient()->m_Controls.m_aMousePos[g_Config.m_ClDummy] = m_GhostFollowPosition;
		m_GhostTarget = SPEC_FREEVIEW;
	}

	Spec.m_Active = true;
	Spec.m_SpectatorId = m_GhostTarget;
	Spec.m_UsePosition = m_GhostTarget >= 0;
	if(Spec.m_UsePosition && Snap.m_aCharacters[m_GhostTarget].m_Active)
	{
		const auto &Character = Snap.m_aCharacters[m_GhostTarget];
		m_GhostFollowPosition = mix(vec2(Character.m_Prev.m_X, Character.m_Prev.m_Y),
			vec2(Character.m_Cur.m_X, Character.m_Cur.m_Y), Client()->IntraGameTick(g_Config.m_ClDummy));
	}
	if(Spec.m_UsePosition)
		Spec.m_Position = m_GhostFollowPosition;
	Spec.m_HasCameraInfo = false;
	Spec.m_Zoom = 1.0f;
	m_GhostCameraApplied = true;
}

void CSpectator::Spectate(int SpectatorId)
{
	if(GhostSpectating())
	{
		if(SpectatorId == SPEC_FREEVIEW || (SpectatorId >= 0 && SpectatorId < MAX_CLIENTS &&
			GameClient()->m_Snap.m_aCharacters[SpectatorId].m_Active))
		{
			m_GhostTarget = SpectatorId;
			UpdateGhostSpectate();
		}
		return;
	}
	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		GameClient()->m_DemoSpecId = std::clamp(SpectatorId, (int)SPEC_FOLLOW, MAX_CLIENTS - 1);
		// The tick must be rendered for the spectator mode to be updated, so we do it manually when demo playback is paused
		// TODO: https://github.com/ddnet/ddnet/issues/11681
		if(DemoPlayer()->BaseInfo()->m_Paused)
			GameClient()->m_Menus.DemoSeekTick(IDemoPlayer::TICK_CURRENT);
		return;
	}

	if(GameClient()->m_Snap.m_SpecInfo.m_SpectatorId == SpectatorId)
		return;

	if(Client()->IsSixup())
	{
		protocol7::CNetMsg_Cl_SetSpectatorMode Msg;
		if(SpectatorId == SPEC_FREEVIEW)
		{
			Msg.m_SpecMode = protocol7::SPEC_FREEVIEW;
			Msg.m_SpectatorId = -1;
		}
		else
		{
			Msg.m_SpecMode = protocol7::SPEC_PLAYER;
			Msg.m_SpectatorId = SpectatorId;
		}
		Client()->SendPackMsgActive(&Msg, MSGFLAG_VITAL, true);
		return;
	}
	CNetMsg_Cl_SetSpectatorMode Msg;
	Msg.m_SpectatorId = SpectatorId;
	Client()->SendPackMsgActive(&Msg, MSGFLAG_VITAL);
}

void CSpectator::SpectateClosest()
{
	if(!CanChangeSpectatorId())
		return;

	const CGameClient::CSnapState &Snap = GameClient()->m_Snap;
	int SpectatorId = Snap.m_SpecInfo.m_SpectatorId;

	int NewSpectatorId = -1;

	vec2 CurPosition = GameClient()->m_Camera.m_Center;
	if(SpectatorId != SPEC_FREEVIEW)
	{
		const CNetObj_Character &CurCharacter = Snap.m_aCharacters[SpectatorId].m_Cur;
		CurPosition = vec2(CurCharacter.m_X, CurCharacter.m_Y);
	}

	int ClosestDistance = std::numeric_limits<int>::max();
	for(int ClientId = 0; ClientId < MAX_CLIENTS; ClientId++)
	{
		if(ClientId == SpectatorId || !Snap.m_aCharacters[ClientId].m_Active || !Snap.m_apPlayerInfos[ClientId] || Snap.m_apPlayerInfos[ClientId]->m_Team == TEAM_SPECTATORS)
			continue;

		if(Client()->State() != IClient::STATE_DEMOPLAYBACK && ClientId == Snap.m_LocalClientId)
			continue;

		const CNetObj_Character &MaybeClosestCharacter = Snap.m_aCharacters[ClientId].m_Cur;
		int Distance = distance(CurPosition, vec2(MaybeClosestCharacter.m_X, MaybeClosestCharacter.m_Y));
		if(NewSpectatorId == -1 || Distance < ClosestDistance)
		{
			NewSpectatorId = ClientId;
			ClosestDistance = Distance;
		}
	}
	if(NewSpectatorId > -1)
		Spectate(NewSpectatorId);
}
