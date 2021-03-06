#include <exception>
#include <fstream>
#include <optional>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>

#include <ThirdParty/httplib.hpp>
#include <ThirdParty/log.h>
#include <Command/Command.hpp>
#include <State/State.hpp>
#include <Trigger/Trigger.hpp>
#include <Utils/Factory.hpp>
#include <Utils/Utils.hpp>

using namespace std;

namespace Utils
{
	bool BotConfig::FromFile(const string &filepath)
	{
		ifstream ifile(filepath);
		if (ifile.fail())
		{
			logging::WARN("Failed to open file <BotConfig>: " + filepath);
			return false;
		}
		try
		{
			this->config = json::parse(ifile);
		}
		catch (const exception& e)
		{
			logging::WARN("Failed to parse file <BotConfig>: " + string(e.what()));
			return false;
		}
		return true;
	}
}

namespace Utils
{
	random_device rd;
	mt19937 rng_engine(rd());
	BotConfig Configs;

	string exec(const vector<string>& cmd)
	{
		string result = "";
		vector<char *> param;
		for (auto &str : cmd)
			param.push_back(const_cast<char *>(str.c_str()));
		param.push_back(nullptr);

		pid_t pid;
		int p[2];
		if (pipe(p) == -1)
		{
			perror("Failed to open pipe");
			return nullptr;
		}
		if ((pid = fork()) == -1)
		{
			perror("Failed to fork program");
			return nullptr;
		}
		if (pid == 0)
		{
			dup2(p[1], STDOUT_FILENO);
			close(p[0]);
			close(p[1]);
			execvp(param[0], param.data());
			perror(("Failed to execute " + cmd[0]).c_str());
			exit(1);
		}
		close(p[1]);

		char buffer[128];
		ssize_t c;
		while ((c = read(p[0], buffer, 128)) > 0)
			result.append(buffer, c);
		close(p[0]);
		return result;
	}

	void ReplaceMark(string &str)
	{
		static const vector<pair<const string, const string>> ReplaceList =
		{
			{"﹟", "#"},
			{"？", "?"},
			{"＃", "#"},
			{"！", "!"},
			{"。", "."},
			{"，", ","},
			{"“", "\""},
			{"”", "\""},
			{"‘", "\'"},
			{"’", "\'"},
			{"；", ";"},
			{"：", ":"}
		};
		for (const auto &p : ReplaceList)
		{
			string temp;
			const auto end = str.end();
			auto current = str.begin();
			auto next = search(current, end, p.first.begin(), p.first.end());
			while (next != end)
			{
				temp.append(current, next);
				temp.append(p.second);
				current = next + p.first.length();
				next = search(current, end, p.first.begin(), p.first.end());
			}
			temp.append(current, next);
			str = temp;
		}
	}

	void ToLower(string &str)
	{
		transform(str.begin(), str.end(), str.begin(), [](unsigned char c)
			  { return tolower(c); });
	}

	int ToBool(const string& str)
	{
		string s = str;
		ToLower(s);
		const string TrueStr[] = {"1", "true", "on", "yes"};
		const string FalseStr[] = {"0", "false", "off", "no"};
		static_assert(extent<decltype(TrueStr)>::value == extent<decltype(FalseStr)>::value);
		for (int i = 0; i < extent<decltype(TrueStr)>::value; i++)
		{
			if (s == TrueStr[i])
				return 1;
			if (s == FalseStr[i])
				return 0;
		}
		return -1;
	}

	int Tokenize(vector<string> &token, string str, int count)
	{
		istringstream iss(str);
		string s;
		int i = 0;
		while (iss >> quoted(s) && i != count)
		{
			token.push_back(s);
			i++;
		}
		return token.size();
	}

	bool CheckHttpResponse(const httplib_ssl_zlib::Result& result, const string& Caller)
	{
		if (result.error() != httplib_ssl_zlib::Error::Success || !result)
		{
			logging::WARN("Connection to server failed <" + Caller + ">: " + to_string(result.error()));
			return false;
		}
		if (result->status != 200)
		{
			logging::WARN("Error response from server <" + Caller + ">: " + result->body);
			return false;
		}
		return true;
	}

	bool CheckHttpResponse(const httplib_ssl_zlib::Result& result, const string& Caller, int& code)
	{
		if (result.error() != httplib_ssl_zlib::Error::Success || !result)
		{
			logging::WARN("Connection to server failed <" + Caller + ">: " + to_string(result.error()));
			code = -1;
			return false;
		}
		if (result->status != 200)
		{
			logging::WARN("Error response from server <" + Caller + ">: " + result->body);
			code = result->status;
			return false;
		}
		code = 200;
		return true;
	}

	string GetDescription(const Cyan::GroupMessage &gm, bool from)
	{
		string member = gm.Sender.MemberName + "(" + to_string(gm.Sender.QQ.ToInt64()) + ")";
		string group = gm.Sender.Group.Name + "(" + to_string(gm.Sender.Group.GID.ToInt64()) + ")";
		return ((from) ? "\t<- [" : "\t-> [") + member + ", " + group + "]";
	}

	string GetDescription(const Cyan::FriendMessage &fm, bool from)
	{
		string profile = fm.Sender.NickName + "(" + to_string(fm.Sender.QQ.ToInt64()) + ")";
		return ((from) ? "\t<- [" : "\t-> [") + profile + "]";
	}
}