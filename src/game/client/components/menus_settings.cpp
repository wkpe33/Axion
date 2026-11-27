/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/font_icons.h>
#include <game/client/axion_ui.h>
#include "menus.h"

#include <base/dbg.h>
#include <base/math.h>
#include <base/str.h>

#include <engine/graphics.h>
#include <engine/shared/config.h>
#include <engine/storage.h>

#include <game/client/components/menu_background.h>
#include <game/client/gameclient.h>
#include <game/client/ui.h>
#include <game/client/ui_scrollregion.h>
#include <game/localization.h>

#include <algorithm>
#include <charconv>
#include <cstring>
#include <string>
#include <vector>

void CMenus::RenderSettingsAxion(CUIRect View)
{
	static int s_Tab = 0;
	static CButtonContainer s_aTabs[3];
	const char *apTabs[] = {"Visuals", "Game", "Misc"};
	CUIRect TabBar, Row, Label;
	View.HSplitTop(24.0f, &TabBar, &View);
	const float Width = TabBar.w / 3.0f;
	for(int Index = 0; Index < 3; ++Index)
	{
		TabBar.VSplitLeft(Width, &Row, &TabBar);
		if(DoButton_MenuTab(&s_aTabs[Index], apTabs[Index], s_Tab == Index, &Row,
			   Index == 0 ? IGraphics::CORNER_L : Index == 2 ? IGraphics::CORNER_R :
									   IGraphics::CORNER_NONE))
		{
			s_Tab = Index;
			m_AxionMiscPanel = 0;
		}
	}
	View.HSplitTop(10.0f, nullptr, &View);
	if(s_Tab == 2 && m_AxionMiscPanel != 0)
	{
		CUIRect BackRow;
		View.HSplitTop(26, &BackRow, &View);
		BackRow.VSplitLeft(86, &BackRow, nullptr);
		static CButtonContainer s_Back;
		if(DoButton_Menu(&s_Back, Localize("Back"), 0, &BackRow))
			m_AxionMiscPanel = 0;
	}
	static CScrollRegion s_aScroll[4];
	CScrollRegion &s_Scroll = s_aScroll[s_Tab == 2 ? m_AxionMiscPanel + 1 : 0];
	CScrollRegionParams ScrollParams;
	if(s_Tab == 2 && m_AxionMiscPanel != 0)
		ScrollParams.m_ScrollUnit = 76.0f;
	s_Scroll.Begin(&View, &ScrollParams);
	CUIRect Left, Right;
	View.VSplitMid(&Left, &Right, 12.0f);
	const auto BeginPanel = [&](CUIRect &Column, float Height, const char *pTitle) {
		CUIRect Box;
		Column.HSplitTop(Height, &Box, &Column);
		Column.HSplitTop(8.0f, nullptr, &Column);
		s_Scroll.AddRect(Box);
		if(g_Config.m_AxRedesignedMenu)
			AxionUi::Surface(Box, AxionUi::Panel.WithAlpha(0.94f));
		else
			Box.Draw(ColorRGBA(0, 0, 0, 0.25f), IGraphics::CORNER_ALL, 6);
		Box.Margin(8.0f, &Box);
		Box.HSplitTop(22.0f, &Row, &Box);
		Ui()->DoLabel(&Row, pTitle, 15.0f, TEXTALIGN_ML);
		return Box;
	};
	const auto Check = [&](CUIRect &Panel, const char *pText, int &Value) {
		Panel.HSplitTop(22.0f, &Row, &Panel);
		Panel.HSplitTop(4.0f, nullptr, &Panel);
		if(DoButton_CheckBox(&Value, pText, Value, &Row))
			Value ^= 1;
	};
	const auto LabeledRow = [&](CUIRect &Panel, const char *pText) {
		Panel.HSplitTop(22.0f, &Row, &Panel);
		Panel.HSplitTop(4.0f, nullptr, &Panel);
		Row.VSplitLeft(std::min(86.0f, Row.w * 0.40f), &Label, &Row);
		Ui()->DoLabel(&Label, pText, 11.0f, TEXTALIGN_ML);
		return Row;
	};

	if(s_Tab == 2 && m_AxionMiscPanel != 0)
	{
		const bool Effects = m_AxionMiscPanel == 1;
		const int Count = Effects ? (int)AxionPresentation::EEffect::COUNT : AxionPresentation::SOUND_COUNT;
		CUIRect Mine = BeginPanel(Left, 38 + 26.0f * Count, Effects ? Localize("My Effects") : Localize("My Sounds"));
		CUIRect Others = BeginPanel(Right, 38 + 26.0f * Count, Effects ? Localize("Other Effects") : Localize("Other Sounds"));
		if(Effects)
		{
#define AXION_EFFECT(Name, Script, Label) \
	Check(Mine, Localize(Label), g_Config.m_AxEffectMy##Name); \
	Check(Others, Localize(Label), g_Config.m_AxEffectOther##Name);
#define AXION_SOUND(Name, Script, Id, Label)
#include <engine/shared/axion_presentation_catalog.h>
#undef AXION_EFFECT
#undef AXION_SOUND
		}
		else
		{
#define AXION_EFFECT(Name, Script, Label)
#define AXION_SOUND(Name, Script, Id, Label) \
	Check(Mine, Localize(Label), g_Config.m_AxSoundMy##Name); \
	Check(Others, Localize(Label), g_Config.m_AxSoundOther##Name);
#include <engine/shared/axion_presentation_catalog.h>
#undef AXION_EFFECT
#undef AXION_SOUND
		}
		CUIRect Shared = View;
		Shared.y = std::max(Left.y, Right.y);
		CUIRect Panel = BeginPanel(Shared, Effects ? 220.0f : 560.0f, Localize("Shared / unknown owner"));
		Panel.HSplitTop(20, &Row, &Panel);
		Ui()->DoLabel(&Row, Localize("Server events without a player ID use these settings."), 10, TEXTALIGN_ML);
		CUIRect SharedLeft, SharedRight;
		Panel.VSplitMid(&SharedLeft, &SharedRight, 12);
		int Index = 0;
		const auto SharedCheck = [&](const char *pName, int &Value) { Check(Index++ % 2 == 0 ? SharedLeft : SharedRight, Localize(pName), Value); };
		if(Effects)
		{
			SharedCheck("Hammer hit", g_Config.m_AxEffectSharedHammerHit);
			SharedCheck("Grenade explosion", g_Config.m_AxEffectSharedExplosion);
			SharedCheck("Spawn particles", g_Config.m_AxEffectSharedSpawn);
			SharedCheck("Damage indicators", g_Config.m_AxEffectSharedDamage);
			SharedCheck("Finish / birthday confetti", g_Config.m_AxEffectSharedConfetti);
			SharedCheck("Ninja / pickup shine", g_Config.m_AxEffectSharedShine);
			SharedCheck("Projectile smoke trail", g_Config.m_AxEffectSharedSmokeTrail);
			SharedCheck("Bullet trail", g_Config.m_AxEffectSharedBulletTrail);
			SharedCheck("Laser beam / head", g_Config.m_AxEffectSharedLaser);
			SharedCheck("Smoke particles", g_Config.m_AxEffectSharedSmoke);
			SharedCheck("All particle effects", g_Config.m_AxEffectSharedParticles);
		}
		else
		{
#define AXION_EFFECT(Name, Script, Label)
#define AXION_SOUND(Name, Script, Id, Label) SharedCheck(Label, g_Config.m_AxSoundShared##Name);
#include <engine/shared/axion_presentation_catalog.h>
#undef AXION_EFFECT
#undef AXION_SOUND
		}
		s_Scroll.End();
		return;
	}

	if(s_Tab == 0)
	{
		CUIRect Panel = BeginPanel(Left, 66.0f, "Visuals");
		Check(Panel, Localize("Short KoG / E-Gores names"), g_Config.m_AxionShortServerNames);
		Panel = BeginPanel(Left, 66.0f, "HUD");
		Panel.HSplitTop(22.0f, &Row, &Panel);
		static CButtonContainer s_EditHud;
		if(DoButton_Menu(&s_EditHud, Localize("Edit HUD"), 0, &Row))
			GameClient()->m_AxionOverlay.Edit();

		const bool NewHud = g_Config.m_AxNewHud != 0;
		Panel = BeginPanel(Left, NewHud ? 224.0f : 68.0f, Localize("New HUD"));
		Check(Panel, Localize("Enable New HUD"), g_Config.m_AxNewHud);
		if(NewHud && g_Config.m_AxNewHud)
		{
			Check(Panel, Localize("Game timer"), g_Config.m_AxNewHudTimer);
			Check(Panel, Localize("Unfrozen players / team size"), g_Config.m_AxNewHudRemaining);
			Check(Panel, Localize("Frozen tees"), g_Config.m_AxNewHudFrozen);
			Check(Panel, Localize("FPS"), g_Config.m_ClShowfps);
			Check(Panel, Localize("Prediction time"), g_Config.m_ClShowpred);
			Check(Panel, Localize("Show jumps indicator"), g_Config.m_ClShowhudJumpsIndicator);
		}

		Panel = BeginPanel(Right, g_Config.m_BcCustomAspectRatioMode == 2 ? 176.0f : 124.0f, "Aspect Ratio");
		CUIRect Control = LabeledRow(Panel, Localize("Preset"));
		const char *apRatios[] = {Localize("Native"), "5:4", "4:3", "3:2", "16:9", "16:10", Localize("Custom")};
		const int aValues[] = {0, 125, 133, 150, 178, 160};
		int Current = g_Config.m_BcCustomAspectRatioMode == 2 ? 6 : 0;
		if(g_Config.m_BcCustomAspectRatioMode == 1)
			for(int Index = 1; Index < 6; ++Index)
				if(g_Config.m_BcCustomAspectRatio == aValues[Index])
					Current = Index;
		static CUi::SDropDownState s_RatioState;
		static CScrollRegion s_RatioScroll;
		s_RatioState.m_SelectionPopupContext.m_pScrollRegion = &s_RatioScroll;
		const int Selected = Ui()->DoDropDown(&Control, Current, apRatios, std::size(apRatios), s_RatioState);
		if(Selected != Current)
		{
			g_Config.m_BcCustomAspectRatioMode = Selected == 6 ? 2 : Selected == 0 ? 0 :
												 1;
			if(Selected < 6)
				g_Config.m_BcCustomAspectRatio = aValues[Selected];
		}
		Control = LabeledRow(Panel, Localize("Apply"));
		const char *apApplyModes[] = {Localize("Game"), Localize("Full"), Localize("Game without HUD")};
		static CUi::SDropDownState s_ApplyState;
		static CScrollRegion s_ApplyScroll;
		s_ApplyState.m_SelectionPopupContext.m_pScrollRegion = &s_ApplyScroll;
		g_Config.m_BcCustomAspectRatioApplyMode = Ui()->DoDropDown(&Control, g_Config.m_BcCustomAspectRatioApplyMode, apApplyModes, std::size(apApplyModes), s_ApplyState);
		Check(Panel, Localize("Cursor as without stretch"), g_Config.m_BcCustomAspectRatioCursorNative);
		if(g_Config.m_BcCustomAspectRatioMode == 2)
		{
			static CLineInputNumber s_Width(16), s_Height(9);
			static int s_ConfigWidth = -1, s_ConfigHeight = -1;
			if(s_ConfigWidth != g_Config.m_BcCustomAspectRatioNum || s_ConfigHeight != g_Config.m_BcCustomAspectRatioDen)
			{
				s_Width.SetInteger(g_Config.m_BcCustomAspectRatioNum);
				s_Height.SetInteger(g_Config.m_BcCustomAspectRatioDen);
				s_ConfigWidth = g_Config.m_BcCustomAspectRatioNum;
				s_ConfigHeight = g_Config.m_BcCustomAspectRatioDen;
			}
			Control = LabeledRow(Panel, Localize("Custom size"));
			CUIRect WidthBox, Separator, HeightBox;
			Control.VSplitMid(&WidthBox, &HeightBox, 14.0f);
			Separator = {WidthBox.x + WidthBox.w, Control.y, 14.0f, Control.h};
			Ui()->DoEditBox(&s_Width, &WidthBox, 12.0f);
			Ui()->DoLabel(&Separator, ":", 12.0f, TEXTALIGN_MC);
			Ui()->DoEditBox(&s_Height, &HeightBox, 12.0f);
			const auto ParseSize = [](const CLineInputNumber &Input, int &Value) {
				const char *pStart = Input.GetString();
				const char *pEnd = pStart + std::strlen(pStart);
				const auto Parsed = std::from_chars(pStart, pEnd, Value);
				return Parsed.ec == std::errc{} && Parsed.ptr == pEnd && Value >= 1 && Value <= 10000;
			};
			int NewWidth = 0, NewHeight = 0;
			const bool Valid = ParseSize(s_Width, NewWidth) && ParseSize(s_Height, NewHeight);
			Control = LabeledRow(Panel, "");
			static CButtonContainer s_Apply;
			if(DoButton_Menu(&s_Apply, Valid ? Localize("Apply") : Localize("Enter 1–10000"), 0, &Control) && Valid)
			{
				g_Config.m_BcCustomAspectRatioNum = NewWidth;
				g_Config.m_BcCustomAspectRatioDen = NewHeight;
				s_ConfigWidth = NewWidth;
				s_ConfigHeight = NewHeight;
			}
		}
	}
	else if(s_Tab == 1)
	{
		CUIRect Panel = BeginPanel(Left, 68.0f, Localize("Prediction"));
		Check(Panel, Localize("Predict moving tiles (Ego)"), g_Config.m_AxMovingTilePrediction);
	}
	else if(s_Tab == 2)
	{
		CUIRect Panel = BeginPanel(Left, 90.0f, Localize("Gameplay"));
		static CButtonContainer s_Effects, s_Sounds;
		Panel.HSplitTop(22, &Row, &Panel);
		if(DoButton_Menu(&s_Effects, Localize("Effects"), 0, &Row))
			m_AxionMiscPanel = 1;
		Panel.HSplitTop(4, nullptr, &Panel);
		Panel.HSplitTop(22, &Row, &Panel);
		if(DoButton_Menu(&s_Sounds, Localize("Sounds"), 0, &Row))
			m_AxionMiscPanel = 2;
		Panel = BeginPanel(Right, 94.0f, Localize("Chat"));
		Check(Panel, Localize("Silent typing"), g_Config.m_BcSilentTyping);
		Check(Panel, Localize("Use vote binds for team invitations"), g_Config.m_AxInvitationVoteBinds);
		Panel = BeginPanel(Right, 66.0f, "HUD");
		Check(Panel, Localize("Show spectator names"), g_Config.m_AxSpectatorNames);
		Panel = BeginPanel(Right, 278.0f, Localize("Animated menu"));
		Check(Panel, Localize("Axion menu redesign"), g_Config.m_AxRedesignedMenu);
		const char *apQuality[] = {Localize("Off"), Localize("Low"), Localize("Medium"), Localize("High"), Localize("Ultra")};
		char aQuality[96];
		str_format(aQuality, sizeof(aQuality), "%s: %s", Localize("Background quality"), apQuality[g_Config.m_AxMenuQuality]);
		static CButtonContainer s_Quality;
		Panel.HSplitTop(23, &Row, &Panel);
		if(DoButton_Menu(&s_Quality, aQuality, 0, &Row))
			g_Config.m_AxMenuQuality = (g_Config.m_AxMenuQuality + 1) % std::size(apQuality);
		Panel.HSplitTop(6, nullptr, &Panel);
		const auto Slider = [&](int &Value, const char *pLabel, int Min, int Max) {
			Panel.HSplitTop(23, &Row, &Panel);
			Ui()->DoScrollbarOption(&Value, &Value, &Row, pLabel, Min, Max, &CUi::ms_LinearScrollbarScale, 0, "%");
		};
		Slider(g_Config.m_AxMenuParticles, Localize("Particles"), 20, 150);
		Slider(g_Config.m_AxMenuSpeed, Localize("Animation speed"), 0, 200);
		Slider(g_Config.m_AxMenuBrightness, Localize("Background brightness"), 0, 100);
		Slider(g_Config.m_AxMenuParallax, Localize("Cursor parallax"), 0, 100);
		Check(Panel, Localize("Startup animation"), g_Config.m_AxStartupAnimation);
		Check(Panel, Localize("Interface animations"), g_Config.m_AxUiAnimations);
	}

	s_Scroll.End();
}

void CMenus::SetNeedSendInfo()
{
	if(m_Dummy)
		m_NeedSendDummyinfo = true;
	else
		m_NeedSendinfo = true;
}

void CMenus::RenderSettings(CUIRect MainView)
{
	if(s_AudioPackEditorOpen)
	{
		RenderAudioPackEditor(MainView);
		return;
	}

	if(AssetsEditorIsOpen() && !AssetsEditorPicking())
	{
		RenderAssetsEditorScreen(MainView);
		return;
	}
	// render background
	CUIRect Button, TabBar, RestartBar;
	const bool Redesigned = g_Config.m_AxRedesignedMenu;
	MainView.VSplitRight(Redesigned ? std::clamp(MainView.w * 0.16f, 100.0f, 145.0f) : 120.0f, &MainView, &TabBar);
	if(Redesigned)
		TabBar.VSplitLeft(10.0f, nullptr, &TabBar);
	const ColorRGBA PanelColor = AxionUi::Background.WithAlpha(0.52f);
	if(Redesigned)
		AxionUi::Surface(MainView, PanelColor, IGraphics::CORNER_ALL, 10);
	else
		MainView.Draw(ms_ColorTabbarActive, IGraphics::CORNER_B, 10);
	MainView.Margin(20.0f, &MainView);

	const bool NeedRestart = m_NeedRestartGraphics || m_NeedRestartSound || m_NeedRestartUpdate;
	if(NeedRestart)
	{
		MainView.HSplitBottom(20.0f, &MainView, &RestartBar);
		MainView.HSplitBottom(10.0f, &MainView, nullptr);
	}

	if(Redesigned)
		TabBar.HSplitTop(9.0f, nullptr, &TabBar);
	else
	{
		TabBar.HSplitTop(50, &Button, &TabBar);
		Button.Draw(ms_ColorTabbarActive, IGraphics::CORNER_BR, 10);
	}
	const float CategoryHeight = std::min(28.0f, TabBar.h / SETTINGS_LENGTH);

	const char *apTabs[SETTINGS_LENGTH] = {
		Localize("Language"),
		Localize("General"),
		Localize("Player"),
		Client()->IsSixup() ? "Tee 0.7" : Localize("Tee"),
		Localize("Appearance"),
		Localize("Controls"),
		Localize("Graphics"),
		Localize("Sound"),
		Localize("DDNet"),
		Localize("Assets"),
		TCLocalize("TClient"), // TClient
		Localize("Profiles"), // TClient
		Localize("Configs"), // TClient
		"Axion"};
	static CButtonContainer s_aTabButtons[SETTINGS_LENGTH];
	const char *apIcons[SETTINGS_LENGTH] = {
		FontIcon::EARTH_AMERICAS, FontIcon::GEAR, FontIcon::USER, FontIcon::USER,
		FontIcon::STAR, FontIcon::KEYBOARD, FontIcon::GEAR, FontIcon::PLAY,
		FontIcon::NETWORK_WIRED, FontIcon::LAYER_GROUP, FontIcon::TERMINAL,
		FontIcon::ICON_USERS, FontIcon::FILE, FontIcon::LAYER_GROUP};

	for(int Row = 0; Row < SETTINGS_LENGTH; Row++)
	{
		int i = Row;
		if(Row == SETTINGS_TCLIENT + 1)
			i = SETTINGS_AXION;
		else if(Row > SETTINGS_TCLIENT + 1)
			i = Row - 1;
		if(Redesigned)
		{
			TabBar.HSplitTop(CategoryHeight, &Button, &TabBar);
			Button.HMargin(2, &Button);
		}
		else
		{
			TabBar.HSplitTop(10, nullptr, &TabBar);
			TabBar.HSplitTop(26, &Button, &TabBar);
		}
		if(DoButton_MenuTab(&s_aTabButtons[i], apTabs[i], g_Config.m_UiSettingsPage == i, &Button, Redesigned ? IGraphics::CORNER_ALL : IGraphics::CORNER_R, i == SETTINGS_AXION ? nullptr : &m_aAnimatorsSettingsTab[i]))
			g_Config.m_UiSettingsPage = i;
		if(!Redesigned)
			continue;
		CUIRect Icon;
		Button.VSplitLeft(20, &Icon, nullptr);
		Icon.Margin(3, &Icon);
		Ui()->DrawButton_FontIcon(apIcons[i], &Icon, AxionUi::Gray(0, 0));
	}

	if(g_Config.m_UiSettingsPage == SETTINGS_LANGUAGE)
	{
		GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_LANGUAGE);
		RenderLanguageSettings(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_GENERAL)
	{
		GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_GENERAL);
		RenderSettingsGeneral(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_PLAYER)
	{
		GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_PLAYER);
		RenderSettingsPlayer(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_TEE)
	{
		GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_TEE);
		if(Client()->IsSixup())
			RenderSettingsTee7(MainView);
		else
			RenderSettingsTee(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_APPEARANCE)
	{
		GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_APPEARANCE);
		RenderSettingsAppearance(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_CONTROLS)
	{
		GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_CONTROLS);
		m_MenusSettingsControls.Render(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_GRAPHICS)
	{
		GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_GRAPHICS);
		RenderSettingsGraphics(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_SOUND)
	{
		GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_SOUND);
		RenderSettingsSound(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_DDNET)
	{
		GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_DDNET);
		RenderSettingsDDNet(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_ASSETS)
	{
		GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_ASSETS);
		RenderSettingsAssets(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_TCLIENT) // TClient
	{
		GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_LANGUAGE);
		RenderSettingsTClient(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_PROFILES) // TClient
	{
		GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_GENERAL);
		RenderSettingsTClientProfiles(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_CONFIGS) // TClient
	{
		GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_PLAYER);
		RenderSettingsTClientConfigs(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_AXION)
	{
		RenderSettingsAxion(MainView);
	}
	else
	{
		dbg_assert_failed("ui_settings_page invalid");
	}

	if(NeedRestart)
	{
		CUIRect RestartWarning, RestartButton;
		RestartBar.VSplitRight(125.0f, &RestartWarning, &RestartButton);
		RestartWarning.VSplitRight(10.0f, &RestartWarning, nullptr);
		if(m_NeedRestartUpdate)
		{
			TextRender()->TextColor(1.0f, 0.4f, 0.4f, 1.0f);
			Ui()->DoLabel(&RestartWarning, Localize("DDNet Client needs to be restarted to complete update!"), 14.0f, TEXTALIGN_ML);
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
		}
		else
		{
			Ui()->DoLabel(&RestartWarning, Localize("You must restart the game for all settings to take effect."), 14.0f, TEXTALIGN_ML);
		}

		static CButtonContainer s_RestartButton;
		if(DoButton_Menu(&s_RestartButton, Localize("Restart"), 0, &RestartButton))
		{
			if(Client()->State() == IClient::STATE_ONLINE || GameClient()->Editor()->HasUnsavedData())
			{
				m_Popup = POPUP_RESTART;
			}
			else
			{
				Client()->Restart();
			}
		}
	}
}

bool CMenus::RenderHslaScrollbars(CUIRect *pRect, unsigned int *pColor, bool Alpha, float DarkestLight)
{
	const unsigned PrevPackedColor = *pColor;
	ColorHSLA Color(*pColor, Alpha);
	const ColorHSLA OriginalColor = Color;
	const char *apLabels[] = {Localize("Hue"), Localize("Sat."), Localize("Lht."), Localize("Alpha")};
	const float SizePerEntry = 20.0f;
	const float MarginPerEntry = 5.0f;
	const float PreviewMargin = 2.5f;
	const float PreviewHeight = 40.0f + 2 * PreviewMargin;
	const float OffY = (SizePerEntry + MarginPerEntry) * (3 + (Alpha ? 1 : 0)) - PreviewHeight;

	CUIRect Preview;
	pRect->VSplitLeft(PreviewHeight, &Preview, pRect);
	Preview.HSplitTop(OffY / 2.0f, nullptr, &Preview);
	Preview.HSplitTop(PreviewHeight, &Preview, nullptr);

	Preview.Draw(ColorRGBA(0.15f, 0.15f, 0.15f, 1.0f), IGraphics::CORNER_ALL, 4.0f + PreviewMargin);
	Preview.Margin(PreviewMargin, &Preview);
	Preview.Draw(color_cast<ColorRGBA>(Color.UnclampLighting(DarkestLight)), IGraphics::CORNER_ALL, 4.0f + PreviewMargin);

	auto &&RenderHueRect = [&](CUIRect *pColorRect) {
		float CurXOff = pColorRect->x;
		const float SizeColor = pColorRect->w / 6;

		// red to yellow
		{
			Graphics()->SetColor4(
				ColorRGBA(1, 0, 0, 1),
				ColorRGBA(1, 1, 0, 1),
				ColorRGBA(1, 1, 0, 1),
				ColorRGBA(1, 0, 0, 1));

			IGraphics::CFreeformItem Freeform(
				CurXOff, pColorRect->y,
				CurXOff + SizeColor, pColorRect->y,
				CurXOff, pColorRect->y + pColorRect->h,
				CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}

		// yellow to green
		CurXOff += SizeColor;
		{
			Graphics()->SetColor4(
				ColorRGBA(1, 1, 0, 1),
				ColorRGBA(0, 1, 0, 1),
				ColorRGBA(0, 1, 0, 1),
				ColorRGBA(1, 1, 0, 1));

			IGraphics::CFreeformItem Freeform(
				CurXOff, pColorRect->y,
				CurXOff + SizeColor, pColorRect->y,
				CurXOff, pColorRect->y + pColorRect->h,
				CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}

		CurXOff += SizeColor;
		// green to turquoise
		{
			Graphics()->SetColor4(
				ColorRGBA(0, 1, 0, 1),
				ColorRGBA(0, 1, 1, 1),
				ColorRGBA(0, 1, 1, 1),
				ColorRGBA(0, 1, 0, 1));

			IGraphics::CFreeformItem Freeform(
				CurXOff, pColorRect->y,
				CurXOff + SizeColor, pColorRect->y,
				CurXOff, pColorRect->y + pColorRect->h,
				CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}

		CurXOff += SizeColor;
		// turquoise to blue
		{
			Graphics()->SetColor4(
				ColorRGBA(0, 1, 1, 1),
				ColorRGBA(0, 0, 1, 1),
				ColorRGBA(0, 0, 1, 1),
				ColorRGBA(0, 1, 1, 1));

			IGraphics::CFreeformItem Freeform(
				CurXOff, pColorRect->y,
				CurXOff + SizeColor, pColorRect->y,
				CurXOff, pColorRect->y + pColorRect->h,
				CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}

		CurXOff += SizeColor;
		// blue to purple
		{
			Graphics()->SetColor4(
				ColorRGBA(0, 0, 1, 1),
				ColorRGBA(1, 0, 1, 1),
				ColorRGBA(1, 0, 1, 1),
				ColorRGBA(0, 0, 1, 1));

			IGraphics::CFreeformItem Freeform(
				CurXOff, pColorRect->y,
				CurXOff + SizeColor, pColorRect->y,
				CurXOff, pColorRect->y + pColorRect->h,
				CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}

		CurXOff += SizeColor;
		// purple to red
		{
			Graphics()->SetColor4(
				ColorRGBA(1, 0, 1, 1),
				ColorRGBA(1, 0, 0, 1),
				ColorRGBA(1, 0, 0, 1),
				ColorRGBA(1, 0, 1, 1));

			IGraphics::CFreeformItem Freeform(
				CurXOff, pColorRect->y,
				CurXOff + SizeColor, pColorRect->y,
				CurXOff, pColorRect->y + pColorRect->h,
				CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}
	};

	auto &&RenderSaturationRect = [&](CUIRect *pColorRect, const ColorRGBA &CurColor) {
		ColorHSLA LeftColor = color_cast<ColorHSLA>(CurColor);
		ColorHSLA RightColor = color_cast<ColorHSLA>(CurColor);

		LeftColor.s = 0.0f;
		RightColor.s = 1.0f;

		const ColorRGBA LeftColorRGBA = color_cast<ColorRGBA>(LeftColor);
		const ColorRGBA RightColorRGBA = color_cast<ColorRGBA>(RightColor);

		Graphics()->SetColor4(LeftColorRGBA, RightColorRGBA, RightColorRGBA, LeftColorRGBA);

		IGraphics::CFreeformItem Freeform(
			pColorRect->x, pColorRect->y,
			pColorRect->x + pColorRect->w, pColorRect->y,
			pColorRect->x, pColorRect->y + pColorRect->h,
			pColorRect->x + pColorRect->w, pColorRect->y + pColorRect->h);
		Graphics()->QuadsDrawFreeform(&Freeform, 1);
	};

	auto &&RenderLightingRect = [&](CUIRect *pColorRect, const ColorRGBA &CurColor) {
		ColorHSLA LeftColor = color_cast<ColorHSLA>(CurColor);
		ColorHSLA RightColor = color_cast<ColorHSLA>(CurColor);

		LeftColor.l = DarkestLight;
		RightColor.l = 1.0f;

		const ColorRGBA LeftColorRGBA = color_cast<ColorRGBA>(LeftColor);
		const ColorRGBA RightColorRGBA = color_cast<ColorRGBA>(RightColor);

		Graphics()->SetColor4(LeftColorRGBA, RightColorRGBA, RightColorRGBA, LeftColorRGBA);

		IGraphics::CFreeformItem Freeform(
			pColorRect->x, pColorRect->y,
			pColorRect->x + pColorRect->w, pColorRect->y,
			pColorRect->x, pColorRect->y + pColorRect->h,
			pColorRect->x + pColorRect->w, pColorRect->y + pColorRect->h);
		Graphics()->QuadsDrawFreeform(&Freeform, 1);
	};

	auto &&RenderAlphaRect = [&](CUIRect *pColorRect, const ColorRGBA &CurColorFull) {
		const ColorRGBA LeftColorRGBA = color_cast<ColorRGBA>(color_cast<ColorHSLA>(CurColorFull).WithAlpha(0.0f));
		const ColorRGBA RightColorRGBA = color_cast<ColorRGBA>(color_cast<ColorHSLA>(CurColorFull).WithAlpha(1.0f));

		Graphics()->SetColor4(LeftColorRGBA, RightColorRGBA, RightColorRGBA, LeftColorRGBA);

		IGraphics::CFreeformItem Freeform(
			pColorRect->x, pColorRect->y,
			pColorRect->x + pColorRect->w, pColorRect->y,
			pColorRect->x, pColorRect->y + pColorRect->h,
			pColorRect->x + pColorRect->w, pColorRect->y + pColorRect->h);
		Graphics()->QuadsDrawFreeform(&Freeform, 1);
	};

	for(int i = 0; i < 3 + Alpha; i++)
	{
		CUIRect Button, Label;
		pRect->HSplitTop(SizePerEntry, &Button, pRect);
		pRect->HSplitTop(MarginPerEntry, nullptr, pRect);
		Button.VSplitLeft(140.0f, &Label, &Button);
		Label.VMargin(10.0f, &Label);

		Button.Draw(ColorRGBA(0.15f, 0.15f, 0.15f, 1.0f), IGraphics::CORNER_ALL, 1.0f);

		CUIRect Rail;
		Button.Margin(2.0f, &Rail);

		char aBuf[32];

		// Hue
		if(i == 0)
			str_format(aBuf, sizeof(aBuf), "%s: %.1f° (%03d)", apLabels[i], Color[i] * 360.0f, round_to_int(Color[i] * 255.0f));
		// Lht
		else if(i == 2)
		{
			// handle internal light clamping, see `UnclampLighting`
			float Lht = DarkestLight + Color[i] * (1.0f - DarkestLight);
			str_format(aBuf, sizeof(aBuf), "%s: %.1f%% (%03d)", apLabels[i], Lht * 100.0f, round_to_int(Color[i] * 255.0f));
		}
		// Sat and Alpha
		else
			str_format(aBuf, sizeof(aBuf), "%s: %.1f%% (%03d)", apLabels[i], Color[i] * 100.0f, round_to_int(Color[i] * 255.0f));
		Ui()->DoLabel(&Label, aBuf, 12.0f, TEXTALIGN_ML);

		ColorRGBA HandleColor;
		Graphics()->TextureClear();
		Graphics()->TrianglesBegin();
		if(i == 0)
		{
			RenderHueRect(&Rail);
			HandleColor = color_cast<ColorRGBA>(ColorHSLA(Color.h, 1.0f, 0.5f, 1.0f));
		}
		else if(i == 1)
		{
			RenderSaturationRect(&Rail, color_cast<ColorRGBA>(ColorHSLA(Color.h, 1.0f, 0.5f, 1.0f)));
			HandleColor = color_cast<ColorRGBA>(ColorHSLA(Color.h, Color.s, 0.5f, 1.0f));
		}
		else if(i == 2)
		{
			RenderLightingRect(&Rail, color_cast<ColorRGBA>(ColorHSLA(Color.h, Color.s, 0.5f, 1.0f)));
			HandleColor = color_cast<ColorRGBA>(ColorHSLA(Color.h, Color.s, Color.l, 1.0f).UnclampLighting(DarkestLight));
		}
		else if(i == 3)
		{
			RenderAlphaRect(&Rail, color_cast<ColorRGBA>(ColorHSLA(Color.h, Color.s, Color.l, 1.0f).UnclampLighting(DarkestLight)));
			HandleColor = color_cast<ColorRGBA>(Color.UnclampLighting(DarkestLight));
		}
		Graphics()->TrianglesEnd();

		Color[i] = Ui()->DoScrollbarH(&((char *)pColor)[i], &Button, Color[i], &HandleColor);
	}

	if(OriginalColor != Color)
	{
		*pColor = Color.Pack(Alpha);
	}
	return PrevPackedColor != *pColor;
}
