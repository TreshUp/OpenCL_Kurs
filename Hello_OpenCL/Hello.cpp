#include <stdio.h>
#include <stdlib.h>
#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif
#define VECTOR_SIZE 1024
#define MAX_SOURCE_SIZE (0x100000)
int main(void)
{
	//Это все для чтения кернела из файла Hello.cl
	FILE *fp;
	const char fileName[] = "../Hello_OpenCL/Hello.cl";
	size_t source_size;
	char *source_str;
	int i;

	try {
		fp = fopen(fileName, "r");
		if (!fp) {
			fprintf(stderr, "Failed to load kernel.\n");
			exit(1);
		}
		source_str = (char *)malloc(MAX_SOURCE_SIZE);
		source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);//считывает массив размером MAX_SOURCE_SIZE, каждый размеров  1 // возвращает число успешно считанных элементов
		fclose(fp);
	}
	catch (int a) {
		printf("%f", a);
	}

	// Allocate space for vectors A, B and C
	float alpha = 2.0;
	float *A = (float*)malloc(sizeof(float)*VECTOR_SIZE);
	float *B = (float*)malloc(sizeof(float)*VECTOR_SIZE);
	float *C = (float*)malloc(sizeof(float)*VECTOR_SIZE);
	for (i = 0; i < VECTOR_SIZE; i++)
	{
		A[i] = i;
		B[i] = VECTOR_SIZE - i;
		C[i] = 1;
	}
	// Get platform and device information
	cl_platform_id * platforms = NULL; //массив найденных платформ
	cl_uint num_platforms; //кол-во платформ

	//Set up the Platform
	cl_int clStatus = clGetPlatformIDs(0, NULL, &num_platforms); //первый параметр - число платформ на "add"; на выходе в num_platforms записано число платформ
	platforms = (cl_platform_id *)malloc(sizeof(cl_platform_id)*num_platforms); //динамическое выделение памяти
	clStatus = clGetPlatformIDs(num_platforms, platforms, NULL); //получение списка доступных платформ; на выходе в массиве содер. все платформы

	//Get the devices list and choose the device you want to run on
	cl_device_id *device_list = NULL; //массив найденных устройств
	cl_uint num_devices; //кол-во устройств
	clStatus = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, 0,NULL, &num_devices); //первый параметр - какая платформа; второй - тип; третий - число устройств на добавление; на выходе в num_devices записано число устройств
	device_list = (cl_device_id *)malloc(sizeof(cl_device_id)*num_devices);//динамическое выделение памяти
	clStatus = clGetDeviceIDs(platforms[0],CL_DEVICE_TYPE_GPU, num_devices, device_list, NULL); //получение списка доступных устройств; на выходе в массиве содер. все устройства

	// Create one OpenCL context for each device in the platform
	cl_context context;
	context = clCreateContext(NULL, num_devices, device_list,NULL, NULL, &clStatus); //создание контекста

	// Create a command queue
	cl_command_queue command_queue = clCreateCommandQueue(context, device_list[0], 0, &clStatus); // создание очереди из команд для устройства;properties - список свойств очереди команд

	// Create memory buffers on the device for each vector
	//выделение памяти для каждого из векторов
	cl_mem A_clmem = clCreateBuffer(context, CL_MEM_READ_ONLY,VECTOR_SIZE * sizeof(float), NULL, &clStatus);
	cl_mem B_clmem = clCreateBuffer(context, CL_MEM_READ_ONLY,VECTOR_SIZE * sizeof(float), NULL, &clStatus);
	cl_mem C_clmem = clCreateBuffer(context, CL_MEM_WRITE_ONLY,VECTOR_SIZE * sizeof(float), NULL, &clStatus);

	// Copy the Buffer A and B to the device
	//копирование векторов А и В из памяти хоста в буфер; CL_TRUE - вектор А мб использован и после вызова функции clEnqueueWriteBuffer
	clStatus = clEnqueueWriteBuffer(command_queue, A_clmem,	CL_TRUE, 0, VECTOR_SIZE * sizeof(float),A, 0, NULL, NULL);
	clStatus = clEnqueueWriteBuffer(command_queue, B_clmem,	CL_TRUE, 0, VECTOR_SIZE * sizeof(float),B, 0, NULL, NULL);

	// Create a program from the kernel source
	//создание программы, используя контекст, код функции-вычисления
	cl_program program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &clStatus);

	// Build the program
	//компилирование и линковка программы
	//первый параметр - программа; второй -число устройств; третий - список устройств; пятый - If pfn_notify is NULL, clBuildProgram does not return until the build has completed.
	clStatus = clBuildProgram(program, 1, device_list, NULL,NULL, NULL);

	// Create the OpenCL kernel
	//второй параметр - название функции, объявленной в программе как __kernel; 
	cl_kernel kernel = clCreateKernel(program, "VectorAdd",	&clStatus);

	// Set the arguments of the kernel
	clStatus = clSetKernelArg(kernel, 0, sizeof(float),	(void *)&alpha);
	clStatus = clSetKernelArg(kernel, 1, sizeof(cl_mem),(void *)&A_clmem);
	clStatus = clSetKernelArg(kernel, 2, sizeof(cl_mem),(void *)&B_clmem);
	clStatus = clSetKernelArg(kernel, 3, sizeof(cl_mem),(void *)&C_clmem);

	// Execute the OpenCL kernel on the list
	size_t global_size = VECTOR_SIZE; // Process the entire lists
	size_t local_size = 64; // Process one item at a time
	//!!
	//выполнение кода кернела; пятый - число work-items in work-groups;
	clStatus = clEnqueueNDRangeKernel(command_queue, kernel, 1,	NULL, &global_size, &local_size, 0, NULL, NULL);

	// Read the cl memory C_clmem on device to the host variable C
	//копирование информации из буфера в хостовый вектор с
	clStatus = clEnqueueReadBuffer(command_queue, C_clmem,CL_TRUE, 0, VECTOR_SIZE * sizeof(float), C, 0, NULL, NULL);

	// Clean up and wait for all the comands to complete.
	//все команды добавлены в очередь
	clStatus = clFlush(command_queue);
	//выполнение всех оставшихся команд
	clStatus = clFinish(command_queue);

	// Display the result to the screen
	for (i = 0; i < VECTOR_SIZE; i++)
		printf("%f * %f + %f = %f\n", alpha, A[i], B[i], C[i]);

	// Finally release all OpenCL allocated objects and	host buffers.
	//Освобождение всех объектов и буферов
	clStatus = clReleaseKernel(kernel);
	clStatus = clReleaseProgram(program);
	clStatus = clReleaseMemObject(A_clmem);
	clStatus = clReleaseMemObject(B_clmem);
	clStatus = clReleaseMemObject(C_clmem);
	clStatus = clReleaseCommandQueue(command_queue);
	clStatus = clReleaseContext(context);
	free(A);
	free(B);
	free(C);
	free(platforms);
	free(device_list);
	system("pause");
	return 0;
}