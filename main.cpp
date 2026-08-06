#include "server.hpp"
#include <signal.h>
#include <cerrno>
#include <cstdlib>
volatile sig_atomic_t g_stop = 0;

void stopServer(int signal)
{
	(void)signal;
	g_stop = 1;
}

int main(int argc, char **argv)
{
	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, stopServer);
	signal(SIGTERM, stopServer);
	if (argc != 3)
	{
		std::cerr << "./ircserv [port] [password]" << std::endl;
		return 1;
	}

	errno = 0;
	char *end = NULL;
	long port = std::strtol(argv[1], &end, 10);
	if (errno != 0 || argv[1][0] == '\0' || *end != '\0' || port < 1 || port > 65535)
	{
		std::cerr << "Invalid port" << std::endl;
		return 1;
	}

	server serv(static_cast<int>(port), argv[2]);
	try
	{
		serv.start();
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		return 1;
	}

	return 0;
}
