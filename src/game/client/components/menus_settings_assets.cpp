#include "menus.h"

#include <base/dbg.h>
#include <base/fs.h>
#include <base/io.h>
#include <generated/client_data.h>
#include <base/str.h>
#include <base/time.h>

#include <engine/font_icons.h>
#include <engine/gfx/image_manipulation.h>
#include <engine/shared/axion_texture_limits.h>
#include <engine/shared/config.h>
#include <engine/storage.h>
#include <engine/textrender.h>

#include <game/client/gameclient.h>
#include <game/client/ui_listbox.h>
#include <game/client/ui_scrollregion.h>
#include <game/localization.h>

#include <chrono>
#include <string>
#include <vector>

using namespace std::chrono_literals;

typedef std::function<void()> TMenuAssetScanLoadedFunc;

struct SMenuAssetScanUser
{
	void *m_pUser;
	TMenuAssetScanLoadedFunc m_LoadedFunc;
};

// IDs of the tabs in the Assets menu
enum
{
	ASSETS_TAB_ENTITIES = 0,
	ASSETS_TAB_GAME = 1,
	ASSETS_TAB_EMOTICONS = 2,
	ASSETS_TAB_PARTICLES = 3,
	ASSETS_TAB_HUD = 4,
	ASSETS_TAB_EXTRAS = 5,
	ASSETS_TAB_AUDIO = 6,
	NUMBER_OF_ASSETS_TABS = 7,
};

static IGraphics::CTextureHandle LoadAssetPreview(IGraphics *pGraphics, const char *pPath)
{
	CImageInfo Image;
	if(!pGraphics->LoadPng(Image, pPath, IStorage::TYPE_ALL))
		return pGraphics->LoadTexture(pPath, IStorage::TYPE_ALL);
	const auto [W, H] = AxionBoundImage(Image.m_Width, Image.m_Height, 256, 256);
	if(W != (int)Image.m_Width || H != (int)Image.m_Height)
		ResizeImage(Image, W, H);
	return pGraphics->LoadTextureRawMove(Image, 0, pPath);
}

void CMenus::LoadEntities(SCustomEntities *pEntitiesItem, void *pUser)
{
	auto *pRealUser = (SMenuAssetScanUser *)pUser;
	auto *pThis = (CMenus *)pRealUser->m_pUser;

	char aPath[IO_MAX_PATH_LENGTH];
	if(str_comp(pEntitiesItem->m_aName, "default") == 0)
	{
		for(int i = 0; i < MAP_IMAGE_MOD_TYPE_COUNT; ++i)
		{
			str_format(aPath, sizeof(aPath), "editor/entities_clear/%s.png", gs_apModEntitiesNames[i]);
			pEntitiesItem->m_aImages[i].m_Texture = LoadAssetPreview(pThis->Graphics(), aPath);
			if(!pEntitiesItem->m_RenderTexture.IsValid() || pEntitiesItem->m_RenderTexture.IsNullTexture())
				pEntitiesItem->m_RenderTexture = pEntitiesItem->m_aImages[i].m_Texture;
		}
	}
	else
	{
		for(int i = 0; i < MAP_IMAGE_MOD_TYPE_COUNT; ++i)
		{
			str_format(aPath, sizeof(aPath), "assets/entities/%s/%s.png", pEntitiesItem->m_aName, gs_apModEntitiesNames[i]);
			pEntitiesItem->m_aImages[i].m_Texture = LoadAssetPreview(pThis->Graphics(), aPath);
			if(pEntitiesItem->m_aImages[i].m_Texture.IsNullTexture())
			{
				str_format(aPath, sizeof(aPath), "assets/entities/%s.png", pEntitiesItem->m_aName);
				pEntitiesItem->m_aImages[i].m_Texture = LoadAssetPreview(pThis->Graphics(), aPath);
			}
			if(!pEntitiesItem->m_RenderTexture.IsValid() || pEntitiesItem->m_RenderTexture.IsNullTexture())
				pEntitiesItem->m_RenderTexture = pEntitiesItem->m_aImages[i].m_Texture;
		}
	}
}

int CMenus::EntitiesScan(const char *pName, int IsDir, int DirType, void *pUser)
{
	auto *pRealUser = (SMenuAssetScanUser *)pUser;
	auto *pThis = (CMenus *)pRealUser->m_pUser;
	if(IsDir)
	{
		if(pName[0] == '.')
			return 0;

		// default is reserved
		if(str_comp(pName, "default") == 0)
			return 0;

		SCustomEntities EntitiesItem;
		str_copy(EntitiesItem.m_aName, pName);

		pThis->m_vEntitiesList.push_back(EntitiesItem);
	}
	else
	{
		if(str_endswith(pName, ".png"))
		{
			char aName[IO_MAX_PATH_LENGTH];
			str_truncate(aName, sizeof(aName), pName, str_length(pName) - 4);
			// default is reserved
			if(str_comp(aName, "default") == 0)
				return 0;

			SCustomEntities EntitiesItem;
			str_copy(EntitiesItem.m_aName, aName);

			pThis->m_vEntitiesList.push_back(EntitiesItem);
		}
	}

	pRealUser->m_LoadedFunc();

	return 0;
}

template<typename TName>
static void LoadAsset(TName *pAssetItem, const char *pAssetName, IGraphics *pGraphics)
{
	char aPath[IO_MAX_PATH_LENGTH];
	if(str_comp(pAssetItem->m_aName, "default") == 0)
	{
		str_format(aPath, sizeof(aPath), "%s.png", pAssetName);
		pAssetItem->m_RenderTexture = LoadAssetPreview(pGraphics, aPath);
	}
	else
	{
		str_format(aPath, sizeof(aPath), "assets/%s/%s.png", pAssetName, pAssetItem->m_aName);
		pAssetItem->m_RenderTexture = LoadAssetPreview(pGraphics, aPath);
		if(pAssetItem->m_RenderTexture.IsNullTexture())
		{
			str_format(aPath, sizeof(aPath), "assets/%s/%s/%s.png", pAssetName, pAssetItem->m_aName, pAssetName);
			pAssetItem->m_RenderTexture = LoadAssetPreview(pGraphics, aPath);
		}
	}
}

