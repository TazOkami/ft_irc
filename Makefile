# Makefile (42 style) — C++98

NAME     = ircserv
BOT      = ircbot

CXX      = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -MMD -MP

-include $(DEP)

INCDIR   = include
SRCDIR   = src
OBJDIR   = build

SRC = \
	$(SRCDIR)/main.cpp \
	$(SRCDIR)/Server_Loop.cpp \
	$(SRCDIR)/Poller.cpp \
	$(SRCDIR)/Parser.cpp \
	$(SRCDIR)/Server_IO.cpp \
	$(SRCDIR)/Server_Dispatch.cpp \
	$(SRCDIR)/Handlers_Core.cpp \
	$(SRCDIR)/Handlers_Registration.cpp \
	$(SRCDIR)/Handlers_Channel_Modes.cpp \
	$(SRCDIR)/Handlers_Extras.cpp \
	$(SRCDIR)/Handlers_Utils.cpp \
	$(SRCDIR)/Utils.cpp

OBJ = $(SRC:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
DEP = $(OBJ:.o=.d)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) $(OBJ) -o $(NAME)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

all: $(NAME)

bot: $(BOT)

$(BOT): tools/ircbot.cpp
	$(CXX) $(CXXFLAGS) -o $(BOT) $<

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(NAME) $(BOT)

re: fclean all

-include $(DEP)

.PHONY: all clean fclean re bot
