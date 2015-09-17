/**
 * Copyright 1993-2015 NVIDIA Corporation.  All rights reserved.
 *
 * Please refer to the NVIDIA end user license agreement (EULA) associated
 * with this source code for terms and conditions that govern your use of
 * this software. Any use, reproduction, disclosure, or distribution of
 * this software and related documentation outside the terms of the EULA
 * is strictly prohibited.
 *
 */

/* This sample illustrates the usage of Cuda Dynamic Parallelism (CDP)
 */

// includes, system
#include <stdio.h>
// includes, cublas
#include <cublas_v2.h>
// helper functions
#include "helper_string.h"
#include "helper_cuda.h"
// includes, project
#include "cdp_lu.h"
#include "cdp_lu_utils.h"

#include <sys/time.h>
#include <omp.h>

#define WILL_REUSE true
#define WILL_NOT_REUSE false

double mysecond()
{
   struct timeval tp;
   struct timezone tzp;
   int i = gettimeofday(&tp,&tzp);
   return ( (double) tp.tv_sec + (double) tp.tv_usec * 1.e-6 );
}

void memsetup(Parameters &host_params)
{
    srand(host_params.seed);

    // Initialise with the base params, and do any necessary randomisation
    unsigned long long lda_ull = (unsigned long long) host_params.lda;
    host_params.flop_count += lda_ull*lda_ull*lda_ull * 2ull / 3ull;

    host_params.data_size = sizeof(double);
    host_params.data_len = host_params.n * host_params.lda;
    host_params.piv_len = MIN(host_params.m, host_params.n);

    size_t len = host_params.data_len * host_params.data_size;
    size_t piv_len = host_params.piv_len * sizeof(int);

    // Allocate memories
    if (!(host_params.hostmem_ready)) host_params.host_A = (double *) malloc(len);
    host_params.host_LU = (double *) malloc(len);
    host_params.host_piv = (int *) malloc(piv_len);
    if (!(host_params.hostmem_ready)) host_params.gold_LU = (double *) malloc(len);
    if (!(host_params.hostmem_ready)) host_params.gold_piv = (int *) malloc(piv_len);

    checkCudaErrors(cudaMalloc((void **)&host_params.device_A, len));
    checkCudaErrors(cudaMalloc((void **)&host_params.device_LU, len));
    checkCudaErrors(cudaMalloc((void **)&host_params.device_piv, piv_len));
    checkCudaErrors(cudaMalloc((void **)&host_params.device_info, sizeof(int)));

    if (!(host_params.hostmem_ready))
    {
      printf("Preparing host memory...\n");
      FILE *finput, *fgold;
      if (host_params.generate == 1)
      {
        printf("Generating input...");
        // Initialise source with random (seeded) data
        // srand(params[b].seed);
        double *ptr = host_params.host_A;

        for (int i=0; i<host_params.data_len; i++)
            ptr[i] = (double) rand() / 32768.0;

        if (finput = fopen(host_params.inputName, "wb"))
        {
          fwrite(host_params.host_A, len, 1, finput);
          fclose(finput);
          printf("Done.\n");
        }
        else
        {
          printf("\nWarning: Could not open input file in wb mode. %s\n", host_params.inputName);
        }
      }
      else
      {
        if (!(finput = fopen(host_params.inputName, "rb")))
        {
          printf("Error: Could not open input file in rb mode. %s\n", host_params.inputName);
          exit(EXIT_FAILURE);
        }
        else
        {
          fread(host_params.host_A, len, 1, finput);
          fclose(finput);
        }
        if (host_params.generate == 0)
        {
          if (!(fgold = fopen(host_params.goldName, "rb")))
          {
            printf("Error: Could not open gold file in rb mode. %s\n", host_params.goldName);
            exit(EXIT_FAILURE);
          }
          else
          {
            fread(host_params.gold_LU, len, 1, fgold);
            fread(host_params.gold_piv, piv_len, 1, fgold);
            fclose(fgold);
          }
        }
      }
      printf("Done.\n");
    }
    host_params.hostmem_ready = true;

    memset(host_params.host_piv, 0, piv_len);
    host_params.host_info = 0;

    memcpy(host_params.host_LU, host_params.host_A, len);   // copy reference data

    // Now upload it to the GPU. TODO: copy A to LU on the device..
    checkCudaErrors(cudaMemcpy(host_params.device_A, host_params.host_A, len, cudaMemcpyHostToDevice));
    checkCudaErrors(cudaMemcpy(host_params.device_LU, host_params.host_LU, len, cudaMemcpyHostToDevice));
    checkCudaErrors(cudaMemcpy(host_params.device_piv, host_params.host_piv, piv_len, cudaMemcpyHostToDevice));
    checkCudaErrors(cudaMemcpy(host_params.device_info, &host_params.device_info, sizeof(int), cudaMemcpyHostToDevice));
}

