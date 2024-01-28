NAME = Webserve

CC = c++
CFLAGS	= -Wall -Wextra -Werror -Wshadow -Wno-shadow -std=c++98 #-fsanitize=address

RM		= rm -rf

#Colors:
GREEN		=	\e[92;5;118m
YELLOW		=	\e[93;5;226m
GRAY		=	\e[33;2;37m
RESET		=	\e[0m
CURSIVE		=	\e[33;3m

SRC_DIR = srcs/
OBJ_DIR = objs/
OBJS = $(SRCS:.cpp=.o)

INCS = -Iincludes/

SRCS = config_handler.cpp \
	main.cpp \
	server.cpp \
	response_handler.cpp \
	utility.cpp \
	forTest.cpp 

DBG = *.dSYM

all: $(NAME)

$(NAME): $(addprefix $(OBJ_DIR),$(OBJS))
	@printf "$(CURSIVE)$(GRAY) 	- Compiling $(NAME)... $(RESET)\n"
	@$(CC) $(CFLAGS) $(addprefix $(OBJ_DIR),$(OBJS)) -o $(NAME)
	@printf "$(GREEN)    - $(NAME) Executable ready.\n$(RESET)"

$(OBJ_DIR)%.o: $(SRC_DIR)%.cpp
	@mkdir -p $(OBJ_DIR)
	@$(CC) $(CFLAGS) -c $< $(INCS) -o $@

clean:
	@$(RM) $(DBG) $(OBJ_DIR)
	@printf "$(YELLOW)    - $(NAME) Objects removed.$(RESET)\n"

fclean: clean
	@$(RM) $(NAME) $(DBG)
	@printf "$(YELLOW)    - $(NAME) Executable removed.$(RESET)\n"

re: fclean all

.PHONY: all clean re fclean
