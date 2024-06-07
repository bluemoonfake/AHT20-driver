CC = gcc
CFLAGS = -Wall -Iinclude

SRC = src/aht20.c
OBJ = $(SRC:.c=.o)

TARGET = libaht20.a

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ) $(TARGET)