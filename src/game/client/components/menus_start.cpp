#include <game/client/axion_ui.h>
/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "menus_start.h"

#include <base/process.h>
#include <base/time.h>
#include <engine/storage.h>
#include <engine/client/updater.h>
#include <engine/font_icons.h>
#include <engine/graphics.h>
#include <engine/keys.h>
#include <engine/serverbrowser.h>
#include <engine/shared/config.h>
#include <engine/textrender.h>

#include <generated/client_data.h>

#include <game/client/gameclient.h>
#include <game/client/ui.h>
#include <game/localization.h>
#include <game/version.h>

#if defined(CONF_PLATFORM_ANDROID)
#include <android/android_main.h>
#endif

void CMenusStart::RenderStartMenu(CUIRect MainView)
{
	if(!g_Config.m_AxRedesignedMenu)
		return RenderStartMenuClassic(MainView);

	int NewPage = -1;
	const float Width = std::min(MainView.w * 0.15f, 96.0f);
	const float Gap = Width * 0.15f;
	const float Height = Width * 0.64f;
	const float Left = MainView.x + (MainView.w - 5 * Width - 4 * Gap) / 2;
	const float Top = MainView.y + MainView.h * 0.52f;
	const float VMargin = MainView.w * 0.18f;
	const float SinceStartup = std::chrono::duration<float>(time_get_nanoseconds() - AxionUi::StartupFinished).count();
	const float LogoAlpha = g_Config.m_AxStartupAnimation && !g_Config.m_AxReduceMotion ?
		std::clamp(0.32f + SinceStartup * 2.8f, 0.0f, 1.0f) : 1.0f;
	AxionUi::Logo(Graphics(), g_pData->m_aImages[IMAGE_BANNER].m_Id, MainView, 0.36f, LogoAlpha);
	static float s_aHover[6]{};
	const auto DrawAction = [&](CButtonContainer *pButton, int Index, const char *pIcon, const char *pLabel) {
		const CUIRect Hit = Index == 5 ?
			CUIRect{MainView.x + MainView.w - 38.0f, MainView.y + MainView.h - 38.0f, 30.0f, 30.0f} :
			CUIRect{Left + Index * (Width + Gap), Top, Width, Height};
		const bool Enabled = g_Config.m_AxUiAnimations && !g_Config.m_AxReduceMotion;
		s_aHover[Index] = AxionUi::Animate(s_aHover[Index], Ui()->HotItem() == pButton ? 1 : 0, Client()->RenderFrameTime(), Enabled);
		CUIRect Visual = Hit;
		const float Scale = Enabled ? (Ui()->ActiveItem() == pButton ? 0.97f : 1.0f + 0.045f * s_aHover[Index]) : 1;
		Visual.Margin((1 - Scale) * Hit.h * 0.5f, &Visual);
		const float Corner = Index == 5 ? 9.0f : Height * 0.28f;
		const auto Shade = AxionUi::Blend(Index == 0 ? AxionUi::Active : AxionUi::Button, AxionUi::Gray(0.28f), s_aHover[Index]);
		AxionUi::Surface(Visual, Shade, IGraphics::CORNER_ALL, Corner);
		CUIRect Inset = Visual;
		Inset.Margin(1, &Inset);
		Graphics()->TextureClear();
		Graphics()->DrawRect4(Inset.x, Inset.y, Inset.w, Inset.h,
			Shade, AxionUi::Blend(Shade, AxionUi::Background, 0.23f),
			AxionUi::Blend(Shade, AxionUi::Background, 0.10f), AxionUi::Blend(Shade, AxionUi::Background, 0.4f),
			IGraphics::CORNER_ALL, Corner - 1);
		CUIRect Icon;
		Visual.Margin(Visual.h * 0.20f, &Icon);
		Ui()->DrawButton_FontIcon(pIcon, &Icon, AxionUi::Gray(0, 0), IGraphics::CORNER_ALL);
		GameClient()->m_Tooltips.DoToolTip(pButton, &Hit, pLabel);
		return Ui()->DoButtonLogic(pButton, 0, &Hit, BUTTONFLAG_LEFT);
	};
	static CButtonContainer s_QuitButton;
	bool UsedEscape = false;
	if(DrawAction(&s_QuitButton, 5, FontIcon::POWER_OFF, Localize("Quit")) || (UsedEscape = Ui()->ConsumeHotkey(CUi::HOTKEY_ESCAPE)) || CheckHotKey(KEY_Q))
	{
		if(UsedEscape || GameClient()->Editor()->HasUnsavedData() || (GameClient()->CurrentRaceTime() / 60 >= g_Config.m_ClConfirmQuitTime && g_Config.m_ClConfirmQuitTime >= 0))
		{
			GameClient()->m_Menus.ShowQuitPopup();
		}
		else
		{
			Client()->Quit();
		}
	}

	static CButtonContainer s_SettingsButton;
	if(DrawAction(&s_SettingsButton, 4, FontIcon::GEAR, Localize("Settings")) || CheckHotKey(KEY_S))
		NewPage = CMenus::PAGE_SETTINGS;

	static CButtonContainer s_LocalServerButton;

	const bool LocalServerRunning = GameClient()->m_LocalServer.IsServerRunning();
	if(DrawAction(&s_LocalServerButton, 3, FontIcon::NETWORK_WIRED, LocalServerRunning ? Localize("Stop server") : Localize("Run server")) || (CheckHotKey(KEY_R) && Input()->KeyPress(KEY_R)))
	{
		if(LocalServerRunning)
		{
			GameClient()->m_LocalServer.KillServer();
		}
		else
		{
			GameClient()->m_LocalServer.RunServer({});
		}
	}

	static CButtonContainer s_MapEditorButton;
	if(DrawAction(&s_MapEditorButton, 2, FontIcon::PEN_TO_SQUARE, Localize("Editor")) || CheckHotKey(KEY_E))
	{
		g_Config.m_ClEditor = 1;
		Input()->MouseModeRelative();
	}

	static CButtonContainer s_DemoButton;
	if(DrawAction(&s_DemoButton, 1, FontIcon::FILM, Localize("Demos")) || CheckHotKey(KEY_D))
	{
		NewPage = CMenus::PAGE_DEMOS;
	}

	static CButtonContainer s_PlayButton;
	if(DrawAction(&s_PlayButton, 0, FontIcon::PLAY, Localize("Play", "Start menu")) || Ui()->ConsumeHotkey(CUi::HOTKEY_ENTER) || CheckHotKey(KEY_P))
	{
		NewPage = g_Config.m_UiPage >= CMenus::PAGE_INTERNET && g_Config.m_UiPage <= CMenus::PAGE_FAVORITE_COMMUNITY_5 ? g_Config.m_UiPage : CMenus::PAGE_INTERNET;
	}


	if(CheckHotKey(KEY_N))
		NewPage = CMenus::PAGE_NEWS;

		CUIRect VersionUpdate;
	MainView.HSplitBottom(20.0f, nullptr, &VersionUpdate);
	VersionUpdate.VMargin(VMargin, &VersionUpdate);
	CUIRect VersionRow, Version, UpdateButton, Status;
	MainView.HSplitBottom(24, nullptr, &VersionRow);
	VersionRow.VSplitLeft(80, &Version, &VersionRow);
	VersionRow.VSplitLeft(7, nullptr, &VersionRow);
	VersionRow.VSplitLeft(108, &UpdateButton, &Status);
	Status.VSplitLeft(8, nullptr, &Status);
	Status.VSplitRight(45, &Status, nullptr);
	char aVersion[40];
	str_format(aVersion, sizeof(aVersion), "Axion %s", CTClient::AXION_UPDATE_VERSION);
	SLabelProperties VersionStyle;
	VersionStyle.SetColor(AxionUi::Muted);
	Ui()->DoLabel(&Version, aVersion, 12, TEXTALIGN_ML, VersionStyle);
#if defined(CONF_FAMILY_WINDOWS) && !defined(CONF_AUTOUPDATE)
	static CButtonContainer s_AxionUpdateButton;
	static bool s_UpdateLaunchFailed = false;
	static bool s_UpdateLaunched = false;
	static bool s_ManualCheck = false;
	static float s_UpdateBlend = 0.0f;
	auto &TClient = GameClient()->m_TClient;
	const bool Checking = TClient.m_pTClientInfoTask && !TClient.m_pTClientInfoTask->Done();
	const bool Update = TClient.NeedUpdate() && CTClient::s_UpdateCheckSucceeded;
	s_UpdateBlend = AxionUi::Animate(s_UpdateBlend, Update ? 1.0f : 0.0f, Client()->RenderFrameTime(),
		g_Config.m_AxUiAnimations && !g_Config.m_AxReduceMotion);
	AxionUi::Surface(UpdateButton, AxionUi::Blend(Ui()->HotItem() == &s_AxionUpdateButton ? AxionUi::Hover : AxionUi::Button,
		AxionUi::Active, s_UpdateBlend));
	if(Checking || s_UpdateLaunched)
		Ui()->DoLabel(&UpdateButton, s_UpdateLaunched ? Localize("Updating…") : Localize("Checking…"), 11, TEXTALIGN_MC);
	else
	{
		SLabelProperties CheckStyle, UpdateStyle;
		CheckStyle.SetColor(AxionUi::Gray(0.96f, 1.0f - s_UpdateBlend));
		UpdateStyle.SetColor(AxionUi::Gray(0.96f, s_UpdateBlend));
		if(s_UpdateBlend < 0.99f)
			Ui()->DoLabel(&UpdateButton, Localize("Check update"), 11, TEXTALIGN_MC, CheckStyle);
		if(s_UpdateBlend > 0.01f)
			Ui()->DoLabel(&UpdateButton, Localize("Update"), 11, TEXTALIGN_MC, UpdateStyle);
		if(Ui()->DoButtonLogic(&s_AxionUpdateButton, 0, &UpdateButton, BUTTONFLAG_LEFT))
		{
			if(Update)
			{
				char aPath[IO_MAX_PATH_LENGTH], aPid[24];
				Storage()->GetBinaryPathAbsolute("AxionUpdater.exe", aPath, sizeof(aPath));
				str_format(aPid, sizeof(aPid), "%d", process_id());
				const char *apArgs[] = {aPid, CTClient::AXION_UPDATE_VERSION};
				s_UpdateLaunched = process_execute(aPath, EShellExecuteWindowState::FOREGROUND, apArgs, std::size(apArgs));
				s_UpdateLaunchFailed = !s_UpdateLaunched;
			}
			else
			{
				s_UpdateLaunchFailed = false;
				s_ManualCheck = true;
				TClient.FetchTClientInfo();
			}
		}
	}
	if(s_UpdateLaunchFailed)
		Ui()->DoLabel(&Status, Localize("Could not start Axion updater"), 11, TEXTALIGN_ML);
	else if(s_ManualCheck && !Checking && !Update && !s_UpdateLaunched)
		Ui()->DoLabel(&Status, CTClient::s_UpdateCheckSucceeded ? Localize("No updates available") : Localize("Could not check for updates"), 11, TEXTALIGN_ML);
#endif
#if defined(CONF_AUTOUPDATE)
	CUIRect UpdateButton;
	VersionUpdate.VSplitRight(100.0f, &VersionUpdate, &UpdateButton);
	VersionUpdate.VSplitRight(10.0f, &VersionUpdate, nullptr);

	char aBuf[128];
	const IUpdater::EUpdaterState State = Updater()->GetCurrentState();
	const bool NeedUpdate = GameClient()->m_TClient.NeedUpdate();

	if(State == IUpdater::CLEAN && NeedUpdate)
	{
		static CButtonContainer s_VersionUpdate;
		if(GameClient()->m_Menus.DoButton_Menu(&s_VersionUpdate, Localize("Update now"), 0, &UpdateButton, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)))
		{
			Updater()->InitiateUpdate();
		}
	}
	else if(State == IUpdater::NEED_RESTART)
	{
		static CButtonContainer s_VersionUpdate;
		if(GameClient()->m_Menus.DoButton_Menu(&s_VersionUpdate, Localize("Restart"), 0, &UpdateButton, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)))
		{
			Client()->Restart();
		}
	}
	else if(State >= IUpdater::GETTING_MANIFEST && State < IUpdater::NEED_RESTART)
	{
		Ui()->RenderProgressBar(UpdateButton, Updater()->GetCurrentPercent() / 100.0f);
	}

	if(State == IUpdater::CLEAN && NeedUpdate)
	{
		str_format(aBuf, sizeof(aBuf), Localize("TClient %s is out!"), GameClient()->m_TClient.m_aVersionStr);
		TextRender()->TextColor(1.0f, 0.4f, 0.4f, 1.0f);
	}
	else if(State == IUpdater::CLEAN)
	{
		aBuf[0] = '\0';
	}
	else if(State >= IUpdater::GETTING_MANIFEST && State < IUpdater::NEED_RESTART)
	{
		char aCurrentFile[64];
		Updater()->GetCurrentFile(aCurrentFile, sizeof(aCurrentFile));
		str_format(aBuf, sizeof(aBuf), Localize("Downloading %s:"), aCurrentFile);
	}
	else if(State == IUpdater::FAIL)
	{
		str_copy(aBuf, Localize("Update failed! Check log…"));
		TextRender()->TextColor(1.0f, 0.4f, 0.4f, 1.0f);
	}
	else if(State == IUpdater::NEED_RESTART)
	{
		str_copy(aBuf, Localize("DDNet Client updated!"));
		TextRender()->TextColor(1.0f, 0.4f, 0.4f, 1.0f);
	}
	Ui()->DoLabel(&VersionUpdate, aBuf, 14.0f, TEXTALIGN_ML);
	TextRender()->TextColor(TextRender()->DefaultTextColor());
