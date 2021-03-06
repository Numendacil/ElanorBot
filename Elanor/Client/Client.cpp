#include "Client.hpp"

#include <mirai/defs/QQType.hpp>
#include <ThirdParty/log.h>

#include <chrono>
#include <mirai.h>
#include <memory>
#include <stdexcept>
#include <thread>

using namespace std;

namespace Bot
{

Client::Client()
{
	this->Connected = false;
	this->interval = chrono::milliseconds(500);
	this->max_retry = 3;
	this->client = make_unique<Cyan::MiraiBot>();
}
Client::~Client()
{
	if (this->Connected)
	{
		this->Connected = false;
		this->cv.notify_all();
		if (this->th.joinable())
			this->th.join();
		this->client->Disconnect();
	}
}

void Client::MsgQueue()
{
	while(true)
	{
		Message msg;
		{
			unique_lock<mutex> lk(this->q_mtx);
			this->cv.wait(lk, [this]() -> bool
			{ 
				if (!this->Connected)
					return true;
				return !this->message.empty();
			});
			if (!this->Connected)
				return;
			msg = move(this->message.front());
			this->message.pop();
		}

		try
		{
			this_thread::sleep_for(this->interval);
			switch (msg.type)
			{
			case Message::GROUP:
				this->client->SendMessage(msg.gid, msg.msg, msg.mid);
				break;
			case Message::FRIEND:
				this->client->SendMessage(msg.qqid, msg.msg, msg.mid);
				break;
			case Message::TEMP:
				this->client->SendMessage(msg.gid, msg.qqid, msg.msg, msg.mid);
				break;
			default:
				logging::ERROR("waht");
			}
		}
		catch(runtime_error& e)
		{
			logging::WARN(string("MsgQueue: ") + e.what());
			if (msg.count < this->max_retry)
			{
				unique_lock<mutex> lk(this->q_mtx);
				msg.count++;
				this->message.push(move(msg));
			}
		}
	}
}

void Client::Send(const Cyan::GID_t& gid, const Cyan::MessageChain& msg, Cyan::MessageId_t mid)
{
	unique_lock<mutex> lk(this->q_mtx);
	this->message.emplace(gid, (Cyan::QQ_t)0, msg, mid, Message::GROUP, 0);
	this->cv.notify_all();
}

void Client::Send(const Cyan::QQ_t& qqid, const Cyan::MessageChain& msg, Cyan::MessageId_t mid)
{
	unique_lock<mutex> lk(this->q_mtx);
	this->message.emplace((Cyan::GID_t)0, qqid, msg, mid, Message::FRIEND, 0);
	this->cv.notify_all();
}

void Client::Send(const Cyan::GID_t& gid, const Cyan::QQ_t& qqid, const Cyan::MessageChain& msg, Cyan::MessageId_t mid)
{
	unique_lock<mutex> lk(this->q_mtx);
	this->message.emplace(gid, qqid, msg, mid, Message::TEMP, 0);
	this->cv.notify_all();
}

void Client::Connect(const Cyan::SessionOptions &opts)
{
	this->client->Connect(opts);
	this->Connected = true;
	this->th = thread(&Client::MsgQueue, this);
}

void Client::Reconnect()
{
	this->Connected = false;
	this->cv.notify_all();
	if (this->th.joinable())
		this->th.join();
	
	this->client->Reconnect();
	this->Connected = true;
	this->th = thread(&Client::MsgQueue, this);
}

void Client::Disconnect()
{
	this->Connected = false;
	this->cv.notify_all();
	if (this->th.joinable())
		this->th.join();
	this->client->Disconnect();
}

}