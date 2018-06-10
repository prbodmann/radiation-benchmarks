#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <string>
#include <omp.h>
#include <random>
#include <cuda_fp16.h>

#ifdef LOGS
#include "log_helper.h"
#endif
// The timestamp is updated on every log_helper function call.

// helper functions
#include "helper_string.h"
#include "helper_cuda.h"

#include "half.hpp"

#undef min
#define min( x, y ) ( (x) < (y) ? (x) : (y) )
#undef max
#define max( x, y ) ( (x) > (y) ? (x) : (y) )

#define BLOCK_SIZE 32

#define DEFAULT_INPUT_SIZE 8192

//=========== DEFINE TESTED TYPE
#if defined(test_type_double)
	#define GENERATOR_MAXABSVALUE 4.1e+16
	#define GENERATOR_MINABSVALUE 0
	const char test_type_description[] = "double";
	typedef double tested_type;
	typedef double tested_type_host;
#elif defined(test_type_single)
	#define GENERATOR_MAXABSVALUE 4.1e+2
	#define GENERATOR_MINABSVALUE 0
	const char test_type_description[] = "single";
	typedef float tested_type;
	typedef float tested_type_host;
#elif defined(test_type_half)
	#define GENERATOR_MAXABSVALUE 2.0
	#define GENERATOR_MINABSVALUE 0
	const char test_type_description[] = "half";
	typedef half tested_type;
	typedef half_float::half tested_type_host;
#else 
	#error TEST TYPE NOT DEFINED OR INCORRECT. USE TYPE=<double|single|half>.
#endif

//====================== benchmark+setup configuration
int generate = 0;
int verbose = 0;
int fault_injection = 0;

int k = 0; // k x k matrix size
int matrixSize = 0; // = k * k matrix size
int iterations = 100000000; // global loop iteracion
//=========================

//======== generator configuration
int generate_safechecks = 0;
bool generate_inputmatricesready = false;
bool host_check = false;
bool generator_debug = false;
//=========================

//================== Input paths
char *gold_matrix_path, *a_matrix_path, *b_matrix_path;

FILE* f_A;
FILE* f_B;
FILE* f_GOLD;
//====================================

//================== Host and device matrix ptr's
tested_type_host *A;
tested_type_host *B;
tested_type_host *C[3];
tested_type_host *GOLD;

tested_type *d_A[3];
tested_type *d_B[3];
tested_type *d_C[3];
//====================================

#define checkFrameworkErrors(error) __checkFrameworkErrors(error, __LINE__, __FILE__)

void __checkFrameworkErrors(cudaError_t error, int line, const char* file) {
	if (error == cudaSuccess) {
		return;
	}
	char errorDescription[250];
	snprintf(errorDescription, 250, "CUDA Framework error: %s. Bailing.",
			cudaGetErrorString(error));
#ifdef LOGS
	if (!generate)
		log_error_detail((char *)errorDescription); end_log_file();
#endif
	printf("%s - Line: %d at %s\n", errorDescription, line, file);
	exit (EXIT_FAILURE);
}

void GetDevice() {
//================== Retrieve and set the default CUDA device
	cudaDeviceProp prop;
	int count = 0;
	printf("Get device:");
	checkFrameworkErrors(cudaGetDeviceCount(&count));
	for (int i = 0; i < count; i++) {
		checkFrameworkErrors(cudaGetDeviceProperties(&prop, i));
		printf("Name: %s\n", prop.name);
	}
	int *ndevice;
	int dev = 0;
	ndevice = &dev;
	checkFrameworkErrors(cudaGetDevice(ndevice));

	checkFrameworkErrors(cudaSetDevice(0));
	checkFrameworkErrors(cudaGetDeviceProperties(&prop, 0));
	printf("\ndevice: %d %s\n", *ndevice, prop.name);
}

double mysecond() {
	struct timeval tp;
	struct timezone tzp;
	int i = gettimeofday(&tp, &tzp);
	return ((double) tp.tv_sec + (double) tp.tv_usec * 1.e-6);
}

