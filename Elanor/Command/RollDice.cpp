#include <ThirdParty/log.h>
#include <Utils/Utils.hpp>
#include <Group/Group.hpp>
#include <Client/Client.hpp>

#include "RollDice.hpp"

using namespace std;

namespace GroupCommand
{

bool RollDice::Parse(const Cyan::MessageChain& msg, vector<string>& tokens)
{
	string str = msg.GetPlainText();
	Utils::ReplaceMark(str);
	if (str.length() > char_traits<char>::length("#roll"))
	{
		Utils::ToLower(str);
		if (Utils::Tokenize(tokens, str, 2) < 2)
			return false;
		if (tokens[0] == "#roll")
			return true;
	}
	return false;
}

bool RollDice::Execute(const Cyan::GroupMessage& gm, Bot::Group& group, const vector<string>& tokens) 
{
	int i = 0;
	int j = 0;
	int result[10];
	Bot::Client& client = Bot::Client::GetClient();
	assert(tokens.size() > 1);
	logging::INFO("Calling RollDice <RollDice>" + Utils::GetDescription(gm));
	string command = tokens[1];
	if (command == "help" || command == "h")
	{
		logging::INFO("帮助文档 <RollDice>" + Utils::GetDescription(gm, false));
		client.Send(gm.Sender.Group.GID, Cyan::MessageChain().Plain("usage:\n#roll [x]D[y]"));
		return true;
	}
	while (command[i + j] >= '0' && command[i + j] <= '9')
		j++;
	if (command[i + j] == 'd')
	{
		try
		{
			int round;
			if (j == 0)
				round = 1;
			else
				round = stoi(command.substr(i, j));
			if (round > 10)
			{
				logging::INFO("投掷次数错误 <RollDice>: round = " + to_string(round) + Utils::GetDescription(gm, false));
				client.Send(gm.Sender.Group.GID, Cyan::MessageChain().Plain("骰子太多啦！"));
				return false;
			}
			if (round < 1)
			{
				logging::INFO("投掷次数错误 <RollDice>: round = " + to_string(round) + Utils::GetDescription(gm, false));
				client.Send(gm.Sender.Group.GID, Cyan::MessageChain().Plain("骰子不见了捏，怎么会事捏"));
				return false;
			}


			i = i + j + 1;
			j = 0;
			while (i + j < command.length() && command[i + j] >= '0' && command[i + j] <= '9')
				j++;
			if (j)
			{
				int max = stoi(command.substr(i, j));
				if (max <= 0)
				{
					logging::INFO("骰子面数过小 <RollDice>: " + to_string(max) + Utils::GetDescription(gm, false));
					client.Send(gm.Sender.Group.GID, Cyan::MessageChain().Plain("这是什么奇妙骰子捏，没见过捏"));
					return false;
				}
				uniform_int_distribution<int> rngroll(1, max);
				int ans = 0;
				string msg = "";
				for (int l = 0; l < round; ++l)
				{
					result[i] = rngroll(Utils::rng_engine);
					ans += result[i];
					msg += (l)? " + " + to_string(result[i]) : to_string(result[i]);
				}
				msg += " = ";
				logging::INFO("随机数生成 <RollDice>: " + msg + to_string(ans) + Utils::GetDescription(gm, false));
				if (round == 1)
					client.Send(gm.Sender.Group.GID, Cyan::MessageChain().Plain(gm.Sender.MemberName + " 掷出了: " + to_string(ans)));
				else
					client.Send(gm.Sender.Group.GID, Cyan::MessageChain().Plain(gm.Sender.MemberName + " 掷出了: " + msg + to_string(ans)));
				return true;
			}
		}
		catch (out_of_range &)
		{
			logging::INFO("数字溢出 <RollDice>" + Utils::GetDescription(gm, false));
			client.Send(gm.Sender.Group.GID, Cyan::MessageChain().Plain("数字太、太大了"));
			return false;
		}
	}

	logging::INFO("格式错误 <RollDice>" + Utils::GetDescription(gm, false));
	client.Send(gm.Sender.Group.GID, Cyan::MessageChain().Plain("格式错了捏，使用示例 #roll 1D100"));
	return false;
}

}