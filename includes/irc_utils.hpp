#pragma once

#include <string>

inline char ircCasefoldChar(char c)
{
	if (c >= 'A' && c <= 'Z')
		return static_cast<char>(c + ('a' - 'A'));
	if (c == '[')
		return '{';
	if (c == ']')
		return '}';
	if (c == '\\')
		return '|';
	if (c == '~')
		return '^';

	return c;
}

inline bool ircEquals(const std::string &a, const std::string &b)
{
	if (a.size() != b.size())
		return false;
	for (std::size_t i = 0; i < a.size(); ++i)
	{
		if (ircCasefoldChar(a[i]) != ircCasefoldChar(b[i]))
			return false;
	}
	return true;
}

