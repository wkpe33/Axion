/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_SPECTATOR_H
#define GAME_CLIENT_COMPONENTS_SPECTATOR_H
#include <base/vmath.h>

#include <engine/console.h>
#include <generated/protocol.h>

#include <game/client/component.h>
#include <game/client/ui.h>

#include <array>
#include <vector>

class CSpectator : public CComponent
{
	enum
	{
		MULTI_VIEW = -4,
		NO_SELECTION = -3,
	};

	bool m_Active;
	bool m_WasActive;

	int m_SelectedSpectatorId;
	vec2 m_SelectorMouse;

	CUi::CTouchState m_TouchState;

	float m_MultiViewActivateDelay;
	int m_GhostConnection = -1;
	int m_GhostTarget = -1;
	float m_ZoomBeforeGhost = 1.0f;
	bool m_GhostCameraApplied = false;
	vec2 m_GhostFollowPosition{};

	void StopGhostSpectate();
public:
	struct SCheatCheck
	{
		int m_ClientId = -1;
		int m_Connection = -1;
		int m_LastTick = -1;
		int m_SampledTicks = 0;
		int m_LastDirection = 0;
		int m_LastAttack = 0;
		int m_ValidAimTicks = 0;
		int m_AimDeadline = -1;
		int m_AimKind = 0;
		int m_PulseTick = -1;
		int m_PulseTile = -1;
		int m_ClusterTile = -1;
		int m_ClusterTick = -1;
		int m_ClusterCount = 0;
		int m_AimbotEvents = 0;
		int m_WallEvents = 0;
		int m_WallHooks = 0;
		int m_LastHookState = 0;
		int m_LastHookedPlayer = -1;
		int m_HookAssistEvents = 0;
		int m_HookRiskTick = -1;
		int m_LastJumped = 0;
		vec2 m_LastHookPosition{};
		int m_AimReturns = 0;
		int m_AvoidEvents = 0;
		vec2 m_LastAim{};
		vec2 m_AnchorAim{};
		vec2 m_SnapAim{};
		bool m_CountWallForCurrentHook = true;
		float m_Score = 0.0f;
		bool m_Enabled = false;
		int m_TeleportCluster = -1;
		int m_TeleportCount = 0;
		int m_LastTeleportCorrection = -1;
		char m_aName[64]{};
	};
	const SCheatCheck &CheatCheck(int ClientId) const { return m_aCheatChecks[ClientId]; }
	void StopCheatCheck(int ClientId);
	struct SCheatRecord
	{
		char m_aName[64]{};
		char m_aServer[96]{};
		int m_Score = 0;
		int m_Aimbot = 0;
		int m_Wall = 0;
		int m_WallHooks = 0;
		int m_Avoid = 0;
		int m_AimReturns = 0;
		int m_HookAssist = 0;
	};
	const std::vector<SCheatRecord> &CheatRecords() const { return m_vCheatRecords; }
	void DeleteCheatRecord(int Index);


private:
	SCheatCheck m_aCheatChecks[MAX_CLIENTS];
	std::vector<SCheatRecord> m_vCheatRecords;
	void UpdateCheatRecord(const SCheatCheck &Check);
	int m_CheatHudClient = -1;
	int m_CheatGhostClient = -1;
	void SampleCheatCheck(SCheatCheck &Check);
	void RenderCheatCheck();

	bool CanChangeSpectatorId();
	void SpectateNext(bool Reverse);

	static void ConKeySpectator(IConsole::IResult *pResult, void *pUserData);
	static void ConSpectate(IConsole::IResult *pResult, void *pUserData);
	static void ConSpectateNext(IConsole::IResult *pResult, void *pUserData);
	static void ConSpectatePrevious(IConsole::IResult *pResult, void *pUserData);
	static void ConSpectateClosest(IConsole::IResult *pResult, void *pUserData);
	static void ConMultiView(IConsole::IResult *pResult, void *pUserData);

public:
	CSpectator();
	int Sizeof() const override { return sizeof(*this); }

	void OnConsoleInit() override;
	bool OnCursorMove(float x, float y, IInput::ECursorType CursorType) override;
	bool OnInput(const IInput::CEvent &Event) override;
	void OnRender() override;
	void OnRelease() override;
	void OnReset() override;
	void OnNewSnapshot() override;
	void ToggleGhostSpectate();
	void StartCheatCheck(int ClientId);
	void UpdateGhostSpectate();
	bool GhostSpectating() const;

	void Spectate(int SpectatorId);
	void SpectateClosest();

	bool IsActive() const { return m_Active; }
};

#endif
