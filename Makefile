NAME		=	webserv
CC			=	clang++
FLAGS		=	-I. -I./src -I./$(REGEX_PATH)includes -fsanitize=address -g#-Wall -Wextra -Werror
HEADS		=	src/Chunker.hpp src/Config.hpp src/Connection.hpp src/Date.hpp src/EventLoop.hpp src/FileUtility.hpp \
				src/Multiplexing.hpp src/Parser.hpp src/Utils.hpp src/HeaderParser.hpp src/Response/Handler.hpp src/Logger.hpp
SRCS		=	src/Chunker.cpp src/Config.cpp src/ConfigHandler.cpp src/Connection.cpp src/Date.cpp src/EventLoop.cpp \
				src/FileUtility.cpp src/Multiplexing.cpp src/Parser.cpp src/Utils.cpp webserv.cpp src/HeaderParser.cpp \
				src/ResponseServer.cpp src/Response/LoggerHandler.cpp src/Response/PostAccessHandler.cpp \
				src/Response/IndexHandler.cpp src/Response/AutoIndexHandler.cpp src/Response/FileHandler.cpp src/Response/RedirectHandler.cpp \
				src/Logger.cpp src/Response/ConditionalHandler.cpp src/Response/ResponseServerHandler.cpp src/Response/PreAccessHandler.cpp \
				src/BodyHandler.cpp src/Response/UploadHandler.cpp src/Response/CGIHandler.cpp src/Response/InjectHeaderHandler.cpp
OBJS		=	$(SRCS:.cpp=.o)
REGEX		=	RegexEngine/libregex.a
REGEX_PATH	=	RegexEngine/

all: $(NAME)

$(REGEX):
	make -C $(REGEX_PATH)

$(NAME): $(REGEX)  $(OBJS)
	$(CC) $(FLAGS) $(FLAGS_DEBUG) -o $(NAME) $(OBJS)   -L$(REGEX_PATH) -lregex

%.o: %.cpp $(HEADS)
	$(CC) $(FLAGS) $(FLAGS_DEBUG) -c -o $@ $<

clean:
	make -C $(REGEX_PATH) clean
	rm -rf $(OBJS)

fclean: clean
	make -C $(REGEX_PATH) fclean
	rm -rf $(NAME)

re: fclean all