#elif defined(CONF_INFORM_UPDATE)
	if(str_comp(Client()->LatestVersion(), "0") != 0 && false)
	{
		CUIRect DownloadButton;
		VersionUpdate.VSplitRight(100.0f, &VersionUpdate, &DownloadButton);
		VersionUpdate.VSplitRight(10.0f, &VersionUpdate, nullptr);

		static CButtonContainer s_DownloadButton;
		if(GameClient()->m_Menus.DoButton_Menu(&s_DownloadButton, Localize("Download"), 0, &DownloadButton, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)))
		{
			Client()->ViewLink("https://ddnet.org/downloads/");
		}

		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), Localize("DDNet %s is out!"), Client()->LatestVersion());
		SLabelProperties UpdateLabelProps;
		UpdateLabelProps.SetColor(ColorRGBA(1.0f, 0.4f, 0.4f, 1.0f));
		Ui()->DoLabel(&VersionUpdate, aBuf, 14.0f, TEXTALIGN_ML, UpdateLabelProps);
	}
#endif

	if(NewPage != -1)
	{
		GameClient()->m_Menus.SetShowStart(false);
		GameClient()->m_Menus.SetMenuPage(NewPage);
	}
}

void CMenusStart::RenderStartMenuClassic(CUIRect MainView)
{
	GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_START);

	// render logo
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_BANNER].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1, 1, 1, 1);
	IGraphics::CQuadItem QuadItem(MainView.w / 2 - 180, 52, 360, 120);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	const float Rounding = 10.0f;
	const float VMargin = MainView.w / 2 - 190.0f;

	CUIRect Button;
	int NewPage = -1;

	if(CheckHotKey(KEY_N))
		NewPage = CMenus::PAGE_NEWS;

	CUIRect Menu;
	MainView.VMargin(VMargin, &Menu);
	Menu.HSplitBottom(25.0f, &Menu, nullptr);

	Menu.HSplitBottom(40.0f, &Menu, &Button);
	static CButtonContainer s_QuitButton;
	bool UsedEscape = false;
	if(GameClient()->m_Menus.DoButton_Menu(&s_QuitButton, Localize("Quit"), 0, &Button, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, Rounding, 0.5f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)) || (UsedEscape = Ui()->ConsumeHotkey(CUi::HOTKEY_ESCAPE)) || CheckHotKey(KEY_Q))
	{
		if(UsedEscape || GameClient()->Editor()->HasUnsavedData() || (GameClient()->CurrentRaceTime() / 60 >= g_Config.m_ClConfirmQuitTime && g_Config.m_ClConfirmQuitTime >= 0))
		{
			GameClient()->m_Menus.ShowQuitPopup();
		}
		else
		{
			Client()->Quit();
		}
	}

	Menu.HSplitBottom(100.0f, &Menu, nullptr);
	Menu.HSplitBottom(40.0f, &Menu, &Button);
	static CButtonContainer s_SettingsButton;
	if(GameClient()->m_Menus.DoButton_Menu(&s_SettingsButton, Localize("Settings"), 0, &Button, BUTTONFLAG_LEFT, g_Config.m_ClShowStartMenuImages ? "settings" : nullptr, IGraphics::CORNER_ALL, Rounding, 0.5f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)) || CheckHotKey(KEY_S))
		NewPage = CMenus::PAGE_SETTINGS;

	Menu.HSplitBottom(5.0f, &Menu, nullptr); // little space
	Menu.HSplitBottom(40.0f, &Menu, &Button);
	static CButtonContainer s_LocalServerButton;

	const bool LocalServerRunning = GameClient()->m_LocalServer.IsServerRunning();
	if(GameClient()->m_Menus.DoButton_Menu(&s_LocalServerButton, LocalServerRunning ? Localize("Stop server") : Localize("Run server"), 0, &Button, BUTTONFLAG_LEFT, g_Config.m_ClShowStartMenuImages ? "local_server" : nullptr, IGraphics::CORNER_ALL, Rounding, 0.5f, LocalServerRunning ? ColorRGBA(0.0f, 1.0f, 0.0f, 0.25f) : ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)) || (CheckHotKey(KEY_R) && Input()->KeyPress(KEY_R)))
	{
		if(LocalServerRunning)
		{
			GameClient()->m_LocalServer.KillServer();
		}
		else
		{
			GameClient()->m_LocalServer.RunServer({});
		}
	}

	Menu.HSplitBottom(5.0f, &Menu, nullptr); // little space
	Menu.HSplitBottom(40.0f, &Menu, &Button);
	static CButtonContainer s_MapEditorButton;
	if(GameClient()->m_Menus.DoButton_Menu(&s_MapEditorButton, Localize("Editor"), 0, &Button, BUTTONFLAG_LEFT, g_Config.m_ClShowStartMenuImages ? "editor" : nullptr, IGraphics::CORNER_ALL, Rounding, 0.5f, GameClient()->Editor()->HasUnsavedData() ? ColorRGBA(0.0f, 1.0f, 0.0f, 0.25f) : ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)) || CheckHotKey(KEY_E))
	{
		g_Config.m_ClEditor = 1;
		Input()->MouseModeRelative();
	}

	Menu.HSplitBottom(5.0f, &Menu, nullptr); // little space
	Menu.HSplitBottom(40.0f, &Menu, &Button);
	static CButtonContainer s_DemoButton;
	if(GameClient()->m_Menus.DoButton_Menu(&s_DemoButton, Localize("Demos"), 0, &Button, BUTTONFLAG_LEFT, g_Config.m_ClShowStartMenuImages ? "demos" : nullptr, IGraphics::CORNER_ALL, Rounding, 0.5f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)) || CheckHotKey(KEY_D))
	{
		NewPage = CMenus::PAGE_DEMOS;
	}

	Menu.HSplitBottom(5.0f, &Menu, nullptr); // little space
	Menu.HSplitBottom(40.0f, &Menu, &Button);
	static CButtonContainer s_PlayButton;
	if(GameClient()->m_Menus.DoButton_Menu(&s_PlayButton, Localize("Play", "Start menu"), 0, &Button, BUTTONFLAG_LEFT, g_Config.m_ClShowStartMenuImages ? "play_game" : nullptr, IGraphics::CORNER_ALL, Rounding, 0.5f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)) || Ui()->ConsumeHotkey(CUi::HOTKEY_ENTER) || CheckHotKey(KEY_P))
	{
		NewPage = g_Config.m_UiPage >= CMenus::PAGE_INTERNET && g_Config.m_UiPage <= CMenus::PAGE_FAVORITE_COMMUNITY_5 ? g_Config.m_UiPage : CMenus::PAGE_INTERNET;
	}

	// render version
	CUIRect CurVersion, ConsoleButton;
	MainView.HSplitBottom(45.0f, nullptr, &CurVersion);
	CurVersion.VSplitRight(40.0f, &CurVersion, nullptr);
	CurVersion.HSplitTop(20.0f, &ConsoleButton, &CurVersion);
	CurVersion.HSplitTop(5.0f, nullptr, &CurVersion);
	ConsoleButton.VSplitRight(40.0f, nullptr, &ConsoleButton);
	Ui()->DoLabel(&CurVersion, GAME_RELEASE_VERSION, 14.0f, TEXTALIGN_MR);

	CUIRect TClientVersion;
	MainView.HSplitTop(15.0f, &TClientVersion, &MainView);
	TClientVersion.VSplitRight(40.0f, &TClientVersion, nullptr);
	char aTBuf[64];
	str_format(aTBuf, sizeof(aTBuf), "Axion %s", CTClient::AXION_UPDATE_VERSION);
	Ui()->DoLabel(&TClientVersion, aTBuf, 14.0f, TEXTALIGN_MR);
