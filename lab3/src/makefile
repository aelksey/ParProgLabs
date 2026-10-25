CC = gcc
# Добавили флаг подавления варнингов депрекейтов OpenCL и выставили таргет-версию
CFLAGS = -Wall -Wextra -O2 -I. -Wno-deprecated-declarations -DCL_TARGET_OPENCL_VERSION=300

# Просто передаем саму DLL напрямую линкеру, убирая опасный поиск по -L"System32"
LIBS = C:/Windows/System32/OpenCL.dll
TARGET = cl_benchmark.exe

INPUT = matrix_6_2500x2500i.txt

SRC = main.c utils.c opencl_runner.c
OBJ = $(SRC:.c=.o)

all: $(TARGET)

run:
	$(TARGET) -f $(INPUT) -s 10

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LIBS)

%.o: %.c common.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	del /f /q $(OBJ) $(TARGET)

.PHONY: all clean
