#include "server.hpp"

void server::topic(std::vector<std::string> &params, int clientFd)
{
	std::map<int, client *>::iterator client_it;
	client_it = clientList.find(clientFd);
	if (client_it == clientList.end())
		return;

	client *sender = client_it->second;
	std::string nickname = sender->getNickname();

	if (params.empty())
	{
		std::string msg = ":localhost 461 " + nickname + " TOPIC :Not enough parameters\r\n";
		sendMessage(clientFd, msg);
		return;
	}

	std::map<std::string, channel *>::iterator channel_it;
	channel_it = findChannel(params[0]);
	if (channel_it == channelList.end())
	{
		sendError("403", clientFd, nickname, params[0]);
		return;
	}

	channel *currentChannel = channel_it->second;
	if (!currentChannel->isClientMember(nickname))
	{
		sendError("442", clientFd, nickname, channel_it->first);
		return;
	}

	if (params.size() == 1)
	{
		if (currentChannel->getTopic().empty())
		{
			std::string msg = ":localhost 331 " + nickname + " " + channel_it->first + " :No topic is set\r\n";
			sendMessage(clientFd, msg);
			return;
		}
		std::string msg = ":localhost 332 " + nickname + " " + channel_it->first + " :" + currentChannel->getTopic() + "\r\n";
		sendMessage(clientFd, msg);
		return;
	}

	if (currentChannel->getTopicOpOnly() && !currentChannel->isClientOp(nickname))
	{
		sendError("482", clientFd, nickname, params[0]);
		return;
	}

	std::string newTopic = params[1];
	currentChannel->setTopic(newTopic);
	std::string msg = sender->getPrefix() + " TOPIC " + channel_it->first + " :" + currentChannel->getTopic() + "\r\n";
	sendToChannel(currentChannel, msg, -1);
}

void server::part(std::vector<std::string> &params, int clientFd)
{
	std::map<int, client *>::iterator client_it;
	client_it = clientList.find(clientFd);
	if (client_it == clientList.end())
		return;

	client *sender = client_it->second;
	std::string nickname = sender->getNickname();

	if (params.empty())
	{
		std::string msg = ":localhost 461 " + nickname + " PART :Not enough parameters\r\n";
		sendMessage(clientFd, msg);
		return;
	}
	if (params[0].find(',') != std::string::npos)
	{
		std::vector<std::string> names;
		splitCommaList(params[0], names);
		for (std::size_t i = 0; i < names.size(); ++i)
		{
			std::vector<std::string> onePart = params;
			onePart[0] = names[i];
			part(onePart, clientFd);
		}
		return;
	}
	std::map<std::string, channel *>::iterator channel_it;
	channel_it = findChannel(params[0]);
	if (channel_it == channelList.end())
	{
		sendError("403", clientFd, nickname, params[0]);
		return;
	}

	channel *currentChannel = channel_it->second;
	if (currentChannel->isClientMember(nickname) == false)
	{
		sendError("442", clientFd, nickname, channel_it->first);
		return;
	}

	std::string msg = sender->getPrefix() + " PART " + channel_it->first;
	if (params.size() > 1)
	{
		msg += " :";
		for (std::size_t i = 1; i < params.size(); ++i)
		{
			msg += params[i];
			if (i + 1 < params.size())
				msg += " ";
		}
	}
	msg += "\r\n";

	sendToChannel(currentChannel, msg, -1);
	currentChannel->removeMember(*sender);

	std::vector<channel *> &channels = sender->getChannel();
	for (std::vector<channel *>::iterator it = channels.begin(); it != channels.end(); ++it)
	{
		if (*it == currentChannel)
		{
			channels.erase(it);
			break;
		}
	}

	if (currentChannel->isEmpty())
	{
		delete currentChannel;
		channelList.erase(channel_it);
	}
}

bool server::isValidChannelName(std::string &name)
{
	if (name.empty() || name[0] != '#')
		return (false);
	if (name.size() > 200)
		return (false);
	std::string forbidden = " ,*\a\r\n";
	for (size_t i = 1; i < name.size(); i++)
	{
		if (forbidden.find(name[i]) != std::string::npos)
			return (false);
	}
	return (true);
}

