#include "axion_media.h"

#include <base/detect.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <utility>

#if defined(CONF_FAMILY_WINDOWS)
#ifdef NOGDI
#undef NOGDI
#endif
#include <audiopolicy.h>
#include <mmdeviceapi.h>
#include <windows.h>
#include <wingdi.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Control.h>
#include <winrt/base.h>
namespace Media = winrt::Windows::Media::Control;

// Match CoreAudio sessions to the selected media app. Never change master volume
// or DDNet's game/chat volume when the media app cannot be identified.
static float AppVolume(const std::string &Source, float Requested)
{
	winrt::com_ptr<IMMDeviceEnumerator> Enumerator;
	if(FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(Enumerator.put()))))
		return -1;
	winrt::com_ptr<IMMDevice> Device;
	if(FAILED(Enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, Device.put())))
		return -1;
	winrt::com_ptr<IAudioSessionManager2> Manager;
	if(FAILED(Device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, Manager.put_void())))
		return -1;
	winrt::com_ptr<IAudioSessionEnumerator> Sessions;
	if(FAILED(Manager->GetSessionEnumerator(Sessions.put())))
		return -1;
	std::string LowerSource = Source;
	std::transform(LowerSource.begin(), LowerSource.end(), LowerSource.begin(), [](unsigned char C) { return (char)std::tolower(C); });
	int Count = 0;
	Sessions->GetCount(&Count);
	float Result = -1;
	for(int i = 0; i < Count; ++i)
	{
		winrt::com_ptr<IAudioSessionControl> Control;
		if(FAILED(Sessions->GetSession(i, Control.put())))
			continue;
		auto Control2 = Control.try_as<IAudioSessionControl2>();
		auto Volume = Control.try_as<ISimpleAudioVolume>();
		DWORD Pid = 0;
		if(!Control2 || !Volume || FAILED(Control2->GetProcessId(&Pid)) || Pid == GetCurrentProcessId() || Pid == 0)
			continue;
		HANDLE Process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, Pid);
		if(!Process)
			continue;
		wchar_t aPath[1024];
		DWORD Size = 1024;
		const bool HasPath = QueryFullProcessImageNameW(Process, 0, aPath, &Size) != 0;
		bool SameAppId = false;
		using FGetAppId = LONG(WINAPI *)(HANDLE, UINT32 *, PWSTR);
		static const auto GetAppId = reinterpret_cast<FGetAppId>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "GetApplicationUserModelId"));
		if(GetAppId)
		{
			wchar_t aAppId[512];
			UINT32 AppIdSize = 512;
			if(GetAppId(Process, &AppIdSize, aAppId) == ERROR_SUCCESS)
				SameAppId = Source == winrt::to_string(aAppId);
		}
		CloseHandle(Process);
		if(!HasPath)
			continue;
		std::wstring Path(aPath, Size);
		const auto Slash = Path.find_last_of(L"\\/");
		std::string Name = winrt::to_string(Path.substr(Slash == std::wstring::npos ? 0 : Slash + 1));
		std::transform(Name.begin(), Name.end(), Name.begin(), [](unsigned char C) { return (char)std::tolower(C); });
		const auto Dot = Name.rfind('.');
		const std::string Stem = Name.substr(0, Dot);
		if(!SameAppId && (Stem.empty() || LowerSource.find(Stem) == std::string::npos))
			continue;
		if(Requested >= 0)
			Volume->SetMasterVolume(std::clamp(Requested, 0.0f, 1.0f), nullptr);
		Volume->GetMasterVolume(&Result);
	}
	return Result;
}
#endif

