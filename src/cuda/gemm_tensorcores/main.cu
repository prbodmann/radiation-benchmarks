#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <fstream>      // std::ifstream
#include <sstream>      // std::stringstream

#include "half.hpp"
#include "Log.h"
#include "GEMMWMMA.h"

#ifndef DEFAULT_INPUT_SIZE
#define DEFAULT_INPUT_SIZE 8192
#endif

#define GENERATOR_MAXABSVALUE 2.0
#define GENERATOR_MINABSVALUE 0

typedef half_float::half host_half;

typedef std::vector<host_half> half_vector;

template<class real_t> void generate_matrices_files(half_vector& a_host_vector,
		half_vector& b_host_vector, std::vector<real_t>& c_host_vector,
		Log& log) {

	std::ofstream f_a(log.a_input_path, std::ios::out | std::ios::binary);
	std::ofstream f_b(log.b_input_path, std::ios::out | std::ios::binary);
	std::ofstream f_c(log.c_input_path, std::ios::out | std::ios::binary);
//	std::cout << "entrou generate" << std::endl;

	if (f_a.is_open() && f_b.is_open() && f_c.is_open()) {
		std::random_device rd; //Will be used to obtain a seed for the random number engine
		std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
		std::uniform_real_distribution<double> dis(-GENERATOR_MAXABSVALUE,
		GENERATOR_MAXABSVALUE);
//		std::cout << "entrou if generate1" << std::endl;

//		for (size_t i = 0; i < log.size_matrices; i++) {
//			for (size_t j = 0; j < log.size_matrices; j++) {
//				a_host_vector[i * log.size_matrices + j] = host_half(dis(gen));
//				b_host_vector[i * log.size_matrices + j] = host_half(dis(gen));
//				c_host_vector[i * log.size_matrices + j] = real_t(dis(gen));
//			}
//		}


		for (size_t i = 0; i < 16; i++) {
			for (size_t j = 0; j < 16; j++) {
				a_host_vector[i * 16 + j] = (half) 2.0;
				b_host_vector[i * 16 + j] = (half) 2.0;
				c_host_vector[i * 16 + j] = (float) 2.0;
			}
		}

		for (size_t i = 0; i < 16; i++) {
						for (size_t j = 0; j < 16; j++) {
							std::cout << " Ah= " << a_host_vector[i * 16+ j]  << std::endl;
							std::cout << " Bh= " << b_host_vector[i * 16 + j] <<  std::endl;
							std::cout << " Ch= " << c_host_vector[i * 16+ j] <<  std::endl;
						}
		}

//		std::cout << "entrou generate1" << std::endl;
		host_half zero(0.0);
		host_half nan_ = host_half(half_float::nanh("0"));
		host_half inf_ = host_half(host_half(0x7C00));
//		std::cout << "entrou generate2" << std::endl;

		int numZeros = std::count(a_host_vector.begin(), a_host_vector.end(),
				zero);
		int numNans = std::count(a_host_vector.begin(), a_host_vector.end(),
				nan_);

		int numInfs = std::count(a_host_vector.begin(), a_host_vector.end(),
				inf_);
		std::cout << "Number of zeros/NaNs/INFs on matrix A: " << numZeros
				<< numNans << numInfs << std::endl;

//		std::cout << "entrou generate3" << std::endl;
		numZeros = std::count(b_host_vector.begin(), b_host_vector.end(), zero);
		numNans = std::count(b_host_vector.begin(), b_host_vector.end(), nan_);
		numInfs = std::count(b_host_vector.begin(), b_host_vector.end(), inf_);

		std::cout << "Number of zeros/NaNs/INFs on matrix B: " << numZeros
				<< numNans << numInfs << std::endl;

		numZeros = std::count(c_host_vector.begin(), c_host_vector.end(), zero);
		numNans = std::count(c_host_vector.begin(), c_host_vector.end(), nan_);
		numInfs = std::count(c_host_vector.begin(), c_host_vector.end(), inf_);

		std::cout << "Number of zeros/NaNs/INFs on matrix C: " << numZeros
				<< numNans << numInfs << std::endl;



		f_a.write(reinterpret_cast<char*>(a_host_vector.data()),
				a_host_vector.size() * sizeof(host_half));
		f_b.write(reinterpret_cast<char*>(b_host_vector.data()),
				b_host_vector.size() * sizeof(host_half));
		f_c.write(reinterpret_cast<char*>(c_host_vector.data()),
				c_host_vector.size() * sizeof(real_t));

		f_a.close();
		f_b.close();
		f_c.close();

	} else {
		throw std::runtime_error(
				"Some of the imput files could not be generated\n");
	}

}