template<typename TName>
static int AssetScan(const char *pName, int IsDir, int DirType, std::vector<TName> &vAssetList, const char *pAssetName, IGraphics *pGraphics, void *pUser)
{
	auto *pRealUser = (SMenuAssetScanUser *)pUser;
	if(IsDir)
	{
		if(pName[0] == '.')
			return 0;

		// default is reserved
		if(str_comp(pName, "default") == 0)
			return 0;

		TName AssetItem;
		str_copy(AssetItem.m_aName, pName);

		vAssetList.push_back(AssetItem);
	}
	else
	{
		if(str_endswith(pName, ".png"))
		{
			char aName[IO_MAX_PATH_LENGTH];
			str_truncate(aName, sizeof(aName), pName, str_length(pName) - 4);
			// default is reserved
			if(str_comp(aName, "default") == 0)
				return 0;

			TName AssetItem;
			str_copy(AssetItem.m_aName, aName);

			vAssetList.push_back(AssetItem);
		}
	}

	pRealUser->m_LoadedFunc();

	return 0;
}

int CMenus::GameScan(const char *pName, int IsDir, int DirType, void *pUser)
{
	auto *pRealUser = (SMenuAssetScanUser *)pUser;
	auto *pThis = (CMenus *)pRealUser->m_pUser;
	IGraphics *pGraphics = pThis->Graphics();
	return AssetScan(pName, IsDir, DirType, pThis->m_vGameList, "game", pGraphics, pUser);
}

int CMenus::EmoticonsScan(const char *pName, int IsDir, int DirType, void *pUser)
{
	auto *pRealUser = (SMenuAssetScanUser *)pUser;
	auto *pThis = (CMenus *)pRealUser->m_pUser;
	IGraphics *pGraphics = pThis->Graphics();
	return AssetScan(pName, IsDir, DirType, pThis->m_vEmoticonList, "emoticons", pGraphics, pUser);
}

int CMenus::ParticlesScan(const char *pName, int IsDir, int DirType, void *pUser)
{
	auto *pRealUser = (SMenuAssetScanUser *)pUser;
	auto *pThis = (CMenus *)pRealUser->m_pUser;
	IGraphics *pGraphics = pThis->Graphics();
	return AssetScan(pName, IsDir, DirType, pThis->m_vParticlesList, "particles", pGraphics, pUser);
}

int CMenus::HudScan(const char *pName, int IsDir, int DirType, void *pUser)
{
	auto *pRealUser = (SMenuAssetScanUser *)pUser;
	auto *pThis = (CMenus *)pRealUser->m_pUser;
	IGraphics *pGraphics = pThis->Graphics();
	return AssetScan(pName, IsDir, DirType, pThis->m_vHudList, "hud", pGraphics, pUser);
}

int CMenus::ExtrasScan(const char *pName, int IsDir, int DirType, void *pUser)
{
	auto *pRealUser = (SMenuAssetScanUser *)pUser;
	auto *pThis = (CMenus *)pRealUser->m_pUser;
	IGraphics *pGraphics = pThis->Graphics();
	return AssetScan(pName, IsDir, DirType, pThis->m_vExtrasList, "extras", pGraphics, pUser);
}

static std::vector<const CMenus::SCustomEntities *> gs_vpSearchEntitiesList;
static std::vector<const CMenus::SCustomGame *> gs_vpSearchGamesList;
static std::vector<const CMenus::SCustomEmoticon *> gs_vpSearchEmoticonsList;
static std::vector<const CMenus::SCustomParticle *> gs_vpSearchParticlesList;
static std::vector<const CMenus::SCustomHud *> gs_vpSearchHudList;
static std::vector<const CMenus::SCustomExtras *> gs_vpSearchExtrasList;

static bool gs_aInitCustomList[NUMBER_OF_ASSETS_TABS] = {
	true,
};

static size_t gs_aCustomListSize[NUMBER_OF_ASSETS_TABS] = {
	0,
};

static CLineInputBuffered<64> s_aFilterInputs[NUMBER_OF_ASSETS_TABS];

static int s_CurCustomTab = ASSETS_TAB_ENTITIES;
static int s_EditorImageTab = 0;

static const CMenus::SCustomItem *GetCustomItem(int CurTab, size_t Index)
{
	if(CurTab == ASSETS_TAB_ENTITIES)
		return gs_vpSearchEntitiesList[Index];
	else if(CurTab == ASSETS_TAB_GAME)
		return gs_vpSearchGamesList[Index];
	else if(CurTab == ASSETS_TAB_EMOTICONS)
		return gs_vpSearchEmoticonsList[Index];
	else if(CurTab == ASSETS_TAB_PARTICLES)
		return gs_vpSearchParticlesList[Index];
	else if(CurTab == ASSETS_TAB_HUD)
		return gs_vpSearchHudList[Index];
	else if(CurTab == ASSETS_TAB_EXTRAS)
		return gs_vpSearchExtrasList[Index];
	dbg_assert_failed("Invalid CurTab: %d", CurTab);
}

template<typename TName>
static void ClearAssetList(std::vector<TName> &vList, IGraphics *pGraphics)
{
	for(TName &Asset : vList)
	{
		pGraphics->UnloadTexture(&Asset.m_RenderTexture);
	}
	vList.clear();
}

void CMenus::ClearCustomItems(int CurTab)
{
	if(CurTab == ASSETS_TAB_ENTITIES)
	{
		for(auto &Entity : m_vEntitiesList)
		{
			for(auto &Image : Entity.m_aImages)
			{
				Graphics()->UnloadTexture(&Image.m_Texture);
			}
		}
		m_vEntitiesList.clear();

		// reload current entities
		GameClient()->m_MapImages.ChangeEntitiesPath(g_Config.m_ClAssetsEntities);
	}
	else if(CurTab == ASSETS_TAB_GAME)
	{
		ClearAssetList(m_vGameList, Graphics());

		// reload current game skin
		GameClient()->LoadGameSkin(g_Config.m_ClAssetGame);
	}
	else if(CurTab == ASSETS_TAB_EMOTICONS)
	{
		ClearAssetList(m_vEmoticonList, Graphics());

		// reload current emoticons skin
		GameClient()->LoadEmoticonsSkin(g_Config.m_ClAssetEmoticons);
	}
	else if(CurTab == ASSETS_TAB_PARTICLES)
	{
		ClearAssetList(m_vParticlesList, Graphics());

		// reload current particles skin
		GameClient()->LoadParticlesSkin(g_Config.m_ClAssetParticles);
	}
	else if(CurTab == ASSETS_TAB_HUD)
	{
		ClearAssetList(m_vHudList, Graphics());

		// reload current hud skin
		GameClient()->LoadHudSkin(g_Config.m_ClAssetHud);
	}
	else if(CurTab == ASSETS_TAB_EXTRAS)
	{
		ClearAssetList(m_vExtrasList, Graphics());

		// reload current DDNet particles skin
		GameClient()->LoadExtrasSkin(g_Config.m_ClAssetExtras);
	}
	else
	{
		dbg_assert_failed("Invalid CurTab: %d", CurTab);
	}
	gs_aInitCustomList[CurTab] = true;
}

