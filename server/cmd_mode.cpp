#include "server.hpp"
#include <sstream>
#include <climits>
#include "irc_utils.hpp"

void server::modeUser(std::vector<std::string> &params, int clientFd, std::map<int, client *>::iterator it)
{
	bool add = false;

	for (size_t y = 0; y < params[1].size(); y++)
	{
		if (params[1][y] == '+')
		{
			add = true;
			continue;
		}
		if (params[1][y] == '-')
		{
			add = false;
			continue;
		}
		if (params[1][y] == 'i')
		{
			if (add == true)
				it->second->setModes("i");
			else
				it->second->removeModes("i");
		}
		else
		{
			std::string c(1, params[1][y]);
			return sendError("472", clientFd, it->second->getNickname(), c);
		}
	}
	sendMessage(clientFd, it->second->getPrefix() + " MODE " + it->second->getNickname() + " " + params[1] + "\r\n");
}

void server::execModeChannel(std::vector<std::pair<std::string, std::string> > &pair, std::map<std::string, channel *>::iterator &channel_it, std::map<int, client *>::iterator it)
{
	for (size_t i = 0; i < pair.size(); i++)
	{
		if (pair[i].first[1] == 'k')
		{
			if (pair[i].first[0] == '+')
			{
				if (channel_it->second->getPassword() == "")
				{
					channel_it->second->setPassword(pair[i].second);
					channel_it->second->setModes("k");
				}
				else
					return sendError("467", it->first, it->second->getNickname(), channel_it->first);
			}
			if (pair[i].first[0] == '-')
			{
				channel_it->second->setPassword("");
				channel_it->second->removeModes("k");
			}
		}
		if (pair[i].first[1] == 'l')
		{
			if (pair[i].first[0] == '+')
			{
				char *end;
				long limit = std::strtol(pair[i].second.c_str(), &end, 10);
				if (*end != '\0' || limit <= 0 || limit > INT32_MAX)
					return sendError("461", it->first, it->second->getNickname(), "");
				channel_it->second->setLimit(limit);
				channel_it->second->setModes("l");
			}
			if (pair[i].first[0] == '-')
			{
				channel_it->second->setLimit(0);
				channel_it->second->removeModes("l");
			}
		}
		if (pair[i].first[1] == 't')
		{
			if (pair[i].first[0] == '+')
			{
				channel_it->second->setTopicOpOnly(true);
				channel_it->second->setModes("t");
			}
			if (pair[i].first[0] == '-')
			{
				channel_it->second->setTopicOpOnly(false);
				channel_it->second->removeModes("t");
			}
		}
		if (pair[i].first[1] == 'o')
		{
			if (isClientHere(pair[i].second) == false)
				return sendError("401", it->first, it->second->getNickname(), pair[i].second);
			if (channel_it->second->isClientMember(pair[i].second) == false)
			{
				std::string combined_param = pair[i].second + " " + channel_it->first;
				return sendError("441", it->first, it->second->getNickname(), combined_param);
			}
			if (pair[i].first[0] == '+')
				channel_it->second->setOperator(getClientByName(pair[i].second));
			if (pair[i].first[0] == '-')
				channel_it->second->removeOperator(getClientByName(pair[i].second));
		}
		if (pair[i].first[1] == 'i')
		{
			if (pair[i].first[0] == '+')
			{
				channel_it->second->setInviteOnly(true);
				channel_it->second->setModes("i");
			}
			if (pair[i].first[0] == '-')
			{
				channel_it->second->setInviteOnly(false);
				channel_it->second->removeModes("i");
			}
		}
		std::string msg = it->second->getPrefix() + " MODE " + channel_it->first + " " + pair[i].first;
		if (pair[i].second != "#UNEXPECTED")
			msg += " " + pair[i].second;
		msg += "\r\n";
		sendToChannel(channel_it->second, msg, -1);
	}
}

