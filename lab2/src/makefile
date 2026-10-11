# Имя результирующего исполняемого файла на Linux
TARGET = program.exe

# Используем компилятор MPICH
CC = /usr/lib64/mpich/bin/mpicc

# Флаги компиляции и оптимизации скорости
CFLAGS = -Wall -O3 -std=c99

# Список объектных файлов
OBJS = main.o common.o sequential.o parallel.o

# Список заголовочных файлов
DEPS = common.h

# Правило по умолчанию
all: $(TARGET)

# Сборка финального файла (mpicc сам линкует необходимые библиотеки MPI)
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Универсальное правило компиляции .c в .o
%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

# Запуск на МАКСИМАЛЬНОМ количестве доступных ядер (используется nproc)
run: $(TARGET)
	/usr/lib64/mpich/bin/mpiexec -n $$(nproc) ./$(TARGET) input.txt

# Очистка проекта в стиле Linux (используем rm -f)
clean:
	rm -f *.o $(TARGET) seq_output.txt par_output_rank_*.txt

.PHONY: all clean run