template<typename TName, typename TCaller>
static void InitAssetList(std::vector<TName> &vAssetList, const char *pAssetPath, const char *pAssetName, FS_LISTDIR_CALLBACK pfnCallback, IGraphics *pGraphics, IStorage *pStorage, TCaller Caller)
{
	if(vAssetList.empty())
	{
		TName AssetItem;
		str_copy(AssetItem.m_aName, "default");

		vAssetList.push_back(AssetItem);

		// load assets
		pStorage->ListDirectory(IStorage::TYPE_ALL, pAssetPath, pfnCallback, Caller);
		std::sort(vAssetList.begin(), vAssetList.end());
	}
	if(vAssetList.size() != gs_aCustomListSize[s_CurCustomTab])
		gs_aInitCustomList[s_CurCustomTab] = true;
}

template<typename TName>
static int InitSearchList(std::vector<const TName *> &vpSearchList, std::vector<TName> &vAssetList)
{
	vpSearchList.clear();
	int ListSize = vAssetList.size();
	for(int i = 0; i < ListSize; ++i)
	{
		const TName *pAsset = &vAssetList[i];

		// filter quick search
		if(!s_aFilterInputs[s_CurCustomTab].IsEmpty() && !str_utf8_find_nocase(pAsset->m_aName, s_aFilterInputs[s_CurCustomTab].GetString()))
			continue;

		vpSearchList.push_back(pAsset);
	}
	return vAssetList.size();
}

namespace
{
constexpr int AUDIO_EVENTS[] = {SOUND_HAMMER_HIT, SOUND_HAMMER_FIRE, SOUND_HOOK_ATTACH_GROUND, SOUND_HOOK_ATTACH_PLAYER, SOUND_PLAYER_AIRJUMP, SOUND_PLAYER_SPAWN, SOUND_PLAYER_DIE};
std::vector<std::string> s_Packs;
std::string s_Donor, s_Base, s_Status, s_aSources[std::size(AUDIO_EVENTS)];
CLineInputBuffered<64> s_ExportName;

void ScanAudioPacks(IStorage *pStorage)
{
	s_Packs = {"default"};
	const auto Scan = [](const char *pName, int IsDir, int, void *pData) {
		auto &Packs = *static_cast<std::vector<std::string> *>(pData);
		if(IsDir && pName[0] != '.' && std::find(Packs.begin(), Packs.end(), pName) == Packs.end())
			Packs.emplace_back(pName);
		return 0;
	};
	pStorage->ListDirectory(IStorage::TYPE_ALL, "assets/audio", Scan, &s_Packs);
	pStorage->ListDirectory(IStorage::TYPE_ALL, "audio", Scan, &s_Packs);
	std::sort(s_Packs.begin() + 1, s_Packs.end());
}

bool ExportAudioPack(IStorage *pStorage, const char *pName, const std::string &Base, const std::string (&aSources)[std::size(AUDIO_EVENTS)], std::string &Status)
{
	if(!*pName || str_comp_nocase(pName, "default") == 0 || pName[0] == '.' ||
		pName[str_length(pName) - 1] == '.' || pName[str_length(pName) - 1] == ' ' ||
		std::string(pName).find_first_of("<>:\"/\\|?*") != std::string::npos ||
		std::any_of(pName, pName + str_length(pName), [](unsigned char Ch) { return Ch < 32; }))
	{
		Status = Localize("Enter a valid pack name");
		return false;
	}
	const std::string Folder = std::string("assets/audio/") + pName;
	const std::string Temp = "assets/audio/.axion-export";
	if(pStorage->FolderExists(Folder.c_str(), IStorage::TYPE_ALL) || pStorage->FolderExists((std::string("audio/") + pName).c_str(), IStorage::TYPE_ALL))
	{
		Status = Localize("A sound pack with this name already exists");
		return false;
	}
	pStorage->CreateFolder("assets", IStorage::TYPE_SAVE);
	pStorage->CreateFolder("assets/audio", IStorage::TYPE_SAVE);
	if(pStorage->FolderExists(Temp.c_str(), IStorage::TYPE_SAVE) || !pStorage->CreateFolder(Temp.c_str(), IStorage::TYPE_SAVE))
	{
		Status = Localize("Could not create a temporary folder");
		return false;
	}
	std::vector<std::string> vWritten;
	bool Ok = true;
	for(int Id = 0; Id < g_pData->m_NumSounds && Ok; ++Id)
	{
		const std::string *pPack = &Base;
		for(size_t Index = 0; Index < std::size(AUDIO_EVENTS); ++Index)
			if(AUDIO_EVENTS[Index] == Id && !aSources[Index].empty())
				pPack = &aSources[Index];
		const auto &Set = g_pData->m_aSounds[Id];
		for(int Sample = 0; Sample < Set.m_NumSounds && Ok; ++Sample)
		{
			const char *pFile = Set.m_aSounds[Sample].m_pFilename;
			const char *pShort = str_startswith(pFile, "audio/");
			if(!pShort || str_find(pShort, "/"))
				continue;
			char aPath[IO_MAX_PATH_LENGTH];
			const char *pSource = CSounds::ResolvePackFile(pStorage, pPack->c_str(), pFile, aPath);
			void *pBytes = nullptr;
			unsigned Size = 0;
			if(!pStorage->ReadFile(pSource, IStorage::TYPE_ALL, &pBytes, &Size))
			{
				Ok = false;
				break;
			}
			const std::string Dest = Temp + "/" + pShort;
			IOHANDLE File = pStorage->OpenFile(Dest.c_str(), IOFLAG_WRITE, IStorage::TYPE_SAVE);
			if(File)
			{
				vWritten.push_back(Dest);
				Ok = io_write(File, pBytes, Size) == Size;
				Ok = io_close(File) == 0 && Ok;
			}
			else
				Ok = false;
			free(pBytes);
		}
	}
	if(Ok && pStorage->RenameFile(Temp.c_str(), Folder.c_str(), IStorage::TYPE_SAVE))
	{
		Status = Localize("Sound pack saved");
		return true;
	}
	for(const auto &File : vWritten)
		pStorage->RemoveFile(File.c_str(), IStorage::TYPE_SAVE);
	char aTemp[IO_MAX_PATH_LENGTH];
	pStorage->GetCompletePath(IStorage::TYPE_SAVE, Temp.c_str(), aTemp, sizeof(aTemp));
	(void)fs_removedir(aTemp);
	Status = Localize("Could not save sound pack");
	return false;
}
}

