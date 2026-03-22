
NAME        = ircserv
CXX         = g++
CXXFLAGS    = -Wall -Wextra -Werror -std=c++98 -Iinclude
SRC_DIR     = src
OBJ_DIR     = obj
SRCS        = $(SRC_DIR)/main.cpp \
              $(SRC_DIR)/Server.cpp \
              $(SRC_DIR)/Client.cpp \
              $(SRC_DIR)/Channel.cpp
OBJS        = $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
all: $(NAME)
$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)
clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
