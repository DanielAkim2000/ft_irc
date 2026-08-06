#include "server.hpp"
#include "irc_utils.hpp"

void server::bot(int clientFd)
{
	std::map<int, client *>::iterator it = clientList.find(clientFd);
	if (it == clientList.end())
		return;
	std::string nickname = it->second->getNickname();
	std::string message = ":Bot!bot@localhost PRIVMSG " + nickname + " :pong\r\n";
	sendMessage(clientFd, message);
}

void server::privmsg(std::vector<std::string> &params, int clientFd)
{
	std::map<int, client *>::iterator senderIt;
	senderIt = clientList.find(clientFd);
	if (senderIt == clientList.end())
		return;
	client *sender = senderIt->second;
	std::string nickname = sender->getNickname();
	if (params.empty() || params[0].empty())
	{
		sendError("411", clientFd, nickname, "");
		return;
	}
	if (params.size() < 2 || params[1].empty())
	{
		sendError("412", clientFd, nickname, "");
		return;
	}
	if (params[0].find(',') != std::string::npos)
	{
		std::vector<std::string> targets;
		splitCommaList(params[0], targets);
		for (size_t i = 0; i < targets.size(); i++)
		{
			std::vector<std::string> oneMessage;
			oneMessage = params;
			oneMessage[0] = targets[i];
			privmsg(oneMessage, clientFd);
		}
		return;
	}
	std::string targetName = params[0];
	if (ircEquals(targetName, "Bot"))
	{
		if (ircEquals(params[1], "ping"))
			bot(clientFd);
		return;
	}
	if (targetName[0] == '#')
	{
		std::map<std::string, channel *>::iterator channelIt;
		channelIt = findChannel(targetName);
		if (channelIt == channelList.end())
		{
			sendError("401", clientFd, nickname, targetName);
			return;
		}
		channel *targetChannel = channelIt->second;
		if (!targetChannel->isClientMember(nickname))
		{
			sendError("404", clientFd, nickname, targetChannel->getName());
			return;
		}
		std::string message = sender->getPrefix() + " PRIVMSG " + targetChannel->getName() + " :" + params[1] + "\r\n";
		sendToChannel(targetChannel, message, clientFd);
		return;
	}
	client *target = getClientByName(targetName);
	if (target == NULL)
	{
		sendError("401", clientFd, nickname, targetName);
		return;
	}
	std::string message = sender->getPrefix() + " PRIVMSG " + target->getNickname() + " :" + params[1] + "\r\n";
	sendMessage(target->getFd(), message);
}
