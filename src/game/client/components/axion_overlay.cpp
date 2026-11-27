#include "axion_overlay.h"

#include "axion_game_timer.h"

#include <base/str.h>
#include <base/time.h>

#include <engine/font_icons.h>
#include <engine/shared/axion_cursor.h>
#include <engine/shared/axion_invitation.h>
#include <engine/textrender.h>

#include <game/client/gameclient.h>
#include <game/client/animstate.h>
#include <game/client/render.h>
#include <game/client/ui.h>
#include <game/localization.h>

#include <algorithm>
#include <array>
#include <cmath>

namespace
{
constexpr float MUSIC_PANEL_WIDTH = 88.0f;
constexpr float MUSIC_PANEL_HEIGHT = 33.0f;
constexpr float MUSIC_PANEL_GAP = 6.5f;

// Geometry antialiasing: one physical-pixel alpha fringe, independent of MSAA.
void DrawSmoothPanel(IGraphics *pGraphics, const CUIRect &Rect, ColorRGBA Color, float Radius, int Container = -1)
{
 if(Rect.w <= 0 || Rect.h <= 0 || Color.a <= 0) return;
 constexpr int Steps = 12;
 constexpr int Count = 4 * (Steps + 1);
 static const auto s_Normals = [] {
  std::array<vec2, Count> Result;
  for(int Corner = 0; Corner < 4; ++Corner)
   for(int Step = 0; Step <= Steps; ++Step)
   {
    const float Angle = (Corner + 2 + Step / float(Steps)) * pi / 2;
    Result[Corner * (Steps + 1) + Step] = vec2(std::cos(Angle), std::sin(Angle));
   }
  return Result;
 }();
 Radius = std::clamp(Radius, 0.0f, std::min(Rect.w, Rect.h) / 2);
 const vec2 Pixel = pGraphics->GetScreen().Size() / vec2(std::max(1, pGraphics->ScreenWidth()), std::max(1, pGraphics->ScreenHeight()));
 const vec2 Centers[] = {{Rect.x + Radius, Rect.y + Radius}, {Rect.x + Rect.w - Radius, Rect.y + Radius}, {Rect.x + Rect.w - Radius, Rect.y + Rect.h - Radius}, {Rect.x + Radius, Rect.y + Rect.h - Radius}};
 std::array<vec2, Count> Points, Outer;
 for(int i = 0; i < Count; ++i)
 {
  Points[i] = Centers[i / (Steps + 1)] + s_Normals[i] * Radius;
  Outer[i] = Points[i] + s_Normals[i] * Pixel;
 }
 std::array<IGraphics::CFreeformItem, Count> Geometry;
 const vec2 Center = Rect.Center();
 pGraphics->TextureClear(); pGraphics->QuadsBegin(); pGraphics->SetColor(Color);
 // Each quad carries two adjacent triangles of the fan. Keep the fringe at full resolution.
 for(int i = 0; i < Count; i += 2)
 {
  const vec2 &A = Points[i];
  const vec2 &B = Points[i + 1];
  const vec2 &C = Points[(i + 2) % Count];
  Geometry[i / 2] = IGraphics::CFreeformItem(Center.x, Center.y, A.x, A.y, C.x, C.y, B.x, B.y);
 }
 if(Container >= 0)
  pGraphics->QuadContainerAddQuads(Container, Geometry.data(), Count / 2);
 else
  pGraphics->QuadsDrawFreeform(Geometry.data(), Count / 2);
 pGraphics->SetColor4(Color, Color, Color.WithAlpha(0), Color.WithAlpha(0));
 for(int i = 0; i < Count; ++i)
 {
  const int Next = (i + 1) % Count;
  Geometry[i] = IGraphics::CFreeformItem(Points[i].x,Points[i].y,Points[Next].x,Points[Next].y,Outer[i].x,Outer[i].y,Outer[Next].x,Outer[Next].y);
 }
 if(Container >= 0)
  pGraphics->QuadContainerAddQuads(Container, Geometry.data(), Count);
 else
  pGraphics->QuadsDrawFreeform(Geometry.data(), Count);
 pGraphics->QuadsEnd(); pGraphics->SetColor(1,1,1,1);
}
}

void CAxionOverlay::OnConsoleInit()
{
	Console()->Register("ax_hud_edit", "", CFGFLAG_CLIENT, [](IConsole::IResult *, void *pData) { static_cast<CAxionOverlay *>(pData)->Edit(); }, this, "Edit HUD positions (Escape to finish)");
}

void CAxionOverlay::Edit()
{
	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return;
	m_Editing = true;
	m_Drag = -1;
	m_Down = m_Press = false;
	GameClient()->m_Menus.SetActive(false);
}

