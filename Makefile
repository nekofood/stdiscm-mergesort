# Makefile for concurrent_merge_sort.cpp

CC = g++
CFLAGS = -Wall -Wextra -pthread
TARGET = conc
SRC = concurrent_merge_sort.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
