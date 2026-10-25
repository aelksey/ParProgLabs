#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <CL/cl_platform.h>
#include <CL/cl.h>
#include <malloc.h>

typedef struct
{
	unsigned width; unsigned height; cl_mem elems;
} matrix_t ;
#define ROWS 1024
#define COLROWS 1024
#define COLUMNS 1024
//	const unsigned rows = 1024 , colRows = 1024 , columns = 1024;
	const unsigned BLOCK_SIZE = 16;
	float mem_F [ROWS][COLROWS], mem_S [COLROWS][COLUMNS], mem_R [ROWS][COLUMNS], mem_Q [ROWS][COLUMNS];

char* errCodeToString(int err){
    switch (err) {
        case CL_SUCCESS:                            return "Success!";
        case CL_DEVICE_NOT_FOUND:                   return "Device not found.";
        case CL_DEVICE_NOT_AVAILABLE:               return "Device not available";
        case CL_COMPILER_NOT_AVAILABLE:             return "Compiler not available";
        case CL_MEM_OBJECT_ALLOCATION_FAILURE:      return "Memory object allocation failure";
        case CL_OUT_OF_RESOURCES:                   return "Out of resources";
        case CL_OUT_OF_HOST_MEMORY:                 return "Out of host memory";
        case CL_PROFILING_INFO_NOT_AVAILABLE:       return "Profiling information not available";
        case CL_MEM_COPY_OVERLAP:                   return "Memory copy overlap";
        case CL_IMAGE_FORMAT_MISMATCH:              return "Image format mismatch";
        case CL_IMAGE_FORMAT_NOT_SUPPORTED:         return "Image format not supported";
        case CL_BUILD_PROGRAM_FAILURE:              return "Program build failure";
        case CL_MAP_FAILURE:                        return "Map failure";
        case CL_INVALID_VALUE:                      return "Invalid value";
        case CL_INVALID_DEVICE_TYPE:                return "Invalid device type";
        case CL_INVALID_PLATFORM:                   return "Invalid platform";
        case CL_INVALID_DEVICE:                     return "Invalid device";
        case CL_INVALID_CONTEXT:                    return "Invalid context";
        case CL_INVALID_QUEUE_PROPERTIES:           return "Invalid queue properties";
        case CL_INVALID_COMMAND_QUEUE:              return "Invalid command queue";
        case CL_INVALID_HOST_PTR:                   return "Invalid host pointer";
        case CL_INVALID_MEM_OBJECT:                 return "Invalid memory object";
        case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:    return "Invalid image format descriptor";
        case CL_INVALID_IMAGE_SIZE:                 return "Invalid image size";
        case CL_INVALID_SAMPLER:                    return "Invalid sampler";
        case CL_INVALID_BINARY:                     return "Invalid binary";
        case CL_INVALID_BUILD_OPTIONS:              return "Invalid build options";
        case CL_INVALID_PROGRAM:                    return "Invalid program";
        case CL_INVALID_PROGRAM_EXECUTABLE:         return "Invalid program executable";
        case CL_INVALID_KERNEL_NAME:                return "Invalid kernel name";
        case CL_INVALID_KERNEL_DEFINITION:          return "Invalid kernel definition";
        case CL_INVALID_KERNEL:                     return "Invalid kernel";
        case CL_INVALID_ARG_INDEX:                  return "Invalid argument index";
        case CL_INVALID_ARG_VALUE:                  return "Invalid argument value";
        case CL_INVALID_ARG_SIZE:                   return "Invalid argument size";
        case CL_INVALID_KERNEL_ARGS:                return "Invalid kernel arguments";
        case CL_INVALID_WORK_DIMENSION:             return "Invalid work dimension";
        case CL_INVALID_WORK_GROUP_SIZE:            return "Invalid work group size";
        case CL_INVALID_WORK_ITEM_SIZE:             return "Invalid work item size";
        case CL_INVALID_GLOBAL_OFFSET:              return "Invalid global offset";
        case CL_INVALID_EVENT_WAIT_LIST:            return "Invalid event wait list";
        case CL_INVALID_EVENT:                      return "Invalid event";
        case CL_INVALID_OPERATION:                  return "Invalid operation";
        case CL_INVALID_GL_OBJECT:                  return "Invalid OpenGL object";
        case CL_INVALID_BUFFER_SIZE:                return "Invalid buffer size";
        case CL_INVALID_MIP_LEVEL:                  return "Invalid mip-map level";
        default: return "Unknown";
    }
}

