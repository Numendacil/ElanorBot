#ifndef _PJSK_MUSIC_INFO_HPP_
#define _PJSK_MUSIC_INFO_HPP_

#include <Command/GroupCommandBase.hpp>

namespace GroupCommand
{

class pjskMusicInfo : public GroupCommandBase
{
public:
	virtual bool Parse(const Cyan::MessageChain& msg, std::vector<std::string>& tokens) override;
	virtual bool Execute(const Cyan::GroupMessage& gm, Bot::Group& group, const std::vector<std::string>& tokens) override;
};

}

#endif