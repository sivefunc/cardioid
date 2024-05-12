CC = gcc
OBJ_DIR = objs/
SRC_DIR = src/
OBJS = $(OBJ_DIR)cardioid.o

CFLAGS= -Wall -Wextra -Ofast $(shell pkg-config --cflags --libs sdl2)
LDFLAGS = -lm $(shell pkg-config --libs sdl2)
PROG_NAME = cardioid

$(PROG_NAME) : $(OBJS)
	@$(CC) -o $(PROG_NAME) $(OBJS) $(LDFLAGS) $(CFLAGS)

$(OBJ_DIR)cardioid.o : $(SRC_DIR)cardioid.c
	@$(CC) $(CFLAGS) -c $(SRC_DIR)cardioid.c -o $(OBJ_DIR)cardioid.o

run:
	./$(PROG_NAME)

.PHONY : clean
clean :
	rm $(PROG_NAME) $(OBJS)
