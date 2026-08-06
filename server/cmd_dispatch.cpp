#include "server.hpp"
#include <cctype>

void server::handleCommand(std::string &cmd, std::vector<std::string> &params, int clientFd)
{
	std::map<int, client *>::iterator it = clientList.find(clientFd);
	if (it == clientList.end())
		return;
	bool allowedBeforeRegistration = cmd == "CAP" || cmd == "PASS" || cmd == "NICK" || cmd == "USER" || cmd == "PING" || cmd == "QUIT";
	if (!it->second->getRegistered() && !allowedBeforeRegistration)
	{
		std::string nick = it->second->getNickname();
		if (nick.empty())
			nick = "*";
		sendMessage(clientFd, ":localhost 451 " + nick + " :You have not registered\r\n");
		return;
	}
	if (cmd == "PING")
		ping(cmd, params, clientFd);
	else if (cmd == "USER")
		user(cmd, params, clientFd);
	else if (cmd == "NICK")
		nick(cmd, params, clientFd);
	else if (cmd == "CAP")
		cap(cmd, params, clientFd);
	else if (cmd == "MODE")
		mode(cmd, params, clientFd);
	else if (cmd == "PASS")
		pass(cmd, params, clientFd);
	else if (cmd == "PRIVMSG")
		privmsg(params, clientFd);
	else if (cmd == "TOPIC")
		topic(params, clientFd);
	else if (cmd == "PART")
		part(params, clientFd);
	else if (cmd == "JOIN")
		join(cmd, params, clientFd);
	else if (cmd == "INVITE")
		invite(params, clientFd);
	else if (cmd == "QUIT")
		quit(cmd, params, clientFd);
	else if (cmd == "KICK")
		kick(params, clientFd);
	else
	{
		std::map<int, client *>::iterator it = clientList.find(clientFd);
		if (it == clientList.end())
			return;
		std::string nick = it->second->getNickname();
		if (nick.empty())
			nick = "*";
		sendMessage(clientFd, ":localhost 421 " + nick + " " + cmd + " :Unknown command\r\n");
	}
}

void server::getCommand(std::string &buffer, int clientFd)
{
	std::string cmd;
	std::string current_param;
	std::vector<std::string> params;
	std::string content;
	size_t i = 0;

	while (i < buffer.size() && buffer[i] == ' ')
		i++;
	if (i == buffer.size())
		return;

	while (i < buffer.size() && buffer[i] != ' ')
	{
		cmd += buffer[i];
		i++;
	}

	while (i < buffer.size())
	{
		while (i < buffer.size() && buffer[i] == ' ')
			i++;
		if (i == buffer.size())
			break;

		if (buffer[i] == ':')
		{
			params.push_back(buffer.substr(i + 1));
			break;
		}

		current_param.clear();

		while (i < buffer.size() && buffer[i] != ' ')
		{
			current_param += buffer[i];
			i++;
		}
		params.push_back(current_param);
	}
	for (std::size_t i = 0; i < cmd.size(); ++i)
		cmd[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(cmd[i])));
	handleCommand(cmd, params, clientFd);
}

void server::handleClientBuffer(std::string &clientBuffer, int clientFd)
{
	size_t pos;
	std::map<int, client *>::iterator it;
	it = clientList.find(clientFd);
	if (it == clientList.end())
		return;
	while ((pos = clientBuffer.find('\n')) != std::string::npos && !it->second->getDisconnect())
	{
		if (pos + 1 > 512)
		{
			it->second->setDisconnect(true);
			return;
		}
		size_t lineLength = pos;
		if (lineLength > 0 && clientBuffer[lineLength - 1] == '\r')
			lineLength--;
		std::string line;
		line = clientBuffer.substr(0, lineLength);
		clientBuffer.erase(0, pos + 1);
		if (!line.empty())
		{
			getCommand(line, clientFd);
			tryRegisterClient(clientFd);
		}
	}
	if (clientBuffer.size() > 510)
		it->second->setDisconnect(true);
}
