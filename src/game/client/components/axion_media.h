#ifndef GAME_CLIENT_COMPONENTS_AXION_MEDIA_H
#define GAME_CLIENT_COMPONENTS_AXION_MEDIA_H
#include <cstdint>
#include <memory>
#include <string>

// Windows media-session behaviour adapted from BestClient (BESTCLIENT-LICENSE.txt).
// All OS calls, including app-volume changes, run on the worker.
class CAxionMedia
{
public:
	struct SSnapshot
	{
		bool m_Valid = false, m_Playing = false;
		bool m_CanPrevious = false, m_CanNext = false, m_CanPause = false;
		bool m_CanSeek = false;
		float m_Volume = -1.0f;
		int64_t m_PositionMs = 0, m_DurationMs = 0;
		std::string m_Title, m_Source;
	};
	CAxionMedia();
	~CAxionMedia();
	SSnapshot Snapshot();
	bool SnapshotIfChanged(SSnapshot &Snapshot, uint64_t &Revision);
	void Action(int Action); // -1 previous, 0 play/pause, 1 next
	void Volume(float Volume);
	void Seek(int64_t PositionMs);
	void SetEnabled(bool Enabled);

private:
	class CImpl;
	std::unique_ptr<CImpl> m_pImpl;
};
#endif
