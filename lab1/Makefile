CC = gcc
TARGET = matrix_det
CFLAGS = -O3 -std=c99 -fopenmp -Wall
LIBS = -lm
FILE ?= ./data/matrix_6_500x500f.txt


# Список всех исходных файлов .c, которые нужно собрать
SRCS = ./src/main.c ./src/sequential.c ./src/parallel.c
# Автоматическая генерация имен объектных файлов (.o) из имен (.c)
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

# Линковка исполняемого файла из объектных модулей
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LIBS)

# Правило для компиляции каждого отдельного файла .c в .o
%.o: %.c determinant.h
	$(CC) $(CFLAGS) -c $< -o $@

# Быстрый запуск проверки программы
run: $(TARGET)
	./$(TARGET) ./data/matrix_6_500x500f.txt
	./$(TARGET) ./data/matrix_6_1000x1000f.txt

run_once: $(TARGET)
	./$(TARGET) $(FILE)

clean:
	rm -f ./$(TARGET) $(OBJS)
