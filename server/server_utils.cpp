#include "server.hpp"
#include "irc_utils.hpp"
#include <cctype>

void server::sendError(std::string code, int clientFd, std::string clientName, std::string param1)
{

	if (code == "401")
	{
		std::string msg =
			":localhost 401 " +
			clientName +
			" " +
			param1 +
			" :No such nick/channel\r\n";
		return sendMessage(clientFd, msg);
	}

	if (code == "467")
	{
		std::string msg =
		":localhost 467 " +
		clientName +
		" " +
		param1 +
		" :Channel key already set\r\n";
		return sendMessage(clientFd, msg);
	}

	if (code == "411")
	{
		std::string msg = ":localhost 411 " + clientName + " :No recipient given (PRIVMSG)\r\n";
		sendMessage(clientFd, msg);
		return;
	}
	if (code == "412")
	{
		std::string msg = ":localhost 412 " + clientName + " :No text to send\r\n";
		sendMessage(clientFd, msg);
		return;
	}

	if (code == "502")
	{
		std::string msg =
				":localhost 502 " +
				clientName +
				" :Cannot change mode for other users\r\n";
			return sendMessage(clientFd, msg);
	}

	if (code == "461")
	{
		std::string msg =
				":localhost 461 " +
				clientName +
				" MODE :Not enough parameters\r\n";
			sendMessage(clientFd, msg);
			return;
	}

	if (code == "403")
	{
		std::string msg = ":localhost 403 " + clientName+ " " + param1 + " :No such channel\r\n";
		sendMessage(clientFd, msg);
		return;
	}

	if (code == "472")
	{
		std::string msg = ":localhost 472 " + clientName + " ";
				msg += param1;
				msg += " :is unknown mode char\r\n";
		sendMessage(clientFd, msg);
		return ;
	}

	if (code == "441")
	{
		std::string msg =
			":localhost 441 " +
			clientName +
			" " +
			param1 +
			" :They aren't on that channel\r\n";
		return sendMessage(clientFd, msg);
	}

	if (code == "442")
	{
		std::string msg =
			":localhost 442 " +
			clientName +
			" " +
			param1 +
			" :You're not on that channel\r\n";
		return sendMessage(clientFd, msg);
	}

	if (code == "482")
	{
		std::string msg =
			":localhost 482 " +
			clientName +
			" " +
			param1 +
			" :You're not channel operator\r\n";
		return sendMessage(clientFd, msg);
	}

	if (code == "404")
	{
		std::string msg = ":localhost 404 " + clientName + " " + param1 + " :Cannot send to channel\r\n";
		sendMessage(clientFd, msg);
		return;
	}
}

void server::sendMessage(int clientFd, std::string message)
{
	std::map<int, client *>::iterator it = clientList.find(clientFd);

	if (it == clientList.end())
		return;
	it->second->addOutputBuffer(message);
	epoll_event ev;
	std::memset(&ev, 0, sizeof(ev));
	ev.data.fd = clientFd;
	ev.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP;

	if (epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &ev) == -1)
		std::cerr << "epoll_ctl EPOLLOUT failed: "
				  << std::strerror(errno) << std::endl;
}

bool server::isValidNickname(std::string &nickname)
{
	if (nickname.empty())
		return false;

	std::string firstSpecial = "[]\\`_^{|}";
	if (!std::isalpha(static_cast<unsigned char>(nickname[0])) && firstSpecial.find(nickname[0]) == std::string::npos)
		return false;
	std::string forbidden = " ,*?!@:\r\n";
	for (std::size_t i = 0; i < nickname.size(); i++)
	{
		if (forbidden.find(nickname[i]) != std::string::npos)
			return false;
	}
	return true;
}

bool server::isClientHere(std::string name)
{
	std::map<int, client *>::iterator it = clientList.begin();

	while (it != clientList.end())
	{
		if (ircEquals(it->second->getNickname(), name))
			return true;
		it++;
	}
	return false;
}

client *server::getClientByName(std::string &name)
{
	std::map<int, client *>::iterator it = clientList.begin();

	while (it != clientList.end())
	{
		if (ircEquals(it->second->getNickname(), name))
			return it->second;
		it++;
	}
	return NULL;
}

