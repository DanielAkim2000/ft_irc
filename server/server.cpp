#include "server.hpp"

extern volatile sig_atomic_t g_stop;

server::server(int port, std::string password)
	: port(port), server_fd(-1), epollFd(-1), password(password)
{
}

void server::acceptClient(epoll_event &clientEv)
{
	int clientfd = accept(server_fd, NULL, NULL);
	if (clientfd == -1)
		return;
	client *Client = new client;
	Client->setFd(clientfd);
	if (password.empty())
		Client->setPasswordOk(true);
	int flag = 1;
	setsockopt(clientfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
	clientEv.events = EPOLLIN | EPOLLRDHUP;
	clientEv.data.fd = clientfd;
	epoll_ctl(epollFd, EPOLL_CTL_ADD, clientfd, &clientEv);
	fcntl(clientfd, F_SETFL, O_NONBLOCK);
	clientList[Client->getFd()] = Client;
}

void server::setupSocket()
{
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1)
		throw std::runtime_error("socket failed");

	int opt = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	fcntl(server_fd, F_SETFL, O_NONBLOCK);

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(server_fd, (sockaddr *)(&addr), sizeof(addr)) == -1)
		throw std::runtime_error("bind failed");
	if (listen(server_fd, SOMAXCONN) == -1)
		throw std::runtime_error("listen failed");
}

void server::scheduleClose(int clientFd, const std::string &reason)
{
	std::map<int, client *>::iterator it = clientList.find(clientFd);
	if (it == clientList.end())
		return;
	client *cl = it->second;
	if (!cl->getDisconnect())
	{
		notifyQuit(cl, reason);
		cl->setDisconnect(true);
	}
	if (cl->getOutputBuffer().empty())
	{
		closeConnection(clientFd);
		return;
	}
	epoll_event ev;
	std::memset(&ev, 0, sizeof(ev));
	ev.data.fd = clientFd;
	ev.events = EPOLLOUT | EPOLLRDHUP;
	if (epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &ev) == -1)
		closeConnection(clientFd);
}

void server::closeConnection(int clientFd)
{
	std::map<int, client *>::iterator it = clientList.find(clientFd);
	if (it == clientList.end())
		return;
	if (!it->second->getDisconnect())
		notifyQuit(it->second, "Connection closed");
	removeClientFromChannels(it->second);
	epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, NULL);
	close(clientFd);
	delete it->second;
	clientList.erase(it);
}

void server::handleClient(epoll_event evOccured)
{
	int clientFd = evOccured.data.fd;
	std::map<int, client *>::iterator it;
	it = clientList.find(clientFd);
	if (it == clientList.end())
		return;
	char temp[1024];
	ssize_t ret = recv(clientFd, temp, sizeof(temp), 0);
	if (ret > 0)
	{
		it->second->addBuffer(std::string(temp, ret));
		handleClientBuffer(it->second->getBuffer(), clientFd);
		if (it->second->getDisconnect())
			scheduleClose(clientFd, "Connection closed");
	}
	else if (ret == 0)
		scheduleClose(clientFd, "Connection closed");
	else
		return;
}

void server::handleClientWrite(int clientFd)
{
	std::map<int, client *>::iterator it;
	it = clientList.find(clientFd);
	if (it == clientList.end())
		return;
	std::string &output = it->second->getOutputBuffer();
	if (!output.empty())
	{
		ssize_t sent;
		sent = send(clientFd, output.c_str(), output.size(), 0);
		if (sent > 0)
			it->second->eraseOutputBuffer(static_cast<size_t>(sent));
		else
			return;
	}
	if (!output.empty())
		return;
	if (it->second->getDisconnect())
	{
		closeConnection(clientFd);
		return;
	}
	epoll_event ev;
	std::memset(&ev, 0, sizeof(ev));
	ev.data.fd = clientFd;
	ev.events = EPOLLIN | EPOLLRDHUP;

	if (epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &ev) == -1)
		closeConnection(clientFd);
}

void server::monitorClient()
{
	epollFd = epoll_create1(0);
	if (epollFd == -1)
		throw std::runtime_error("epoll_create1 failed");
	epoll_event servEv;
	std::memset(&servEv, 0, sizeof(servEv));
	servEv.events = EPOLLIN;
	servEv.data.fd = server_fd;
	if (epoll_ctl(epollFd, EPOLL_CTL_ADD, server_fd, &servEv) == -1)
		throw std::runtime_error("epoll_ctl failed");
	while (!g_stop)
	{
		epoll_event events[100];
		int eventCount = epoll_wait(epollFd, events, 100, 500);
		if (eventCount == -1)
		{
			if (errno == EINTR)
				continue;
			throw std::runtime_error("epoll_wait failed");
		}
		for (int i = 0; i < eventCount; ++i)
		{
			int fd = events[i].data.fd;
			if (fd == server_fd)
			{
				epoll_event clientEv;
				std::memset(&clientEv, 0, sizeof(clientEv));
				acceptClient(clientEv);
				continue;
			}
			if (events[i].events & EPOLLIN)
				handleClient(events[i]);
			if (clientList.find(fd) == clientList.end())
				continue;
			if (events[i].events & EPOLLOUT)
				handleClientWrite(fd);
			if (clientList.find(fd) == clientList.end())
				continue;
			if (events[i].events & (EPOLLERR | EPOLLHUP))
				closeConnection(fd);
			else if (events[i].events & EPOLLRDHUP)
				scheduleClose(fd, "Connection closed");
		}
	}
}

void server::start()
{
	setupSocket();
	monitorClient();
}

server::~server()
{
	freeEverything();
}