void finalize(Parameters &host_params, bool reuse)
{
    if (reuse==WILL_NOT_REUSE)
    {
      free(host_params.host_A);
      free(host_params.gold_LU);
      free(host_params.gold_piv);

      host_params.hostmem_ready = 0;
    }
    else
    {
      free(host_params.host_LU);
      free(host_params.host_piv);
      checkCudaErrors(cudaFree(host_params.device_A));
      checkCudaErrors(cudaFree(host_params.device_LU));
      checkCudaErrors(cudaFree(host_params.device_piv));
      checkCudaErrors(cudaFree(host_params.device_info));
    }


}

// Figure out the command line
void set_defaults(Parameters &host_params, int matrix_size, char *inputName, char *goldName, int generate, int reps)
{
    // Set default params
    host_params.seed = 4321;
    host_params.m = host_params.n = host_params.lda = matrix_size;
    host_params.inputName = inputName;
    host_params.goldName = goldName;
    host_params.generate = generate;
    host_params.reps = reps;
}

// This is the main launch entry point. We have to package up our pointers
// into arrays and then decide if we're launching pthreads or CNP blocks.
void launch(Parameters &host_params)
{
    // Create a device-side copy of the params array...
    Parameters *device_params;
    checkCudaErrors(cudaMalloc((void **)&device_params, sizeof(Parameters)));
    checkCudaErrors(cudaMemcpy(device_params, &host_params, sizeof(Parameters), cudaMemcpyHostToDevice));
    dgetrf_test(&host_params, device_params);
    checkCudaErrors(cudaFree(device_params));
}