int main(int argc , char ** argv)
{
	//parallel part
unsigned rows = ROWS;
unsigned columns = COLUMNS;
unsigned colRows = COLROWS;

	cl_int clerr;
cl_uint qty_platforms = 0;
cl_platform_id* platforms;
cl_uint ui;
cl_uint *qty_devices;
cl_device_id **devices;
cl_context cntx;
cl_event kEvent;
long long tStart = 0l;
long long tEnd = 0l;
	cl_command_queue cq;
	cl_device_id * ds; // devices
	cl_program p;
	cl_kernel k;
	char* program_source = "__kernel void matrix_multiply ("
	"unsigned firstHeight , unsigned firstWidth , __global float * firstElems ,"
	"unsigned secondHeight , unsigned secondWidth , __global float * secondElems ,"
	"unsigned resultHeight , unsigned resultWidth , __global float * resultElems)"
	"{"
	"float sum = 0;"
	"int i = get_global_id(0);/* >> 8;*/"
	"int j = get_global_id(1);/*(0) % 256;*/"
	"int k;"
	"for(k = 0; k < firstWidth; k += 1)"
	"{"
	"sum += firstElems[i * firstWidth + k] * secondElems[k * secondWidth + j];"
	"}"
	"resultElems[i * resultWidth + j] = sum;"
	"}"; // kernel source code
	
	size_t global_work_size [2] = {rows, columns};
	size_t local_work_size [2] = { BLOCK_SIZE , BLOCK_SIZE };
	size_t cb;
	matrix_t firstMatrix, secondMatrix, resultMatrix;
	int i = -1;
	int j;
	// ...
	
	clerr = clGetPlatformIDs(0, NULL, &qty_platforms);
	if(clerr != CL_SUCCESS){
		fprintf(stderr, "Error, code = %d.\n", clerr);
		return 1;
	}	
	platforms = (cl_platform_id*)malloc(sizeof(cl_platform_id)*qty_platforms);
	devices = (cl_device_id**)malloc(sizeof(cl_device_id*)*qty_platforms);
	qty_devices = (cl_uint*)malloc(sizeof(cl_uint)*qty_platforms);
	clerr = clGetPlatformIDs(qty_platforms, platforms, NULL);
	for (ui=0; ui < qty_platforms; ui++){
		clerr = clGetDeviceIDs(platforms[ui], CL_DEVICE_TYPE_ALL,0, NULL, &qty_devices[ui]);
		if(qty_devices[ui]){
			devices[ui] = (cl_device_id*)malloc(qty_devices[ui] * sizeof(cl_device_id));
			clerr = clGetDeviceIDs(platforms[ui], CL_DEVICE_TYPE_ALL, qty_devices[ui], devices[ui], NULL);
		}
	}
//	clerr = clGetDeviceInfo(*devices[0],CL_DEVICE_MAX_WORK_GROUP_SIZE,sizeof(cl_uint),&workGroupSize,NULL);
//	clerr = clGetDeviceInfo(*devices[0],CL_DEVICE_MAX_COMPUTE_UNITS,sizeof(cl_uint),&computeUnits,NULL);
//	clerr = clGetDeviceInfo(*devices[0],CL_DEVICE_MAX_WORK_ITEM_SIZES,3 * sizeof(cl_uint),&workItemSizes,NULL);
//	if(clerr)
//		errCodeToString(clerr);
	clerr = CL_SUCCESS;
	cntx = clCreateContext(0, qty_devices[0], devices[0], NULL, NULL, &clerr);
	cq = clCreateCommandQueue(cntx, devices[0][0], CL_QUEUE_PROFILING_ENABLE, &clerr);
	p = clCreateProgramWithSource(cntx, 1, (const char**)&program_source, NULL, &clerr);
	//	sprintf(clcompileflags, "-cl -mad enable");

	clerr = clBuildProgram(p, 0, NULL,  NULL /*clcompileflags*/, NULL, NULL);
	if (clerr == CL_BUILD_PROGRAM_FAILURE) {
		// Determine the size of the log
		size_t log_size;
		clGetProgramBuildInfo(p, devices[0][0], CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);

		// Allocate memory for the log
		char *log = (char *) malloc(log_size);

		// Get the log
		clGetProgramBuildInfo(p, devices[0][0], CL_PROGRAM_BUILD_LOG, log_size, log, NULL);

		// Print the log
		FILE* flog = fopen("errorLog.txt","w");
		fprintf(flog,"%s\n", log);
		fclose(flog);
		return 1;
	}

	k = clCreateKernel(p, "matrix_multiply", &clerr);
	
/*	
	cntx = clCreateContextFromType (NULL , CL_DEVICE_TYPE_GPU , NULL , NULL , NULL );
	clGetContextInfo (cntx , CL_CONTEXT_DEVICES , 0, NULL , &cb );
	ds = (cl_device_id *)malloc (cb );
	clGetContextInfo (cntx , CL_CONTEXT_DEVICES , cb , ds , NULL );
	cq = clCreateCommandQueue (cntx , ds [0] , 0, NULL );
	free(ds );

	p = clCreateProgramWithSource (cntx , 1, (const char**)&program_source , NULL , NULL );
	clBuildProgram (p, 0, NULL , NULL , NULL , NULL );
	k = clCreateKernel (p, " matrix_multiply ", NULL );
*/
	
	for(i = 0; i < rows; i++) {
		for(j = 0; j < rows; j++) {
			mem_F[i][j] = 0.0;
			mem_S[i][j] = 0.0;
		}
	}
	
	for(i = 0; i < rows; i++){
		mem_F[0][i] = 1;
		mem_S[i][0] = 1;
	}
	
	
	firstMatrix.height = rows; firstMatrix.width = colRows;
	firstMatrix.elems = clCreateBuffer (cntx , CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR ,
		firstMatrix.height * firstMatrix.width * sizeof ( cl_float ), mem_F , NULL );
	secondMatrix.height = colRows; secondMatrix.width = columns;
	secondMatrix.elems = clCreateBuffer (cntx , CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR ,
		secondMatrix.height * secondMatrix.width * sizeof ( cl_float ), mem_S , NULL );
	resultMatrix.height = rows; resultMatrix.width = columns;
	resultMatrix.elems = clCreateBuffer (cntx , CL_MEM_READ_WRITE , resultMatrix.height * resultMatrix.width * sizeof ( cl_float ), NULL , NULL );

	i=-1;
	clSetKernelArg (k, i += 1, sizeof ( unsigned ), &firstMatrix.height);
	clSetKernelArg (k, i += 1, sizeof ( unsigned ), &firstMatrix.width);
	clSetKernelArg (k, i += 1, sizeof ( cl_mem ), &firstMatrix.elems);
	clSetKernelArg (k, i += 1, sizeof ( unsigned ), &secondMatrix.height);
	clSetKernelArg (k, i += 1, sizeof ( unsigned ), &secondMatrix.width);
	clSetKernelArg (k, i += 1, sizeof ( cl_mem ), &secondMatrix.elems);
	clSetKernelArg (k, i += 1, sizeof ( unsigned ), &resultMatrix.height);
	clSetKernelArg (k, i += 1, sizeof ( unsigned ), &resultMatrix.width);
	clSetKernelArg (k, i += 1, sizeof ( cl_mem ), &resultMatrix.elems);

	kEvent = clCreateUserEvent(cntx, &clerr);
//check clerr need

	clerr = clEnqueueNDRangeKernel (cq , k, 2, NULL ,
		global_work_size , NULL /*local_work_size*/ , 0, NULL , &kEvent );
	if(clerr != 0)
		printf("\n%s\n", errCodeToString(clerr));
	clerr = clEnqueueReadBuffer (cq , resultMatrix.elems, CL_TRUE , 0,	resultMatrix.width * resultMatrix.height * sizeof ( cl_float ), mem_R , 0, NULL , NULL );

	clerr = clGetEventProfilingInfo(kEvent, CL_PROFILING_COMMAND_START, sizeof(tStart), &tStart, NULL);
	clerr = clGetEventProfilingInfo(kEvent, CL_PROFILING_COMMAND_END, sizeof(tEnd), &tEnd, NULL);
	printf("\nKernel time:%lld", tEnd - tStart);

	clReleaseMemObject (firstMatrix.elems);
	clReleaseMemObject (secondMatrix.elems);
	clReleaseMemObject (resultMatrix.elems);
	clReleaseKernel (k);
	clReleaseProgram (p);
	clReleaseCommandQueue (cq );
	clReleaseContext (cntx );

	// sequential part
	int l;
	for(i = 0; i < rows; i++)
		for(j = 0; j < columns; j++) {
			mem_Q[i][j] = 0;
			for(l = 0; l < colRows; l++)
				mem_Q[i][j] += mem_F[i][l] * mem_S[l][j];
		}
	// results comparison
	int ok = 1;
	int cnt = 0;
	float r_err;
	for(i = 0; i < rows; i++){
		for(j = 0; j < columns; j++){
			r_err = mem_R[i][j] -  mem_Q[i][j];
			if(r_err < 0)
			    r_err = - r_err;
			if(r_err > 0.0000001){
				ok = 0;
				printf("\n%d,%d: %f %f", i, j, mem_R[i][j], mem_Q[i][j]);
				if(cnt++ > 10)
				break;
			}
		}
		if(ok == 0)
		    break;
	}
	if(ok == 1)
		printf("\nOk\n");
	else
		printf("\nWow...\n");
	return 0;
}