void CAxionOverlay::ResetInteraction()
{
	m_Editing = m_Interactive = m_Down = m_Press = m_VolumeDrag = false;
	m_Drag = -1;
	m_MusicRect = {};
}

void CAxionOverlay::OnReset()
{
	ResetInteraction();
	for(auto &Invitation : m_aInvitations)
		Invitation = {};
}
void CAxionOverlay::OnStateChange(int NewState, int)
{
	if(m_pMedia)
		m_pMedia->SetEnabled(NewState == IClient::STATE_ONLINE || NewState == IClient::STATE_DEMOPLAYBACK);
	if(NewState != IClient::STATE_ONLINE)
		OnReset();
}
void CAxionOverlay::OnWindowResize()
{
	TextRender()->DeleteTextContainer(m_TimerTextContainer);
	m_aTimerText[0] = 0;
}
void CAxionOverlay::OnShutdown()
{
	m_pMedia.reset();
	Graphics()->DeleteQuadContainer(m_TimerPanel.m_Container);
	Graphics()->DeleteQuadContainer(m_TeamPanel.m_Container);
	TextRender()->DeleteTextContainer(m_TimerTextContainer);
}
void CAxionOverlay::NotifyTeamInvitation(const char *pMessage, int Connection)
{
	if(Client()->State() != IClient::STATE_ONLINE || !GameClient()->m_GameInfo.m_EntitiesDDRace || Connection < 0 || Connection > 1)
		return;
	SAxionInvitation Parsed;
	if(!AxionParseInvitation(pMessage, Parsed, MAX_CLIENTS) || Parsed.m_Name.size() >= sizeof(m_aInvitations[0].m_aName))
		return;
	int Inviter = -1;
	for(int Id = 0; Id < MAX_CLIENTS; ++Id)
	{
		const auto &Player = GameClient()->m_aClients[Id];
		if(Player.m_Active && Parsed.m_Name == Player.m_aName && GameClient()->m_Teams.Team(Id) == Parsed.m_Team)
		{
			if(Inviter != -1)
				return; // Ambiguous nickname: don't guess.
			Inviter = Id;
		}
	}
	if(Inviter < 0)
		return;
	auto &Invitation = m_aInvitations[Connection];
	if(Invitation.m_Inviter == Inviter && Invitation.m_Team == Parsed.m_Team && time() < Invitation.m_Expires)
		return;
	Invitation = {};
	Invitation.m_Inviter = Inviter;
	Invitation.m_Team = Parsed.m_Team;
	str_copy(Invitation.m_aName, GameClient()->m_aClients[Inviter].m_aName);
	Invitation.m_Expires = time() + 10 * time_freq();
}
void CAxionOverlay::UpdateTimerText()
{
	const auto Timer = AxionGameTimer(GameClient()->m_Snap.m_pGameInfoObj, Client()->GameTick(g_Config.m_ClDummy), Client()->GameTickSpeed());
	char aTime[64];
	if(Timer.m_SuddenDeath)
		str_copy(aTime, Localize("Sudden Death"));
	else
		str_time(Timer.m_Seconds * 100, ETimeFormat::DAYS, aTime, sizeof(aTime));
	const float Font = 66 / 10.0f;
	if(str_comp(m_aTimerText, aTime) != 0 || Font != m_TimerFont || !m_TimerTextContainer.Valid())
	{
		str_copy(m_aTimerText, aTime);
		m_TimerFont = Font;
		m_TimerTextWidth = TextRender()->TextWidth(Font, aTime);
		CTextCursor Cursor;
		Cursor.SetPosition(vec2(0, 0));
		Cursor.m_FontSize = Font;
		TextRender()->RecreateTextContainer(m_TimerTextContainer, &Cursor, aTime);
	}
}
void CAxionOverlay::DrawTimerText(const CUIRect &Rect, ColorRGBA Color)
{
	const auto Timer = AxionGameTimer(GameClient()->m_Snap.m_pGameInfoObj, Client()->GameTick(g_Config.m_ClDummy), Client()->GameTickSpeed());
	if(Timer.m_Warning)
		Color = {1, 0.25f, 0.25f, Timer.m_Urgent && (2 * time() / time_freq()) % 2 ? 0.5f : 1.0f};
	if(m_TimerTextContainer.Valid())
		TextRender()->RenderTextContainer(m_TimerTextContainer, Color, TextRender()->DefaultTextOutlineColor().WithAlpha(0.15f), Rect.x + (Rect.w - m_TimerTextWidth) / 2, Rect.y + (Rect.h - m_TimerFont) / 2);
}
void CAxionOverlay::DrawFramedPanel(const CUIRect &Rect, ColorRGBA Border, ColorRGBA Fill, SCachedPanel *pCache)
{
	int Container = -1;
	if(pCache && Graphics()->IsQuadContainerBufferingEnabled())
	{
		const vec2 Pixel = Graphics()->GetScreen().Size() / vec2(std::max(1, Graphics()->ScreenWidth()), std::max(1, Graphics()->ScreenHeight()));
		const CUIRect &Old = pCache->m_Rect;
		if(pCache->m_Container >= 0 && Rect.x == Old.x && Rect.y == Old.y && Rect.w == Old.w && Rect.h == Old.h && Pixel == pCache->m_Pixel)
		{
			Graphics()->TextureClear();
			Graphics()->RenderQuadContainer(pCache->m_Container, -1);
			return;
		}
		Graphics()->DeleteQuadContainer(pCache->m_Container);
		Container = Graphics()->CreateQuadContainer(false);
		pCache->m_Container = Container;
		pCache->m_Rect = Rect;
		pCache->m_Pixel = Pixel;
	}

	const float Radius = std::min(5.0f, std::min(Rect.w, Rect.h) / 2);
	DrawSmoothPanel(Graphics(), Rect, Border, Radius, Container);
	CUIRect Inner;
	Rect.Margin(0.45f, &Inner);
	DrawSmoothPanel(Graphics(), Inner, Fill, std::max(0.0f, Radius - 0.45f), Container);
	if(Container >= 0)
	{
		Graphics()->QuadContainerUpload(Container);
		Graphics()->RenderQuadContainer(Container, -1);
	}
}

