#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "common.h"

void run_parallel(const int* matrix, int rows, int cols, int tri_size, int local_size_param,
                  unsigned long long* out_sum, int* out_r, int* out_c, int* out_triangle, unsigned long long* out_count) {
    cl_platform_id platforms[10];
    cl_uint num_platforms = 0;
    cl_device_id device = NULL;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_int err;

    // 1. Получаем список всех доступных OpenCL платформ на сервере
    err = clGetPlatformIDs(10, platforms, &num_platforms);
    if (err != CL_SUCCESS || num_platforms == 0) {
        fprintf(stderr, "[OpenCL Error]: No OpenCL platforms found on this server!\n");
        exit(1);
    }

    // 2. Умный поиск: перебираем платформы, пока не найдем физический GPU (например, NVIDIA)
    int gpu_found = 0;
    for (cl_uint i = 0; i < num_platforms; i++) {
        char platform_name[128];
        clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, sizeof(platform_name), platform_name, NULL);
        
        err = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU, 1, &device, NULL);
        if (err == CL_SUCCESS && device != NULL) {
            char device_name[128];
            clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
            printf("[OpenCL Init]: Found GPU '%s' on platform '%s'\n", device_name, platform_name);
            gpu_found = 1;
            break; 
        }
    }

    // 3. Резервный вариант: если GPU нет, ищем CPU-рантайм на любой из платформ
    if (!gpu_found) {
        printf("[OpenCL Warning]: Active GPU not found. Searching for CPU fallback...\n");
        for (cl_uint i = 0; i < num_platforms; i++) {
            err = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_CPU, 1, &device, NULL);
            if (err == CL_SUCCESS && device != NULL) {
                char device_name[128];
                clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
                printf("[OpenCL Init]: Falling back to CPU '%s'\n", device_name);
                gpu_found = 1;
                break;
            }
        }
    }

    if (!gpu_found || device == NULL) {
        fprintf(stderr, "[OpenCL Error]: No compute devices (GPU/CPU) are accessible for this user!\n");
        exit(1);
    }

    // 4. Создание контекста и очереди команд
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[OpenCL Error]: Context creation failed. Code: %d\n", err);
        exit(1);
    }
    queue = clCreateCommandQueue(context, device, 0, &err);

    // 5. Загрузка и JIT-компиляция ядра
    char* kernel_src = load_kernel_source("kernel.cl");
    program = clCreateProgramWithSource(context, 1, (const char**)&kernel_src, NULL, &err);
    
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char *log = (char *)malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        fprintf(stderr, "[OpenCL JIT Compilation Error]:\n%s\n", log);
        free(log);
        exit(1);
    }

    kernel = clCreateKernel(program, "find_min_triangle", &err);

    // 6. Расчет расширенных размерностей сетки потоков (скаляры во избежание багов Си)
    size_t padded_cols = ((cols + local_size_param - 1) / local_size_param) * local_size_param;
    size_t padded_rows = ((rows + local_size_param - 1) / local_size_param) * local_size_param;

    size_t global_size[2] = {padded_cols, padded_rows}; // Dim 0 = columns, Dim 1 = rows
    size_t local_size[2] = {(size_t)local_size_param, (size_t)local_size_param};

    size_t padded_elements = padded_rows * padded_cols;
    size_t matrix_elements = (size_t)rows * cols;

    // Выделение памяти под плоские результаты на хосте (строго int для Windows/Linux совместимости)
    int* h_res_r = (int*)malloc(padded_elements * sizeof(int));
    int* h_res_c = (int*)malloc(padded_elements * sizeof(int));
    unsigned long long* h_res_sum = (unsigned long long*)malloc(padded_elements * sizeof(unsigned long long));

    // Создание буферов в видеопамяти устройства
    cl_mem d_matrix = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, matrix_elements * sizeof(int), (void*)matrix, &err);
    cl_mem d_res_r = clCreateBuffer(context, CL_MEM_WRITE_ONLY, padded_elements * sizeof(int), NULL, &err);
    cl_mem d_res_c = clCreateBuffer(context, CL_MEM_WRITE_ONLY, padded_elements * sizeof(int), NULL, &err);
    cl_mem d_res_sum = clCreateBuffer(context, CL_MEM_WRITE_ONLY, padded_elements * sizeof(unsigned long long), NULL, &err);

    // Передача аргументов в ядро
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &d_matrix);
    clSetKernelArg(kernel, 1, sizeof(int), &rows);
    clSetKernelArg(kernel, 2, sizeof(int), &cols);
    clSetKernelArg(kernel, 3, sizeof(int), &tri_size);
    clSetKernelArg(kernel, 4, sizeof(cl_mem), &d_res_r);
    clSetKernelArg(kernel, 5, sizeof(cl_mem), &d_res_c);
    clSetKernelArg(kernel, 6, sizeof(cl_mem), &d_res_sum);

    // 7. Запуск вычислений и ожидание завершения
    err = clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global_size, local_size, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[OpenCL Error]: Kernel launch failed. Code: %d\n", err);
        exit(1);
    }
    clFinish(queue);

    // Чтение результатов обратно в оперативную память хоста (строго по размерам типов данных)
    clEnqueueReadBuffer(queue, d_res_r, CL_TRUE, 0, padded_elements * sizeof(int), h_res_r, 0, NULL, NULL);
    clEnqueueReadBuffer(queue, d_res_c, CL_TRUE, 0, padded_elements * sizeof(int), h_res_c, 0, NULL, NULL);
    clEnqueueReadBuffer(queue, d_res_sum, CL_TRUE, 0, padded_elements * sizeof(unsigned long long), h_res_sum, 0, NULL, NULL);

    unsigned long long min_sum = 0xFFFFFFFFFFFFFFFFUL; // ULONG_MAX
    unsigned long long count = 0;
    int last_r = -1, last_c = -1;

    // Этап 1 редукции: Находим глобальный минимум среди всех потоков
    for (size_t i = 0; i < padded_elements; i++) {
        if (h_res_r[i] == -1 || h_res_c[i] == -1) continue;
        if (h_res_sum[i] < min_sum) {
            min_sum = h_res_sum[i];
        }
    }

    // Этап 2 редукции: Обходим строго в порядке CPU, чтобы посчитать точное количество и вытащить ПОСЛЕДНИЙ
    for (int r = 0; r <= rows - tri_size; r++) {
        for (int c = 0; c <= cols - tri_size; c++) {
            size_t i = (size_t)r * padded_cols + (size_t)c;
            if (h_res_sum[i] == min_sum) {
                count++;
                last_r = h_res_r[i];
                last_c = h_res_c[i];
            }
        }
    }

    *out_sum = min_sum;
    *out_r = last_r;
    *out_c = last_c;
    *out_count = count;

    // Извлекаем элементы финальной подматрицы, найденной параллельной версией
    int idx = 0;
    for (int i = 0; i < tri_size; i++) {
        int row_offset = (last_r + i) * cols;
        for (int j = 0; j <= i; j++) {
            out_triangle[idx++] = matrix[row_offset + (last_c + j)];
        }
    }

    // Освобождение ресурсов
    free(h_res_r); free(h_res_c); free(h_res_sum); free(kernel_src);
    clReleaseMemObject(d_matrix); clReleaseMemObject(d_res_r);
    clReleaseMemObject(d_res_c); clReleaseMemObject(d_res_sum);
    clReleaseKernel(kernel); clReleaseProgram(program);
    clReleaseCommandQueue(queue); clReleaseContext(context);
}
