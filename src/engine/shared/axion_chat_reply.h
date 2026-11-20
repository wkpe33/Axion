#ifndef ENGINE_SHARED_AXION_CHAT_REPLY_H
#define ENGINE_SHARED_AXION_CHAT_REPLY_H
#include <optional>
#include <string>
#include <string_view>
inline std::optional<std::string> AxionChatReply(std::string_view Name, std::string_view Draft, std::string_view PreviousPrefix, size_t Capacity)
{
	std::string Prefix(Name);
	Prefix += ": ";
	if(Draft.starts_with(Prefix))
		return std::string(Draft);
	if(!PreviousPrefix.empty() && Draft.starts_with(PreviousPrefix))
		Draft.remove_prefix(PreviousPrefix.size());
	if(Prefix.size() + Draft.size() >= Capacity)
		return std::nullopt;
	Prefix.append(Draft);
	return Prefix;
}
#endif
