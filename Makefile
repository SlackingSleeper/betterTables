OBJS = Main.cpp

CC = g++

#DEBUG_FLAGS makes it go debug
DEBUG_FLAGS = -g

OBJ_NAME = betterTables

all : $(OBJS)
	$(CC) $(DEBUG_FLAGS) $(OBJS) -o $(OBJ_NAME)