#if defined(CONF_AUTOUPDATE)
	CUIRect UpdateToDateText;
	MainView.HSplitTop(15.0f, &UpdateToDateText, nullptr);
	UpdateToDateText.VSplitRight(40.0f, &UpdateToDateText, nullptr);
	if(!GameClient()->m_TClient.NeedUpdate() && GameClient()->m_TClient.m_FetchedTClientInfo)
	{
		Ui()->DoLabel(&UpdateToDateText, TCLocalize("(On Latest)"), 14.0f, TEXTALIGN_MR);
	}
	else
	{
		Ui()->DoLabel(&UpdateToDateText, TCLocalize("(Fetching Update Info)"), 14.0f, TEXTALIGN_MR);
	}
#endif
	static CButtonContainer s_ConsoleButton;
	TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
	TextRender()->SetRenderFlags(ETextRenderFlags::TEXT_RENDER_FLAG_ONLY_ADVANCE_WIDTH | ETextRenderFlags::TEXT_RENDER_FLAG_NO_X_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_Y_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_PIXEL_ALIGNMENT | ETextRenderFlags::TEXT_RENDER_FLAG_NO_OVERSIZE);
	if(GameClient()->m_Menus.DoButton_Menu(&s_ConsoleButton, FontIcon::TERMINAL, 0, &ConsoleButton, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.1f)))
	{
		GameClient()->m_GameConsole.Toggle(CGameConsole::CONSOLETYPE_LOCAL);
	}
	TextRender()->SetRenderFlags(0);
	TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);

	CUIRect VersionUpdate;
	MainView.HSplitBottom(20.0f, nullptr, &VersionUpdate);
	VersionUpdate.VMargin(VMargin, &VersionUpdate);