bool checkresult(Parameters &host_params)
{
    printf("Checking results... ");

    size_t len = host_params.data_len * host_params.data_size;
    size_t piv_len = host_params.piv_len * sizeof(int);

    double *device_I;
    double *host_I = (double *)malloc(len);
    // initialize iidentity matrix
    memset(host_I, 0, len);

    for (int i = 0; i < host_params.n; i++)
        host_I[i*host_params.lda+i]=1.0;

    checkCudaErrors(cudaMalloc((void **)&(device_I), len));
    checkCudaErrors(cudaMemcpy(device_I, host_I, len, cudaMemcpyHostToDevice));

    // allocate arrays for result checking
    double *device_result;
    double *host_result = (double *)malloc(len);
    checkCudaErrors(cudaMalloc((void **)&(device_result), len));
    checkCudaErrors(cudaMemset(device_result, 0, len));

    cublasHandle_t cb_handle = NULL;
    cudaStream_t stream;
    cublasStatus_t status = cublasCreate_v2(&cb_handle);
    cublasSetPointerMode_v2(cb_handle, CUBLAS_POINTER_MODE_HOST);
    cudaStreamCreate(&stream);
    cublasSetStream_v2(cb_handle, stream);

    double alpha = 1;
    int lda = host_params.lda;
    status = cublasDtrmm_v2(cb_handle, CUBLAS_SIDE_LEFT, CUBLAS_FILL_MODE_LOWER, CUBLAS_OP_N, CUBLAS_DIAG_UNIT, host_params.m, host_params.n, &alpha, host_params.device_LU, lda, device_I, lda, device_result, lda);

    if (status != CUBLAS_STATUS_SUCCESS)
        errorExit("checkresult: cublas failed");

    status = cublasDtrmm_v2(cb_handle, CUBLAS_SIDE_RIGHT, CUBLAS_FILL_MODE_UPPER, CUBLAS_OP_N, CUBLAS_DIAG_NON_UNIT, host_params.m, host_params.n, &alpha, host_params.device_LU, lda, device_result, lda, device_result, lda);

    if (status != CUBLAS_STATUS_SUCCESS)
        errorExit("checkresult: cublas failed");

    checkCudaErrors(cudaStreamSynchronize(stream));

    // Copy device_LU to host_LU.
    checkCudaErrors(cudaMemcpy(host_params.host_LU, host_params.device_LU, len, cudaMemcpyDeviceToHost));
    checkCudaErrors(cudaMemcpy(host_params.host_piv, host_params.device_piv, piv_len, cudaMemcpyDeviceToHost));

    // Rebuild the permutation vector.
    int mn = MIN(host_params.m, host_params.n);
    int *perm = (int *) malloc(mn*sizeof(int));

    for (int i = 0 ; i < mn ; ++i)
        perm[i] = i;

    for (int i = 0 ; i < mn ; ++i)
    {
        int j = host_params.host_piv[i];

        if (j >= mn)
            errorExit("Invalid pivot");

        if (i != j)
        {
            int tmp = perm[i];
            perm[i] = perm[j];
            perm[j] = tmp;
        }
    }

    const double tol = 1.0e-6;

    // Verify that L*U = A.
    checkCudaErrors(cudaMemcpy(host_result, device_result, len, cudaMemcpyDeviceToHost));
    bool ok = true;

    for (int i = 0; i < host_params.m; i++)
        for (int j = 0; j < host_params.n; j++)
            if (fabs(host_result[lda*j+i] - host_params.host_A[j*lda + perm[i]]) > tol)
            {
                printf("(%d,%d): found=%f, expected=%f\n", i, j, host_result[lda*j+i], host_params.host_A[j*lda + perm[i]]);
                ok = false;
                break;
            }

    status = cublasDestroy(cb_handle);
    if (status != CUBLAS_STATUS_SUCCESS)
       errorExit("checkresult: cublas failed");

    free(perm);
    free(host_I);
    free(host_result);
    checkCudaErrors(cudaFree(device_I));
    checkCudaErrors(cudaFree(device_result));
    printf("done\n");

    if (ok&&(host_params.generate))
    {
      printf("Writing gold to file...");
      FILE *fgold;
      if (fgold = fopen(host_params.goldName, "wb"))
      {
        fwrite(host_params.host_LU, len, 1, fgold);
        fwrite(host_params.host_piv, piv_len, 1, fgold);
        fclose(fgold);
      }
      else
      {
        printf("Warning: Could not open gold file in wb mode. %s\n", host_params.goldName);
      }
      printf("Done.\n");
    }
    return ok;
}

void test_check(Parameters &host_params)
{
  for (unsigned int i=0; i<)
}

bool launch_test(Parameters &host_params)
{
    for (unsigned int i=1; i<=host_params.reps; i++)
    {
      memsetup(host_params);
      launch(host_params);
      if (host_params.generate)
      {
        bool result = checkresult(host_params);
        finalize(host_params, WILL_NOT_REUSE);
        return result;
      }
      test_check(host_params);
      finalize(host_params, WILL_REUSE);
    }
    finalize(host_params, WILL_NOT_REUSE);
    return true;
}

void print_usage(const char *exec_name)
{
    printf("Usage: %s -matrix_size=N <-device=N> <-reps=N> <-generate> <-generategold> <-input=inputpath> <-gold=goldpath> (optional)\n", exec_name);
    printf("  matrix_size: the size of a NxN matrix. It must be greater than 0.\n");
}

