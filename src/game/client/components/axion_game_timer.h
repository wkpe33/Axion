#ifndef GAME_CLIENT_COMPONENTS_AXION_GAME_TIMER_H
#define GAME_CLIENT_COMPONENTS_AXION_GAME_TIMER_H
#include <generated/protocol.h>

#include <algorithm>
#include <cstdint>
// Timer rules shared with BestClient's media HUD, including race/countdown modes.
struct SAxionGameTimer
{
	bool m_Valid = false, m_Warning = false, m_Urgent = false, m_SuddenDeath = false;
	int64_t m_Seconds = 0;
};
inline SAxionGameTimer AxionGameTimer(const CNetObj_GameInfo *pInfo, int Tick, int TickSpeed)
{
	SAxionGameTimer Result;
	if(!pInfo || TickSpeed <= 0)
		return Result;
	Result.m_Valid = true;
	Result.m_SuddenDeath = (pInfo->m_GameStateFlags & GAMESTATEFLAG_SUDDENDEATH) != 0;
	if(pInfo->m_TimeLimit && pInfo->m_WarmupTimer <= 0)
	{
		Result.m_Seconds = (int64_t)pInfo->m_TimeLimit * 60 - ((int64_t)Tick - pInfo->m_RoundStartTick) / TickSpeed;
		if(pInfo->m_GameStateFlags & GAMESTATEFLAG_GAMEOVER)
			Result.m_Seconds = 0;
		Result.m_Warning = Result.m_Seconds <= 60;
		Result.m_Urgent = Result.m_Seconds <= 10;
	}
	else if(pInfo->m_GameStateFlags & GAMESTATEFLAG_RACETIME)
		Result.m_Seconds = ((int64_t)Tick + pInfo->m_WarmupTimer) / TickSpeed;
	else
		Result.m_Seconds = ((int64_t)Tick - pInfo->m_RoundStartTick) / TickSpeed;
	Result.m_Seconds = std::max<int64_t>(0, Result.m_Seconds);
	return Result;
}
#endif
