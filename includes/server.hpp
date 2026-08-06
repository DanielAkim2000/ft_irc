#pragma once

#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

#include "client.hpp"
#include "channel.hpp"

class server
{
private:
	int port;
	int server_fd;
	int epollFd;

	std::string password;
	std::map<std::string, channel *> channelList;
	std::map<int, client *> clientList;

public:
	server(int, std::string);
	~server();

	// SERVER
	void start();
	void setupSocket();
	void monitorClient();
	void acceptClient(epoll_event &);
	void handleClient(epoll_event);
	void handleClientWrite(int clientFd);
	void welcomeClient(epoll_event, client &);
	void closeConnection(int clientFd);
	void scheduleClose(int clientFd, const std::string &reason);

	// DISPATCH
	void handleClientBuffer(std::string &clientBuffer, int clientFd);
	void getCommand(std::string &, int clientFd);
	void handleCommand(std::string &cmd, std::vector<std::string> &params, int clientFd);

	// REGISTRATION
	void cap(std::string &cmd, std::vector<std::string> &params, int clientFd);
	void pass(std::string &cmd, std::vector<std::string> &params, int clientFd);
	void nick(std::string &cmd, std::vector<std::string> &params, int clientFd);
	void user(std::string &cmd, std::vector<std::string> &params, int clientFd);
	void quit(std::string &cmd, std::vector<std::string> &params, int clientFd);
	void ping(std::string &cmd, std::vector<std::string> &params, int clientFd);

	// CHANNELS
	void join(std::string &cmd, std::vector<std::string> &params, int clientFd);
	void joinOneChannel(client *cl, int clientFd, std::string &nick, std::string &chanName, std::string &key);
	void execJoinChannel(client *cl, channel *ch, std::string &chanName);
	bool checkJoinAllowed(client *cl, channel *ch, std::string &nick, std::string &chanName, std::string &key, int clientFd);
	channel *getOrCreateChannel(std::string &chanName);
	void sendJoinReplies(client *cl, channel *ch);
	void splitCommaList(std::string &list, std::vector<std::string> &out);
	bool isValidChannelName(std::string &name);
	void part(std::vector<std::string> &params, int clientFd);
	void topic(std::vector<std::string> &params, int clientFd);
	void invite(std::vector<std::string> &params, int clientFd);
	void kick(std::vector<std::string> &params, int clientFd);
	void kickOneChannel(client *sender, channel *ch, std::string &channelName, std::string &targetNick, std::string &comment, int clientFd);

	// MODE
	void mode(std::string &cmd, std::vector<std::string> &params, int clientFd);
	void modeUser(std::vector<std::string> &params, int clientFd, std::map<int, client *>::iterator it);
	void modeChannel(std::vector<std::string> &params, int clientFd, std::map<int, client *>::iterator it);
	void execModeChannel(std::vector<std::pair<std::string, std::string> > &pair, std::map<std::string, channel *>::iterator &channel_it, std::map<int, client *>::iterator it);

	// MESSAGING
	void privmsg(std::vector<std::string> &params, int clientFd);
	void bot(int clientFd);

	// UTILS
	void sendMessage(int clientFd, std::string message);
	void sendError(std::string code, int clientFd, std::string clientName, std::string param1);
	bool isValidNickname(std::string &nickname);
	bool isClientHere(std::string name);
	client *getClientByName(std::string &name);
	std::map<std::string, channel *>::iterator findChannel(const std::string &name);
	void freeEverything();
	void removeClientFromChannels(client *client);
	void sendToChannel(channel *ch, std::string message, int excludedFd);
	void sendToSharedClients(client *sender, std::string message);
	void tryRegisterClient(int clientFd);
	void notifyQuit(client *sender, std::string reason);
};
