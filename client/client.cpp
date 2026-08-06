#include "client.hpp"

client::client() : hostname("localhost"), registered(false), passwordOk(false), disconnect(false), capNegotiating(false)
{
}

std::string &client::getNickname()
{
	return nickname;
}

std::string &client::getHostname()
{
	return hostname;
}

std::string &client::getUsername()
{
	return username;
}

std::string &client::getRealname()
{
	return realname;
}

bool &client::getCapNegotiating()
{
	return this->capNegotiating;
}

void client::setCapNegotiating(bool value)
{
	this->capNegotiating = value;
}

std::string &client::getModes()
{
	return modes;
}


void client::setNickname(std::string &newNickname)
{
	nickname = newNickname;
}

void client::setHostname(std::string &newHostname)
{
	hostname = newHostname;
}

void client::setUsername(std::string &newUsername)
{
	username = newUsername;
}

void client::setRealname(std::string &newRealname)
{
	realname = newRealname;
}

std::vector<channel *> &client::getChannel()
{
	return this->channelIn;
}


void client::setModes(std::string modes)
{
	for (size_t i = 0; i < modes.size(); i++)
	{
		if (this->modes.find(modes[i]) == std::string::npos)
			this->modes += modes[i];
	}
}

void client::removeModes(std::string modes)
{
	for (size_t i = 0; i < modes.size(); i++)
	{
		size_t mode_pos = this->modes.find(modes[i]);
		if (mode_pos != std::string::npos)
			this->modes.erase(mode_pos, 1);
	}
}


bool &client::getPasswordOk()
{
	return this->passwordOk;
}

void client::setPasswordOk(bool a)
{
	this->passwordOk = a;
}

bool &client::getDisconnect()
{
	return this->disconnect;
}

void client::setDisconnect(bool a)
{
	this->disconnect = a;
}


void client::addBuffer(std::string message)
{
	this->buffer += message;
}

std::string &client::getBuffer()
{
	return this->buffer;
}


std::string client::getPrefix()
{
	return ":" + nickname + "!" + username + "@" + hostname;
}

void client::setFd(int fd)
{
	this->fd = fd;
}

bool &client::getRegistered()
{
	return this->registered;
}

void client::setRegistered(bool a)
{
	this->registered = a;
}

int &client::getFd()
{
	return this->fd;
}

client::~client()
{
}

void client::addOutputBuffer(const std::string &message)
{
	outputBuffer += message;
}

std::string &client::getOutputBuffer()
{
	return outputBuffer;
}

void client::eraseOutputBuffer(size_t count)
{
	outputBuffer.erase(0, count);
}