void* safe_cudaMalloc(size_t size) {
	void* devicePtr;
	void* goldPtr;
	void* outputPtr;

	// First, alloc DEVICE proposed memory and HOST memory for device memory checking
	checkFrameworkErrors(cudaMalloc(&devicePtr, size));
	outputPtr = malloc(size);
	goldPtr = malloc(size);
	if ((outputPtr == NULL) || (goldPtr == NULL)) {
		log_error_detail((char *) "error host malloc");
		end_log_file();
		printf("error host malloc\n");
		exit (EXIT_FAILURE);
	}

	// ===> FIRST PHASE: CHECK SETTING BITS TO 10101010
	checkFrameworkErrors(cudaMemset(devicePtr, 0xAA, size));
	memset(goldPtr, 0xAA, size);

	checkFrameworkErrors(
			cudaMemcpy(outputPtr, devicePtr, size, cudaMemcpyDeviceToHost));
	if (memcmp(outputPtr, goldPtr, size)) {
		// Failed
		free(outputPtr);
		free(goldPtr);
		void* newDevicePtr = safe_cudaMalloc(size);
		checkFrameworkErrors(cudaFree(devicePtr));
		return newDevicePtr;
	}
	// ===> END FIRST PHASE

	// ===> SECOND PHASE: CHECK SETTING BITS TO 01010101
	checkFrameworkErrors(cudaMemset(devicePtr, 0x55, size));
	memset(goldPtr, 0x55, size);

	checkFrameworkErrors(
			cudaMemcpy(outputPtr, devicePtr, size, cudaMemcpyDeviceToHost));
	if (memcmp(outputPtr, goldPtr, size)) {
		// Failed
		free(outputPtr);
		free(goldPtr);
		void* newDevicePtr = safe_cudaMalloc(size);
		checkFrameworkErrors(cudaFree(devicePtr));
		return newDevicePtr;
	}
	// ===> END SECOND PHASE

	free(outputPtr);
	free(goldPtr);
	return devicePtr;
}

void allocCudaMemory() {
	d_A[0] = (tested_type*) safe_cudaMalloc(matrixSize * sizeof(tested_type));
	d_A[1] = (tested_type*) safe_cudaMalloc(matrixSize * sizeof(tested_type));
	d_A[2] = (tested_type*) safe_cudaMalloc(matrixSize * sizeof(tested_type));

	d_B[0] = (tested_type*) safe_cudaMalloc(matrixSize * sizeof(tested_type));
	d_B[1] = (tested_type*) safe_cudaMalloc(matrixSize * sizeof(tested_type));
	d_B[2] = (tested_type*) safe_cudaMalloc(matrixSize * sizeof(tested_type));

	d_C[0] = (tested_type*) safe_cudaMalloc(matrixSize * sizeof(tested_type));
	d_C[1] = (tested_type*) safe_cudaMalloc(matrixSize * sizeof(tested_type));
	d_C[2] = (tested_type*) safe_cudaMalloc(matrixSize * sizeof(tested_type));
}

void freeCudaMemory() {
	checkFrameworkErrors(cudaFree(d_A[0]));
	checkFrameworkErrors(cudaFree(d_A[1]));
	checkFrameworkErrors(cudaFree(d_A[2]));

	checkFrameworkErrors(cudaFree(d_B[0]));
	checkFrameworkErrors(cudaFree(d_B[1]));
	checkFrameworkErrors(cudaFree(d_B[2]));

	checkFrameworkErrors(cudaFree(d_C[0]));
	checkFrameworkErrors(cudaFree(d_C[1]));
	checkFrameworkErrors(cudaFree(d_C[2]));
}

void copyCudaMemory() {
	checkFrameworkErrors(cudaMemset(d_C[0], 0x00, matrixSize * sizeof(tested_type)));
	checkFrameworkErrors(cudaMemset(d_C[1], 0x00, matrixSize * sizeof(tested_type)));
	checkFrameworkErrors(cudaMemset(d_C[2], 0x00, matrixSize * sizeof(tested_type)));

	checkFrameworkErrors(
			cudaMemcpy(d_A[0], A, matrixSize * sizeof(tested_type),
					cudaMemcpyHostToDevice)); // PUSH A
	checkFrameworkErrors(
			cudaMemcpy(d_A[1], A, matrixSize * sizeof(tested_type),
					cudaMemcpyHostToDevice)); // PUSH A
	checkFrameworkErrors(
			cudaMemcpy(d_A[2], A, matrixSize * sizeof(tested_type),
					cudaMemcpyHostToDevice)); // PUSH A

	checkFrameworkErrors(
			cudaMemcpy(d_B[0], B, matrixSize * sizeof(tested_type),
					cudaMemcpyHostToDevice)); // PUSH B
	checkFrameworkErrors(
			cudaMemcpy(d_B[1], B, matrixSize * sizeof(tested_type),
					cudaMemcpyHostToDevice)); // PUSH B
	checkFrameworkErrors(
			cudaMemcpy(d_B[2], B, matrixSize * sizeof(tested_type),
					cudaMemcpyHostToDevice)); // PUSH B
}