void CAxionOverlay::DrawHudPill(const CUIRect &Rect, SCachedPanel *pCache)
{
	DrawFramedPanel(Rect, m_Palette.m_Border.WithAlpha(0.65f), m_Palette.m_Background.WithAlpha(0.94f), pCache);
}

void CAxionOverlay::RenderCompactHud(float W)
{
	constexpr float Height = 11.0f;
	constexpr float Font = 6.6f;
	const auto Bounds = [&](AxionHud::EModule Module) {
		CUIRect Rect = ModuleRect(Module, W);
		Rect.x = std::clamp(Rect.x, 0.0f, std::max(0.0f, W - Rect.w));
		Rect.y = std::clamp(Rect.y, 0.0f, 300.0f - Rect.h);
		return Rect;
	};
	const auto Label = [&](AxionHud::EModule Module, const char *pText, bool Right) {
		const float Width = TextRender()->TextWidth(Font, pText) + 10.0f;
		AxionHud::Record(Module, {Right ? W - Width - 5.0f : 5.0f, 3.0f, Width, Height}, W, 300.0f);
		const CUIRect Rect = Bounds(Module);
		DrawHudPill(Rect, Module == AxionHud::FROZENTEXT ? &m_TeamPanel : nullptr);
		TextRender()->TextColor(m_Palette.m_Text);
		Ui()->DoLabel(&Rect, pText, Font, TEXTALIGN_MC);
		TextRender()->TextColor(TextRender()->DefaultTextColor());
		return Width;
	};
	const bool ShowFps = g_Config.m_ClShowfps;
	const bool ShowPrediction = g_Config.m_ClShowpred && Client()->State() != IClient::STATE_DEMOPLAYBACK;
	if(ShowFps || ShowPrediction)
	{
		char aInfo[64];
		const int Fps = std::min(99999, round_to_int(1.0f / std::max(0.00001f, Client()->FrameTimeAverage())));
		if(ShowFps && ShowPrediction)
			str_format(aInfo, sizeof(aInfo), "%d FPS  /  %d ms", Fps, Client()->GetPredictionTime());
		else if(ShowFps)
			str_format(aInfo, sizeof(aInfo), "%d FPS", Fps);
		else
			str_format(aInfo, sizeof(aInfo), "%d ms", Client()->GetPredictionTime());
		Label(AxionHud::INFO, aInfo, true);
	}

	if(!g_Config.m_AxNewHudRemaining && !g_Config.m_AxNewHudFrozen)
		return;

	const auto &Snap = GameClient()->m_Snap;
	const int PlayerId = Snap.m_SpecInfo.m_Active ? Snap.m_SpecInfo.m_SpectatorId : Snap.m_LocalClientId;
	if(!GameClient()->m_GameInfo.m_EntitiesDDRace || PlayerId < 0 || PlayerId >= MAX_CLIENTS ||
		!Snap.m_apPlayerInfos[PlayerId] || Snap.m_apPlayerInfos[PlayerId]->m_Team == TEAM_SPECTATORS)
		return;

	const int Team = GameClient()->m_Teams.Team(PlayerId);
	constexpr int MaxVisible = 6;
	std::array<int, MaxVisible> aFrozenIds{};
	int Total = 0, Frozen = 0;
	for(int Id = 0; Id < MAX_CLIENTS; ++Id)
	{
		if(!Snap.m_apPlayerInfos[Id] || Snap.m_apPlayerInfos[Id]->m_Team == TEAM_SPECTATORS ||
			(Team > TEAM_FLOCK && Team < TEAM_SUPER ? GameClient()->m_Teams.Team(Id) != Team : Id != PlayerId))
			continue;
		++Total;
		const auto &Player = GameClient()->m_aClients[Id];
		if(Player.m_FreezeEnd > 0 || Player.m_DeepFrozen)
		{
			if(Frozen < MaxVisible)
				aFrozenIds[Frozen] = Id;
			++Frozen;
		}
	}
	char aRemaining[24];
	str_format(aRemaining, sizeof(aRemaining), "%d/%d", Total - Frozen, Total);
	const float FrozenStart = g_Config.m_AxNewHudRemaining ? Label(AxionHud::FROZENTEXT, aRemaining, false) + 8.0f : 5.0f;
	if(!g_Config.m_AxNewHudFrozen || Frozen == 0)
		return;

	constexpr float Spacing = 8.0f;
	const int Shown = std::min(Frozen, MaxVisible);
	char aOverflow[16] = "";
	if(Frozen > Shown)
		str_format(aOverflow, sizeof(aOverflow), "+%d", Frozen - Shown);
	const float OverflowWidth = aOverflow[0] ? TextRender()->TextWidth(Font, aOverflow) + 3.0f : 0.0f;
	AxionHud::Record(AxionHud::FROZEN, {FrozenStart, 3.0f, Shown * Spacing + 6.0f + OverflowWidth, Height}, W, 300.0f);
	const CUIRect Rect = Bounds(AxionHud::FROZEN);
	DrawHudPill(Rect);
	const auto *pIdle = CAnimState::GetIdle();
	for(int Index = 0; Index < Shown; ++Index)
	{
		CTeeRenderInfo Info = GameClient()->m_aClients[aFrozenIds[Index]].m_RenderInfo;
		Info.m_Size = 7.0f;
		vec2 CenterOffset;
		CRenderTools::GetRenderTeeOffsetToRenderedTee(pIdle, &Info, CenterOffset);
		RenderTools()->RenderTee(pIdle, &Info, EMOTE_PAIN, vec2(1, 0),
			vec2(Rect.x + 3.0f + (Index + 0.5f) * Spacing, Rect.Center().y) + CenterOffset, 0.85f);
	}
	if(aOverflow[0])
	{
		const CUIRect Overflow = {Rect.x + 3.0f + Shown * Spacing, Rect.y, OverflowWidth, Rect.h};
		TextRender()->TextColor(m_Palette.m_Text);
		Ui()->DoLabel(&Overflow, aOverflow, Font, TEXTALIGN_MC);
		TextRender()->TextColor(TextRender()->DefaultTextColor());
	}
}
bool CAxionOverlay::InvitationValid(const SInvitation &Invitation) const
{
	if(Invitation.m_Dismissed || time() >= Invitation.m_Expires ||
		Invitation.m_Inviter < 0 || Invitation.m_Inviter >= MAX_CLIENTS)
		return false;
	const auto &Inviter = GameClient()->m_aClients[Invitation.m_Inviter];
	return Inviter.m_Active && str_comp(Invitation.m_aName, Inviter.m_aName) == 0 &&
		GameClient()->m_Teams.Team(Invitation.m_Inviter) == Invitation.m_Team;
}