bool CMenus::s_AudioPackEditorOpen = false;

void CMenus::RenderAudioPackEditor(CUIRect View)
{
	if(s_Packs.empty() || Input()->KeyPress(KEY_F5))
		ScanAudioPacks(Storage());
	bool RefreshAfterExport = false;
	static CButtonContainer s_Close, s_Save, s_aPick[2];
	static CButtonContainer s_aPlay[2][std::size(AUDIO_EVENTS)], s_aCopy[std::size(AUDIO_EVENTS)], s_aReset[std::size(AUDIO_EVENTS)];
	static CUi::SSelectionPopupContext s_aPicker[2];
	static CScrollRegion s_aPickerScroll[2];
	static int s_Drag = -1;
	View.Margin(8, &View);
	View.Draw(ColorRGBA(0.075f, 0.08f, 0.09f, 0.98f), IGraphics::CORNER_ALL, 10);
	View.Margin(14, &View);
	CUIRect Header, Close, Footer, Save, Status;
	View.HSplitTop(21, &Header, &View);
	Header.VSplitRight(78, &Header, &Close);
	Ui()->DoLabel(&Header, Localize("Sound pack editor"), 17, TEXTALIGN_ML);
	if(DoButton_Menu(&s_Close, Localize("Back"), 0, &Close) || (!Ui()->IsPopupOpen() && Ui()->ConsumeHotkey(CUi::HOTKEY_ESCAPE)))
	{
		s_Drag = -1;
		s_AudioPackEditorOpen = false;
		return;
	}
	View.HSplitTop(10, nullptr, &View);
	View.HSplitBottom(21, &View, &Footer);
	View.HSplitBottom(20, &View, &Status);
	Footer.VSplitRight(118, &Footer, &Save);
	Footer.VSplitRight(8, &Footer, nullptr);
	s_ExportName.SetEmptyText(Localize("New sound pack name"));
	Ui()->DoEditBox(&s_ExportName, &Footer, 13);
	if(DoButton_Menu(&s_Save, Localize("Save sound pack"), 0, &Save))
		RefreshAfterExport = ExportAudioPack(Storage(), s_ExportName.GetString(), s_Base, s_aSources, s_Status);

	View.HSplitTop(std::min(View.h, 62.0f + 29.0f * std::size(AUDIO_EVENTS)), &View, nullptr);
	CUIRect aPanels[2];
	View.VSplitMid(&aPanels[0], &aPanels[1], 14);
	std::string *apPacks[] = {&s_Donor, &s_Base};
	const char *apTitles[] = {Localize("Source pack"), Localize("Base pack and result")};
	const char *apLabels[] = {Localize("Hammer hit"), Localize("Hammer swing"), Localize("Hook attaches to wall"), Localize("Hook attaches to player"), Localize("Double jump"), Localize("Respawn"), Localize("Spectating")};
	const bool Released = !Ui()->MouseButton(0) && Ui()->LastMouseButton(0);
	for(int Side = 0; Side < 2; ++Side)
	{
		CUIRect Panel = aPanels[Side];
		Panel.Draw(ColorRGBA(1, 1, 1, 0.06f), IGraphics::CORNER_ALL, 7);
		Panel.Margin(9, &Panel);
		CUIRect Title, Picker;
		Panel.HSplitTop(19, &Title, &Panel);
		Ui()->DoLabel(&Title, apTitles[Side], 14, TEXTALIGN_ML);
		Panel.HSplitTop(20, &Picker, &Panel);
		auto &Popup = s_aPicker[Side];
		if(Popup.m_pSelection)
		{
			*apPacks[Side] = *Popup.m_pSelection;
			Popup.m_pSelection = nullptr;
		}
		char aPickLabel[128];
		str_format(aPickLabel, sizeof(aPickLabel), Localize("Sound pack: %s"), apPacks[Side]->c_str());
		if(DoButton_Menu(&s_aPick[Side], aPickLabel, 0, &Picker))
		{
			Popup.Reset();
			Popup.m_pScrollRegion = &s_aPickerScroll[Side];
			Popup.m_vEntries = s_Packs;
			Popup.m_Width = Picker.w;
			Ui()->ShowPopupSelection(Picker.x, Picker.y + Picker.h, &Popup);
		}
		Panel.HSplitTop(6, nullptr, &Panel);
		const float RowHeight = Panel.h / std::size(AUDIO_EVENTS);
		for(size_t Index = 0; Index < std::size(AUDIO_EVENTS); ++Index)
		{
			CUIRect Row, Listen, Action;
			Panel.HSplitTop(RowHeight, &Row, &Panel);
			Row.HMargin(2, &Row);
			const bool Over = Row.Inside(Ui()->MousePos());
			const bool Drop = Side == 1 && s_Drag == (int)Index;
			Row.Draw(ColorRGBA(1, 1, 1, Drop ? (Over ? 0.27f : 0.15f) : 0.07f), IGraphics::CORNER_ALL, 4);
			Row.VSplitRight(19, &Row, &Action);
			Row.VSplitRight(4, &Row, nullptr);
			Row.VSplitRight(19, &Row, &Listen);
			Action.Margin(1, &Action);
			Listen.Margin(1, &Listen);
			Row.VMargin(6, &Row);
			if(Side == 0 && !Ui()->IsPopupOpen() && Row.Inside(Ui()->MousePos()) && Ui()->MouseButtonClicked(0))
				s_Drag = Index;
			if(Side == 0 && Ui()->DoButton_FontIcon(&s_aCopy[Index], FontIcon::CHEVRON_RIGHT, 0, &Action, BUTTONFLAG_LEFT))
				s_aSources[Index] = s_Donor;
			if(Side == 1 && !s_aSources[Index].empty() && Ui()->DoButton_FontIcon(&s_aReset[Index], FontIcon::ARROW_ROTATE_LEFT, 0, &Action, BUTTONFLAG_LEFT))
				s_aSources[Index].clear();
			if(Drop && Released && Over && !Ui()->IsPopupOpen())
				s_aSources[Index] = s_Donor;
			const std::string &Pack = Side == 1 && !s_aSources[Index].empty() ? s_aSources[Index] : *apPacks[Side];
			if(Ui()->DoButton_FontIcon(&s_aPlay[Side][Index], FontIcon::PLAY, 0, &Listen, BUTTONFLAG_LEFT))
				s_Status = GameClient()->m_Sounds.PreviewPackSound(AUDIO_EVENTS[Index], Pack.c_str()) ? "" : Localize("Sound unavailable");
			SLabelProperties Props;
			Props.m_MaxWidth = Row.w;
			Props.m_EllipsisAtEnd = true;
			if(Side == 1)
			{
				CUIRect Origin;
				Row.HSplitBottom(Row.h * 0.43f, &Row, &Origin);
				Ui()->DoLabel(&Origin, Pack.c_str(), std::min(10.0f, Origin.h), TEXTALIGN_ML, Props);
			}
			Ui()->DoLabel(&Row, apLabels[Index], std::min(13.0f, Row.h), TEXTALIGN_ML, Props);
		}
	}
	if(!Ui()->MouseButton(0))
		s_Drag = -1;
	Ui()->DoLabel(&Status, s_Status.empty() ? Localize("Drag a sound to the right or click its arrow") : s_Status.c_str(), 11, TEXTALIGN_ML);
	if(RefreshAfterExport)
		s_Packs.clear();
}

