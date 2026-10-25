CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99 -D_POSIX_C_SOURCE=199309L
INCLUDES = -I/usr/local/cuda/include/
LIBS = -L/usr/local/cuda/lib64/ -lOpenCL

TARGET = cl_benchmark

SRC = main.c utils.c opencl_runner.c
OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(TARGET) $(OBJ) $(LIBS)

%.o: %.c common.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

run: $(TARGET)
	./$(TARGET) -f matrix_6_1750x1750i.txt -s 10 -g 16

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean run