bool CAxionOverlay::RespondToInvitation(bool Accept)
{
	if(Client()->State() != IClient::STATE_ONLINE || m_Editing)
		return false;
	auto &Invitation = m_aInvitations[g_Config.m_ClDummy];
	if(!InvitationValid(Invitation))
		return false;
	if(Accept)
	{
		char aCommand[32];
		str_format(aCommand, sizeof(aCommand), "/team %d", Invitation.m_Team);
		GameClient()->m_Chat.SendChat(0, aCommand);
	}
	Invitation.m_Dismissed = true;
	return true;
}

void CAxionOverlay::RenderInvitation(float W)
{
	m_InviteAccept = m_InviteDecline = {};
	auto &Invitation = m_aInvitations[g_Config.m_ClDummy];
	if(Invitation.m_Inviter < 0)
		return;
	const bool Valid = InvitationValid(Invitation);
	Invitation.m_Animation += ((Valid ? 1.0f : 0.0f) - Invitation.m_Animation) * AxionTransition(Client()->RenderFrameTime());
	if(!Valid && Invitation.m_Animation < 0.01f)
	{
		Invitation = {};
		return;
	}
	char aText[128];
	str_format(aText, sizeof(aText), "%s invited you to team %d", Invitation.m_aName, Invitation.m_Team);
	const float Width = std::min(W, TextRender()->TextWidth(6, aText) + 38);
	AxionHud::Record(AxionHud::INVITATION, {W / 2 - Width / 2, std::max((float)10,66/10.0f+4)+3, Width, 11}, W, 300);
	CUIRect Rect = ModuleRect(AxionHud::INVITATION, W);
	if(!g_Config.m_AxReduceMotion)
		Rect.y -= 3 * (1 - Invitation.m_Animation);
	const float Alpha = Invitation.m_Animation;
	DrawSmoothPanel(Graphics(), Rect, m_Palette.m_Border.WithAlpha(Alpha * 0.65f), 3);
	CUIRect Inner;
	Rect.Margin(0.4f, &Inner);
	DrawSmoothPanel(Graphics(), Inner, m_Palette.m_Background.WithAlpha(Alpha), 2.6f);
	CUIRect Text = Inner;
	Text.w -= 28;
	TextRender()->TextColor(m_Palette.m_Text.WithAlpha(Alpha));
	Ui()->DoLabel(&Text, aText, 6, TEXTALIGN_MC);
	const CUIRect Accept = {Rect.x + Rect.w - 25, Rect.y + 1, 11, 9};
	const CUIRect Decline = {Rect.x + Rect.w - 13, Rect.y + 1, 11, 9};
	for(const auto &Button : {Accept, Decline})
		DrawSmoothPanel(Graphics(), Button, m_Palette.m_Text.WithAlpha((m_Interactive && Button.Inside(m_Mouse) ? 0.18f : 0.07f) * Alpha), 2);
	TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
	Ui()->DoLabel(&Accept, FontIcon::CHECK, 6, TEXTALIGN_MC);
	Ui()->DoLabel(&Decline, FontIcon::XMARK, 6, TEXTALIGN_MC);
	TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
	TextRender()->TextColor(TextRender()->DefaultTextColor());
	CUIRect Progress = {Rect.x + 2, Rect.y + Rect.h - 1.1f, (Rect.w - 4) * std::clamp((Invitation.m_Expires - time()) / (float)(10 * time_freq()), 0.0f, 1.0f), 0.6f};
	if(Progress.w > 0)
		Progress.Draw(m_Palette.m_Text.WithAlpha(0.5f * Alpha), 0, 0);
	if(Valid && !m_Editing)
	{
		m_InviteAccept = Accept;
		m_InviteDecline = Decline;
		if(m_Interactive && m_Press && !m_MusicRect.Inside(m_Mouse))
		{
			if(Accept.Inside(m_Mouse))
				RespondToInvitation(true);
			else if(Decline.Inside(m_Mouse))
				RespondToInvitation(false);
		}
	}
}

