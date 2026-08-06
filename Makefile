NAME = ircserv
SRC = main.cpp \
	server/server.cpp \
	server/server_utils.cpp \
	server/cmd_dispatch.cpp \
	server/cmd_registration.cpp \
	server/cmd_channels.cpp \
	server/cmd_mode.cpp \
	server/cmd_messaging.cpp \
	client/client.cpp \
	channel/channel.cpp
OBJ_DIR = obj
OBJ = $(SRC:%.cpp=$(OBJ_DIR)/%.o)
CC = c++ -Wall -Wextra -Werror -std=c++98
CFLAGS = -Iincludes
HEADER = includes/server.hpp includes/client.hpp includes/channel.hpp includes/irc_utils.hpp

all: $(NAME)

$(NAME): $(OBJ) $(HEADER)
	$(CC) $(CFLAGS) $(OBJ) -o $(NAME)

$(OBJ_DIR)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -rf $(NAME)

re : fclean all
