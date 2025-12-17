# Makefile (42 style) — C++98

NAME     = ircserv

CXX      = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

INCDIR   = include
SRCDIR   = src
OBJDIR   = build

SRC = \
	$(SRCDIR)/main.cpp \
	$(SRCDIR)/Server.cpp \
	$(SRCDIR)/Poller.cpp \
	$(SRCDIR)/Client.cpp \
	$(SRCDIR)/Parser.cpp \
	$(SRCDIR)/Channel.cpp \
	$(SRCDIR)/ServerIO.cpp \
	$(SRCDIR)/ServerDispatch.cpp \
	$(SRCDIR)/Handlers_Core.cpp \
	$(SRCDIR)/Handlers_Channel.cpp \
	$(SRCDIR)/Handlers_Channel_Modes.cpp \
	$(SRCDIR)/Handlers_Extras.cpp \
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

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

-include $(DEP)

.PHONY: all clean fclean re