void server::sendJoinReplies(client *cl, channel *ch)
{
	std::string nick = cl->getNickname();
	int fd = cl->getFd();

	// RPL_TOPIC (332) ou RPL_NOTOPIC (331)
	if (ch->getTopic().empty())
	{
		std::string msg = ":localhost 331 " + nick + " " + ch->getName() + " :No topic is set\r\n";
		sendMessage(fd, msg);
	}
	else
	{
		std::string msg = ":localhost 332 " + nick + " " + ch->getName() + " :" + ch->getTopic() + "\r\n";
		sendMessage(fd, msg);
	}

	// RPL_NAMREPLY (353) — liste des membres
	std::string names = ":localhost 353 " + nick + " = " + ch->getName() + " :";
	std::vector<client *> &members = ch->getMembers();

	for (size_t i = 0; i < members.size(); i++)
	{
		if (ch->isClientOp(members[i]->getNickname()))
			names += "@" + members[i]->getNickname() + " ";
		else
			names += members[i]->getNickname() + " ";
	}

	std::string msg353 = names + "\r\n";
	sendMessage(fd, msg353);

	// RPL_ENDOFNAMES (366)
	std::string msg366 = ":localhost 366 " + nick + " " + ch->getName() + " :End of /NAMES list\r\n";
	sendMessage(fd, msg366);
}

void server::splitCommaList(std::string &list, std::vector<std::string> &out)
{
	std::string current;

	for (size_t i = 0; i < list.size(); i++)
	{
		if (list[i] == ',')
		{
			out.push_back(current);
			current.clear();
		}
		else
			current += list[i];
	}
	if (!current.empty())
		out.push_back(current);
}

channel *server::getOrCreateChannel(std::string &chanName)
{
	std::map<std::string, channel *>::iterator ch_it = findChannel(chanName);

	if (ch_it == channelList.end())
	{
		channel *ch = new channel();
		ch->setName(chanName);
		channelList[chanName] = ch;
		return (ch);
	}
	return (ch_it->second);
}

bool server::checkJoinAllowed(client *cl, channel *ch, std::string &nick, std::string &chanName, std::string &key, int clientFd)
{
	(void)cl;
	if (ch->isClientMember(nick))
		return (false);

	if (ch->getInviteOnly() == true && ch->isClientInvited(nick) == false)
	{
		sendMessage(clientFd, ":localhost 473 " + nick + " " + chanName + " :Cannot join channel (+i)\r\n");
		return (false);
	}

	if (ch->getModes().find('k') != std::string::npos && key != ch->getPassword())
	{
		sendMessage(clientFd, ":localhost 475 " + nick + " " + chanName + " :Cannot join channel (+k)\r\n");
		return (false);
	}

	if (ch->getModes().find('l') != std::string::npos)
	{
		if (ch->getLimit() > 0 && ch->getMembersCount() >= ch->getLimit())
		{
			sendMessage(clientFd, ":localhost 471 " + nick + " " + chanName + " :Cannot join channel (+l)\r\n");
			return (false);
		}
	}
	return (true);
}

void server::execJoinChannel(client *cl, channel *ch, std::string &chanName)
{
	ch->addMember(*cl);
	cl->getChannel().push_back(ch);

	if (ch->getMembersCount() == 1)
		ch->setOperator(cl);

	ch->removeInvited(cl);

	std::string joinMsg = cl->getPrefix() + " JOIN " + chanName + "\r\n";
	sendToChannel(ch, joinMsg, -1);
	sendJoinReplies(cl, ch);
}

void server::joinOneChannel(client *cl, int clientFd, std::string &nick, std::string &chanName, std::string &key)
{
	if (!isValidChannelName(chanName))
	{
		sendMessage(clientFd, ":localhost 403 " + nick + " " + chanName + " :No such channel\r\n");
		return ((void)0);
	}

	channel *ch = getOrCreateChannel(chanName);

	if (checkJoinAllowed(cl, ch, nick, chanName, key, clientFd) == false)
		return ((void)0);

	execJoinChannel(cl, ch, ch->getName());
}

void server::join(std::string &cmd, std::vector<std::string> &params, int clientFd)
{
	(void)cmd;
	std::map<int, client *>::iterator it = clientList.find(clientFd);
	client *cl = it->second;
	std::string nick = cl->getNickname();

	if (cl->getRegistered() == false)
	{
		sendMessage(clientFd, ":localhost 451 " + nick + " :You have not registered\r\n");
		return ((void)0);
	}

	if (params.empty())
	{
		sendMessage(clientFd, ":localhost 461 " + nick + " JOIN :Not enough parameters\r\n");
		return ((void)0);
	}

	if (params[0] == "0")
	{
		std::vector<std::string> channelNames;
		std::vector<channel *> &joinedChannels = cl->getChannel();
		for (size_t i = 0; i < joinedChannels.size(); ++i)
			channelNames.push_back(joinedChannels[i]->getName());
		for (size_t i = 0; i < channelNames.size(); ++i)
		{
			std::vector<std::string> partParams;
			partParams.push_back(channelNames[i]);
			part(partParams, clientFd);
		}
		return;
	}

	std::vector<std::string> channels;
	splitCommaList(params[0], channels);

	std::vector<std::string> keys;
	if (params.size() > 1)
		splitCommaList(params[1], keys);

	for (size_t c = 0; c < channels.size(); c++)
	{
		std::string key = "";
		if (c < keys.size())
			key = keys[c];
		joinOneChannel(cl, clientFd, nick, channels[c], key);
	}
}

