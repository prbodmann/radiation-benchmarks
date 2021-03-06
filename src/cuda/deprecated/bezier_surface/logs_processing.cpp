/*
 * logs_processing.cpp
 *
 *  Created on: 12/10/2017
 *      Author: fernando
 */
#include "logs_processing.h"
#include <cmath>
#include <fstream>
#include <assert.h>
#include <iostream>
#include <sys/time.h>

#ifdef LOGS
#include "log_helper.h"
#endif


 // Returns the current system time in microseconds
double get_time() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (double(tv.tv_sec * 1000000) + double(tv.tv_usec)) /1000000;
}


int compare_and_log(XYZ *found, XYZ *gold, int RESOLUTIONI, int RESOLUTIONJ) {
	int errors = 0;

	double sum_delta2_x, sum_delta2_y, sum_delta2_z, sum_ref2, L1norm2;
	sum_delta2_x = 0;
	sum_delta2_y = 0;
	sum_delta2_z = 0;

	sum_ref2 = 0;
	L1norm2 = 0;
	for (int i = 0; i < RESOLUTIONI; i++) {
		for (int j = 0; j < RESOLUTIONJ; j++) {
//            sum_delta2 += fabs(outp[i * RESOLUTIONJ + j].x - outpCPU[i * RESOLUTIONJ + j].x);
//            sum_ref2 += fabs(outpCPU[i * RESOLUTIONJ + j].x);

			std::string to_write_error = "";
			bool somethin_is_wrong = false;
			sum_delta2_x = fabs(
					found[i * RESOLUTIONJ + j].x - gold[i * RESOLUTIONJ + j].x)
					/ fabs(gold[i * RESOLUTIONJ + j].x);

			if (sum_delta2_x >= MAX_ERROR_THRESHOLD) {
				char error_detail[200];
				sprintf(error_detail, "X, p: [%d, %d], r: %f, e: %f ", i, j,
						found[i * RESOLUTIONJ + j].x,
						gold[i * RESOLUTIONJ + j].x);
				to_write_error += std::string(error_detail);
				somethin_is_wrong = true;
			}
//            sum_delta2 += fabs(outp[i * RESOLUTIONJ + j].y - outpCPU[i * RESOLUTIONJ + j].y);
//            sum_ref2 += fabs(outpCPU[i * RESOLUTIONJ + j].y);

			sum_delta2_y = fabs(
					found[i * RESOLUTIONJ + j].y - gold[i * RESOLUTIONJ + j].y)
					/ fabs(gold[i * RESOLUTIONJ + j].y);
			if (sum_delta2_y >= MAX_ERROR_THRESHOLD) {
				char error_detail[200];
				sprintf(error_detail, "Y, p: [%d, %d], r: %f, e: %f ", i, j,
						found[i * RESOLUTIONJ + j].y,
						gold[i * RESOLUTIONJ + j].y);
				to_write_error += std::string(error_detail);
				somethin_is_wrong = true;

			}
//            sum_delta2 += fabs(outp[i * RESOLUTIONJ + j].z - outpCPU[i * RESOLUTIONJ + j].z);
//            sum_ref2 += fabs(outpCPU[i * RESOLUTIONJ + j].z);

			sum_delta2_z = fabs(
					found[i * RESOLUTIONJ + j].z - gold[i * RESOLUTIONJ + j].z)
					/ fabs(gold[i * RESOLUTIONJ + j].z);
			if (sum_delta2_z >= MAX_ERROR_THRESHOLD) {
				char error_detail[200];
				sprintf(error_detail, "Z, p: [%d, %d], r: %f, e: %f", i, j,
						found[i * RESOLUTIONJ + j].z,
						gold[i * RESOLUTIONJ + j].z);
				to_write_error += std::string(error_detail);
				somethin_is_wrong = true;
			}

			if (somethin_is_wrong) {
				errors++;
#ifdef LOGS
				log_error_detail((char*)to_write_error.c_str());
#endif
			}

		}
	}

#ifdef LOGS
	log_error_count(errors);
#endif

	return errors;
}

void start_benchmark(std::string gold_path, int n_reps, int n_gpu_threads,
		int n_gpu_blocks, int n_warmup, float alpha,
		std::string input_file_name, int in_size_i, int in_size_j,
		int out_size_i, int out_size_j) {
#ifdef LOGS
	std::string header = "gold: " + gold_path + " iterations: " + std::to_string(n_reps) + " n_gpu_threads: " + std::to_string(n_gpu_threads) +
	" n_gpu_blocks: " + std::to_string(n_gpu_blocks) + " n_warmup: " + std::to_string(n_warmup) + " alpha: " + std::to_string(alpha) + " input_file_name: " +
	input_file_name + " in_size_i: " + std::to_string(in_size_i) + " in_size_j: " + std::to_string(in_size_j) +
	" out_size_i: " + std::to_string(out_size_i) + " out_size_j: " + std::to_string(out_size_j);

	start_log_file("cudaHeterogeneousBezierSurface", (char*)header.c_str());
#endif
}

void end_benchmark() {
#ifdef LOGS
	end_log_file();
#endif
}

void start_iteration_call() {
#ifdef LOGS
	start_iteration();
#endif
}

void end_iteration_call() {
#ifdef LOGS
	end_iteration();
#endif
}

void save_gold(XYZ *gold, int size, std::string gold_path) {
	std::ofstream gold_out(gold_path, std::ios::out | std::ios::binary);
	assert(gold_out.is_open() && "Gold file not opened for writing");
	gold_out.write(reinterpret_cast<char*>(&size), sizeof(int));
	gold_out.write(reinterpret_cast<char*>(gold), sizeof(XYZ) * size);
	gold_out.close();
}

void load_gold(XYZ* gold, int size, std::string gold_path) {
	std::ifstream gold_in(gold_path, std::ios::in | std::ios::binary);
	assert(gold_in.is_open() && "Gold file not opened for reading");
	int temp_size;

	gold_in.read(reinterpret_cast<char*>(&temp_size), sizeof(int));
	assert(size == temp_size && "Gold Size is not ok");

//	gold = (XYZ*) calloc(size, sizeof(XYZ));
	assert(gold != nullptr && "Could not allocate gold array");
	gold_in.read(reinterpret_cast<char*>(gold), sizeof(XYZ) * size);

	gold_in.close();
}