void CMenus::RenderSettingsAssets(CUIRect MainView)
{
	CUIRect TabBar, CustomList, QuickSearch, DirectoryButton, ReloadButton;
	const bool Picking = AssetsEditorPicking();
	if(Picking)
	{
		CUIRect Back;
		MainView.HSplitTop(20.0f, &Back, &MainView);
		Back.VSplitLeft(130.0f, &Back, nullptr);
		static CButtonContainer s_Back;
		if(DoButton_Menu(&s_Back, Localize("Back to editor"), 0, &Back) || Ui()->ConsumeHotkey(CUi::HOTKEY_ESCAPE))
		{
			AssetsEditorCancelPick();
			return;
		}
		MainView.HSplitTop(5.0f, nullptr, &MainView);
		if(s_CurCustomTab == ASSETS_TAB_AUDIO)
			s_CurCustomTab = ASSETS_TAB_GAME;
	}
	else
	{
		CUIRect Editor;
		MainView.HSplitTop(22.0f, &Editor, &MainView);
		Editor.VSplitRight(s_CurCustomTab == ASSETS_TAB_AUDIO ? 190.0f : 140.0f, nullptr, &Editor);
		static CButtonContainer s_Editor;
		if(DoButton_Menu(&s_Editor, s_CurCustomTab == ASSETS_TAB_AUDIO ? Localize("Create sound pack") : Localize("Assets editor"), 0, &Editor))
		{
			if(s_CurCustomTab == ASSETS_TAB_AUDIO)
			{
				s_AudioPackEditorOpen = true;
				s_Base = *g_Config.m_SndPack ? g_Config.m_SndPack : "default";
				s_Donor = s_Base;
				for(auto &Source : s_aSources)
					Source.clear();
				s_Status.clear();
				s_ExportName.Clear();
				return;
			}
			else
			{
				AssetsEditorOpen();
				return;
			}
		}
		MainView.HSplitTop(5.0f, nullptr, &MainView);
	}

	MainView.HSplitTop(20.0f, &TabBar, &MainView);
	const int StandardTabs = Picking ? ASSETS_TAB_AUDIO : NUMBER_OF_ASSETS_TABS;
	const int VisibleTabs = StandardTabs + (Picking ? 2 : 0);
	const float TabWidth = TabBar.w / VisibleTabs;
	static CButtonContainer s_aPageTabs[NUMBER_OF_ASSETS_TABS] = {};
	const char *apTabNames[NUMBER_OF_ASSETS_TABS] = {
		Localize("Entities"),
		Localize("Game"),
		Localize("Emoticons"),
		Localize("Particles"),
		Localize("HUD"),
		Localize("Extras"),
		Localize("Audio")};

	for(int Tab = ASSETS_TAB_ENTITIES; Tab < StandardTabs; ++Tab)
	{
		CUIRect Button;
		TabBar.VSplitLeft(TabWidth, &Button, &TabBar);
		const int Corners = Tab == ASSETS_TAB_ENTITIES ? IGraphics::CORNER_L : (Tab == VisibleTabs - 1 ? IGraphics::CORNER_R : IGraphics::CORNER_NONE);
		if(DoButton_MenuTab(&s_aPageTabs[Tab], apTabNames[Tab], (!Picking || s_EditorImageTab == 0) && s_CurCustomTab == Tab, &Button, Corners, nullptr, nullptr, nullptr, nullptr, 4.0f))
		{
			s_CurCustomTab = Tab;
			s_EditorImageTab = 0;
		}
	}

	if(Picking)
	{
		static CButtonContainer s_aImageTabs[2];
		const char *apNames[] = {Localize("Cursor"), Localize("Arrow")};
		for(int i = 0; i < 2; ++i)
		{
			CUIRect Button;
			TabBar.VSplitLeft(TabWidth, &Button, &TabBar);
			if(DoButton_MenuTab(&s_aImageTabs[i], apNames[i], s_EditorImageTab == i + 1, &Button, i == 1 ? IGraphics::CORNER_R : IGraphics::CORNER_NONE, nullptr))
				s_EditorImageTab = i + 1;
		}
		if(s_EditorImageTab)
		{
			AssetsEditorRenderSingleImagePicker(MainView, ASSETS_TAB_AUDIO + s_EditorImageTab - 1);
			return;
		}
	}
	else
		s_EditorImageTab = 0;


	if(s_CurCustomTab == ASSETS_TAB_AUDIO)
	{
		static CLineInputBuffered<64> s_Search;
		static CListBox s_AudioList;
		static CButtonContainer s_RefreshAudio, s_DirectoryAudio;
		if(s_Packs.empty() || Input()->KeyPress(KEY_F5))
			ScanAudioPacks(Storage());

		MainView.HSplitTop(10, nullptr, &MainView);
		CUIRect Footer, Search, Directory, Refresh;
		MainView.HSplitBottom(24, &MainView, &Footer);
		Footer.VSplitRight(68, &Footer, &Refresh);
		Footer.VSplitRight(6, &Footer, nullptr);
		Footer.VSplitRight(110, &Search, &Directory);
		Search.VSplitRight(6, &Search, nullptr);
		if(DoButton_Menu(&s_RefreshAudio, Localize("Refresh"), 0, &Refresh))
		{
			s_Packs.clear();
			return;
		}
		Ui()->DoEditBox_Search(&s_Search, &Search, 12, !Ui()->IsPopupOpen());
		if(DoButton_Menu(&s_DirectoryAudio, Localize("Audio directory"), 0, &Directory))
		{
			Storage()->CreateFolder("assets", IStorage::TYPE_SAVE);
			Storage()->CreateFolder("assets/audio", IStorage::TYPE_SAVE);
			char aPath[IO_MAX_PATH_LENGTH];
			Storage()->GetCompletePath(IStorage::TYPE_SAVE, "assets/audio", aPath, sizeof(aPath));
			Client()->ViewFile(aPath);
		}
		MainView.HSplitBottom(8, &MainView, nullptr);
		std::vector<const std::string *> Visible;
		int Selected = -1;
		for(const auto &Pack : s_Packs)
			if(s_Search.IsEmpty() || str_utf8_find_nocase(Pack.c_str(), s_Search.GetString()))
			{
				if(Pack == g_Config.m_SndPack)
					Selected = Visible.size();
				Visible.push_back(&Pack);
			}
		s_AudioList.DoStart(28, Visible.size(), 1, 1, Selected, &MainView);
		for(size_t i = 0; i < Visible.size(); ++i)
		{
			const auto Item = s_AudioList.DoNextItem(Visible[i], (int)i == Selected);
			if(Item.m_Visible)
			{
				CUIRect Label = Item.m_Rect;
				Label.VMargin(10, &Label);
				Ui()->DoLabel(&Label, Visible[i]->c_str(), 13, TEXTALIGN_ML);
			}
		}
		const int NewSelected = s_AudioList.DoEnd();
		if(NewSelected >= 0 && NewSelected < (int)Visible.size() && NewSelected != Selected)
		{
			str_copy(g_Config.m_SndPack, Visible[NewSelected]->c_str());
			for(int Event : AUDIO_EVENTS)
				CSounds::OverridePack(Event)[0] = 0;
			CSounds::OverridePack(SOUND_TEE_CRY)[0] = 0;
		}
		return;
	}

	auto LoadStartTime = time_get_nanoseconds();
	SMenuAssetScanUser User;
	User.m_pUser = this;
	User.m_LoadedFunc = [&]() {
		if(time_get_nanoseconds() - LoadStartTime > 500ms)
			RenderLoading(Localize("Loading assets"), "", 0);
	};
	if(s_CurCustomTab == ASSETS_TAB_ENTITIES)
	{
		if(m_vEntitiesList.empty())
		{
			SCustomEntities EntitiesItem;
			str_copy(EntitiesItem.m_aName, "default");

			m_vEntitiesList.push_back(EntitiesItem);

			// load entities
			Storage()->ListDirectory(IStorage::TYPE_ALL, "assets/entities", EntitiesScan, &User);
			std::sort(m_vEntitiesList.begin(), m_vEntitiesList.end());
		}
		if(m_vEntitiesList.size() != gs_aCustomListSize[s_CurCustomTab])
			gs_aInitCustomList[s_CurCustomTab] = true;
	}
	else if(s_CurCustomTab == ASSETS_TAB_GAME)
	{
		InitAssetList(m_vGameList, "assets/game", "game", GameScan, Graphics(), Storage(), &User);
	}
	else if(s_CurCustomTab == ASSETS_TAB_EMOTICONS)
	{
		InitAssetList(m_vEmoticonList, "assets/emoticons", "emoticons", EmoticonsScan, Graphics(), Storage(), &User);
	}
	else if(s_CurCustomTab == ASSETS_TAB_PARTICLES)
	{
		InitAssetList(m_vParticlesList, "assets/particles", "particles", ParticlesScan, Graphics(), Storage(), &User);
	}
	else if(s_CurCustomTab == ASSETS_TAB_HUD)
	{
		InitAssetList(m_vHudList, "assets/hud", "hud", HudScan, Graphics(), Storage(), &User);
	}
	else if(s_CurCustomTab == ASSETS_TAB_EXTRAS)
	{
		InitAssetList(m_vExtrasList, "assets/extras", "extras", ExtrasScan, Graphics(), Storage(), &User);
	}
	else
	{
		dbg_assert_failed("Invalid s_CurCustomTab: %d", s_CurCustomTab);
	}

	MainView.HSplitTop(10.0f, nullptr, &MainView);

	// skin selector
	MainView.HSplitTop(MainView.h - 10.0f - ms_ButtonHeight, &CustomList, &MainView);
	if(gs_aInitCustomList[s_CurCustomTab])
	{
		int ListSize = 0;
		if(s_CurCustomTab == ASSETS_TAB_ENTITIES)
		{
			gs_vpSearchEntitiesList.clear();
			ListSize = m_vEntitiesList.size();
			for(int i = 0; i < ListSize; ++i)
			{
				const SCustomEntities *pEntity = &m_vEntitiesList[i];

				// filter quick search
				if(!s_aFilterInputs[s_CurCustomTab].IsEmpty() && !str_utf8_find_nocase(pEntity->m_aName, s_aFilterInputs[s_CurCustomTab].GetString()))
					continue;

				gs_vpSearchEntitiesList.push_back(pEntity);
			}
		}
		else if(s_CurCustomTab == ASSETS_TAB_GAME)
		{
			ListSize = InitSearchList(gs_vpSearchGamesList, m_vGameList);
		}
		else if(s_CurCustomTab == ASSETS_TAB_EMOTICONS)
		{
			ListSize = InitSearchList(gs_vpSearchEmoticonsList, m_vEmoticonList);
		}
		else if(s_CurCustomTab == ASSETS_TAB_PARTICLES)
		{
			ListSize = InitSearchList(gs_vpSearchParticlesList, m_vParticlesList);
		}
		else if(s_CurCustomTab == ASSETS_TAB_HUD)
		{
			ListSize = InitSearchList(gs_vpSearchHudList, m_vHudList);
		}
		else if(s_CurCustomTab == ASSETS_TAB_EXTRAS)
		{
			ListSize = InitSearchList(gs_vpSearchExtrasList, m_vExtrasList);
		}
		gs_aInitCustomList[s_CurCustomTab] = false;
		gs_aCustomListSize[s_CurCustomTab] = ListSize;
	}

	int OldSelected = -1;
	float Margin = 10;
	float TextureWidth = 150;
	float TextureHeight = 150;

	size_t SearchListSize = 0;

	if(s_CurCustomTab == ASSETS_TAB_ENTITIES)
	{
		SearchListSize = gs_vpSearchEntitiesList.size();
	}
	else if(s_CurCustomTab == ASSETS_TAB_GAME)
	{
		SearchListSize = gs_vpSearchGamesList.size();
		TextureHeight = 75;
	}
	else if(s_CurCustomTab == ASSETS_TAB_EMOTICONS)
	{
		SearchListSize = gs_vpSearchEmoticonsList.size();
	}
	else if(s_CurCustomTab == ASSETS_TAB_PARTICLES)
	{
		SearchListSize = gs_vpSearchParticlesList.size();
	}
	else if(s_CurCustomTab == ASSETS_TAB_HUD)
	{
		SearchListSize = gs_vpSearchHudList.size();
	}
	else if(s_CurCustomTab == ASSETS_TAB_EXTRAS)
	{
		SearchListSize = gs_vpSearchExtrasList.size();
	}

	static CListBox s_ListBox;
	s_ListBox.DoStart(TextureHeight + 15.0f + 10.0f + Margin, SearchListSize, CustomList.w / (Margin + TextureWidth), 1, OldSelected, &CustomList, false);
	for(size_t i = 0; i < SearchListSize; ++i)
	{
		const SCustomItem *pItem = GetCustomItem(s_CurCustomTab, i);
		if(pItem == nullptr)
			continue;

		if(s_CurCustomTab == ASSETS_TAB_ENTITIES)
		{
			if(str_comp(pItem->m_aName, g_Config.m_ClAssetsEntities) == 0)
				OldSelected = i;
		}
		else if(s_CurCustomTab == ASSETS_TAB_GAME)
		{
			if(str_comp(pItem->m_aName, g_Config.m_ClAssetGame) == 0)
				OldSelected = i;
		}
		else if(s_CurCustomTab == ASSETS_TAB_EMOTICONS)
		{
			if(str_comp(pItem->m_aName, g_Config.m_ClAssetEmoticons) == 0)
				OldSelected = i;
		}
		else if(s_CurCustomTab == ASSETS_TAB_PARTICLES)
		{
			if(str_comp(pItem->m_aName, g_Config.m_ClAssetParticles) == 0)
				OldSelected = i;
		}
		else if(s_CurCustomTab == ASSETS_TAB_HUD)
		{
			if(str_comp(pItem->m_aName, g_Config.m_ClAssetHud) == 0)
				OldSelected = i;
		}
		else if(s_CurCustomTab == ASSETS_TAB_EXTRAS)
		{
			if(str_comp(pItem->m_aName, g_Config.m_ClAssetExtras) == 0)
				OldSelected = i;
		}

		const CListboxItem Item = s_ListBox.DoNextItem(pItem, !Picking && OldSelected >= 0 && (size_t)OldSelected == i);
		CUIRect ItemRect = Item.m_Rect;
		ItemRect.Margin(Margin / 2, &ItemRect);
		auto *pMutable = const_cast<SCustomItem *>(pItem); // Owned mutable lists above.
		if(!Item.m_Visible)
		{
			if(s_CurCustomTab == ASSETS_TAB_ENTITIES)
			{
				auto *pEntity = const_cast<SCustomEntities *>(gs_vpSearchEntitiesList[i]);
				for(auto &Image : pEntity->m_aImages)
					Graphics()->UnloadTexture(&Image.m_Texture);
				pEntity->m_RenderTexture.Invalidate();
			}
			else
				Graphics()->UnloadTexture(&pMutable->m_RenderTexture);
			continue;
		}
		if(!pMutable->m_RenderTexture.IsValid())
		{
			if(s_CurCustomTab == ASSETS_TAB_ENTITIES)
				LoadEntities(const_cast<SCustomEntities *>(gs_vpSearchEntitiesList[i]), &User);
			else
			{
				const char *apAssetNames[] = {"entities", "game", "emoticons", "particles", "hud", "extras"};
				LoadAsset(pMutable, apAssetNames[s_CurCustomTab], Graphics());
			}
		}

		CUIRect TextureRect;
		ItemRect.HSplitTop(15, &ItemRect, &TextureRect);
		TextureRect.HSplitTop(10, nullptr, &TextureRect);
		Ui()->DoLabel(&ItemRect, pItem->m_aName, ItemRect.h - 2, TEXTALIGN_MC);
		if(pItem->m_RenderTexture.IsValid())
		{
			Graphics()->WrapClamp();
			Graphics()->TextureSet(pItem->m_RenderTexture);
			Graphics()->QuadsBegin();
			Graphics()->SetColor(1, 1, 1, 1);
			IGraphics::CQuadItem QuadItem(TextureRect.x + (TextureRect.w - TextureWidth) / 2, TextureRect.y + (TextureRect.h - TextureHeight) / 2, TextureWidth, TextureHeight);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();
			Graphics()->WrapNormal();
		}
	}

	const int NewSelected = s_ListBox.DoEnd();
	if(Picking)
		OldSelected = -1;
	if(Picking && NewSelected >= 0 && NewSelected < (int)SearchListSize &&
		(NewSelected != OldSelected || s_ListBox.WasItemActivated()))
	{
		AssetsEditorPick(s_CurCustomTab, GetCustomItem(s_CurCustomTab, NewSelected)->m_aName);
		return;
	}
	if(!Picking && OldSelected != NewSelected && NewSelected >= 0 && NewSelected < (int)SearchListSize)
	{
		if(GetCustomItem(s_CurCustomTab, NewSelected)->m_aName[0] != '\0')
		{
			if(s_CurCustomTab == ASSETS_TAB_ENTITIES)
			{
				str_copy(g_Config.m_ClAssetsEntities, GetCustomItem(s_CurCustomTab, NewSelected)->m_aName);
				GameClient()->m_MapImages.ChangeEntitiesPath(GetCustomItem(s_CurCustomTab, NewSelected)->m_aName);
			}
			else if(s_CurCustomTab == ASSETS_TAB_GAME)
			{
				str_copy(g_Config.m_ClAssetGame, GetCustomItem(s_CurCustomTab, NewSelected)->m_aName);
				GameClient()->LoadGameSkin(g_Config.m_ClAssetGame);
			}
			else if(s_CurCustomTab == ASSETS_TAB_EMOTICONS)
			{
				str_copy(g_Config.m_ClAssetEmoticons, GetCustomItem(s_CurCustomTab, NewSelected)->m_aName);
				GameClient()->LoadEmoticonsSkin(g_Config.m_ClAssetEmoticons);
			}
			else if(s_CurCustomTab == ASSETS_TAB_PARTICLES)
			{
				str_copy(g_Config.m_ClAssetParticles, GetCustomItem(s_CurCustomTab, NewSelected)->m_aName);
				GameClient()->LoadParticlesSkin(g_Config.m_ClAssetParticles);
			}
			else if(s_CurCustomTab == ASSETS_TAB_HUD)
			{
				str_copy(g_Config.m_ClAssetHud, GetCustomItem(s_CurCustomTab, NewSelected)->m_aName);
				GameClient()->LoadHudSkin(g_Config.m_ClAssetHud);
			}
			else if(s_CurCustomTab == ASSETS_TAB_EXTRAS)
			{
				str_copy(g_Config.m_ClAssetExtras, GetCustomItem(s_CurCustomTab, NewSelected)->m_aName);
				GameClient()->LoadExtrasSkin(g_Config.m_ClAssetExtras);
			}
		}
	}

	// Quick search
	MainView.HSplitBottom(ms_ButtonHeight, &MainView, &QuickSearch);
	QuickSearch.VSplitLeft(220.0f, &QuickSearch, &DirectoryButton);
	QuickSearch.HSplitTop(5.0f, nullptr, &QuickSearch);
	if(Ui()->DoEditBox_Search(&s_aFilterInputs[s_CurCustomTab], &QuickSearch, 14.0f, !Ui()->IsPopupOpen() && !GameClient()->m_GameConsole.IsActive()))
	{
		gs_aInitCustomList[s_CurCustomTab] = true;
	}

	DirectoryButton.HSplitTop(5.0f, nullptr, &DirectoryButton);
	DirectoryButton.VSplitRight(175.0f, nullptr, &DirectoryButton);
	DirectoryButton.VSplitRight(25.0f, &DirectoryButton, &ReloadButton);
	DirectoryButton.VSplitRight(10.0f, &DirectoryButton, nullptr);
	static CButtonContainer s_AssetsDirId;
	if(DoButton_Menu(&s_AssetsDirId, Localize("Assets directory"), 0, &DirectoryButton))
	{
		char aBuf[IO_MAX_PATH_LENGTH];
		char aBufFull[IO_MAX_PATH_LENGTH + 7];
		if(s_CurCustomTab == ASSETS_TAB_ENTITIES)
			str_copy(aBufFull, "assets/entities");
		else if(s_CurCustomTab == ASSETS_TAB_GAME)
			str_copy(aBufFull, "assets/game");
		else if(s_CurCustomTab == ASSETS_TAB_EMOTICONS)
			str_copy(aBufFull, "assets/emoticons");
		else if(s_CurCustomTab == ASSETS_TAB_PARTICLES)
			str_copy(aBufFull, "assets/particles");
		else if(s_CurCustomTab == ASSETS_TAB_HUD)
			str_copy(aBufFull, "assets/hud");
		else if(s_CurCustomTab == ASSETS_TAB_EXTRAS)
			str_copy(aBufFull, "assets/extras");
		Storage()->GetCompletePath(IStorage::TYPE_SAVE, aBufFull, aBuf, sizeof(aBuf));
		Storage()->CreateFolder("assets", IStorage::TYPE_SAVE);
		Storage()->CreateFolder(aBufFull, IStorage::TYPE_SAVE);
		Client()->ViewFile(aBuf);
	}
	GameClient()->m_Tooltips.DoToolTip(&s_AssetsDirId, &DirectoryButton, Localize("Open the directory to add custom assets"));

	TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
	TextRender()->SetRenderFlags(ETextRenderFlags::TEXT_RENDER_FLAG_ONLY_ADVANCE_WIDTH | ETextRenderFlags::TEXT_RENDER_FLAG_NO_X_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_Y_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_PIXEL_ALIGNMENT | ETextRenderFlags::TEXT_RENDER_FLAG_NO_OVERSIZE);
	static CButtonContainer s_AssetsReloadBtnId;
	if(DoButton_Menu(&s_AssetsReloadBtnId, FontIcon::ARROW_ROTATE_RIGHT, 0, &ReloadButton) || Input()->KeyPress(KEY_F5) || (Input()->KeyPress(KEY_R) && Input()->ModifierIsPressed()))
	{
		ClearCustomItems(s_CurCustomTab);
	}
	TextRender()->SetRenderFlags(0);
	TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
}

