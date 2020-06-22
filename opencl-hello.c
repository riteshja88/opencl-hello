/* Simple Hello World for OpenCL, written in C.
 * For real code, check for errors. The error code is stored in all calls here,
 * but no checking is done, which is certainly bad. It'll likely simply crash
 * right after a failing call.
 *
 * On GNU/Linux with nVidia OpenCL, program builds with -lOpenCL.
 * Not sure about other platforms.
 */

#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <CL/cl.h>

void rot13(char *buf,int len)
{
    int index=0;
    char c = buf[index];
    while (index < len) {
        if (c < 'A' || c > 'z' || (c > 'Z' && c < 'a')) {
            buf[index] = buf[index];
        } else {
            if (c > 'm' || (c > 'M' && c < 'a')) {
				buf[index] = buf[index] - 13;
			} else {
				buf[index] = buf[index] + 13;
			}
        }
		c = buf[++index];
    }
}

int main(int argc, char *argv[]) {
	size_t worksize = atol(argv[1]);

	/* Host CPU */
	char buf[worksize];
	//strcpy(buf,"hello world");
	struct timespec start, end; 
    // start timer. 
	clock_gettime(CLOCK_MONOTONIC, &start);
	rot13(buf, worksize);	// scramble using the host CPU
	clock_gettime(CLOCK_MONOTONIC, &end);
	double time_taken;
	time_taken = (end.tv_sec - start.tv_sec) * 1e9; 
    time_taken = (time_taken + (end.tv_nsec - start.tv_nsec)) * 1e-9;
	printf("cpu = %.9f\n", time_taken);
	//puts(buf);	// Just to demonstrate the plaintext is destroyed/rot13'd

	cl_int rc;
	cl_platform_id platform;
	cl_uint platforms;

	/* GPU discovery */
	// Fetch the Platform and Device IDs; we only want one.
	rc = clGetPlatformIDs(1, &platform, &platforms);
	if(CL_SUCCESS != rc) {
		printf("clGetPlatformIDs() returned %d.\n", rc);
		exit(1);
	}

	cl_device_id device;
	cl_uint devices;
	rc = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, &devices);
	if(CL_SUCCESS != rc) {
		printf("clGetDeviceIDs() returned %d.\n", rc);
		exit(1);
	}

	cl_context_properties properties[] = {
		CL_CONTEXT_PLATFORM,
		(cl_context_properties)platform,
		0
	};
	// Note that nVidia's OpenCL requires the platform property
	cl_context context = clCreateContext(properties, 1, &device, NULL, NULL, &rc);
	if(CL_SUCCESS != rc) {
		printf("clCreateContext() returned %d.\n", rc);
		exit(1);
	}

	cl_command_queue cq = clCreateCommandQueue(context, device, 0, &rc);
	if(CL_SUCCESS != rc) {
		printf("clCreateCommandQueue() returned %d.\n", rc);
		exit(1);
	}

	/* JIT compile */
	int rot13_cl_fd = open("rot13.cl", O_RDONLY);
	if(rot13_cl_fd < 0) {
		printf("open() failed.\n");
		exit(1);
	}
	int rot13_cl_len = lseek(rot13_cl_fd, 0, SEEK_END);
	if(rot13_cl_len < 0) {
		printf("lseek() failed.\n");
		exit(1);
	}
	const char *rot13_cl_contents = mmap(0, rot13_cl_len, PROT_READ, MAP_PRIVATE, rot13_cl_fd, 0);
	if(MAP_FAILED == rot13_cl_contents) {
		printf("mmap() failed.\n");
		exit(1);
	}

	const char *src = rot13_cl_contents;
	size_t srcsize = strlen(rot13_cl_contents);

	const char *srcptr[]={
		src,
	};
	// Submit the source code of the rot13 kernel to OpenCL
	cl_program prog = clCreateProgramWithSource(context,
												1,
												srcptr,
												&srcsize,
												&rc);
	if(CL_SUCCESS != rc) {
		printf("clCreateProgramWithSource() returned %d.\n", rc);
		exit(1);
	}

	// and compile it (after this we could extract the compiled version)
	rc = clBuildProgram(prog, 0, NULL, "", NULL, NULL);
	if(CL_SUCCESS != rc) {
		printf("clBuildProgram() returned %d.\n", rc);
		exit(1);
	}

	/* Prepare Memory for GPU */
	// Allocate memory for the kernel to work with
	cl_mem mem1, mem2;
	mem1 = clCreateBuffer(context, CL_MEM_READ_ONLY, worksize, NULL, &rc); /* kernel input */
	if(CL_SUCCESS != rc) {
		printf("clCreateBuffer() returned %d.\n", rc);
		exit(1);
	}
	mem2 = clCreateBuffer(context, CL_MEM_WRITE_ONLY, worksize, NULL, &rc); /* kernel output */
	if(CL_SUCCESS != rc) {
		printf("clCreateBuffer() returned %d.\n", rc);
		exit(1);
	}

	// get a handle and map parameters for the kernel
	cl_kernel k_rot13 = clCreateKernel(prog, "rot13", &rc);
	if(CL_SUCCESS != rc) {
		printf("clCreateKernel() returned %d.\n", rc);
		exit(1);
	}
	clSetKernelArg(k_rot13, 0, sizeof(mem1), &mem1);
	clSetKernelArg(k_rot13, 1, sizeof(mem2), &mem2);

	// Target buffer just so we show we got the data from OpenCL
	char buf2[worksize];
	buf2[0]='a';
	buf2[worksize]=0;

	clock_gettime(CLOCK_MONOTONIC, &start);
	#if 0
	// Send input data to OpenCL (async, don't alter the buffer!)
	rc = clEnqueueWriteBuffer(cq, mem1, CL_FALSE, 0, worksize, buf, 0, NULL, NULL);
	if(CL_SUCCESS != rc) {
		printf("clEnqueueWriteBuffer() returned %d.\n", rc);
		exit(1);
	}
	#endif
	// Perform the operation
	rc = clEnqueueNDRangeKernel(cq, k_rot13, 1, NULL, &worksize, &worksize, 0, NULL, NULL);
	if(CL_SUCCESS != rc) {
		printf("clEnqueueNDRangeKernel() returned %d.\n", rc);
		exit(1);
	}
	#if 0
	// Read the result back into buf2
	rc = clEnqueueReadBuffer(cq, mem2, CL_FALSE, 0, worksize, buf2, 0, NULL, NULL);
	if(CL_SUCCESS != rc) {
		printf("clEnqueueReadBuffer() returned %d.\n", rc);
		exit(1);
	}
	#endif
	// Await completion of all the above
	rc = clFinish(cq);
	if(CL_SUCCESS != rc) {
		printf("clFinish() returned %d.\n", rc);
		exit(1);
	}

	clock_gettime(CLOCK_MONOTONIC, &end);

	time_taken = (end.tv_sec - start.tv_sec) * 1e9; 
	time_taken = (time_taken + (end.tv_nsec - start.tv_nsec)) * 1e-9;
	printf("gpu = %.9f\n", time_taken);	

	// Finally, output out happy message.
	//puts(buf2);

	return 0;
}