template<class real_t>
void write_gold_to_file(std::string gold_path, std::vector<real_t>& gold) {
	std::ofstream f_gold(gold_path, std::ofstream::out | std::ofstream::binary);
	if (f_gold.is_open()) {
		f_gold.write(reinterpret_cast<char*>(gold.data()),
				sizeof(real_t) * gold.size());
		f_gold.close();
	} else {
		throw std::runtime_error("Could not write gold file\n");
	}
}

template<class real_t> int is_output_ok(std::vector<real_t>& d0,
		std::vector<real_t>& d1, std::vector<real_t>& d2,
		std::vector<real_t>& correct_vector) {

//	<<<<<<< HEAD
	int memory_errors = 0;
	for (size_t i = 0; i < d0.size(); i++) {
		real_t val_output0 = d0[i];
		real_t val_output1 = d1[i];
		real_t val_output2 = d2[i];
		real_t val_output = val_output0;
		if ((val_output0 != val_output1) || (val_output0 != val_output2)) {
			memory_errors++;

			if ((val_output0 != val_output1) && (val_output1 != val_output2)
					&& (val_output0 != val_output2)) {
				// All 3 values diverge
				memory_errors++;
			} else if (val_output1 == val_output2) {
				// Only value 0 diverge
				val_output = val_output1;
			} else if (val_output0 == val_output2) {
				// Only value 1 diverge
				val_output = val_output0;
			} else if (val_output0 == val_output1) {
				// Only value 2 diverge
				val_output = val_output0;
			}
		}
		correct_vector[i] = val_output;
	}
	return memory_errors;
//	=======
//
//	if (d0 == d1) {
//		correct_vector = d0.
//	}
//	else if (d1==d2) {
//		correct_vector = d1;
//	}
//	else if (d2==d3) {
//		correct_vector = d2;
//	}
//	else if (d1==d3) {
//		correct_vector = d1;
//	}
//	else if (d1 != d2 && d2 != d3 && d1 != d3) {
//		return false;
//	}
//
//	//TODO: Pedro tem que fazer a verificacao
//	// se o output que vai ser salvo no gold esta ok
//	// tem que fazer uma votacao melhor de tres
//	// e salvar no correct_vector
//	return true;
//>>>>>>> 9383aaede052c495a645216e82bcbe29f861b5dc
}

template<class real_t> void retrieve_matrices(half_vector& a_host_vector,
		half_vector& b_host_vector, std::vector<real_t>& c_host_vector,
		std::vector<real_t>& gold_host_vector, Log& log) {

	double start = log.mysecond();
	std::ifstream f_a(log.a_input_path, std::ios::in | std::ios::binary);
	std::ifstream f_b(log.b_input_path, std::ios::in | std::ios::binary);
	std::ifstream f_c(log.c_input_path, std::ios::in | std::ios::binary);
	std::ifstream f_gold(log.gold_inout_path,
			std::ifstream::in | std::ifstream::binary);

	if (f_a.is_open() && f_b.is_open() && f_c.is_open() && f_gold) {

		f_a.seekg(0, std::ios::beg);
		f_a.read(reinterpret_cast<char*>(a_host_vector.data()),
				sizeof(host_half) * a_host_vector.size());

		f_b.seekg(0, std::ios::beg);
		f_b.read(reinterpret_cast<char*>(b_host_vector.data()),
				sizeof(host_half) * b_host_vector.size());

		f_c.seekg(0, std::ios::beg);
		f_c.read(reinterpret_cast<char*>(c_host_vector.data()),
				sizeof(host_half) * c_host_vector.size());

		f_gold.seekg(0, std::ios::beg);
		f_gold.read(reinterpret_cast<char*>(gold_host_vector.data()),
				sizeof(host_half) * gold_host_vector.size());

		f_a.close();
		f_b.close();
		f_c.close();
		f_gold.close();
	} else {
		log.log_error("Could not retrieve the matrices");
		throw std::runtime_error("Could not retrieve the matrices\n");
	}

	std::cout << "Done with reading matrices " << log.mysecond() - start
			<< "s\n";
}