bool CAxionOverlay::OnCursorMove(float X, float Y, IInput::ECursorType CursorType)
{
	if(!m_Editing && !((GameClient()->m_Chat.IsActive() || GameClient()->m_Scoreboard.CursorActive()) && !GameClient()->m_Menus.IsActive()))
		return false;
	Ui()->ConvertMouseMove(&X, &Y, CursorType);
	Ui()->OnCursorMove(X, Y);
	// Use the menu's pixel cursor and clamp, then project it into the HUD canvas.
	// No independent sensitivity multiplier, recentering or second cursor state.
	m_Mouse = AxionCursorForCanvas(Ui()->UpdatedMousePos(), vec2(Graphics()->WindowWidth(), Graphics()->WindowHeight()), vec2(m_CanvasWidth, 300));
	return true;
}

bool CAxionOverlay::OnInput(const IInput::CEvent &Event)
{
	if(m_Editing)
	{
		if(Event.m_Flags & IInput::FLAG_PRESS && Event.m_Key == KEY_ESCAPE)
		{
			ResetInteraction();
			return true;
		}
	}
	if((m_Editing || ((GameClient()->m_Chat.IsActive() || GameClient()->m_Scoreboard.CursorActive()) && !GameClient()->m_Menus.IsActive())) && Event.m_Key == KEY_MOUSE_1)
	{
		const bool Capture = m_Editing || m_MusicRect.Inside(m_Mouse) || m_InviteAccept.Inside(m_Mouse) || m_InviteDecline.Inside(m_Mouse) || m_VolumeDrag;
		if(Event.m_Flags & IInput::FLAG_PRESS && Capture)
		{
			m_Down = m_Press = true;
			return true;
		}
		if(Event.m_Flags & IInput::FLAG_RELEASE)
		{
			const bool WasDown = m_Down;
			m_Down = m_VolumeDrag = false;
			m_Drag = -1;
			return Capture || WasDown;
		}
	}
	return m_Editing;
}

CUIRect CAxionOverlay::ModuleRect(AxionHud::EModule Module, float W) const
{
	CUIRect Rect = AxionHud::Rect(Module, W, 300.0f);
	const vec2 Offset = AxionHud::Offset(Module, W, 300.0f);
	Rect.x += Offset.x;
	Rect.y += Offset.y;
	return Rect;
}

