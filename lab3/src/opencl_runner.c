#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "common.h"

void run_parallel(const int* matrix, int rows, int cols, int tri_size, int local_size_param,
                  unsigned long long* out_sum, int* out_r, int* out_c, 
                  int* out_triangle, unsigned long long* out_count) {
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_int err;

    clGetPlatformIDs(1, &platform, NULL);
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device, NULL); 
    }

    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    queue = clCreateCommandQueue(context, device, 0, &err);

    char* kernel_src = load_kernel_source("kernel.cl");
    program = clCreateProgramWithSource(context, 1, (const char**)&kernel_src, NULL, &err);
    clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    kernel = clCreateKernel(program, "find_min_triangle", &err);

    size_t padded_rows = ((rows + local_size_param - 1) / local_size_param) * local_size_param;
    size_t padded_cols = ((cols + local_size_param - 1) / local_size_param) * local_size_param;

    size_t global_size[2] = {padded_rows, padded_cols};
    size_t local_size[2] = {(size_t)local_size_param, (size_t)local_size_param};

    size_t padded_elements = padded_rows * padded_cols;
    size_t matrix_elements = (size_t)rows * cols;

    int* h_res_r = (int*)malloc(padded_elements * sizeof(int));
    int* h_res_c = (int*)malloc(padded_elements * sizeof(int));
    unsigned long long* h_res_sum = (unsigned long long*)malloc(padded_elements * sizeof(unsigned long long));

    cl_mem d_matrix = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, matrix_elements * sizeof(int), (void*)matrix, &err);
    cl_mem d_res_r = clCreateBuffer(context, CL_MEM_WRITE_ONLY, padded_elements * sizeof(int), NULL, &err);
    cl_mem d_res_c = clCreateBuffer(context, CL_MEM_WRITE_ONLY, padded_elements * sizeof(int), NULL, &err);
    cl_mem d_res_sum = clCreateBuffer(context, CL_MEM_WRITE_ONLY, padded_elements * sizeof(unsigned long long), NULL, &err);

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &d_matrix);
    clSetKernelArg(kernel, 1, sizeof(int), &rows);
    clSetKernelArg(kernel, 2, sizeof(int), &cols);
    clSetKernelArg(kernel, 3, sizeof(int), &tri_size);
    clSetKernelArg(kernel, 4, sizeof(cl_mem), &d_res_r);
    clSetKernelArg(kernel, 5, sizeof(cl_mem), &d_res_c);
    clSetKernelArg(kernel, 6, sizeof(cl_mem), &d_res_sum);

    clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global_size, local_size, 0, NULL, NULL);
    clFinish(queue);

    clEnqueueReadBuffer(queue, d_res_r, CL_TRUE, 0, padded_elements * sizeof(long), h_res_r, 0, NULL, NULL);
    clEnqueueReadBuffer(queue, d_res_c, CL_TRUE, 0, padded_elements * sizeof(int), h_res_c, 0, NULL, NULL);
    clEnqueueReadBuffer(queue, d_res_sum, CL_TRUE, 0, padded_elements * sizeof(unsigned long long), h_res_sum, 0, NULL, NULL);

    unsigned long long min_sum = 18446744073709551615ULL;
    unsigned long long count = 0;
    int last_r = -1, last_c = -1;

    // Шаг 1: Ищем глобальный минимум по всей сетке GPU
    for (size_t i = 0; i < padded_elements; i++) {
        if (h_res_r[i] == -1 || h_res_c[i] == -1) continue;
        if (h_res_sum[i] < min_sum) {
            min_sum = h_res_sum[i];
        }
    }

    // Шаг 2: Обходим строго в порядке строк и столбцов (как CPU), чтобы найти точное количество 
    // и зафиксировать идентичный САМЫЙ ПОСЛЕДНИЙ элемент
    for (size_t r = 0; r < padded_rows; r++) {
        for (size_t c = 0; c < padded_cols; c++) {
            size_t i = r * padded_cols + c;
            if (h_res_r[i] == -1 || h_res_c[i] == -1) continue;

            if (h_res_sum[i] == min_sum) {
                count++;
                last_r = (int)h_res_r[i];
                last_c = (int)h_res_c[i];
            }
        }
    }

    *out_sum = min_sum;
    *out_r = last_r;
    *out_c = last_c;
    *out_count = count;

    // Копируем элементы последней матрицы, найденной параллельным методом
    int idx = 0;
    for (int i = 0; i < tri_size; i++) {
        int row_offset = (last_r + i) * cols;
        for (int j = 0; j <= i; j++) {
            out_triangle[idx++] = matrix[row_offset + (last_c + j)];
        }
    }

    free(h_res_r); free(h_res_c); free(h_res_sum); free(kernel_src);
    clReleaseMemObject(d_matrix); clReleaseMemObject(d_res_r);
    clReleaseMemObject(d_res_c); clReleaseMemObject(d_res_sum);
    clReleaseKernel(kernel); clReleaseProgram(program);
    clReleaseCommandQueue(queue); clReleaseContext(context);
}
