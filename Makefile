CC = gcc
CFLAGS = -Wall -g


SRC_DIR = src
INC_DIR = include
TEST_DIR = tests

SRC = $(SRC_DIR)/commands.c $(SRC_DIR)/fat32.c $(SRC_DIR)/main.c $(SRC_DIR)/output.c $(SRC_DIR)/support.c $(TEST_DIR)/fat32_test.c

OBJ = $(SRC:.c=.o)
EXEC = fat32_program

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) -o $(EXEC) $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

clean:
	rm -f $(OBJ) $(EXEC)
