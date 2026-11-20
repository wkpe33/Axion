#ifndef GAME_CLIENT_COMPONENTS_AXION_HUD_LAYOUT_H
#define GAME_CLIENT_COMPONENTS_AXION_HUD_LAYOUT_H
#include <engine/graphics.h>
#include <engine/shared/config.h>

#include <game/client/ui_rect.h>

#include <algorithm>
namespace AxionHud
{
	enum EModule
	{
		HEALTH,
		JUMPS,
		STATE,
		TIMER,
		SCORE,
		MOVEMENT,
		SPECTATORS,
		DUMMY,
		INFO,
		CLOCK,
		NOTICES,
		FINISH,
		RECORD,
		VOTES,
		CHAT,
		KILLFEED,
		BROADCAST,
		STATUS,
		SPECTATOR,
		MUSIC,
		DEBUG,
		FROZEN,
		FROZENTEXT,
		LAST,
		INVITATION,
		SCOREBOARD,
		COUNT
	};
	inline const char *Name(EModule Module)
	{
		static const char *apNames[] = {"Health / ammo", "Jumps", "DDRace abilities", "Game timer", "Score", "Movement", "Spectator count", "Dummy actions", "FPS / prediction", "Local time", "Notifications", "Race / finish time", "Records", "Voting", "Chat", "Killfeed", "Broadcast", "Status bar", "Spectator HUD", "Timer / music player", "Debug information", "Frozen team", "Frozen count", "Last player", "Team invitation", "Scoreboard"};
		return apNames[Module];
	}
	inline int &X(EModule Module)
	{
		switch(Module)
		{
		case HEALTH: return g_Config.m_AxionHudHealthX;
		case JUMPS: return g_Config.m_AxionHudJumpsX;
		case STATE: return g_Config.m_AxionHudStateX;
		case TIMER: return g_Config.m_AxionHudTimerX;
		case SCORE: return g_Config.m_AxionHudScoreX;
		case MOVEMENT: return g_Config.m_AxionHudMovementX;
		case SPECTATORS: return g_Config.m_AxionHudSpectatorsX;
		case DUMMY: return g_Config.m_AxionHudDummyX;
		case INFO: return g_Config.m_AxionHudInfoX;
		case CLOCK: return g_Config.m_AxionHudClockX;
		case NOTICES: return g_Config.m_AxionHudNoticesX;
		case FINISH: return g_Config.m_AxionHudFinishX;
		case RECORD: return g_Config.m_AxionHudRecordX;
		case VOTES: return g_Config.m_AxionHudVotesX;
		case CHAT: return g_Config.m_AxionHudChatX;
		case KILLFEED: return g_Config.m_AxionHudKillfeedX;
		case BROADCAST: return g_Config.m_AxionHudBroadcastX;
		case STATUS: return g_Config.m_AxionHudStatusX;
		case SPECTATOR: return g_Config.m_AxionHudSpectatorX;
		case MUSIC: return g_Config.m_AxionHudMusicX;
		case DEBUG: return g_Config.m_AxionHudDebugX;
		case FROZEN: return g_Config.m_AxionHudFrozenX;
		case FROZENTEXT: return g_Config.m_AxionHudFrozenTextX;
		case LAST: return g_Config.m_AxionHudLastX;
		case INVITATION: return g_Config.m_AxHudInvitationX;
		case SCOREBOARD: return g_Config.m_AxHudScoreboardX;
		default: return g_Config.m_AxionHudMusicX;
		}
	}
	inline int &Y(EModule Module)
	{
		switch(Module)
		{
		case HEALTH: return g_Config.m_AxionHudHealthY;
		case JUMPS: return g_Config.m_AxionHudJumpsY;
		case STATE: return g_Config.m_AxionHudStateY;
		case TIMER: return g_Config.m_AxionHudTimerY;
		case SCORE: return g_Config.m_AxionHudScoreY;
		case MOVEMENT: return g_Config.m_AxionHudMovementY;
		case SPECTATORS: return g_Config.m_AxionHudSpectatorsY;
		case DUMMY: return g_Config.m_AxionHudDummyY;
		case INFO: return g_Config.m_AxionHudInfoY;
		case CLOCK: return g_Config.m_AxionHudClockY;
		case NOTICES: return g_Config.m_AxionHudNoticesY;
		case FINISH: return g_Config.m_AxionHudFinishY;
		case RECORD: return g_Config.m_AxionHudRecordY;
		case VOTES: return g_Config.m_AxionHudVotesY;
		case CHAT: return g_Config.m_AxionHudChatY;
		case KILLFEED: return g_Config.m_AxionHudKillfeedY;
		case BROADCAST: return g_Config.m_AxionHudBroadcastY;
		case STATUS: return g_Config.m_AxionHudStatusY;
		case SPECTATOR: return g_Config.m_AxionHudSpectatorY;
		case MUSIC: return g_Config.m_AxionHudMusicY;
		case DEBUG: return g_Config.m_AxionHudDebugY;
		case FROZEN: return g_Config.m_AxionHudFrozenY;
		case FROZENTEXT: return g_Config.m_AxionHudFrozenTextY;
		case LAST: return g_Config.m_AxionHudLastY;
		case INVITATION: return g_Config.m_AxHudInvitationY;
		case SCOREBOARD: return g_Config.m_AxHudScoreboardY;
		default: return g_Config.m_AxionHudMusicY;
		}
	}
	inline vec2 Offset(EModule Module, float W, float H)
	{
		return vec2(X(Module) * W / 10000.0f, Y(Module) * H / 10000.0f);
	}
	inline CUIRect DefaultRect(EModule Module, float W, float H)
	{
		switch(Module)
		{
		case HEALTH: return {5, 5, 120, 36};
		case JUMPS: return {5, 42, 120, 12};
		case STATE: return {5, 58, 115, 36};
		case TIMER: return {W / 2 - 45, 2, 90, 14};
		case SCORE: return {W - 80, H - 95, 80, 56};
		case MOVEMENT: return {W - 80, H - 145, 80, 40};
		case SPECTATORS: return {W - 45, H - 180, 45, 12};
		case DUMMY: return {W - 35, H - 130, 35, 29};
		case INFO: return {W - 70, 5, 65, 30};
		case CLOCK: return {W * 3 / 7 - 25, 0, 50, 13};
		case NOTICES: return {W / 2 - 75, 50, 150, 20};
		case FINISH: return {W / 2 - 90, 30, 180, 16};
		case RECORD: return {5, 70, 100, 24};
		case VOTES: return {0, 60, 120, 38};
		case CHAT: return {5, H - 105, 190, 85};
		case KILLFEED: return {W - 155, 38, 150, 50};
		case BROADCAST: return {W / 2 - 110, 40, 220, 20};
		case STATUS: return {0, H - 10, W, 10};
		case SPECTATOR: return {W - 180, H - 15, 180, 15};
		case MUSIC: return {W / 2 - 28, 0, 56, 10};
		case DEBUG: return {4, 30, 80, 60};
		case FROZEN: return {W / 2 + 32, 0, 100, 25};
		case FROZENTEXT: return {W / 2 - 25, 12, 50, 12};
		case LAST: return {g_Config.m_TcNotifyWhenLastX * W / 100.0f, g_Config.m_TcNotifyWhenLastY * H / 100.0f, 90, 20};
		case INVITATION: return {W / 2 - 95, std::max((float)10,66/10.0f+4)+3, 190, 11};
		case SCOREBOARD: return {W / 2 - 187.5f, 37.5f, 375.0f, 245.0f};
		default: return {0, 0, 40, 12};
		}
	}
	// Bounds are recorded in each renderer's native coordinate system and exposed
	// on the editor's 300-unit canvas. Hidden modules retain a compact placeholder.
	inline CUIRect s_aBounds[COUNT] = {};
	inline bool s_aHasBounds[COUNT] = {};
	inline bool s_aVisible[COUNT] = {};
	inline void BeginFrame()
	{
		std::fill_n(s_aVisible, COUNT, false);
	}
	inline void Record(EModule Module, CUIRect Rect, float W, float H)
	{
		Rect.x /= W;
		Rect.w /= W;
		Rect.y /= H;
		Rect.h /= H;
		if(s_aVisible[Module])
		{
			const CUIRect &Old = s_aBounds[Module];
			const float Left = std::min(Rect.x, Old.x), Top = std::min(Rect.y, Old.y);
			Rect = {Left, Top, std::max(Rect.x + Rect.w, Old.x + Old.w) - Left,
				std::max(Rect.y + Rect.h, Old.y + Old.h) - Top};
		}
		s_aBounds[Module] = Rect;
		s_aHasBounds[Module] = s_aVisible[Module] = true;
	}
	inline float ScoreboardScale(const CUIRect &Bounds, float W, float H)
	{
		return std::min({g_Config.m_AxHudScoreboardScale / 100.0f, W / Bounds.w, H / Bounds.h});
	}
	inline CUIRect ScoreboardBase(const CUIRect &Bounds, float W, float H)
	{
		const float Scale = ScoreboardScale(Bounds, W, H);
		return {Bounds.Center().x - Bounds.w * Scale / 2, Bounds.y, Bounds.w * Scale, Bounds.h * Scale};
	}
	inline CUIRect Rect(EModule Module, float W, float H)
	{
		CUIRect Result = DefaultRect(Module, W, H);
		if(s_aHasBounds[Module])
		{
			Result = s_aBounds[Module];
			Result.x *= W;
			Result.w *= W;
			Result.y *= H;
			Result.h *= H;
		}
		return Module == SCOREBOARD ? ScoreboardBase(Result, W, H) : Result;
	}
	// Translate projection rather than cached geometry; HUD text caches stay valid.
	class CScope
	{
		IGraphics *m_pGraphics;
		CScreenRect m_Screen;
		bool m_Moved;

	public:
		CScope(IGraphics *pGraphics, EModule Module) :
			m_pGraphics(pGraphics), m_Screen(pGraphics->GetScreen()), m_Moved(X(Module) != 0 || Y(Module) != 0)
		{
			if(m_Moved)
				pGraphics->MapScreen(m_Screen.Move(-Offset(Module, m_Screen.Width(), m_Screen.Height())));
		}
		CScope(const CScope &) = delete;
		CScope &operator=(const CScope &) = delete;
		~CScope()
		{
			if(m_Moved)
				m_pGraphics->MapScreen(m_Screen);
		}
	};
}
#endif