void readMatricesFromFile() {
	int i;
	f_A = fopen(a_matrix_path, "rb");
	f_B = fopen(b_matrix_path, "rb");
	f_GOLD = fopen(gold_matrix_path, "rb");
	if (!(f_A && f_B && f_GOLD)) {
		printf("Cant open matrices.\n");
#ifdef LOGS
		if (!generate)
			log_error_detail((char *)"Cant open matrices"); end_log_file();
#endif
		exit(-3);
	}

	size_t ret_value[3];
	for (i = 0; i < k; i++) {
		ret_value[0] = fread(&(A[k * i]), sizeof(tested_type) * k, 1, f_A);
		ret_value[1] = fread(&(B[k * i]), sizeof(tested_type) * k, 1, f_B);
		ret_value[2] = fread(&(GOLD[k * i]), sizeof(tested_type) * k, 1, f_GOLD);
		if ((ret_value[0] != 1) || (ret_value[1] != 1) || (ret_value[2] != 1)) {
			printf("Bad input/gold formatting: %lu ; %lu ; %lu .\n",
					ret_value[0], ret_value[1], ret_value[2]);
#ifdef LOGS
			if (!generate)
				log_error_detail((char *)"Bad input/gold formatting."); end_log_file();
#endif
			exit(-3);
		}
	}

	fclose(f_A);
	fclose(f_B);
	fclose(f_GOLD);
}

void generateInputMatrices()
{
	FILE *f_A, *f_B;
	tested_type_host *h_A, *h_B;

	if (k==DEFAULT_INPUT_SIZE) {
		h_A = A;
		h_B = B;
	} else {
		h_A = (tested_type_host*) malloc(DEFAULT_INPUT_SIZE * DEFAULT_INPUT_SIZE * sizeof(tested_type));
		h_B = (tested_type_host*) malloc(DEFAULT_INPUT_SIZE * DEFAULT_INPUT_SIZE * sizeof(tested_type));
		if (!(h_A && h_B)) {
			printf("Could not alloc h_A or h_B");
			exit(EXIT_FAILURE);
		}
	}

	std::random_device rd; //Will be used to obtain a seed for the random number engine
	std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
	std::uniform_real_distribution<double> dis(-GENERATOR_MAXABSVALUE, GENERATOR_MAXABSVALUE);

	if (!generator_debug) {
		for (int i=0; i<DEFAULT_INPUT_SIZE; i++) {
			for (int j=0; j<DEFAULT_INPUT_SIZE; j++) {
				h_A[i * DEFAULT_INPUT_SIZE + j] = (tested_type_host)dis(gen);
				h_B[i * DEFAULT_INPUT_SIZE + j] = (tested_type_host)dis(gen);
			}
		}
	} else {
		for (int i=0; i<DEFAULT_INPUT_SIZE; i++) {
			for (int j=0; j<DEFAULT_INPUT_SIZE; j++) {
				h_A[i * DEFAULT_INPUT_SIZE + j] = (tested_type_host)2.0;
				h_B[i * DEFAULT_INPUT_SIZE + j] = (tested_type_host)2.0;
			}
		}
	}

	if (h_A != A) {
		memcpy(A, h_A, matrixSize * sizeof(tested_type));
		memcpy(B, h_B, matrixSize * sizeof(tested_type));
	} 

	int numZeros;
    int numNans;
    int numInfs;
// printf("Write\n");
	f_A = fopen(a_matrix_path, "wb");
	f_B = fopen(b_matrix_path, "wb");
	if (!(f_A&&f_B)) {
		printf("Could not open f_A or f_B\n");
		exit(EXIT_FAILURE);
	}

    tested_type_host val;

	numZeros = 0;
    numNans = 0;
    numInfs = 0;
	for (int i = 0; i<DEFAULT_INPUT_SIZE*DEFAULT_INPUT_SIZE; i++) {
        val=h_A[i];
		if (val == 0) numZeros++;
        if (isnan(val)) numNans++;
        if (isinf(val)) numInfs++;
	}
	printf("Number of zeros/NaNs/INFs on matrix A: %d/%d/%d\n", numZeros, numNans, numInfs);

	numZeros = 0;
    numNans = 0;
    numInfs = 0;
	for (int i = 0; i<DEFAULT_INPUT_SIZE*DEFAULT_INPUT_SIZE; i++) {
        val=h_B[i];
		if (val == 0) numZeros++;
        if (isnan(val)) numNans++;
        if (isinf(val)) numInfs++;
	}
	printf("Number of zeros/NaNs/INFs on matrix B: %d/%d/%d\n", numZeros, numNans, numInfs);

	for(int i=0; i<DEFAULT_INPUT_SIZE; i++)
	{
		fwrite(&(h_A[i * DEFAULT_INPUT_SIZE]), sizeof(tested_type) * DEFAULT_INPUT_SIZE, 1, f_A);
	}

	printf("Element 32 of matrix A: %f\n", (double)A[32]);

	printf("Element 50 of matrix B: %f\n", (double)B[50]);


	for(int i=0; i<DEFAULT_INPUT_SIZE; i++)
	{
		fwrite(&(h_B[i * DEFAULT_INPUT_SIZE]), sizeof(tested_type_host) * DEFAULT_INPUT_SIZE, 1, f_B);
	}
	printf("Done\n");

	fclose(f_A);
	fclose(f_B);
	if (h_A != A) {
		free(h_A);
		free(h_B);
	}
	return;
}

