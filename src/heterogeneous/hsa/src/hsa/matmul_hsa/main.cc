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
 * Author: Yifan Sun (yifansun@coe.neu.edu)
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

#include "src/common/benchmark/benchmark_runner.h"
#include "src/common/time_measurement/time_measurement.h"
#include "src/common/time_measurement/time_measurement_impl.h"
#include "src/common/command_line_option/command_line_option.h"
#include "src/hsa/matmul_hsa/matmul_benchmark.h"

int main(int argc, const char **argv) {
  // Setup command line option
  CommandLineOption command_line_option(
    "====== Matrix Multiplication Benchmark "
    "(HSA mode) ======",
    "This benchmark runs matrix multiplication algorithm.");
  command_line_option.AddArgument("Help", "bool", "false",
      "-h", "--help", "Dump help information");
  command_line_option.AddArgument("MatrixSize", "int", "1024",
      "-i", "--input-file",
      "Size of the square matrices.");
  /*command_line_option.AddArgument("Verify", "bool", "false",
      "-v", "--verify",
      "Verify the calculation result");*/
  command_line_option.AddArgument("GenInputs", "bool", "false",
      "-g", "--generate",
      "Generate inputs and gold.");
  
  command_line_option.Parse(argc, argv);
  if (command_line_option.GetArgumentValue("Help")->AsBool()) {
    command_line_option.Help();
    return 0;
  }

  //bool verify = command_line_option.GetArgumentValue("Verify")->AsBool();
  int input_size = command_line_option.GetArgumentValue("MatrixSize")
    ->AsInteger();
  bool gen_inputs = command_line_option.GetArgumentValue("GenInputs")->AsBool();

  // Create and setup benchmarks
  std::unique_ptr<MatmulBenchmark> benchmark(new MatmulBenchmark());
  benchmark->SetMatrixInputSize(input_size);
  benchmark->SetGenInputs(gen_inputs);

  // Run benchmark
  std::unique_ptr<TimeMeasurement> timer(new TimeMeasurementImpl());
  BenchmarkRunner runner(benchmark.get(), timer.get());
  //runner.set_verification_mode(verify);
  runner.Run();
  runner.Summarize();
}