void CMenus::ConchainAssetsEntities(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	CMenus *pThis = (CMenus *)pUserData;
	if(pResult->NumArguments() == 1)
	{
		const char *pArg = pResult->GetString(0);
		if(str_comp(pArg, g_Config.m_ClAssetsEntities) != 0)
		{
			pThis->GameClient()->m_MapImages.ChangeEntitiesPath(pArg);
		}
	}

	pfnCallback(pResult, pCallbackUserData);
}

void CMenus::ConchainAssetGame(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	CMenus *pThis = (CMenus *)pUserData;
	if(pResult->NumArguments() == 1)
	{
		const char *pArg = pResult->GetString(0);
		if(str_comp(pArg, g_Config.m_ClAssetGame) != 0)
		{
			pThis->GameClient()->LoadGameSkin(pArg);
		}
	}

	pfnCallback(pResult, pCallbackUserData);
}

void CMenus::ConchainAssetParticles(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	CMenus *pThis = (CMenus *)pUserData;
	if(pResult->NumArguments() == 1)
	{
		const char *pArg = pResult->GetString(0);
		if(str_comp(pArg, g_Config.m_ClAssetParticles) != 0)
		{
			pThis->GameClient()->LoadParticlesSkin(pArg);
		}
	}

	pfnCallback(pResult, pCallbackUserData);
}

