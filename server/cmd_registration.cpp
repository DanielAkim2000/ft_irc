#include "server.hpp"
#include "irc_utils.hpp"

void server::quit(std::string &cmd, std::vector<std::string> &params, int clientFd)
{
	(void)cmd;
	std::map<int, client *>::iterator it = clientList.find(clientFd);
	if (it == clientList.end())
		return;
	client *sender = it->second;
	std::string reason = "Leaving";

	if (!params.empty())
		reason = params[0];
	notifyQuit(sender, reason);
	sender->setDisconnect(true);
}

void server::cap(std::string &cmd, std::vector<std::string> &params, int clientFd)
{
	(void)cmd;
	std::map<int, client *>::iterator it = clientList.find(clientFd);
	if (it == clientList.end() || params.empty() || it->second->getRegistered())
		return;
	if (params[0] == "LS")
	{
		it->second->setCapNegotiating(true);
		sendMessage(clientFd, ":localhost CAP * LS :\r\n");
	}
	else if (params[0] == "REQ")
	{
		it->second->setCapNegotiating(true);
		std::string requested;
		if (params.size() > 1)
			requested = params[1];
		sendMessage(clientFd, ":localhost CAP * NAK :" + requested + "\r\n");
	}
	else if (params[0] == "END")
		it->second->setCapNegotiating(false);
}

void server::ping(std::string &cmd, std::vector<std::string> &params, int clientFd)
{
	(void)cmd;
	std::map<int, client *>::iterator it = clientList.find(clientFd);
	if (it == clientList.end())
		return;
	std::string nick = it->second->getNickname();
	if (nick.empty())
		nick = "*";
	if (params.empty())
	{
		sendMessage(clientFd, ":localhost 409 " + nick + " :No origin specified\r\n");
		return;
	}
	sendMessage(clientFd, ":localhost PONG localhost :" + params[0] + "\r\n");
}

void server::nick(std::string &cmd, std::vector<std::string> &params, int clientFd)
{
	(void)cmd;
	std::string msg;
	std::string user;
	std::map<int, client *>::iterator it;
	it = clientList.find(clientFd);
	if (it == clientList.end())
		return;
	user = it->second->getNickname();
	if (user == "")
		user = "*";

	if (params.size() == 0)
	{
		msg = ":localhost 431 " + user + " :No nickname given\r\n";
		sendMessage(clientFd, msg);
	}
	else if (!isValidNickname(params[0]) || params.size() != 1)
	{
		msg = ":localhost 432 " + user + " " + params[0] + " :Erroneous nickname\r\n";
		sendMessage(clientFd, msg);
	}
	else
	{
		if (ircEquals(params[0], "Bot"))
		{
			msg = ":localhost 433 " + user + " " + params[0] + " :Nickname is already in use\r\n";
			sendMessage(clientFd, msg);
			return;
		}
		if (params[0] == it->second->getNickname())
			return;
		it = clientList.begin();
		while (it != clientList.end())
		{
			if (it->first != clientFd && ircEquals(it->second->getNickname(), params[0]))
			{
				msg = ":localhost 433 " + user + " " + params[0] + " :Nickname is already in use\r\n";
				sendMessage(clientFd, msg);
				return;
			}
			it++;
		}
		it = clientList.find(clientFd);
		if (it == clientList.end())
			return;
		if (it->second->getRegistered())
		{
			msg = it->second->getPrefix() + " NICK " + params[0] + "\r\n";
			sendToSharedClients(it->second, msg);
		}
		it->second->setNickname(params[0]);
	}
}

void server::pass(std::string &cmd, std::vector<std::string> &params, int clientFd)
{
	(void)cmd;
	std::map<int, client *>::iterator it = clientList.find(clientFd);
	std::string user = it->second->getNickname();
	if (user == "")
		user = "*";

	if (it->second->getRegistered() == true)
	{
		sendMessage(clientFd, ":localhost 462 " + user + " :You may not reregister\r\n");
		return;
	}
	if (params.size() != 1)
	{
		sendMessage(clientFd, ":localhost 461 " + user + " PASS :Not enough parameters\r\n");
		it->second->setDisconnect(true);
		return;
	}
	if (params[0] != this->password)
	{
		sendMessage(clientFd, ":localhost 464 " + user + " :Password incorrect\r\n");
		it->second->setDisconnect(true);
		return;
	}
	it->second->setPasswordOk(true);
}

void server::user(std::string &cmd, std::vector<std::string> &params, int clientFd)
{
	(void)cmd;
	std::string msg;
	std::map<int, client *>::iterator it = clientList.find(clientFd);
	std::string user = it->second->getNickname();
	if (user == "")
		user = "*";
	if (params.size() != 4)
	{
		msg = ":localhost 461 " + user + " USER :Not enough parameters\r\n";
		sendMessage(clientFd, msg);
	}
	else if (it->second->getRegistered() == true)
	{
		msg = ":localhost 462 " + user + " :You may not reregister\r\n";
		sendMessage(clientFd, msg);
	}
	else
	{
		it->second->setUsername(params[0]);
		it->second->setRealname(params[3]);
	}
}