void server::freeEverything()
{
	for (std::map<int, client *>::iterator it = clientList.begin(); it != clientList.end(); ++it)
	{
		close(it->first);
		delete it->second;
	}
	clientList.clear();
	for (std::map<std::string, channel *>::iterator it = channelList.begin(); it != channelList.end(); ++it)
		delete it->second;
	channelList.clear();
	if (epollFd != -1)
	{
		close(epollFd);
		epollFd = -1;
	}
	if (server_fd != -1)
	{
		close(server_fd);
		server_fd = -1;
	}
}

void server::removeClientFromChannels(client *client)
{
	std::map<std::string, channel *>::iterator it;
	it = channelList.begin();

	while (it != channelList.end())
	{
		channel *ch = it->second;
		std::map<std::string, channel *>::iterator current = it++;
		ch->removeInvited(client);
		if (ch->isClientMember(client->getNickname()))
			ch->removeMember(*client);
		if (ch->isEmpty())
		{
			delete ch;
			channelList.erase(current);
		}
	}
}

void server::sendToChannel(channel *ch, std::string message, int excludedFd)
{
	std::vector<client *> &members = ch->getMembers();

	for (size_t i = 0; i < members.size(); ++i)
	{
		if (members[i]->getFd() != excludedFd)
			sendMessage(members[i]->getFd(), message);
	}
}

void server::tryRegisterClient(int clientFd)
{
	std::map<int, client *>::iterator it = clientList.find(clientFd);
	if (it == clientList.end())
		return;
	client *cl = it->second;
	if (cl->getDisconnect() || cl->getRegistered())
		return;
	if (cl->getNickname().empty() || cl->getUsername().empty() || !cl->getPasswordOk() || cl->getCapNegotiating())
		return;
	cl->setRegistered(true);
	std::string nick = cl->getNickname();
	std::string msg;
	msg += ":localhost 001 " + nick + " :Welcome to the Internet Relay Network " + nick + "\r\n";
	msg += ":localhost 002 " + nick + " :Your host is localhost, running version ft_irc\r\n";
	msg += ":localhost 003 " + nick + " :This server was created today\r\n";
	msg += ":localhost 004 " + nick + " localhost ft_irc o o\r\n";
	sendMessage(clientFd, msg);
}

void server::notifyQuit(client *sender, std::string reason)
{
	std::vector<int> recipients;
	std::vector<channel *> &channels = sender->getChannel();
	for (size_t i = 0; i < channels.size(); ++i)
	{
		std::vector<client *> &members = channels[i]->getMembers();

		for (size_t j = 0; j < members.size(); ++j)
		{
			int fd = members[j]->getFd();
			if (fd != sender->getFd() && std::find(recipients.begin(), recipients.end(), fd) == recipients.end())
				recipients.push_back(fd);
		}
	}
	std::string msg = sender->getPrefix() + " QUIT :" + reason + "\r\n";
	for (size_t i = 0; i < recipients.size(); ++i)
		sendMessage(recipients[i], msg);
}

void server::sendToSharedClients(client *sender, std::string message)
{
	std::vector<int> recipients;
	recipients.push_back(sender->getFd());
	std::vector<channel *> &channels = sender->getChannel();
	for (size_t i = 0; i < channels.size(); ++i)
	{
		std::vector<client *> &members = channels[i]->getMembers();
		for (size_t j = 0; j < members.size(); ++j)
		{
			int fd = members[j]->getFd();
			if (std::find(recipients.begin(), recipients.end(), fd) == recipients.end())
				recipients.push_back(fd);
		}
	}
	for (size_t i = 0; i < recipients.size(); ++i)
		sendMessage(recipients[i], message);
}

std::map<std::string, channel *>::iterator
server::findChannel(const std::string &name)
{
	std::map<std::string, channel *>::iterator it = channelList.begin();
	while (it != channelList.end())
	{
		if (ircEquals(it->first, name))
			return it;
		++it;
	}
	return channelList.end();
}