#if defined(CONF_FAMILY_WINDOWS) && !defined(CONF_AUTOUPDATE)
	CUIRect AxionUpdateButton;
	VersionUpdate.VSplitRight(140.0f, &VersionUpdate, &AxionUpdateButton);
	static CButtonContainer s_AxionUpdateButton;
	static bool s_UpdateLaunchFailed = false;
	static bool s_UpdateLaunched = false;
	if(GameClient()->m_TClient.NeedUpdate() && !s_UpdateLaunched &&
		GameClient()->m_Menus.DoButton_Menu(&s_AxionUpdateButton, Localize("Update Axion"), 0, &AxionUpdateButton))
	{
		char aPath[IO_MAX_PATH_LENGTH];
		Storage()->GetBinaryPathAbsolute("AxionUpdater.exe", aPath, sizeof(aPath));
		char aPid[24];
		str_format(aPid, sizeof(aPid), "%d", process_id());
		const char *apArgs[] = {aPid, CTClient::AXION_UPDATE_VERSION};
		s_UpdateLaunched = process_execute(aPath, EShellExecuteWindowState::FOREGROUND, apArgs, std::size(apArgs));
		s_UpdateLaunchFailed = !s_UpdateLaunched;
	}
	if(s_UpdateLaunchFailed)
		Ui()->DoLabel(&VersionUpdate, Localize("Could not start Axion updater"), 12.0f, TEXTALIGN_MR);
	else if(s_UpdateLaunched)
		Ui()->DoLabel(&VersionUpdate, Localize("Axion updater is running"), 12.0f, TEXTALIGN_MR);
