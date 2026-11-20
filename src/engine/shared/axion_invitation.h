#ifndef ENGINE_SHARED_AXION_INVITATION_H
#define ENGINE_SHARED_AXION_INVITATION_H
#include <charconv>
#include <string_view>
// Exact stock DDNet server message. This parser must only receive server chat.
struct SAxionInvitation
{
	std::string_view m_Name;
	int m_Team = 0;
};
inline bool AxionParseInvitation(std::string_view Message, SAxionInvitation &Out, int MaxTeams)
{
	constexpr std::string_view Marker = "' invited you to team ";
	constexpr std::string_view Middle = ". Use /team ";
	constexpr std::string_view End = " to join.";
	if(Message.empty() || Message.front() != '\'')
		return false;
	const auto Pos = Message.rfind(Marker);
	if(Pos == std::string_view::npos || Pos < 2 || !Message.ends_with(End))
		return false;
	auto Rest = Message.substr(Pos + Marker.size());
	const auto Split = Rest.find(Middle);
	if(Split == std::string_view::npos)
		return false;
	int Team = 0, CommandTeam = 0;
	auto A = Rest.substr(0, Split);
	auto B = Rest.substr(Split + Middle.size());
	B.remove_suffix(End.size());
	const auto PA = std::from_chars(A.data(), A.data() + A.size(), Team);
	const auto PB = std::from_chars(B.data(), B.data() + B.size(), CommandTeam);
	if(PA.ec != std::errc{} || PA.ptr != A.data() + A.size() || PB.ec != std::errc{} || PB.ptr != B.data() + B.size() || Team != CommandTeam || Team <= 0 || Team >= MaxTeams)
		return false;
	Out = {Message.substr(1, Pos - 1), Team};
	return true;
}
#endif
