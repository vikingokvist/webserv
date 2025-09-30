NAME = webserv

CPP = c++
CPPFLAGS = -Wall -Wextra -Werror -std=c++98

RM = rm -rf

SRCS = src/main.cpp src/ConfigParser.cpp  src/ServerWrapper.cpp \
	src/HttpReceive.cpp src/utils/connection_utils.cpp src/HttpSend.cpp src/Connection.cpp

OBJS = $(SRCS:.cpp=.o)

all: $(NAME)

%.o: %.cpp
	$(CPP) $(CPPFLAGS) -o $@ -c $<

$(NAME): $(OBJS)
	$(CPP) $(CPPFLAGS) $(OBJS) -o $(NAME)

clean:
	$(RM) $(OBJS)

fclean:
	$(RM) $(OBJS) $(NAME)

re: fclean all