void CAxionOverlay::RenderEditor(float W)
{
	constexpr float H = 300.0f;
	const auto Bounds = [&](AxionHud::EModule Module) {
		CUIRect Rect = ModuleRect(Module, W);
		if(Module == AxionHud::MUSIC)
			return m_MusicRect;
		if(Module == AxionHud::SCOREBOARD || Module == AxionHud::INVITATION ||
			(g_Config.m_AxNewHud && (Module == AxionHud::INFO || Module == AxionHud::FROZEN || Module == AxionHud::FROZENTEXT)))
			Rect.x = std::clamp(Rect.x, 0.0f, std::max(0.0f, W - Rect.w));
		if(Module == AxionHud::SCOREBOARD ||
			(g_Config.m_AxNewHud && (Module == AxionHud::INFO || Module == AxionHud::FROZEN || Module == AxionHud::FROZENTEXT)))
			Rect.y = std::clamp(Rect.y, 0.0f, std::max(0.0f, H - Rect.h));
		return Rect;
	};

	if(m_Drag >= 0 && !AxionHud::s_aVisible[m_Drag])
		m_Drag = -1;
	int Hover = m_Drag;
	float Smallest = W * H + 1;
	if(Hover < 0)
		for(int i = 0; i < AxionHud::COUNT; ++i)
		{
			if(!AxionHud::s_aVisible[i])
				continue;
			const CUIRect Rect = Bounds(static_cast<AxionHud::EModule>(i));
			if(Rect.Inside(m_Mouse) && Rect.w * Rect.h <= Smallest)
			{
				Hover = i;
				Smallest = Rect.w * Rect.h;
			}
		}
	if(Hover < 0)
		return;

	const auto Module = static_cast<AxionHud::EModule>(Hover);
	const CUIRect Rect = Bounds(Module);
	if(m_Press && m_Down)
	{
		m_Drag = Hover;
		m_DragStart = m_Mouse;
		m_DragOrigin = AxionHud::Rect(Module, W, H).TopLeft();
		m_DragPosition = Rect.TopLeft();
	}
	if(m_Down && m_Drag == Hover && m_Mouse != m_DragStart)
	{
		const vec2 Target = m_DragPosition + (m_Mouse - m_DragStart);
		const float Reserved = Module == AxionHud::MUSIC ? MUSIC_PANEL_GAP + MUSIC_PANEL_HEIGHT : 0;
		AxionHud::X(Module) = round_to_int((std::clamp(Target.x, 0.0f, std::max(0.0f, W - Rect.w)) - m_DragOrigin.x) * 10000.0f / W);
		AxionHud::Y(Module) = round_to_int((std::clamp(Target.y, 0.0f, std::max(0.0f, H - Rect.h - Reserved)) - m_DragOrigin.y) * 10000.0f / H);
	}

	const float L = std::max(0.0f, Rect.x - 1), T = std::max(0.0f, Rect.y - 1);
	const float R = std::min(W, Rect.x + Rect.w + 1), B = std::min(H, Rect.y + Rect.h + 1);
	if(L >= R || T >= B)
		return;
	const float Phase = g_Config.m_AxReduceMotion ? 0.0f : std::fmod(time() / (double)time_freq(), 2.0) * pi;
	const CUIRect aEdges[] = {{L, T, R - L, 0.65f}, {L, B - 0.65f, R - L, 0.65f},
		{L, T, 0.65f, B - T}, {R - 0.65f, T, 0.65f, B - T}};
	for(int i = 0; i < 4; ++i)
	{
		const float Gray = 0.24f + 0.20f * std::sin(Phase + i * pi / 2);
		aEdges[i].Draw(ColorRGBA(Gray, Gray, Gray, 1), 0, 0);
	}
}

