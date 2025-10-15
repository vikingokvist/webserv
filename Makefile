NAME = webserv

CPP = c++
CPPFLAGS = -Wall -Wextra -Werror -std=c++98 -MMD -MP

RM = rm -rf

SRCS = src/main.cpp src/ConfigParser.cpp  src/ServerWrapper.cpp \
	src/HttpReceive.cpp src/utils.cpp src/HttpSend.cpp \
	src/Connection.cpp src/Logger.cpp src/cookies.cpp

OBJS = $(SRCS:.cpp=.o)
DEPS = $(OBJS:.o=.d)

all: $(NAME)

%.o: %.cpp
	$(CPP) $(CPPFLAGS) -o $@ -c $<

$(NAME): $(OBJS)
	$(CPP) $(CPPFLAGS) $(OBJS) -o $(NAME)

clean:
	$(RM) $(OBJS) $(DEPS)

fclean:
	$(RM) $(NAME) $(OBJS) $(DEPS)

re: fclean all

-include $(DEPS)