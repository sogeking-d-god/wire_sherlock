CC = gcc
CFLAGS = -Wall -Wextra -I./include -O3
LDFLAGS = -lm

SRC = src/pelt.c src/stats.c
OBJ = $(SRC:.c=.o)
TARGET = libanalytics.a

all: $(TARGET)

$(TARGET): $(OBJ)
	ar rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(TARGET)