class CAxionMedia::CImpl
{
public:
	std::mutex m_Mutex;
	std::condition_variable m_Condition;
	bool m_Stop = false, m_Enabled = true;
	std::deque<int> m_Actions;
	float m_RequestedVolume = -1;
	int64_t m_RequestedPosition = -1;
	SSnapshot m_Snapshot;
	uint64_t m_Revision=0;
	std::thread m_Worker;
	CImpl() :
		m_Worker([this] { Run(); }) {}
	~CImpl()
	{
		{
			std::lock_guard<std::mutex> Lock(m_Mutex);
			m_Stop = true;
		}
		m_Condition.notify_one();
		m_Worker.join();
	}
	void Run()
	{
#if defined(CONF_FAMILY_WINDOWS)
		try
		{
			winrt::init_apartment(winrt::apartment_type::multi_threaded);
		}
		catch(...)
		{
			return;
		}
		{
			Media::GlobalSystemMediaTransportControlsSessionManager Manager{nullptr};
			Media::GlobalSystemMediaTransportControlsSession Selected{nullptr};
			std::string TrackedSource;
			while(true)
			{
				std::deque<int> Actions;
				float RequestedVolume;
				int64_t RequestedPosition;
				{
					std::unique_lock<std::mutex> Lock(m_Mutex);
					m_Condition.wait(Lock, [this] { return m_Stop || m_Enabled; });
					if(m_Stop)
						break;
					Actions.swap(m_Actions);
					RequestedVolume = std::exchange(m_RequestedVolume, -1.0f);
					RequestedPosition = std::exchange(m_RequestedPosition, -1);
				}
				SSnapshot Snapshot;
				try
				{
					if(!Manager)
						Manager = Media::GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
					int BestScore = -1;
					auto Current = Manager.GetCurrentSession();
					const auto CurrentId = Current ? Current.SourceAppUserModelId() : winrt::hstring();
					Selected = nullptr;
					// BestClient's session priority: playing, tracked, Windows current.
					for(auto Session : Manager.GetSessions())
					{
						auto Info = Session.GetPlaybackInfo();
						const auto Status = Info.PlaybackStatus();
						const bool Playing = Status == Media::GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing;
						if(Status == Media::GlobalSystemMediaTransportControlsSessionPlaybackStatus::Closed || Status == Media::GlobalSystemMediaTransportControlsSessionPlaybackStatus::Stopped)
							continue;
						const auto Id = Session.SourceAppUserModelId();
						const int Score = (Playing ? 20 : 10) + (winrt::to_string(Id) == TrackedSource ? 5 : 0) + (Id == CurrentId ? 3 : 0);
						if(Score > BestScore)
						{
							Selected = Session;
							BestScore = Score;
						}
					}
					if(Selected)
					{
						const auto NewSource = winrt::to_string(Selected.SourceAppUserModelId());
						const bool SameSource = NewSource == TrackedSource;
						// Commands belong to the source that was displayed when queued.
						if(NewSource == TrackedSource)
							for(int Action : Actions)
							{
								if(Action < 0)
									Selected.TrySkipPreviousAsync().get();
								else if(Action > 0)
									Selected.TrySkipNextAsync().get();
								else
									Selected.TryTogglePlayPauseAsync().get();
							}
						Snapshot.m_Volume = AppVolume(NewSource, NewSource == TrackedSource ? RequestedVolume : -1);
						TrackedSource = NewSource;
						Snapshot.m_Source = NewSource;
						auto Info = Selected.GetPlaybackInfo();
						auto Controls = Info.Controls();
						Snapshot.m_Playing = Info.PlaybackStatus() == Media::GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing;
						Snapshot.m_CanPrevious = Controls.IsPreviousEnabled();
						Snapshot.m_CanNext = Controls.IsNextEnabled();
						Snapshot.m_CanPause = Controls.IsPlayEnabled() || Controls.IsPauseEnabled() || Controls.IsPlayPauseToggleEnabled();
						auto Timeline = Selected.GetTimelineProperties();
						Snapshot.m_CanSeek = Controls.IsPlaybackPositionEnabled();
						if(SameSource && RequestedPosition >= 0 && Snapshot.m_CanSeek)
						{
							Selected.TryChangePlaybackPositionAsync(Timeline.StartTime().count() + RequestedPosition * 10000).get();
							Timeline = Selected.GetTimelineProperties();
						}
						Snapshot.m_PositionMs = std::max<int64_t>(0, (Timeline.Position() - Timeline.StartTime()).count() / 10000);
						Snapshot.m_DurationMs = std::max<int64_t>(0, (Timeline.EndTime() - Timeline.StartTime()).count() / 10000);
						// Windows timeline positions are timestamps, not continuously updated counters.
						if(Snapshot.m_Playing)
							Snapshot.m_PositionMs += std::clamp<int64_t>((winrt::clock::now() - Timeline.LastUpdatedTime()).count() / 10000, 0, 86400000);
						if(Snapshot.m_DurationMs > 0)
							Snapshot.m_PositionMs = std::min(Snapshot.m_PositionMs, Snapshot.m_DurationMs);
						Snapshot.m_Title = winrt::to_string(Selected.TryGetMediaPropertiesAsync().get().Title());
						Snapshot.m_Valid = true;
					}
				}
				catch(...)
				{
					Manager = nullptr;
				}
				{
					std::unique_lock<std::mutex> Lock(m_Mutex);
					m_Snapshot = std::move(Snapshot);
					++m_Revision;
					m_Condition.wait_for(Lock, std::chrono::milliseconds(500), [this] { return m_Stop || !m_Actions.empty() || m_RequestedVolume >= 0 || m_RequestedPosition >= 0; });
				}
			}
		}
		winrt::uninit_apartment();
#endif
	}
};

