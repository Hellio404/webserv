NAME    = webserv
CC      = clang++
FLAGS   = -g -fsanitize=address #-Wall -Wextra -Werror
HEADS   = src/Chunker.hpp src/Config.hpp src/Connection.hpp src/Date.hpp src/EventLoop.hpp src/FileUtility.hpp src/Multiplexing.hpp src/Parser.hpp src/Utils.hpp 
SRCS = src/Chunker.cpp src/Config.cpp src/ConfigHandler.cpp src/Connection.cpp src/Date.cpp \
	 src/EventLoop.cpp src/FileUtility.cpp src/Multiplexing.cpp src/Parser.cpp src/Utils.cpp webserv.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(NAME)
$(NAME): $(OBJS)
	$(CC) $(FLAGS) $(FLAGS_DEBUG) -o $(NAME) $(OBJS)

%.o: %.cpp $(HEADS)
	$(CC) $(FLAGS) $(FLAGS_DEBUG) -c -o $@ $<
clean:
	rm -rf $(OBJS)
fclean: clean
	rm -rf $(NAME)
re: fclean all