template<class real_t>
std::pair<int, int> compare_output_matrices(long long host_is_memory_bad,
		std::vector<real_t>& gold, std::vector<real_t>& c0,
		std::vector<real_t>& c1, std::vector<real_t>& c2, Log& log) {

	int host_errors = 0;
	int memory_errors = 0;

	if (host_is_memory_bad != 0) {
		std::string info_detail = "b: is_memory_bad: "
				+ std::to_string(host_is_memory_bad);
		if (log.verbose)
			std::cout << info_detail << std::endl;

		log.log_error(info_detail);
		memory_errors++;
	}

#pragma omp parallel for shared(host_errors)
	for (size_t i = 0; i < gold.size(); i++) {
		register bool checkFlag = true;
		register real_t valGold = gold[i];
		register real_t valOutput0 = c0[i];
		register real_t valOutput1 = c1[i];
		register real_t valOutput2 = c2[i];
		register real_t valOutput = valOutput0;
		if ((valOutput0 != valOutput1) || (valOutput0 != valOutput2)) {
#pragma omp critical
			{
//				char info_detail[200];
//				snprintf(info_detail, 150,
//						"m: [%d, %d], r0: %1.20e, r1: %1.20e, r2: %1.20e",
//						int(floor(i / log.size_matrices)),
//						int(i % log.size_matrices), (double) valOutput0,
//						(double) valOutput1, (double) valOutput2);
//
				std::stringstream info_detail("");
				info_detail << "m: [" << int(floor(i / log.size_matrices))
						<< ", " << i % log.size_matrices << "], r0: "
						<< valOutput0 << ", r1: " << valOutput1 << ", r2: "
						<< valOutput2;

				if (log.verbose && (memory_errors < 10))
					std::cout << info_detail.str() << std::endl;

				log.log_info(info_detail.str());
				memory_errors++;
			}
			if ((valOutput0 != valOutput1) && (valOutput1 != valOutput2)
					&& (valOutput0 != valOutput2)) {
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
//						char info_detail[200];
//						snprintf(info_detail, 150,
//								"t: [%d, %d], r0: %1.20e, r1: %1.20e, r2: %1.20e, e: %1.20e",
//								int(floor(i / log.size_matrices)),
//								int(i % log.size_matrices), (double) valOutput0,
//								(double) valOutput1, (double) valOutput2,
//								(double) valGold);
						std::stringstream info_detail("");
						info_detail << "t: ["
								<< int(floor(i / log.size_matrices)) << ", "
								<< i % log.size_matrices << "], r0: "
								<< valOutput0 << ", r1: " << valOutput1
								<< ", r2: " << valOutput2 << ", e: " << valGold;

						if (log.verbose && (memory_errors < 10))
							std::cout << info_detail.str() << std::endl;

						log.log_info(std::string(info_detail.str()));

						memory_errors++;
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

		if (valGold != valOutput) {
			if (checkFlag) {
#pragma omp critical
				{
//				char error_detail[200];
//				snprintf(error_detail, 150,
//						"p: [%lu, %lu], r: %1.20e, e: %1.20e",
//						int(floor(i / log.size_matrices)),
//						int(i % log.size_matrices), (double) valOutput,
//						(double) valGold);
					std::stringstream error_detail("");
					error_detail << "p: [" << int(floor(i / log.size_matrices))
							<< ", " << i % log.size_matrices << "], r: "
							<< valOutput << ", e: " << valGold;

					if (log.verbose && (host_errors < 10))
						std::cout << error_detail.str() << std::endl;

					log.log_error(error_detail.str());
					host_errors++;
				}
			}
		}
	}

// printf("numErrors:%d", host_errors);

	log.update_info_count(memory_errors);
	log.update_error_count(host_errors);

	if (memory_errors != 0)
		std::cout << "M";
	if (host_errors != 0)
		std::cout << "#";

	std::pair<int, int> res(memory_errors, host_errors);
	return res;
}

template<class real_t>
void call_mxm(half_vector& host_matrix_a, half_vector& host_matrix_b,
		Log& log_obj) {

// C matrix
	std::vector<real_t> host_matrix_c(
			log_obj.size_matrices * log_obj.size_matrices);
	std::vector<real_t> host_gold(
			log_obj.size_matrices * log_obj.size_matrices);
// D Matrix
	std::vector<real_t> host_matrix_d1(
			log_obj.size_matrices * log_obj.size_matrices);
	std::vector<real_t> host_matrix_d2(
			log_obj.size_matrices * log_obj.size_matrices);
	std::vector<real_t> host_matrix_d3(
			log_obj.size_matrices * log_obj.size_matrices);
//		std::cout << "passou declaracao host" << std::endl;
	if (!log_obj.generate) {
		retrieve_matrices<real_t>(host_matrix_a, host_matrix_b, host_matrix_c,
				host_gold, log_obj);
	} else {
		//			std::cout << "entrou else" << std::endl;
		generate_matrices_files<real_t>(host_matrix_a, host_matrix_b,
				host_matrix_c, log_obj);
	}


//GOLD Matrix
	std::vector<real_t> host_matrix_gold(
			log_obj.size_matrices * log_obj.size_matrices);
	GEMMWMMA<host_half, half, real_t> mult_enviroment(host_matrix_a,
			host_matrix_b.data(), host_matrix_c.data(), log_obj.size_matrices,
			log_obj.size_matrices, log_obj.size_matrices);

	int tries = 0;
	for (int it = 0; it < log_obj.iterations; it++) {
		log_obj.start_iteration_app();
		mult_enviroment.mul();
		log_obj.end_iteration_app();

		mult_enviroment.pull_array(host_matrix_d1.data(), host_matrix_d2.data(),
				host_matrix_d3.data());



		for (size_t i = 0; i < 16; i++) {

			std::cout << " d1= " << host_matrix_d1[i]  << std::endl;

		}

		for (size_t i = 0; i < 16; i++) {

			std::cout << " d2= " << host_matrix_d2[i] <<  std::endl;


		}

		for (size_t i = 0; i < 16; i++) {

			std::cout << " d3= " << host_matrix_d3[i] <<  std::endl;
		}

		for (size_t i = 0; i < 16; i++) {

					std::cout << " gold host = " << host_gold[i] <<  std::endl;
		}
		double start = log_obj.mysecond();
		std::pair<int, int> errors = compare_output_matrices(
				mult_enviroment.get_memory_errors(), host_gold, host_matrix_d1,
				host_matrix_d2, host_matrix_d3, log_obj);
		double end = log_obj.mysecond();

		std::cout << "Iteration: " << it << " memory errors " << errors.first
				<< " radiation errors " << errors.second
				<< ". Time spent on comparing " << end - start << "s."
				<< std::endl;

		//TODO check this
		if (log_obj.generate) {
			tries++;
			int has_errors = is_output_ok(host_matrix_d1, host_matrix_d2,
					host_matrix_d3, host_gold);
			if (has_errors != 0)
				it--;

			if (tries > 5)
				throw std::runtime_error(
						"More than 5 tries on matrix generate\n");
		}
		//If errors != 0 reload matrices to gpu
		if (errors.first != 0 || errors.second != 0) {
			mult_enviroment.push_arrays(host_matrix_a,
					host_matrix_b.data(), host_matrix_c.data());
		}
	}
	if (log_obj.generate) {
		write_gold_to_file<real_t>(log_obj.gold_inout_path, host_gold);
	}
}

void usage(char **argv) {
	std::cout << "./" << argv[0]
			<< " --generate 0/1 --gold <gold file, DEFAULT=./gold.matrix > --size <matrix size, DEFAULT=8192> "
					"--iterations <how many iterations, optional> --input_a <input A, DEFAUL=./input_a.matrix> "
					"--input_b <input B, DEFAUL=./input_b.matrix> --input_c <input C, DEFAUL=./input_c.matrix>  --precision <float/double, DEFAULT=float>"
			<< std::endl;
}

int main(int argc, char** argv) {
	Log log_obj(argc, argv, DEFAULT_INPUT_SIZE);

	std::cout << "Generate: " << log_obj.generate << std::endl;
	std::cout << "A input path: " << log_obj.a_input_path << std::endl;
	std::cout << "B input path: " << log_obj.b_input_path << std::endl;
	std::cout << "C input path: " << log_obj.c_input_path << std::endl;
	std::cout << "Gold in/out path: " << log_obj.gold_inout_path << std::endl;
	std::cout << "Iterations: " << log_obj.iterations << std::endl;
	std::cout << "Matrix size: " << log_obj.size_matrices << std::endl;
	std::cout << "Precision: " << log_obj.precision << std::endl;
	std::cout << "Verbose: " << log_obj.verbose << std::endl;

// Alloc all memories on host
	half_vector host_matrix_a(log_obj.size_matrices * log_obj.size_matrices);
	half_vector host_matrix_b(log_obj.size_matrices * log_obj.size_matrices);

	if (log_obj.precision == "float") {
		call_mxm<float>(host_matrix_a, host_matrix_b, log_obj);
	}

	if (log_obj.precision == "double") {
		call_mxm<double>(host_matrix_a, host_matrix_b, log_obj);
	}

	std::cout << "Finished computation\n";
	return 0;
}