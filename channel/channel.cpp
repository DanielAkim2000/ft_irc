#include "channel.hpp"
#include "irc_utils.hpp"

channel::channel(/* args */) : limit(0), invite_only(false), topic_op_only(false)
{
}

channel::~channel()
{
}

std::string &channel::getName()
{
	return this->name;
}

std::string &channel::getModes()
{
	return this->modes;
}

std::string &channel::getPassword()
{
	return this->password;
}

size_t channel::getLimit()
{
	return this->limit;
}

std::vector<client *> &channel::getOperators()
{
	return this->operators;
}

void channel::setName(std::string &name)
{
	this->name = name;
}

void channel::setModes(std::string modes)
{
	for (size_t i = 0; i < modes.size(); i++)
	{
		if (this->modes.find(modes[i]) == std::string::npos)
			this->modes += modes[i];
	}
}

void channel::removeModes(std::string modes)
{
	for (size_t i = 0; i < modes.size(); i++)
	{
		size_t pos = this->modes.find(modes[i]);
		if (pos != std::string::npos)
			this->modes.erase(pos, 1);
	}
}

bool channel::getTopicOpOnly()
{
	return this->topic_op_only;
}

void channel::setTopicOpOnly(bool value)
{
	this->topic_op_only = value;
}

void channel::setPassword(std::string password)
{
	this->password = password;
}

void channel::setLimit(size_t limit)
{
	this->limit = limit;
}

void channel::setOperator(client *op)
{
	if (std::find(this->operators.begin(), this->operators.end(), op) == this->operators.end())
		this->operators.push_back(op);
}

void channel::removeOperator(client *op)
{
	std::vector<client *>::iterator it = std::find(this->operators.begin(), this->operators.end(), op);

	if (it != this->operators.end())
		this->operators.erase(it);
}
bool channel::isClientOp(std::string name)
{
	for (size_t i = 0; i < operators.size(); i++)
	{
		if (ircEquals(operators[i]->getNickname(), name))
			return true;
	}
	return false;
}

bool channel::isClientMember(std::string name)
{
	for (size_t i = 0; i < members.size(); i++)
	{
		if (ircEquals(members[i]->getNickname(), name))
			return true;
	}
	return false;
}

void channel::setInviteOnly(bool a)
{
	this->invite_only = a;
}


/* ajout pour TOPIC */
std::string &channel::getTopic()
{
	return this->topic;
}

void channel::setTopic(std::string &newTopic)
{
	this->topic = newTopic;
}

// void channel::sendMessageExcept(std::string &message, int excludedFd)
// {
// 	for (std::size_t i = 0; i < members.size(); ++i)
// 	{
// 		if (members[i]->getFd() != excludedFd)
// 			send(members[i]->getFd(), message.c_str(), message.size(), 0);
// 	}
// }

bool channel::isEmpty()
{
	return members.empty();
}

bool &channel::getInviteOnly()
{
	return this->invite_only;
}

// void channel::sendMessage(std::string &message)
// {
// 	for (size_t i = 0; i < members.size(); i++)
// 		send(members[i]->getFd(), message.c_str(), message.size(), 0);
// }

void channel::addMember(client &member)
{
	members.push_back(&member);
}

void channel::removeMember(client &member)
{
	for (size_t i = 0; i < members.size(); i++)
	{
		if (members[i] == &member)
			members.erase(members.begin() + i);
	}
	for (size_t i = 0; i < operators.size(); i++)
	{
		if (operators[i] == &member)
			operators.erase(operators.begin() + i);
	}
}

bool channel::isClientInvited(std::string name)
{
	for (size_t i = 0; i < invited.size(); i++)
	{
		if (ircEquals(invited[i]->getNickname(), name))
			return (true);
	}
	return (false);
}

void channel::addInvited(client *c)
{
	if (std::find(invited.begin(), invited.end(), c) == invited.end())
		invited.push_back(c);
}

void channel::removeInvited(client *c)
{
	std::vector<client *>::iterator it = std::find(invited.begin(), invited.end(), c);
	if (it != invited.end())
		invited.erase(it);
}

size_t channel::getMembersCount(void)
{
	return (members.size());
}

std::vector<client *> &channel::getMembers(void)
{
	return (members);
}