#endif
#if defined(CONF_AUTOUPDATE)
	CUIRect UpdateButton;
	VersionUpdate.VSplitRight(100.0f, &VersionUpdate, &UpdateButton);
	VersionUpdate.VSplitRight(10.0f, &VersionUpdate, nullptr);

	char aBuf[128];
	const IUpdater::EUpdaterState State = Updater()->GetCurrentState();
	const bool NeedUpdate = GameClient()->m_TClient.NeedUpdate();

	if(State == IUpdater::CLEAN && NeedUpdate)
	{
		static CButtonContainer s_VersionUpdate;
		if(GameClient()->m_Menus.DoButton_Menu(&s_VersionUpdate, Localize("Update now"), 0, &UpdateButton, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)))
		{
			Updater()->InitiateUpdate();
		}
	}
	else if(State == IUpdater::NEED_RESTART)
	{
		static CButtonContainer s_VersionUpdate;
		if(GameClient()->m_Menus.DoButton_Menu(&s_VersionUpdate, Localize("Restart"), 0, &UpdateButton, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)))
		{
			Client()->Restart();
		}
	}
	else if(State >= IUpdater::GETTING_MANIFEST && State < IUpdater::NEED_RESTART)
	{
		Ui()->RenderProgressBar(UpdateButton, Updater()->GetCurrentPercent() / 100.0f);
	}

	if(State == IUpdater::CLEAN && NeedUpdate)
	{
		str_format(aBuf, sizeof(aBuf), Localize("TClient %s is out!"), GameClient()->m_TClient.m_aVersionStr);
		TextRender()->TextColor(1.0f, 0.4f, 0.4f, 1.0f);
	}
	else if(State == IUpdater::CLEAN)
	{
		aBuf[0] = '\0';
	}
	else if(State >= IUpdater::GETTING_MANIFEST && State < IUpdater::NEED_RESTART)
	{
		char aCurrentFile[64];
		Updater()->GetCurrentFile(aCurrentFile, sizeof(aCurrentFile));
		str_format(aBuf, sizeof(aBuf), Localize("Downloading %s:"), aCurrentFile);
	}
	else if(State == IUpdater::FAIL)
	{
		str_copy(aBuf, Localize("Update failed! Check log…"));
		TextRender()->TextColor(1.0f, 0.4f, 0.4f, 1.0f);
	}
	else if(State == IUpdater::NEED_RESTART)
	{
		str_copy(aBuf, Localize("DDNet Client updated!"));
		TextRender()->TextColor(1.0f, 0.4f, 0.4f, 1.0f);
	}
	Ui()->DoLabel(&VersionUpdate, aBuf, 14.0f, TEXTALIGN_ML);
	TextRender()->TextColor(TextRender()->DefaultTextColor());