CAxionMedia::CAxionMedia() :
	m_pImpl(std::make_unique<CImpl>()) {}
CAxionMedia::~CAxionMedia() = default;
CAxionMedia::SSnapshot CAxionMedia::Snapshot()
{
	std::lock_guard<std::mutex> Lock(m_pImpl->m_Mutex);
	return m_pImpl->m_Snapshot;
}
void CAxionMedia::Action(int Action)
{
	{
		std::lock_guard<std::mutex> Lock(m_pImpl->m_Mutex);
		if(m_pImpl->m_Actions.size() < 16)
			m_pImpl->m_Actions.push_back(Action);
	}
	m_pImpl->m_Condition.notify_one();
}
void CAxionMedia::Seek(int64_t PositionMs)
{
	{
		std::lock_guard<std::mutex> Lock(m_pImpl->m_Mutex);
		m_pImpl->m_RequestedPosition = PositionMs;
	}
	m_pImpl->m_Condition.notify_one();
}

void CAxionMedia::Volume(float Volume)
{
	{
		std::lock_guard<std::mutex> Lock(m_pImpl->m_Mutex);
		m_pImpl->m_RequestedVolume = std::clamp(Volume, 0.0f, 1.0f);
	}
	m_pImpl->m_Condition.notify_one();
}

void CAxionMedia::SetEnabled(bool Enabled)
{
	{
		std::lock_guard<std::mutex> Lock(m_pImpl->m_Mutex);
		if(m_pImpl->m_Enabled == Enabled) return;
		m_pImpl->m_Enabled = Enabled;
		if(!Enabled)
		{
			m_pImpl->m_Actions.clear();
			m_pImpl->m_RequestedVolume = -1;
			m_pImpl->m_RequestedPosition = -1;
		}
	}
	m_pImpl->m_Condition.notify_one();
}

bool CAxionMedia::SnapshotIfChanged(SSnapshot &Snapshot, uint64_t &Revision)
{
 std::unique_lock<std::mutex> Lock(m_pImpl->m_Mutex,std::try_to_lock);
 if(!Lock.owns_lock() || Revision==m_pImpl->m_Revision) return false;
 Snapshot=m_pImpl->m_Snapshot;
 Revision=m_pImpl->m_Revision;
 return true;
}