void retrieveInputMatrices() {
//================== Read inputs to HOST memory
	double time = mysecond();

	if (verbose)
		printf("Preparing input matrices... ");

	if (generate && !generate_inputmatricesready) {
		generateInputMatrices();
	} else {
		readMatricesFromFile();
	}

	if ((generate) && (generator_debug) && (k <= 16)) {
		printf("\nMatrix A: \n");
		for (int i = 0; i<k*k; i++) {
			printf(" %.2e", (float)A[i]);
			if ((i+1)%k == 0) printf("\n");
		}
		printf("\nMatrix B: \n");
		for (int i = 0; i<k*k; i++) {
			printf(" %.2e", (float)B[i]);
			if ((i+1)%k == 0) printf("\n");
		}
	}

	if (fault_injection) {
		A[3] = (tested_type_host) 6.5;
		printf("!! Injected 6.5 on position A[3]\n");
	}
	
	if (verbose)
		printf("Done reading matrices in %.2fs\n", mysecond() - time);
}

void writeGoldtoFile() {
	int i;
	f_GOLD = fopen(gold_matrix_path, "wb");
	if (!f_GOLD) {
		printf("Could not open f_GOLD\n");
		exit(EXIT_FAILURE);
	}

	for(i=0; i<k; i++)
	{
		fwrite( &(GOLD[i * k]), sizeof(double)*k, 1, f_GOLD );
	}

	fclose(f_GOLD);
}
__global__ void MatrixMulKernel(tested_type *d_A0, tested_type *d_A1, tested_type *d_A2,
		tested_type *d_B0, tested_type *d_B1, tested_type *d_B2, tested_type *d_C0, tested_type *d_C1,
		tested_type *d_C2, int n) {

#if defined(test_type_double) or defined(test_type_single)
	register int tx = blockIdx.x * BLOCK_SIZE + threadIdx.x;
	register int ty = blockIdx.y * BLOCK_SIZE + threadIdx.y;
	register int k;

	register size_t offset_A;
	register tested_type in_A;
	register tested_type in_A1_half2;

	register size_t offset_B;
	register tested_type in_B;
	register tested_type in_B1_half2;

	register tested_type acc = 0.0;
	for (k = 0; k < n; k++) {

		offset_A = ty * n + k;
		in_A = d_A0[offset_A];
		in_A1_half2 = d_A1[offset_A];
		if (in_A != in_A1_half2) {
			if (in_A != d_A2[offset_A]) {
				in_A = in_A1_half2;
			}
		}

		offset_B = k * n + tx;
		in_B = d_B0[offset_B];
		in_B1_half2 = d_B1[offset_B];
		if (in_B != in_B1_half2) {
			if (in_B != d_B2[offset_B]) {
				in_B = in_B1_half2;
			}
		}

		acc += in_A * in_B;
	}

	register size_t offset_C = ty * n + tx;

	d_C0[offset_C] = acc;
	d_C1[offset_C] = acc;
	d_C2[offset_C] = acc;

#elif defined(test_type_half)

	register int tx = (blockIdx.x * BLOCK_SIZE) / 2.0 + threadIdx.x;
	register int ty = blockIdx.y * BLOCK_SIZE + threadIdx.y;
	register int n2 = n / 2.0;
	register int k;

	register half2 *d_B0_half2 = (half2*)d_B0;
	register half2 *d_B1_half2 = (half2*)d_B1;
	register half2 *d_B2_half2 = (half2*)d_B2;

	register size_t offset_C;
	register half2 *d_C0_half2 = (half2*)d_C0;
	register half2 *d_C1_half2 = (half2*)d_C1;
	register half2 *d_C2_half2 = (half2*)d_C2;

	register size_t offset_A;
	register half2 in_A_half2;
	register half2 in_A1_half2;

	register size_t offset_B;
	register half2 in_B_half2;
	register half2 in_B1_half2;

	register half2 acc = __float2half2_rn(0.0);
	for (k = 0; k < n; k++) {

		// First half
		offset_A = ty * n + k;
		in_A_half2 = __half2half2(d_A0[offset_A]);
		in_A1_half2 = __half2half2(d_A1[offset_A]);
		if (__hbne2(in_A_half2, in_A1_half2)) { // __hbne2: true if both halves are equal
			if (__hbne2(in_A_half2, __half2half2(d_A2[offset_A]))) {
				in_A_half2 = in_A1_half2;
			}
		}
		offset_B = k * n2 + tx;
		in_B_half2 = d_B0_half2[offset_B];
		in_B1_half2 = d_B1_half2[offset_B];
		if (__hbne2(in_B_half2, in_B1_half2)) {
			if (__hbne2(in_B_half2, d_B2_half2[offset_B])) {
				in_B_half2 = in_B1_half2;
			}
		}

		acc = __hfma2(in_A_half2, in_B_half2, acc);
	}

	offset_C = ty * n2 + tx;

	d_C0_half2[offset_C] = acc;
	d_C1_half2[offset_C] = acc;
	d_C2_half2[offset_C] = acc;

	// int n2 = n / 2.0;
	// half2 *d_B2 = (half2*)d_B;
	// half2 *d_C_T2 = (half2*)d_C_T;
	
	// for (k = 0;  k < n; k++)
	// 	// c[ty * n + tx] += a[ty * n + k] *  b[k * n + tx];
	// 	// c[ty * n + tx + 1] += a[ty * n + k + 1] *  b[k * n + tx + 1];
	// 	d_C_T2[ty * n2 + tx] = __hfma2(__half2half2(d_A[ty * n + k]), d_B2[k * n2 + tx], d_C_T2[ty * n2 + tx]);
#endif

}