void server::modeChannel(std::vector<std::string> &params, int clientFd, std::map<int, client *>::iterator it)
{
	bool add = false;
	int expected_params = 0;
	std::string charset = "itkol";
	std::map<std::string, channel *>::iterator channel_it = findChannel(params[0]);

	if (channel_it == channelList.end())
		return sendError("403", clientFd, it->second->getNickname(), params[0]);
	if (channel_it->second->isClientMember(it->second->getNickname()) == false)
		return sendError("442", clientFd, it->second->getNickname(), params[0]);
	if (channel_it->second->isClientOp(it->second->getNickname()) == false)
		return sendError("482", it->first, it->second->getNickname(), channel_it->first);

	std::vector<std::pair<std::string, std::string> > pair;
	for (size_t i = 0; i < params[1].size(); i++)
	{
		if (params[1][i] == '+')
		{
			add = true;
			continue;
		}
		if (params[1][i] == '-')
		{
			add = false;
			continue;
		}
		if (charset.find(params[1][i]) == std::string::npos)
		{
			std::string c(1, params[1][i]);
			return sendError("472", clientFd, it->second->getNickname(), c);
		}
		else
		{
			std::pair<std::string, std::string> temp;
			if (add == true)
				temp.first = "+";
			else
				temp.first = "-";
			temp.first += params[1][i];
			if (add == true)
			{
				if (params[1][i] == 'k' || params[1][i] == 'o' || params[1][i] == 'l')
				{
					temp.second = "#EXPECTED";
					expected_params++;
				}
				else
					temp.second = "#UNEXPECTED";
			}
			else
			{
				if (params[1][i] == 'o')
				{
					temp.second = "#EXPECTED";
					expected_params++;
				}
				else
					temp.second = "#UNEXPECTED";
			}
			pair.push_back(temp);
		}
	}
	if (params.size() < 2 + static_cast<size_t>(expected_params))
		return sendError("461", clientFd, it->second->getNickname(), "");
	int arg = 2;

	for (size_t i = 0; i < pair.size(); i++)
	{
		if (pair[i].second == "#EXPECTED")
		{
			pair[i].second = params[arg];
			arg++;
		}
	}
	execModeChannel(pair, channel_it, it);
}

void server::mode(std::string &cmd, std::vector<std::string> &params, int clientFd)
{
	(void)cmd;
	std::map<int, client *>::iterator it = clientList.find(clientFd);
	if (it == clientList.end())
		return;
	if (params.empty() || params[0].empty())
		return sendError("461", clientFd, it->second->getNickname(), "");
	if (params.size() == 1)
	{
		if (ircEquals(params[0], it->second->getNickname()))
		{
			sendMessage(clientFd, ":localhost 221 " + it->second->getNickname() + " +" + it->second->getModes() + "\r\n");
			return;
		}
		if (params[0][0] == '#')
		{
			std::map<std::string, channel *>::iterator channel_it;
			channel_it = findChannel(params[0]);
			if (channel_it == channelList.end())
				return sendError("403", clientFd, it->second->getNickname(), params[0]);
			std::string modes;
			modes = channel_it->second->getModes();
			std::string msg;
			msg = ":localhost 324 " + it->second->getNickname() + " " + channel_it->first + " +" + modes;
			for (size_t i = 0; i < modes.size(); i++)
			{
				if (modes[i] == 'k')
				{
					msg += " ";
					msg += channel_it->second->getPassword();
				}
				else if (modes[i] == 'l')
				{
					std::ostringstream limit;
					limit << channel_it->second->getLimit();
					msg += " ";
					msg += limit.str();
				}
			}
			msg += "\r\n";
			sendMessage(clientFd, msg);
			return;
		}
		return sendError("502", clientFd, it->second->getNickname(), "");
	}

	if (params[1].size() < 2 || (params[1][0] != '+' && params[1][0] != '-'))
		return sendError("461", clientFd, it->second->getNickname(), "");
	if (ircEquals(params[0], it->second->getNickname()))
		return modeUser(params, clientFd, it);
	if (params[0][0] == '#')
		return modeChannel(params, clientFd, it);
	return sendError("502", clientFd,
					 it->second->getNickname(), "");
}