void CMenus::ConchainAssetEmoticons(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	CMenus *pThis = (CMenus *)pUserData;
	if(pResult->NumArguments() == 1)
	{
		const char *pArg = pResult->GetString(0);
		if(str_comp(pArg, g_Config.m_ClAssetEmoticons) != 0)
		{
			pThis->GameClient()->LoadEmoticonsSkin(pArg);
		}
	}

	pfnCallback(pResult, pCallbackUserData);
}

void CMenus::ConchainAssetHud(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	CMenus *pThis = (CMenus *)pUserData;
	if(pResult->NumArguments() == 1)
	{
		const char *pArg = pResult->GetString(0);
		if(str_comp(pArg, g_Config.m_ClAssetHud) != 0)
		{
			pThis->GameClient()->LoadHudSkin(pArg);
		}
	}

	pfnCallback(pResult, pCallbackUserData);
}

void CMenus::ConchainAssetExtras(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	CMenus *pThis = (CMenus *)pUserData;
	if(pResult->NumArguments() == 1)
	{
		const char *pArg = pResult->GetString(0);
		if(str_comp(pArg, g_Config.m_ClAssetExtras) != 0)
		{
			pThis->GameClient()->LoadExtrasSkin(pArg);
		}
	}

	pfnCallback(pResult, pCallbackUserData);
}
