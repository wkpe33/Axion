#include "axion_background_presets.h"

#include <base/io.h>
#include <base/str.h>

#include <engine/shared/json.h>
#include <engine/shared/jsonwriter.h>
#include <engine/storage.h>

#include <algorithm>
#include <memory>

bool SAxionBackground::Valid() const
{
	return m_Mode >= 0 && m_Mode <= 1 && m_Direction >= 0 && m_Direction <= 2 && m_Primary <= 0xffffff && m_Secondary <= 0xffffff;
}
int CAxionBackgroundPresets::Find(const char *pName) const
{
	for(size_t Index = 0; Index < m_Presets.size(); ++Index)
		if(str_utf8_comp_nocase(m_Presets[Index].m_Name.c_str(), pName) == 0)
			return (int)Index;
	return -1;
}
bool CAxionBackgroundPresets::ValidateName(const char *pName)
{
	if(!pName || !*pName || str_length(pName) > 63 || !str_utf8_check(pName))
	{
		m_Error = "Enter a valid name (1-63 UTF-8 bytes)";
		return false;
	}
	bool NonSpace = false;
	for(const unsigned char *p = (const unsigned char *)pName; *p; ++p)
	{
		if(*p < 32 || *p == 127)
		{
			m_Error = "Names cannot contain control characters";
			return false;
		}
		NonSpace |= *p != ' ';
	}
	if(!NonSpace)
	{
		m_Error = "Enter a preset name";
		return false;
	}
	return true;
}
void CAxionBackgroundPresets::Load(IStorage *pStorage)
{
	m_Loaded = true;
	m_ReadOnly = false;
	m_Presets.clear();
	m_Error.clear();
	IOHANDLE File = pStorage->OpenFile(FILE_NAME, IOFLAG_READ, IStorage::TYPE_SAVE);
	if(!File)
	{
		if(pStorage->FileExists(FILE_NAME, IStorage::TYPE_SAVE))
		{
			m_ReadOnly = true;
			m_Error = "Could not read presets; original file preserved";
		}
		return;
	}
	const int64_t Length = io_length(File);
	if(Length < 2 || Length > 2 * 1024 * 1024)
	{
		io_close(File);
		m_Error = "Invalid preset file size; original file preserved";
		m_ReadOnly = true;
		return;
	}
	std::string Data((size_t)Length, '\0');
	const bool ReadOk = io_read(File, Data.data(), Data.size()) == Data.size();
	io_close(File);
	json_settings Settings{};
	Settings.max_memory = 16 * 1024 * 1024;
	char aError[256];
	std::unique_ptr<json_value, decltype(&json_value_free)> Root(ReadOk ? JsonParseEx(&Settings, Data.data(), Data.size(), aError) : nullptr, json_value_free);
	if(!Root || Root->type != json_object)
	{
		m_Error = "Malformed presets JSON; original file preserved";
		m_ReadOnly = true;
		return;
	}
	const auto *pVersion = json_object_get(Root.get(), "version");
	const auto *pArray = json_object_get(Root.get(), "presets");
	if(!pVersion || pVersion->type != json_integer || pVersion->u.integer != 1 || !pArray || pArray->type != json_array || pArray->u.array.length > MAX_PRESETS)
	{
		m_Error = "Unsupported preset file; original file preserved";
		m_ReadOnly = true;
		return;
	}
	for(unsigned Index = 0; Index < pArray->u.array.length; ++Index)
	{
		const auto *pItem = json_array_get(pArray, Index);
		const auto *pName = json_object_get(pItem, "name");
		const auto *pMode = json_object_get(pItem, "mode");
		const auto *pPrimary = json_object_get(pItem, "primary");
		const auto *pSecondary = json_object_get(pItem, "secondary");
		const auto *pDirection = json_object_get(pItem, "direction");
		const bool Valid = pItem && pItem->type == json_object && pName && pName->type == json_string && pName->u.string.length == (unsigned)str_length(json_string_get(pName)) &&
				   pMode && pMode->type == json_integer && pMode->u.integer >= 0 && pMode->u.integer <= 1 &&
				   pPrimary && pPrimary->type == json_integer && pPrimary->u.integer >= 0 && pPrimary->u.integer <= 0xffffff &&
				   pSecondary && pSecondary->type == json_integer && pSecondary->u.integer >= 0 && pSecondary->u.integer <= 0xffffff &&
				   pDirection && pDirection->type == json_integer && pDirection->u.integer >= 0 && pDirection->u.integer <= 2;
		if(!Valid || !ValidateName(json_string_get(pName)) || Find(json_string_get(pName)) >= 0)
		{
			m_ReadOnly = true;
			continue;
		}
		m_Presets.push_back({json_string_get(pName), {(int)pMode->u.integer, (unsigned)pPrimary->u.integer, (unsigned)pSecondary->u.integer, (int)pDirection->u.integer}});
	}
	if(m_ReadOnly)
		m_Error = "Invalid/duplicate presets skipped; file preserved read-only. Repair then reload.";
	else
		m_Error.clear();
}
bool CAxionBackgroundPresets::Save(IStorage *pStorage, const std::vector<SPreset> &Candidate)
{
	if(m_ReadOnly)
	{
		m_Error = "Preset file is read-only after a parse error; repair then reload";
		return false;
	}
	CJsonStringWriter Json;
	Json.BeginObject();
	Json.WriteAttribute("version");
	Json.WriteIntValue(1);
	Json.WriteAttribute("presets");
	Json.BeginArray();
	for(const auto &Preset : Candidate)
	{
		Json.BeginObject();
		Json.WriteAttribute("name");
		Json.WriteStrValue(Preset.m_Name.c_str());
		Json.WriteAttribute("mode");
		Json.WriteIntValue(Preset.m_Background.m_Mode);
		Json.WriteAttribute("primary");
		Json.WriteIntValue(Preset.m_Background.m_Primary);
		Json.WriteAttribute("secondary");
		Json.WriteIntValue(Preset.m_Background.m_Secondary);
		Json.WriteAttribute("direction");
		Json.WriteIntValue(Preset.m_Background.m_Direction);
		Json.EndObject();
	}
	Json.EndArray();
	Json.EndObject();
	const std::string Output = Json.GetOutputString();
	IOHANDLE File = pStorage->OpenFile("axion_entities_backgrounds.json.tmp", IOFLAG_WRITE, IStorage::TYPE_SAVE);
	if(!File)
	{
		m_Error = "Could not create temporary preset file";
		return false;
	}
	const bool Written = io_write(File, Output.data(), Output.size()) == Output.size();
	const bool Closed = io_close(File) == 0;
	if(!Written || !Closed || !pStorage->RenameFile("axion_entities_backgrounds.json.tmp", FILE_NAME, IStorage::TYPE_SAVE))
	{
		m_Error = "Could not save presets; original file preserved";
		return false;
	}
	m_Presets = Candidate;
	m_Error.clear();
	return true;
}
bool CAxionBackgroundPresets::Create(IStorage *pStorage, const char *pName, SAxionBackground Background)
{
	if(!ValidateName(pName))
		return false;
	if(Find(pName) >= 0)
	{
		m_Error = "Name already exists; use Update to overwrite";
		return false;
	}
	if(!Background.Valid() || m_Presets.size() >= MAX_PRESETS)
	{
		m_Error = "Invalid background or preset limit reached";
		return false;
	}
	auto Candidate = m_Presets;
	Candidate.push_back({pName, Background});
	return Save(pStorage, Candidate);
}
bool CAxionBackgroundPresets::Update(IStorage *pStorage, int Index, SAxionBackground Background)
{
	if(Index < 0 || Index >= (int)m_Presets.size() || !Background.Valid())
		return false;
	auto Candidate = m_Presets;
	Candidate[Index].m_Background = Background;
	return Save(pStorage, Candidate);
}
bool CAxionBackgroundPresets::Rename(IStorage *pStorage, int Index, const char *pName)
{
	if(Index < 0 || Index >= (int)m_Presets.size() || !ValidateName(pName))
		return false;
	const int Existing = Find(pName);
	if(Existing >= 0 && Existing != Index)
	{
		m_Error = "Name already exists";
		return false;
	}
	auto Candidate = m_Presets;
	Candidate[Index].m_Name = pName;
	return Save(pStorage, Candidate);
}
bool CAxionBackgroundPresets::Delete(IStorage *pStorage, int Index)
{
	if(Index < 0 || Index >= (int)m_Presets.size())
		return false;
	auto Candidate = m_Presets;
	Candidate.erase(Candidate.begin() + Index);
	return Save(pStorage, Candidate);
}