void usage(int argc, char* argv[]) {
	printf("Usage: %s -size=N [-generate] [-input_a=<path>] [-input_b=<path>] [-gold=<path>] [-iterations=N] [-verbose] [-no-warmup]\n", argv[0]);
}

// Returns true if no errors are found. False if otherwise.
// Set votedOutput pointer to retrieve the voted matrix
bool checkOutputErrors(tested_type_host* votedOutput = NULL, bool check = true) {
	int host_errors = 0;
	int memory_errors = 0;

#pragma omp parallel for shared(host_errors)
	for (int i = 0; i < matrixSize; i++) {
		register bool checkFlag = true;
		register tested_type_host valGold = GOLD[i];
		register tested_type_host valOutput0 = C[0][i];
		register tested_type_host valOutput1 = C[1][i];
		register tested_type_host valOutput2 = C[2][i];
		register tested_type_host valOutput = valOutput0;
		if ((valOutput0 != valOutput1) || (valOutput0 != valOutput2)) {
#pragma omp critical
			{
				char info_detail[150];
				snprintf(info_detail, 150,
						"m: [%d, %d], r0: %1.20e, r1: %1.20e, r2: %1.20e",
						(int) floor(i / k), i % k, 
						(double)valOutput0, (double)valOutput1,
						(double)valOutput2);
				if (verbose && (memory_errors < 10))
					printf("%s\n", info_detail);

#ifdef LOGS
				if (!generate) 
					log_info_detail(info_detail);
#endif
				memory_errors += 1;
			}
			if ((valOutput0 != valOutput1) && (valOutput1 != valOutput2) && (valOutput0 != valOutput2)) {
				// All 3 values diverge
				if (valOutput0 == valGold) {
					valOutput = valOutput0;
				} else if (valOutput1 == valGold) {
					valOutput = valOutput1;
				} else if (valOutput2 == valGold) {
					valOutput = valOutput2;
				} else {
					// NO VALUE MATCHES THE GOLD AND ALL 3 DIVERGE!
					checkFlag = false;
#pragma omp critical
					{
						char error_detail[150];
						snprintf(error_detail, 150,
								"f: [%d, %d], r0: %1.20e, r1: %1.20e, r2: %1.20e, e: %1.20e",
								(int) floor(i / k), i % k, (double)valOutput0,
								(double)valOutput1, (double)valOutput2, (double)valGold);
						if (verbose && (host_errors < 10))
							printf("%s\n", error_detail);

#ifdef LOGS
						if (!generate)
							log_error_detail(error_detail);
#endif
						host_errors++;
					}
				}
			} else if (valOutput1 == valOutput2) {
				// Only value 0 diverge
				valOutput = valOutput1;
			} else if (valOutput0 == valOutput2) {
				// Only value 1 diverge
				valOutput = valOutput0;
			} else if (valOutput0 == valOutput1) {
				// Only value 2 diverge
				valOutput = valOutput0;
			}
		}
		if (votedOutput != NULL) 
			votedOutput[i] = valOutput;
		// if ((fabs((tested_type_host)(valOutput-valGold)/valGold) > 1e-10)||(fabs((tested_type_host)(valOutput-valGold)/valGold) > 1e-10)) {
		if (check) {
			if (valGold != valOutput) {
				if (checkFlag) {
#pragma omp critical
					{
						char error_detail[150];
						snprintf(error_detail, 150,
								"p: [%d, %d], r: %1.20e, e: %1.20e",
								(int) floor(i / k), i % k, (double)valOutput, (double)valGold);
						if (verbose && (host_errors < 10))
							printf("%s\n", error_detail);
#ifdef LOGS
						if (!generate)
							log_error_detail(error_detail);
#endif
						host_errors++;
					}
				}
			}
		}
	}

	// printf("numErrors:%d", host_errors);

	if (host_errors != 0) {
		printf("#");
#ifdef LOGS
		if (!generate)
			log_error_count(host_errors);
#endif
		//================== Release device memory to ensure there is no corrupted data on the inputs of the next iteration
		freeCudaMemory();
		//====================================
		retrieveInputMatrices();
		//================== Init DEVICE memory
		allocCudaMemory();
		copyCudaMemory();
		//====================================
	}
	return host_errors == 0;
}

