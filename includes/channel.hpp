#pragma once

#include "client.hpp"
#include <vector>
#include <algorithm>
#include <sys/socket.h>

class channel
{
private:
	std::string 		  name;
	std::string 		  modes;
	std::string 		  password;
	std::string 		  topic;

	size_t 				  limit;

	bool 				  invite_only;
	bool				  topic_op_only;

	std::vector<client *> members;
	std::vector<client *> operators;
	std::vector<client *> invited;

public:
	channel(/* args */);
	~channel();

	std::string &getName();
	std::string &getModes();
	std::string &getPassword();
	size_t getLimit();
	std::vector<client *> &getOperators();

	void setName(std::string &name);
	void setModes(std::string modes);
	void removeModes(std::string modes);
	void setPassword(std::string password);
	void setLimit(size_t limit);

	std::string &getTopic();
	void setTopic(std::string &topic);
	bool getTopicOpOnly();
	void setTopicOpOnly(bool value);
	bool isClientOp(std::string clientName);
	bool isClientMember(std::string clientName);

	void setOperator(client *op);
	void removeOperator(client *op);

	void setInviteOnly(bool);
	bool &getInviteOnly();
	bool isClientInvited(std::string clientName);
	void addInvited(client *c);
	void removeInvited(client *c);

	std::vector<client *> &getMembers();
	size_t getMembersCount();

	// void sendMessage(std::string &msg);
	void addMember(client &member);
	void removeMember(client &member);
	// void sendMessageExcept(std::string &message, int excludedFd);
	bool isEmpty();
};
