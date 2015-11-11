/*
 * Hetero-mark
 *
 * Copyright (c) 2015 Northeastern University
 * All rights reserved.
 *
 * Developed by:
 *   Northeastern University Computer Architecture Research (NUCAR) Group
 *   Northeastern University
 *   http://www.ece.neu.edu/groups/nucar/
 *
 * Author: Xiangyu Li (xili@ece.neu.edu)
 * Modified by: Yifan Sun (yifansun@coe.neu.edu)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal with the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 *   Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimers.
 *
 *   Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimers in the
 *   documentation and/or other materials provided with the distribution.
 *
 *   Neither the names of NUCAR, Northeastern University, nor the names of
 *   its contributors may be used to endorse or promote products derived
 *   from this Software without specific prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS WITH THE SOFTWARE.
 */

#include "src/hsa/page_rank_intra_beam/page_rank_benchmark.h"

#include <memory>

/* radiation things */
extern "C"
{
#include "../../logHelper/logHelper.h"
}

#define ITERATIONS 1000000


PageRankBenchmark::PageRankBenchmark() {
  workGroupSize = 64;
  maxIter = 300;
}

void PageRankBenchmark::InitBuffer() {
  rowOffset = new int[nr + 1];
  rowOffset_cpu = new int[nr + 1];
  col = new int[nnz];
  col_cpu = new int[nnz];
  val = new float[nnz];
  val_cpu = new float[nnz];
  vector = new float[nr];
  eigenV = new float[nr];
  vector_cpu = new float[nr];
  eigenv_cpu = new float[nr];

  // CKALRA, DLOWELL
  ebsize = nr * workGroupSize;
  errorBuffer = (unsigned int*)calloc(sizeof(unsigned int), ebsize);
}

void PageRankBenchmark::FreeBuffer() {
  csrMatrix.close();
  denseVector.close();
}

void PageRankBenchmark::FillBuffer() {
  FillBufferCpu();
  FillBufferGpu();
}

void PageRankBenchmark::FillBufferGpu() {}

void PageRankBenchmark::FillBufferCpu() {
  while (!csrMatrix.eof()) {
    for (int j = 0; j < nr + 1; j++) {
      csrMatrix >> rowOffset[j];
      rowOffset_cpu[j] = rowOffset[j];
    }
    for (int j = 0; j < nnz; j++) {
      csrMatrix >> col[j];
      col_cpu[j] = col[j];
    }
    for (int j = 0; j < nnz; j++) {
      csrMatrix >> val[j];
      // val[j] = (float)val[j];
      val_cpu[j] = val[j];
    }
  }
  if (isVectorGiven) {
    while (!denseVector.eof()) {
      for (int j = 0; j < nr; j++) {
        denseVector >> vector[j];
        vector_cpu[j] = vector[j];
        eigenV[j] = 0.0;
        eigenv_cpu[j] = 0.0;
      }
    }
  } else {
    for (int j = 0; j < nr; j++) {
      vector[j] = 1.0 / nr;
      vector_cpu[j] = vector[j];
      eigenV[j] = 0.0;
      eigenv_cpu[j] = 0.0;
    }
  }
//DAGO
  csrMatrix.close();
  denseVector.close();

}

void PageRankBenchmark::ReadCsrMatrix() {
  csrMatrix.open(fileName1);
  if (!csrMatrix.good()) {
    std::cout << "cannot open csr matrix file" << std::endl;
    exit(-1);
  }
  csrMatrix >> nnz >> nr;
}

void PageRankBenchmark::ReadDenseVector() {
  if (isVectorGiven) {
    denseVector.open(fileName2);
    if (!denseVector.good()) {
      std::cout << "Cannot open dense vector file" << std::endl;
      exit(-1);
    }
  }
}

void PageRankBenchmark::PrintOutput() {
  std::cout << std::endl
            << "Eigen Vector: " << std::endl;
  for (int i = 0; i < nr; i++) std::cout << eigenV[i] << "\t";
  std::cout << std::endl;
}

void PageRankBenchmark::Print() {
  std::cout << "nnz: " << nnz << std::endl;
  std::cout << "nr: " << nr << std::endl;
  std::cout << "Row Offset: " << std::endl;
  for (int i = 0; i < nr + 1; i++) std::cout << rowOffset[i] << "\t";
  std::cout << std::endl
            << "Columns: " << std::endl;
  for (int i = 0; i < nnz; i++) std::cout << col[i] << "\t";
  std::cout << std::endl
            << "Values: " << std::endl;
  for (int i = 0; i < nnz; i++) std::cout << val[i] << "\t";
  std::cout << std::endl
            << "Vector: " << std::endl;
  for (int i = 0; i < nr; i++) std::cout << vector[i] << "\t";
  std::cout << std::endl
            << "Eigen Vector: " << std::endl;
  for (int i = 0; i < nr; i++) std::cout << eigenV[i] << "\t";
  std::cout << std::endl;
}

void PageRankBenchmark::ExecKernel() {
  global_work_size[0] = nr * workGroupSize;
  local_work_size[0] = workGroupSize;

  SNK_INIT_LPARM(lparm, 0);

  // CKALRA, DLOWELL Doubling global and local sizes
  lparm->ldims[0] = 2 * local_work_size[0];
  lparm->gdims[0] = 2 * nr * workGroupSize;

  for (int j = 0; j < maxIter; j++) {

  	memset(errorBuffer, 0, ebsize*sizeof(unsigned int));
    if (j % 2 == 0) {
      pageRank_kernel(nr, rowOffset, col, val, sizeof(float) * 64, vector,
                      eigenV, errorBuffer, lparm);
    } else {
      pageRank_kernel(nr, rowOffset, col, val, sizeof(float) * 64, eigenV,
                      vector, errorBuffer, lparm);
    }
  }
}

