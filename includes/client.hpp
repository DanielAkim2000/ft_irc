#pragma once

#include <string>
#include <vector>

class channel;

class client
{
private:
	int fd;
	std::string nickname;
	std::string username;
	std::string hostname;
	std::string realname;
	std::string modes;

	std::string buffer;
	std::string outputBuffer;
	bool registered;
	bool passwordOk;
	bool disconnect;
	bool capNegotiating;

	std::vector<channel *> channelIn;

public:
	client();
	~client();

	int &getFd();
	void setFd(int fd);

	bool &getRegistered();
	void setRegistered(bool);


	bool 		&getPasswordOk();
	bool 		&getDisconnect();
	bool &getCapNegotiating();
	void setCapNegotiating(bool);
	std::string  getPrefix();
	std::string &getNickname();
	std::string &getHostname();
	std::string &getUsername();
	std::string &getRealname();
	std::string &getModes();
	std::vector<channel *> &getChannel();

	void setPasswordOk(bool);
	void setDisconnect(bool);
	void setNickname(std::string &);
	void setHostname(std::string &);
	void setUsername(std::string &);
	void setRealname(std::string &);
	void setModes(std::string);
	void removeModes(std::string);


	std::string &getBuffer();
	void addBuffer(std::string message);
	void addOutputBuffer(const std::string &message);
	std::string &getOutputBuffer();
	void eraseOutputBuffer(size_t count);
};
