CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = mysh

all: $(TARGET)

$(TARGET): mysh.c
	$(CC) $(CFLAGS) -o $(TARGET) mysh.c

clean:
	rm -f $(TARGET)

.PHONY: all clean