int main(int argc, char* argv[]) {
//================== Test vars
	int loop2;
	// int kernel_errors=0;
	// int zero = 0;
	double time;
	double kernel_time, global_time;
	double total_kernel_time, min_kernel_time, max_kernel_time;
	int device_warmup = 1;
	// int gpu_check = 1;
//====================================

//================== Read test parameters
	if (argc < 2) {
		usage(argc, argv);
		exit(-1);
	}

	if (checkCmdLineFlag(argc, (const char **) argv, "size")) {
		k = getCmdLineArgumentInt(argc, (const char **) argv, "size");

		if ((k <= 0) || (k % 16 != 0)) {
			printf("Invalid input size given on the command-line: %d\n", k);
			exit (EXIT_FAILURE);
		}
		matrixSize = k * k;
	} else {
		usage(argc, argv);
		exit (EXIT_FAILURE);
	}

	if (checkCmdLineFlag(argc, (const char **) argv, "input_a")) {
		getCmdLineArgumentString(argc, (const char **) argv, "input_a",
				&a_matrix_path);
	} else {
		a_matrix_path = new char[100];
		snprintf(a_matrix_path, 100, "mxm_a_%s_%i.matrix",
				test_type_description, (signed int) DEFAULT_INPUT_SIZE);
		printf("Using default input_a path: %s\n", a_matrix_path);
	}

	if (checkCmdLineFlag(argc, (const char **) argv, "input_b")) {
		getCmdLineArgumentString(argc, (const char **) argv, "input_b",
				&b_matrix_path);
	} else {
		b_matrix_path = new char[100];
		snprintf(b_matrix_path, 100, "mxm_b_%s_%i.matrix",
				test_type_description, (signed int) DEFAULT_INPUT_SIZE);
		printf("Using default input_a path: %s\n", b_matrix_path);
	}

	if (checkCmdLineFlag(argc, (const char **) argv, "gold")) {
		getCmdLineArgumentString(argc, (const char **) argv, "gold",
				&gold_matrix_path);
	} else {
		gold_matrix_path = new char[100];
		snprintf(gold_matrix_path, 100, "mxm_gold_%s_%i.matrix", test_type_description, (signed int) k);
		printf("Using default gold path: %s\n", gold_matrix_path);
	}

	if (checkCmdLineFlag(argc, (const char **) argv, "iterations")) {
		iterations = getCmdLineArgumentInt(argc, (const char **) argv,
				"iterations");
	}

	if (checkCmdLineFlag(argc, (const char **) argv, "verbose")) {
		verbose = 1;
	}

	if (checkCmdLineFlag(argc, (const char **) argv, "debug")) {
		fault_injection = 1;
		printf("!! Will be injected an input error\n");
	}

	if (checkCmdLineFlag(argc, (const char **) argv, "no-warmup")) {
		device_warmup = 0;
		printf("!! The first iteration may not reflect real timing information\n");
	}

	if (checkCmdLineFlag(argc, (const char **) argv, "generate")) {
		generate = 1;
		device_warmup = 0;
		fault_injection = 0;
		iterations = 20;
		generate_safechecks = 5;
		printf("!! Generate !! Disabling device_warmup, fault_injection and iterations limiting.\n");
		printf("!! Generate parameters: generate_safechecks: %d / \n", generate_safechecks);
	}

	if (checkCmdLineFlag(argc, (const char **) argv, "generator_debug")) {
		if (generate) {
			generator_debug = true;
		} else {
			printf("!! generator_debug ignored: generate is not activated. active with -generate.\n");
		}
	}
//====================================

//================== Set block and grid size for MxM kernel
#if defined(test_type_double) or defined(test_type_single)
	int gridsize = k / BLOCK_SIZE < 1 ? 1 : k / BLOCK_SIZE;
	int blocksize = k / BLOCK_SIZE < 1 ? k : BLOCK_SIZE;
	dim3 dimBlock(blocksize, blocksize);
	dim3 dimGrid(gridsize, gridsize);
#elif defined(test_type_half)
	int gridsize = k / BLOCK_SIZE < 1 ? 1 : k / BLOCK_SIZE;
	int blocksize = k / BLOCK_SIZE < 1 ? k : BLOCK_SIZE;
	dim3 dimBlock(blocksize / 2.0, blocksize);
	dim3 dimGrid(gridsize, gridsize);
#endif
//====================================

//================== Init logs
#ifdef LOGS
	if (!generate) {
		char test_info[90];
		char test_name[90];
		snprintf(test_info, 90, "size:%d type:%s-precision-triplicated", k, test_type_description);
		snprintf(test_name, 90, "cuda_trip_%s_mxm", test_type_description);
		start_log_file(test_name, test_info);
	}
#endif
//====================================

//================== Alloc HOST memory
	A = (tested_type_host*) malloc(matrixSize * sizeof(tested_type));
	B = (tested_type_host*) malloc(matrixSize * sizeof(tested_type));
	C[0] = (tested_type_host*) malloc(matrixSize * sizeof(tested_type));
	C[1] = (tested_type_host*) malloc(matrixSize * sizeof(tested_type));
	C[2] = (tested_type_host*) malloc(matrixSize * sizeof(tested_type));

	GOLD = (tested_type_host*) malloc(matrixSize * sizeof(tested_type));

	if (!(A && B && C[0] && C[1] && C[2] && GOLD)) {
		printf("Failed on host malloc.\n");
		exit(-3);
	}
//====================================

//================== Init test environment
	// kernel_errors=0;
	total_kernel_time = 0;
	min_kernel_time = UINT_MAX;
	max_kernel_time = 0;
	GetDevice();
	retrieveInputMatrices();
	printf("cuda_trip_mxm\n");
	fflush (stdout);
//====================================

//================== Init generator if enabled
	int generate_safechecks_count = 0;
//====================================

//================== Init DEVICE memory
	allocCudaMemory();
	copyCudaMemory();
//====================================

	for (loop2 = 0; loop2 < iterations; loop2++) {
		//================== Global test loop

		if (!loop2 && device_warmup)
			printf("First iteration: device warmup. Please wait...\n");

		global_time = mysecond();

		checkFrameworkErrors(
				cudaMemset(d_C[0], 0, matrixSize * sizeof(tested_type)));
		checkFrameworkErrors(
				cudaMemset(d_C[1], 0, matrixSize * sizeof(tested_type)));
		checkFrameworkErrors(
				cudaMemset(d_C[2], 0, matrixSize * sizeof(tested_type)));

		if (verbose)
			printf(",");

		kernel_time = mysecond();
#ifdef LOGS
		if (!generate)
			if (loop2 || !device_warmup)
				start_iteration();
#endif
		//================== Device computation, MxM
		MatrixMulKernel<<<dimGrid, dimBlock>>>(d_A[0], d_A[1], d_A[2], d_B[0],
				d_B[1], d_B[2], d_C[0], d_C[1], d_C[2], k);

		checkFrameworkErrors(cudaPeekAtLastError());

		checkFrameworkErrors(cudaDeviceSynchronize());
		checkFrameworkErrors(cudaPeekAtLastError());
		//====================================
#ifdef LOGS
		if (!generate) 
			if (loop2 || !device_warmup)
				end_iteration();
#endif
		kernel_time = mysecond() - kernel_time;

		if (loop2 || !device_warmup) {
			total_kernel_time += kernel_time;
			min_kernel_time = min(min_kernel_time, kernel_time);
			max_kernel_time = max(max_kernel_time, kernel_time);
		}

		if (loop2 || !device_warmup)
			if (verbose)
				printf("Device kernel time for iteration %d: %.3fs\n", loop2,
						kernel_time);

		//================== Gold check
		if (verbose)
			printf(",");

		time = mysecond();

		if (loop2 || !device_warmup) {
			checkFrameworkErrors(
					cudaMemcpy(C[0], d_C[0], matrixSize * sizeof(tested_type),
							cudaMemcpyDeviceToHost));
			if ((generate) && (k <= 16)) {
				printf("\nMatrix C (0): \n");
				for (int i = 0; i<k*k; i++) {
					printf(" %.2e", (float)C[0][i]);
					if ((i+1)%k == 0) printf("\n");
				}
				printf("\n");
			}

			checkFrameworkErrors(
					cudaMemcpy(C[1], d_C[1], matrixSize * sizeof(tested_type),
							cudaMemcpyDeviceToHost));
			if ((generate) && (k <= 16)) {
				printf("\nMatrix C (1): \n");
				for (int i = 0; i<k*k; i++) {
					printf(" %.2e", (float)C[1][i]);
					if ((i+1)%k == 0) printf("\n");
				}
				printf("\n");
			}

			checkFrameworkErrors(
					cudaMemcpy(C[2], d_C[2], matrixSize * sizeof(tested_type),
							cudaMemcpyDeviceToHost));
			if ((generate) && (k <= 16)) {
				printf("\nMatrix C (2): \n");
				for (int i = 0; i<k*k; i++) {
					printf(" %.2e", (float)C[2][i]);
					if ((i+1)%k == 0) printf("\n");
				}
				printf("\n");
			}
			if (generate) {
				if (generate_safechecks_count == 0) {
					printf("Generate: First generation. Step %d/%d of max. %d \n", generate_safechecks_count, generate_safechecks, iterations);
					checkOutputErrors(GOLD, false); // This will copy the voted matrix to gold
					generate_safechecks_count++;
					if ((generate) && (k <= 16)) {
						printf("\nMatrix GOLD (VOTED): \n");
						for (int i = 0; i<k*k; i++) {
							printf(" %.2e", (float)GOLD[i]);
							if ((i+1)%k == 0) printf("\n");
						}
						printf("\n");
					}
				} else {
					if (!checkOutputErrors()) {
						printf("Generate: Failed on compare. Step %d/%d of max. %d \n", generate_safechecks_count, generate_safechecks, iterations);
						generate_safechecks_count = 0;
					} else {
						printf("Generate: Success on compare. Step %d/%d of max. %d\n", generate_safechecks_count, generate_safechecks, iterations);generate_safechecks_count++;
						if (generate_safechecks_count >= generate_safechecks) {
							writeGoldtoFile();
							loop2 = iterations; // This will make the loop end
						}
					}
				}
			} else {
				checkOutputErrors();
			}
		}
		//====================================

		//================== Console hearthbeat
		printf(".");
		fflush(stdout);
		//====================================

		if (loop2 || !device_warmup)
			if (verbose)
				printf("Gold check time for iteration %d: %.3fs\n", loop2,
						mysecond() - time);

		if (loop2 || !device_warmup)
			if (verbose) {
				/////////// PERF
				double flops = 2.0 * (double) k * k * k;
				double gflops = flops / kernel_time;
				double outputpersec = (double) matrixSize / kernel_time;
				printf("SIZE:%d OUTPUT/S:%f FLOPS:%f (GFLOPS:%.2f)\n", k,
						outputpersec, gflops, gflops / 1000000000);
				///////////
			}

		if (loop2 || !device_warmup)
			if (verbose)
				printf("Iteration #%d time: %.3fs\n\n\n", loop2,
						mysecond() - global_time);
		fflush(stdout);
	}

	double gflops = 2.0 * (double) k * k * k / 1000000000; // Bilion FLoating-point OPerationS
	double averageKernelTime = total_kernel_time
			/ (iterations - (device_warmup ? 1 : 0));
	printf("\n-- END --\n"
			"Total kernel time: %.3fs\n"
			"Iterations: %d\n"
			"Average kernel time: %.3fs (best: %.3fs ; worst: %.3fs)\n"
			"Average GFLOPs: %.2f (best: %.2f ; worst: %.2f)\n",
			total_kernel_time, iterations, averageKernelTime, min_kernel_time,
			max_kernel_time, gflops / averageKernelTime,
			gflops / min_kernel_time, gflops / max_kernel_time);

	//================== Release device memory
	freeCudaMemory();
	//====================================

	free(A);
	free(B);
	free(C[0]);
	free(C[1]);
	free(C[2]);
	free(GOLD);
#ifdef LOGS
	if (!generate) 
		end_log_file();
#endif

	return 0;
}