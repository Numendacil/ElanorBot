#include <exception>
#include <filesystem>
#include <fstream>
#include <State/State.hpp>

#include <ThirdParty/json.hpp>
#include <ThirdParty/log.h>

#include <Utils/Timer.hpp>
#include <memory>

#include "Group.hpp"


using namespace std;
using json = nlohmann::json;

namespace Bot
{

unordered_map<string, unique_ptr<State::StateBase>> RegisterStates()
{
	unordered_map<string, unique_ptr<State::StateBase>> v;

	#define REGISTER(_class_) v[string(_class_::_NAME_)] = make_unique<_class_>();

	REGISTER(State::AccessCtrlList)
	REGISTER(State::Activity)
	REGISTER(State::BililiveList)
	REGISTER(State::CommandPerm)
	REGISTER(State::CoolDown)
	REGISTER(State::LastMessage)
	REGISTER(State::TriggerStatus)

	#undef REGISTER
	
	return v;
}


Group::Group(Cyan::GID_t group_id, Cyan::QQ_t owner_id,
	const std::vector<std::pair<std::string, int>>& command_list,
	const std::vector<std::pair<std::string, bool>>& trigger_list) 
	: gid(group_id), suid(owner_id), States(RegisterStates())
{
	auto command = this->GetState<State::CommandPerm>();
	for (const auto& p : command_list)
		command->AddCommand(p.first, p.second);

	auto trigger = this->GetState<State::TriggerStatus>();
	for (const auto& p : trigger_list)
		trigger->AddTrigger(p.first, p.second);

	this->FromFile();
	
	Utils::Timer::GetInstance().LaunchLoop([this]{ this->ToFile(); }, chrono::hours(1), true);
}

void Group::ToFile(void)
{
	json content;
	{
		lock_guard<mutex> lk(this->mtx);
		for (const auto& p : this->States)
			if (!p.second->Serialize().empty())
				content["States"][p.first] = p.second->Serialize();
	}

	filesystem::path Path = "./bot";
	try
	{
		filesystem::create_directory(Path);
	}
	catch(const std::exception& e)
	{
		logging::WARN("Failed to create directory ./bot/: " + std::string(e.what()));
		return;
	}

	Path /= to_string(this->gid.ToInt64());
	{
		lock_guard<mutex> lk(this->mtx_file);
		ofstream file(Path);
		if (!file)
		{
			logging::WARN("Failed to open file " + string(Path) + " for writing");
			return;
		}
		logging::INFO("Writing to file " + string(Path));
		file << content.dump(1, '\t');
	}
}

void Group::FromFile(void)
{
	json content;
	filesystem::path Path = "./bot";
	Path /= to_string(this->gid.ToInt64());
	if (!filesystem::exists(Path))
		return;
	try
	{
		lock_guard<mutex> lk(this->mtx_file);
		ifstream file(Path);
		if (!file)
		{
			logging::WARN("Failed to open file " + string(Path) + " for reading");
			return;
		}
		content = json::parse(file);
	}
	catch(json::parse_error& e)
	{
		logging::WARN("Failed to parse file " + string(Path) + " :" + e.what());
		return;
	}
	logging::INFO("Reading from file " + string(Path));
	{
		lock_guard<mutex> lk(this->mtx);
		if (content.contains("States"))
		{
			assert(content["States"].type() == json::value_t::object);
			for (const auto& p : content["States"].items())
				if (this->States.count(p.key()))
					this->States.at(p.key())->Deserialize(p.value());
		}
	}
}

}