////////////////////////////////////////////////////////////////////////////////
// Program main
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
#if CUDART_VERSION < 5000
#error cdpLU requires CUDA 5.0 to run, waiving testing...
#endif

    printf("Starting LU Decomposition (CUDA Dynamic Parallelism)\n");

    char *inputName, *goldName;
    int generate=0; // 0 = no / 1 = input+gold / 2 = gold

    int reps=1;
    int matrix_size = 1024;

    if (checkCmdLineFlag(argc, (const char **)argv, "help") ||
        checkCmdLineFlag(argc, (const char **)argv, "h"))
    {
        print_usage(argv[0]);
        exit(EXIT_SUCCESS);
    }

    if (checkCmdLineFlag(argc, (const char **)argv, "matrix_size"))
    {
        matrix_size = getCmdLineArgumentInt(argc, (const char **)argv, "matrix_size");

        if (matrix_size <= 0)
        {
            printf("Invalid matrix size given on the command-line: %d\n", matrix_size);
            exit(EXIT_FAILURE);
        }
    }

    if (checkCmdLineFlag(argc, (const char **)argv, "reps"))
    {
        reps = getCmdLineArgumentInt(argc, (const char **)argv, "reps");

        if (reps <= 0)
        {
            printf("Invalid reps given on the command-line: %d\n", reps);
            exit(EXIT_FAILURE);
        }
    }

    if (checkCmdLineFlag(argc, (const char **)argv, "generategold"))
    {
        generate = 2;
    }

    if (checkCmdLineFlag(argc, (const char **)argv, "generate"))
    {
        generate = 1;
    }

    if (checkCmdLineFlag(argc, (const char **)argv, "gold"))
    {
        getCmdLineArgumentString(argc, (const char **)argv, "gold", &goldName);
    }
    else
    {
        goldName = new char[100];
        snprintf(goldName, 100, "lud_gold_%i", (signed int)matrix_size);
        printf("Using default gold path: %s\n", goldName);
    }

    if (checkCmdLineFlag(argc, (const char **)argv, "input"))
    {
        getCmdLineArgumentString(argc, (const char **)argv, "input", &inputName);
    }
    else
    {
        inputName = new char[100];
        snprintf(inputName, 100, "lud_input_%i", (signed int)matrix_size);
        printf("Using default input path: %s\n", inputName);
    }

    // The test requires CUDA 5 or greater.
    // The test requires an architecture SM35 or greater (CDP capable).
    int cuda_device = findCudaDevice(argc, (const char **)argv);
    cudaDeviceProp deviceProps;
    checkCudaErrors(cudaGetDeviceProperties(&deviceProps, cuda_device));
    int cdpCapable = (deviceProps.major == 3 && deviceProps.minor >= 5) || deviceProps.major >=4;

    printf("GPU device %s has compute capabilities (SM %d.%d)\n", deviceProps.name, deviceProps.major, deviceProps.minor);

    if (!cdpCapable)
    {
        printf("cdpLUDecomposition requires SM 3.5 or higher to use CUDA Dynamic Parallelism.  Exiting...\n");

        // cudaDeviceReset causes the driver to clean up all state. While
        // not mandatory in normal operation, it is good practice.  It is also
        // needed to ensure correct operation when the application is being
        // profiled. Calling cudaDeviceReset causes all profile data to be
        // flushed before the application exits
        cudaDeviceReset();
        exit(EXIT_WAIVED);
    }

    Parameters host_params;
    memset(&host_params, 0, sizeof(Parameters));
    set_defaults(host_params, matrix_size, inputName, goldName, generate, reps);

    printf("Compute LU decomposition of a random %dx%d matrix using CUDA Dynamic Parallelism\n", matrix_size, matrix_size);
    printf("Launching single task from device...\n");
    bool result = launch_test(host_params);

    // cudaDeviceReset causes the driver to clean up all state. While
    // not mandatory in normal operation, it is good practice.  It is also
    // needed to ensure correct operation when the application is being
    // profiled. Calling cudaDeviceReset causes all profile data to be
    // flushed before the application exits
    cudaDeviceReset();

    if (result)
    {
        printf("Tests suceeded\n");
        exit(EXIT_SUCCESS);
    }
    else
    {
        exit(EXIT_FAILURE);
    }
}