void CAxionOverlay::RenderMusic(float W)
{
	const float Width = m_TimerTextWidth + 10.0f;
	const float Height = std::max(10.0f, m_TimerFont + 4);
	constexpr float PanelWidth = MUSIC_PANEL_WIDTH, PanelHeight = MUSIC_PANEL_HEIGHT, Gap = MUSIC_PANEL_GAP;
	AxionHud::Record(AxionHud::MUSIC, {W / 2 - Width / 2, 0, Width, Height}, W, 300);
	const auto Anchor = ModuleRect(AxionHud::MUSIC, W);
	const CUIRect Timer = {std::clamp(Anchor.x, 0.0f, W - Width),
		std::clamp(Anchor.y, 0.0f, 300.0f - Height - Gap - PanelHeight), Width, Height};
	const float Center = Timer.x + Timer.w / 2;
	const float PanelX = std::clamp(Center - PanelWidth / 2, 0.0f, W - PanelWidth);
	const float TimerBottom = Timer.y + Timer.h;
	const float PanelY = TimerBottom + Gap;
	const bool Hover = m_Interactive && (Timer.Inside(m_Mouse) || m_MusicRect.Inside(m_Mouse));
	const float Target = !m_Editing && m_Interactive && (Hover || m_VolumeDrag) ? 1.0f : 0.0f;
	if(m_Expansion != Target)
		m_Expansion += (Target - m_Expansion) * AxionTransition(Client()->RenderFrameTime(), 180.0f);
	if(!m_Interactive || m_Expansion < 0.005f)
		m_Expansion = 0;
	m_MusicRect = Timer;
	const ColorRGBA White(0.96f, 0.96f, 0.96f, 1.0f);
	const auto Panel = [&](CUIRect Rect, float Radius, float Alpha) {
		DrawSmoothPanel(Graphics(), Rect, ColorRGBA(0.33f, 0.33f, 0.33f, Alpha), Radius);
		Rect.Margin(0.45f, &Rect);
		if(Rect.h > 0)
			DrawSmoothPanel(Graphics(), Rect, ColorRGBA(0.055f, 0.055f, 0.055f, Alpha), std::max(0.0f, Radius - 0.45f));
	};
	if(m_Expansion > 0)
	{
		m_MusicRect = {PanelX, Timer.y, PanelWidth, Height + Gap + PanelHeight * m_Expansion};
		const CUIRect Player = {PanelX, PanelY, PanelWidth, PanelHeight * m_Expansion};
		Panel(Player, std::min(5.0f, Player.h / 2), m_Expansion);
	}
	DrawFramedPanel(Timer, ColorRGBA(0.33f, 0.33f, 0.33f, 1), ColorRGBA(0.055f, 0.055f, 0.055f, 1), &m_TimerPanel);
	DrawTimerText(Timer, White);

	if(m_Expansion < 0.9f)
		return;
	m_pMedia->SnapshotIfChanged(m_Media, m_MediaRevision);
	const float Alpha = std::clamp((m_Expansion - 0.9f) / 0.1f, 0.0f, 1.0f);
	const auto Pointer = [&](float Edge, float Direction) {
		const float Tip = Edge + Direction * 2.6f;
		const IGraphics::CFreeformItem Outline(Center - 3.0f, Edge - Direction * 0.35f,
			Center + 3.0f, Edge - Direction * 0.35f, Center, Tip, Center, Tip);
		const IGraphics::CFreeformItem Fill(Center - 2.4f, Edge - Direction * 0.5f,
			Center + 2.4f, Edge - Direction * 0.5f, Center, Tip - Direction * 0.55f, Center, Tip - Direction * 0.55f);
		Graphics()->TextureClear();
		Graphics()->QuadsBegin();
		Graphics()->SetColor(0.33f, 0.33f, 0.33f, Alpha);
		Graphics()->QuadsDrawFreeform(&Outline, 1);
		Graphics()->SetColor(0.055f, 0.055f, 0.055f, Alpha);
		Graphics()->QuadsDrawFreeform(&Fill, 1);
		Graphics()->QuadsEnd();
		Graphics()->SetColor(1, 1, 1, 1);
	};
	Pointer(TimerBottom, 1.0f);
	Pointer(PanelY, -1.0f);
	const CUIRect Play = {PanelX + 37, PanelY + 4, 14, 14};
	const CUIRect Previous = {Play.x - 22, Play.y, 14, 14};
	const CUIRect Next = {Play.x + 22, Play.y, 14, 14};
	const CUIRect Speaker = {PanelX + 7, PanelY + 21, 12, 9};
	m_VolumeRect = {PanelX + 23, Speaker.y, PanelWidth - 33, 9};

	const auto Icon = [&](const CUIRect &Rect, const char *pIcon, bool Enabled) {
		if(Rect.Inside(m_Mouse) && Enabled && m_Interactive)
			Rect.Draw(White.WithAlpha(0.14f * Alpha), IGraphics::CORNER_ALL, 2);
		TextRender()->TextColor(White.WithAlpha(Alpha * (Enabled ? 1.0f : 0.25f)));
		TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
		Ui()->DoLabel(&Rect, pIcon, 7.0f, TEXTALIGN_MC);
		TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
		TextRender()->TextColor(TextRender()->DefaultTextColor());
	};
	Play.Draw(White.WithAlpha(0.12f * Alpha), IGraphics::CORNER_ALL, 3);
	Icon(Previous, FontIcon::BACKWARD_STEP, m_Media.m_CanPrevious);
	Icon(Play, m_Media.m_Playing ? FontIcon::PAUSE : FontIcon::PLAY, m_Media.m_CanPause);
	Icon(Next, FontIcon::FORWARD_STEP, m_Media.m_CanNext);
	Icon(Speaker, m_Media.m_Volume == 0 ? "\uF6A9" : "\uF028", m_Media.m_Volume >= 0);

	const auto Fraction = [&](const CUIRect &Rect) { return std::clamp((m_Mouse.x - Rect.x) / Rect.w, 0.0f, 1.0f); };
	if(!m_VolumeDrag)
		m_DisplayVolume = m_Media.m_Volume;
	if(m_Interactive && m_Press)
	{
		if(Previous.Inside(m_Mouse) && m_Media.m_CanPrevious)
			m_pMedia->Action(-1);
		else if(Play.Inside(m_Mouse) && m_Media.m_CanPause)
			m_pMedia->Action(0);
		else if(Next.Inside(m_Mouse) && m_Media.m_CanNext)
			m_pMedia->Action(1);
		else if(Speaker.Inside(m_Mouse) && m_Media.m_Volume >= 0)
		{
			if(m_Media.m_Volume > 0)
				m_PreMuteVolume = m_Media.m_Volume;
			m_pMedia->Volume(m_Media.m_Volume > 0 ? 0.0f : m_PreMuteVolume);
		}
		else if(m_VolumeRect.Inside(m_Mouse) && m_Media.m_Volume >= 0)
			m_VolumeDrag = true;
	}
	if(m_VolumeDrag && m_Down)
	{
		const float Volume = Fraction(m_VolumeRect);
		if(Volume != m_DisplayVolume)
			m_pMedia->Volume(Volume);
		m_DisplayVolume = Volume;
	}
	const auto Slider = [&](const CUIRect &Rect, float Value, bool Enabled, float Thickness) {
		Value = std::clamp(Value, 0.0f, 1.0f);
		const float Y = Rect.y + Rect.h / 2;
		const float Opacity = Alpha * (Enabled ? 1.0f : 0.35f);
		const CUIRect Track = {Rect.x, Y - Thickness / 2, Rect.w, Thickness};
		DrawSmoothPanel(Graphics(), Track, White.WithAlpha(0.22f * Opacity), Thickness / 2);
		CUIRect Fill = Track;
		Fill.w *= Value;
		if(Fill.w > 0)
			DrawSmoothPanel(Graphics(), Fill, White.WithAlpha(Opacity), std::min(Thickness / 2, Fill.w / 2));
		const CUIRect Thumb = {Rect.x + Rect.w * Value - 1.35f, Y - 1.35f, 2.7f, 2.7f};
		DrawSmoothPanel(Graphics(), Thumb, White.WithAlpha(Opacity), 1.35f);
	};
	Slider(m_VolumeRect, m_DisplayVolume, m_Media.m_Volume >= 0, 1.5f);
}

