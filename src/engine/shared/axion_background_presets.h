#ifndef ENGINE_SHARED_AXION_BACKGROUND_PRESETS_H
#define ENGINE_SHARED_AXION_BACKGROUND_PRESETS_H
#include <string>
#include <vector>
class IStorage;
struct SAxionBackground
{
	int m_Mode = 0;
	unsigned m_Primary = 128, m_Secondary = 128;
	int m_Direction = 0;
	bool Valid() const;
};
class CAxionBackgroundPresets
{
public:
	struct SPreset
	{
		std::string m_Name;
		SAxionBackground m_Background;
	};
	static constexpr const char *FILE_NAME = "axion_entities_backgrounds.json";
	static constexpr size_t MAX_PRESETS = 4096;
	void Load(IStorage *pStorage);
	bool Create(IStorage *pStorage, const char *pName, SAxionBackground Background);
	bool Update(IStorage *pStorage, int Index, SAxionBackground Background);
	bool Rename(IStorage *pStorage, int Index, const char *pName);
	bool Delete(IStorage *pStorage, int Index);
	const std::vector<SPreset> &Presets() const { return m_Presets; }
	const char *Error() const { return m_Error.c_str(); }
	bool Loaded() const { return m_Loaded; }
	bool ReadOnly() const { return m_ReadOnly; }
	int Find(const char *pName) const;

private:
	std::vector<SPreset> m_Presets;
	std::string m_Error;
	bool m_Loaded = false, m_ReadOnly = false;
	bool Save(IStorage *pStorage, const std::vector<SPreset> &Candidate);
	bool ValidateName(const char *pName);
};
#endif
