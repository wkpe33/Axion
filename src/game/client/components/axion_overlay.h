#ifndef GAME_CLIENT_COMPONENTS_AXION_OVERLAY_H
#define GAME_CLIENT_COMPONENTS_AXION_OVERLAY_H
#include "axion_hud_layout.h"
#include "axion_media.h"
#include "axion_theme.h"

#include <engine/textrender.h>

#include <game/client/component.h>

class CAxionOverlay : public CComponent
{
	std::unique_ptr<CAxionMedia> m_pMedia;
	CAxionMedia::SSnapshot m_Media;
	vec2 m_Mouse = vec2(150, 150), m_DragStart, m_DragOrigin, m_DragPosition;
	int m_Drag = -1;
	bool m_Editing = false, m_Interactive = false;
	bool m_Down = false, m_Press = false, m_VolumeDrag = false;
	float m_PreMuteVolume = 0.5f;
	float m_Expansion = 0, m_DisplayVolume = -1;
	float m_CanvasWidth = 500;
	CUIRect m_MusicRect = {}, m_VolumeRect = {};
	const SAxionPalette m_Palette = AxionThemePalette(0);
	struct SInvitation
	{
		int m_Inviter = -1, m_Team = 0;
		char m_aName[64]{};
		int64_t m_Expires = 0;
		float m_Animation = 0;
		bool m_Dismissed = false;
	} m_aInvitations[2];
	bool InvitationValid(const SInvitation &Invitation) const;
	CUIRect m_InviteAccept = {}, m_InviteDecline = {};
	uint64_t m_MediaRevision = 0;
	STextContainerIndex m_TimerTextContainer;
	char m_aTimerText[64]{};
	float m_TimerTextWidth = 0, m_TimerFont = 0;
	void ResetInteraction();
	void UpdateTimerText();
	void DrawTimerText(const CUIRect &Rect, ColorRGBA Color);
	struct SCachedPanel
	{
		int m_Container = -1;
		CUIRect m_Rect{};
		vec2 m_Pixel{};
	};
	SCachedPanel m_TimerPanel, m_TeamPanel;
	void DrawFramedPanel(const CUIRect &Rect, ColorRGBA Border, ColorRGBA Fill, SCachedPanel *pCache);
	void DrawHudPill(const CUIRect &Rect, SCachedPanel *pCache = nullptr);
	void RenderCompactHud(float W);
	void RenderInvitation(float W);
	void RenderMusic(float W);
	void RenderEditor(float W);
	CUIRect ModuleRect(AxionHud::EModule Module, float W) const;

public:
	int Sizeof() const override { return sizeof(*this); }
	void OnRender() override;
	void OnConsoleInit() override;
	void OnReset() override;
	void OnShutdown() override;
	void OnWindowResize() override;
	void OnStateChange(int NewState, int OldState) override;
	void NotifyTeamInvitation(const char *pServerMessage, int Connection);
	bool RespondToInvitation(bool Accept);
	bool OnInput(const IInput::CEvent &Event) override;
	bool OnCursorMove(float X, float Y, IInput::ECursorType CursorType) override;
	bool CapturesScoreboardPointer() const { return m_Interactive && (m_VolumeDrag || m_MusicRect.Inside(m_Mouse) || m_InviteAccept.Inside(m_Mouse) || m_InviteDecline.Inside(m_Mouse)); }
	bool Editing() const { return m_Editing; }
	void Edit();
};
#endif