void CAxionOverlay::OnRender()
{
	const bool InGame = Client()->State() == IClient::STATE_ONLINE || Client()->State() == IClient::STATE_DEMOPLAYBACK;
	if(!InGame || GameClient()->m_Menus.IsActive())
	{
		ResetInteraction();
		return;
	}
	const float W = 300.0f * Graphics()->ScreenAspect();
	m_CanvasWidth = W;
	const bool Interactive = m_Editing || GameClient()->m_Chat.IsActive() || GameClient()->m_Scoreboard.CursorActive();
	if(Interactive)
	{
		Ui()->UpdateCursor();
		m_Mouse = AxionCursorForCanvas(Ui()->UpdatedMousePos(), vec2(Graphics()->WindowWidth(), Graphics()->WindowHeight()), vec2(W, 300));
	}
	m_Interactive = Interactive;
	if(!Interactive)
	{
		m_Down = m_VolumeDrag = false;
		m_Drag = -1;
	}
	const auto Screen = Graphics()->GetScreen();
	Graphics()->MapScreenToSize(W, 300);
	if(g_Config.m_AxNewHud && g_Config.m_ClShowhud && GameClient()->m_Snap.m_pGameInfoObj)
		RenderCompactHud(W);
	if(g_Config.m_ClShowhud)
		RenderInvitation(W);
	const bool ShowTimer = g_Config.m_AxNewHud ? g_Config.m_AxNewHudTimer : g_Config.m_ClShowhudTimer;
	if(ShowTimer && g_Config.m_ClShowhud)
	{
		if(!m_pMedia && !m_Editing)
			m_pMedia = std::make_unique<CAxionMedia>();
		UpdateTimerText();
		RenderMusic(W);
	}
	else
		m_MusicRect = {};
	if(m_Editing)
		RenderEditor(W);
	if(Interactive)
		RenderTools()->RenderCursor(m_Mouse, 12);
	Graphics()->MapScreen(Screen);
	m_Press = false;
}
