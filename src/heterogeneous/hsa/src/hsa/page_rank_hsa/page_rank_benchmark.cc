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

#include "src/hsa/page_rank_hsa/page_rank_benchmark.h"

#include <memory>

/* radiation things */
extern "C"
{
#include "../../logHelper/logHelper.h"
}

#define ITERATIONS 1000000


PageRankBenchmark::PageRankBenchmark() {
  workGroupSize = 64;
  maxIter = 1000;
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
     // val[j] = (float)val[j]*10;
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
      vector[j] = j+1;//1.0 / nr;
      vector_cpu[j] = vector[j];
      eigenV[j] = 0.0;
      eigenv_cpu[j] = 0.0;
    }
  }
//DAGO
csrMatrix.close();
denseVector.close();
//
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
  lparm->ldims[0] = local_work_size[0];
  lparm->gdims[0] = nr * workGroupSize;

  for (int j = 0; j < maxIter; j++) {
    //if(j < 25) printf("kernel inner iteration %d: %f, %f\n", j, eigenV[0], vector[0]); 
    if (j % 2 == 0) {
      pageRank_kernel(nr, rowOffset, col, val, sizeof(float) * 64, vector,
                      eigenV, lparm);
    } else {
      pageRank_kernel(nr, rowOffset, col, val, sizeof(float) * 64, eigenV,
                      vector, lparm);
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
  std::cout << "Running Page Rank. Generating inputs and gold: " << (gen_inputs == true ? "YES" : "NO") << std::endl;
  std::cout << std::endl;

//initialize logs
#ifdef LOGS
  char test_info[100];
  snprintf(test_info, 100, "size:%d, file:%s", nr, fileName1.c_str());
  char test_name[100];
  snprintf(test_name, 100, "hsaPageRank");
  start_log_file(test_name, test_info);
  set_max_errors_iter(500);
  set_iter_interval_print(10);
#endif

//begin loop of iterations
  //for(int iteration = 0; iteration < (gen_inputs == true ? 1 : ITERATIONS); iteration++)
  {
        //if(iteration % 10 == 0)
  	//	std::cout << "Iteration #" << iteration << std::endl;

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

void PageRankBenchmark::Verify() {
  CpuRun();
  for (int i = 0; i < nr; i++) {
    if (eigenv_cpu[i] != eigenV[i]) {
      std::cerr << "Not correct!\n";
      std::cerr << "Index: " << i << ", expected: " << eigenv_cpu[i]
                << ", but get: " << eigenV[i]
                << ", error: " << abs(eigenv_cpu[i] - eigenV[i]) << "\n";
    }
  }
}

void PageRankBenchmark::SaveGold() {
  char gold_file_str[64];
  sprintf(gold_file_str, "output/output_%d", nr);

  FILE* gold_file = fopen(gold_file_str, "wb");
  fwrite(eigenV, nr*sizeof(float), 1, gold_file);
int zero=0;
  
  for(int i = 0; i < nr; i++){
   if(i< 10)
      //printf("%e, %e \n", eigenV[i],vector[i]);   
    if(eigenV[i]==0)
	zero++;
  }
//printf("zeros %d of %d\n",zero, nr);

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
    if(gold[i] != eigenV[i])
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
  //for(int i = 0; i < 10; i++){
      //printf("check: %e, %e \n", eigenV[i],gold[i]);   
  //}

#ifdef LOGS
  log_error_count(errors);
#endif


  free(gold);

  //std::cout << "There were " << errors << " errors in the output!" << std::endl;
  //std::cout << std::endl;
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

/*
   int main(int argc, char const *argv[])
   {
   uint64_t diff;
   struct timespec start, end;
   if (argc < 2) {
   std::cout << "Usage: pagerank input_matrix [input_vector]" << std::endl;
   exit(-1);
   }
   clock_gettime(CLOCK_MONOTONIC, &start);

   std::unique_ptr<PageRankBenchmark> pr;
   std::unique_ptr<PageRankBenchmark> prCpu;
   if (argc == 2) {
   pr.reset(new PageRankBenchmark(argv[1]));
   prCpu.reset(new PageRankBenchmark(argv[1]));
   pr->Run();
   prCpu->CpuRun();
   }
   else if (argc == 3) {
   pr.reset(new PageRankBenchmark(argv[1], argv[2]));
   prCpu.reset(new PageRankBenchmark(argv[1], argv[2]));
   pr->Run();
   prCpu->CpuRun();
   }
   float* eigenGpu = pr->GetEigenV();
   float* eigenCpu = prCpu->GetEigenV();
   for(int i = 0; i < pr->GetLength(); i++) {
//		if( eigenGpu[i] != eigenCpu[i] ) {
if( pr->abs(eigenGpu[i] - eigenCpu[i]) >= 1e-5 ) {
std::cout << "Not Correct!" << std::endl;
std::cout.precision(20);
std::cout << pr->abs(eigenGpu[i] - eigenCpu[i]) << std::endl;
//std::cout << std::abs(1.23f) << std::endl;
//std::cout << eigenGpu[i] << "\t" << eigenCpu[i] << std::endl;
}
}
clock_gettime(CLOCK_MONOTONIC, &end);

diff = BILLION * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec;
printf("Total elapsed time = %llu nanoseconds\n", (long long unsigned int)
diff);
return 0;
}
*/