#elif defined(CONF_INFORM_UPDATE)
	if(str_comp(Client()->LatestVersion(), "0") != 0 && false)
	{
		CUIRect DownloadButton;
		VersionUpdate.VSplitRight(100.0f, &VersionUpdate, &DownloadButton);
		VersionUpdate.VSplitRight(10.0f, &VersionUpdate, nullptr);

		static CButtonContainer s_DownloadButton;
		if(GameClient()->m_Menus.DoButton_Menu(&s_DownloadButton, Localize("Download"), 0, &DownloadButton, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)))
		{
			Client()->ViewLink("https://ddnet.org/downloads/");
		}

		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), Localize("DDNet %s is out!"), Client()->LatestVersion());
		SLabelProperties UpdateLabelProps;
		UpdateLabelProps.SetColor(ColorRGBA(1.0f, 0.4f, 0.4f, 1.0f));
		Ui()->DoLabel(&VersionUpdate, aBuf, 14.0f, TEXTALIGN_ML, UpdateLabelProps);
	}
#endif

	if(NewPage != -1)
	{
		GameClient()->m_Menus.SetShowStart(false);
		GameClient()->m_Menus.SetMenuPage(NewPage);
	}
}

bool CMenusStart::CheckHotKey(int Key) const
{
	return !Input()->ShiftIsPressed() && !Input()->ModifierIsPressed() && !Input()->AltIsPressed() && // no modifier
	       Input()->KeyPress(Key) &&
	       !GameClient()->m_GameConsole.IsActive();
}