void PageRankBenchmark::CpuRun() {
  for (int i = 0; i < maxIter; i++) {
    PageRankCpu();
    if (i != maxIter - 1) {
      for (int j = 0; j < nr; j++) {
        vector_cpu[j] = eigenv_cpu[j];
        eigenv_cpu[j] = 0.0;
      }
    }
  }
}

float* PageRankBenchmark::GetEigenV() { return eigenV; }

void PageRankBenchmark::PageRankCpu() {
  for (int row = 0; row < nr; row++) {
    eigenv_cpu[row] = 0;
    float dot = 0;
    int row_start = rowOffset_cpu[row];
    int row_end = rowOffset_cpu[row + 1];

    for (int jj = row_start; jj < row_end; jj++)
      dot += val_cpu[jj] * vector_cpu[col_cpu[jj]];

    eigenv_cpu[row] += dot;
  }
}

void PageRankBenchmark::Initialize() {
  ReadCsrMatrix();
  ReadDenseVector();
  InitBuffer();
  FillBuffer();
}

void PageRankBenchmark::Run() {
  std::cout << "Running Page Rank RMT INTRA. Generating inputs and gold: " << (gen_inputs == true ? "YES" : "NO") << std::endl;
  std::cout << std::endl;

//initialize logs
#ifdef LOGS
  char test_info[100];
  snprintf(test_info, 100, "size:%d, file:%s", nr, fileName1.c_str());
  char test_name[100];
  snprintf(test_name, 100, "hsaPageRank_RMT_INTRA");
  start_log_file(test_name, test_info);
  set_max_errors_iter(500);
  set_iter_interval_print(10);
#endif

//begin loop of iterations
  for(int iteration = 0; iteration < (gen_inputs == true ? 1 : ITERATIONS); iteration++)
  {
        if(iteration % 10 == 0)
                std::cout << "Iteration #" << iteration << std::endl;

//start iteration
#ifdef LOGS
  start_iteration();
#endif

  	// Execute the kernel
  	ExecKernel();

//end iteration
#ifdef LOGS
  end_iteration();
#endif

        if(gen_inputs == true)
          SaveGold();

        else
          CheckGold();

//DAGO
  ReadCsrMatrix();
  ReadDenseVector();
  FillBuffer();
//

  }

//end log file
#ifdef LOGS
  end_log_file();
#endif
}

void PageRankBenchmark::SaveGold() {
  char gold_file_str[64];
  sprintf(gold_file_str, "output/output_%d", nr);

  FILE* gold_file = fopen(gold_file_str, "wb");
  fwrite(eigenV, nr*sizeof(float), 1, gold_file);
int zero=0;

  for(int i = 0; i < nr; i++){
   if(i< 10)
      printf("%e, %e \n", eigenV[i],vector[i]);
    if(eigenV[i]==0)
        zero++;
  }
printf("zeros %d of %d\n",zero, nr);

  fclose(gold_file);

}

void PageRankBenchmark::CheckGold() {
  float *gold = (float*) malloc(nr * sizeof(float));

  char gold_file_str[64];
  sprintf(gold_file_str, "output/output_%d", nr);

  FILE* gold_file = fopen(gold_file_str, "rb");
  int read = fread(gold, nr*sizeof(float), 1, gold_file);
  if(read != 1)
    read = -1;

  fclose(gold_file);

  int errors = 0;
  for(int i = 0; i < nr; i++)
  {
    if(abs(gold[i] - eigenV[i]) > 1e-5 || eigenV[i] == 0)
    {
        errors++;

        char error_detail[128];
        snprintf(error_detail, 64, "position: [%d], output: %f, gold: %f\n", i, eigenV[i], gold[i]);
        printf("Error: %s\n", error_detail);

#ifdef LOGS
  log_error_detail(error_detail);
#endif

    }
  }
  for(int i = 0; i < 10; i++){
      printf("check: %e, %e \n", eigenV[i],gold[i]);
  }

  char rmt_error_detail[128];

  /* CKALRA, DLOWELL: Process errorBuffer, inthebeam test you can just save to file */
  unsigned int errCount = 0;
  unsigned int totErrCount = 0;
  //printf("errorBuffer size (flat global size): %u\n",ebsize);
  for(unsigned int i=0;i<ebsize;i++){
    if(errorBuffer[i]>0){
      errCount++;
      totErrCount+=errorBuffer[i];
    }
  }

  if(errCount > 0)
  {
    snprintf(rmt_error_detail, 128, "err_buf size: %u, thread_err: %u, total_err: %u\n", ebsize, errCount, totErrCount);
    printf("%s", rmt_error_detail);

#ifdef LOGS
  log_error_detail(rmt_error_detail);
#endif

  }

#ifdef LOGS
  log_error_count(errors);
#endif


  free(gold);
}

void PageRankBenchmark::Verify() {
  CpuRun();
  for (int i = 0; i < nr; i++) {
    if (abs(eigenv_cpu[i] - eigenV[i]) >= 1e-5) {
      std::cerr << "Not correct!\n";
      std::cerr << "Index: " << i << ", expected: " << eigenv_cpu[i]
                << ", but get: " << eigenV[i]
                << ", error: " << abs(eigenv_cpu[i] - eigenV[i]) << "\n";
    }
  }
}

void PageRankBenchmark::Summarize() {}

void PageRankBenchmark::Cleanup() { FreeBuffer(); }

int PageRankBenchmark::GetLength() { return nr; }

float PageRankBenchmark::abs(float num) {
  if (num < 0) {
    num = -num;
  }
  return num;
}