void server::invite(std::vector<std::string> &params, int clientFd)
{
	std::map<int, client *>::iterator senderIt;
	senderIt = clientList.find(clientFd);
	if (senderIt == clientList.end())
		return;
	client *sender = senderIt->second;
	std::string senderNick = sender->getNickname();
	if (senderNick.empty())
		senderNick = "*";
	if (!sender->getRegistered())
	{
		sendMessage(clientFd, ":localhost 451 " + senderNick + " :You have not registered\r\n");
		return;
	}
	if (params.size() < 2)
	{
		sendMessage(clientFd, ":localhost 461 " + senderNick + " INVITE :Not enough parameters\r\n");
		return;
	}
	client *target = getClientByName(params[0]);
	if (target == NULL)
	{
		sendError("401", clientFd, senderNick, params[0]);
		return;
	}
	std::map<std::string, channel *>::iterator channelIt;
	channelIt = findChannel(params[1]);

	if (channelIt == channelList.end())
	{
		sendError("403", clientFd, senderNick, params[1]);
		return;
	}

	channel *targetChannel = channelIt->second;
	std::string targetNick = target->getNickname();
	std::string channelName = targetChannel->getName();
	if (!targetChannel->isClientMember(senderNick))
	{
		sendError("442", clientFd, senderNick, channelName);
		return;
	}
	if (!targetChannel->isClientOp(senderNick))
	{
		sendError("482", clientFd, senderNick, channelName);
		return;
	}
	if (targetChannel->isClientMember(targetNick))
	{
		sendMessage(clientFd, ":localhost 443 " + senderNick + " " + targetNick + " " + channelName + " :is already on channel\r\n");
		return;
	}
	targetChannel->addInvited(target);
	sendMessage(clientFd, ":localhost 341 " + senderNick + " " + targetNick + " " + channelName + "\r\n");
	sendMessage(target->getFd(), sender->getPrefix() + " INVITE " + targetNick + " :" + channelName + "\r\n");
}

void server::kickOneChannel(client *sender, channel *ch, std::string &channelName, std::string &targetNick, std::string &comment, int clientFd)
{
	client *target = getClientByName(targetNick);
	if (target == NULL)
	{
		sendError("401", clientFd, sender->getNickname(), targetNick);
		return;
	}
	if (!ch->isClientMember(target->getNickname()))
	{
		std::string combined = target->getNickname() + " " + channelName;
		sendError("441", clientFd, sender->getNickname(), combined);
		return;
	}

	std::string msg = sender->getPrefix() + " KICK " + channelName + " "
		+ target->getNickname() + " :" + comment + "\r\n";
	sendToChannel(ch, msg, -1);

	ch->removeMember(*target);

	std::vector<channel *> &channels = target->getChannel();
	for (std::vector<channel *>::iterator it = channels.begin(); it != channels.end(); ++it)
	{
		if (*it == ch)
		{
			channels.erase(it);
			break;
		}
	}
}

void server::kick(std::vector<std::string> &params, int clientFd)
{
	std::map<int, client *>::iterator senderIt = clientList.find(clientFd);
	if (senderIt == clientList.end())
		return;
	client *sender = senderIt->second;
	std::string senderNick = sender->getNickname();
	if (senderNick.empty())
		senderNick = "*";

	if (params.size() < 2)
	{
		sendMessage(clientFd, ":localhost 461 " + senderNick + " KICK :Not enough parameters\r\n");
		return;
	}

	std::map<std::string, channel *>::iterator channelIt = findChannel(params[0]);
	if (channelIt == channelList.end())
	{
		sendError("403", clientFd, senderNick, params[0]);
		return;
	}

	channel *ch = channelIt->second;
	std::string channelName = ch->getName();

	if (!ch->isClientMember(senderNick))
	{
		sendError("442", clientFd, senderNick, channelName);
		return;
	}
	if (!ch->isClientOp(senderNick))
	{
		sendError("482", clientFd, senderNick, channelName);
		return;
	}

	std::string comment = senderNick;
	if (params.size() >= 3)
		comment = params[2];

	std::vector<std::string> nicks;
	splitCommaList(params[1], nicks);

	for (size_t i = 0; i < nicks.size(); ++i)
		kickOneChannel(sender, ch, channelName, nicks[i], comment, clientFd);

	if (ch->isEmpty())
	{
		delete ch;
		channelList.erase(channelIt);
	}
}
