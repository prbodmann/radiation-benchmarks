/*
 * Copyright (c) 2016 University of Cordoba and University of Illinois
 * All rights reserved.
 *
 * Developed by:    IMPACT Research Group
 *                  University of Cordoba and University of Illinois
 *                  http://impact.crhc.illinois.edu/
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * with the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 *      > Redistributions of source code must retain the above copyright notice,
 *        this list of conditions and the following disclaimers.
 *      > Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimers in the
 *        documentation and/or other materials provided with the distribution.
 *      > Neither the names of IMPACT Research Group, University of Cordoba, 
 *        University of Illinois nor the names of its contributors may be used 
 *        to endorse or promote products derived from this Software without 
 *        specific prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH
 * THE SOFTWARE.
 *
 */
#ifndef VERIFY_H
#define VERIFY_H
#include "common.h"
#include <math.h>

//*****************************************  LOG  ***********************************//
#ifdef LOGS
#include "log_helper.h"
#endif
//************************************************************************************//

inline int newest_verify(std::atomic_int *h_cost, int num_of_nodes,
		int num_of_nodes_o, Gold *&h_nodes, int it_cpu, int it_gpu) {
	int count_error = 0;
	if (num_of_nodes != num_of_nodes_o) {
		printf("Number of nodes does not match the expected value\n");
		//exit(EXIT_FAILURE);
	}

	for (int i = 0; i < num_of_nodes_o; i++) {
		int j, cost;
		// fscanf(fpo, "%ld %ld", &j, &cost);
		if (i != h_nodes[i].j || h_cost[i].load() * -1 != h_nodes[i].cost) {
			count_error++;
			//printf("Computed node %ld cost (%ld != %ld) does not match the expected value\n", i, h_cost[i].load(), cost);
#ifdef LOGS
			char error_detail[250];
			sprintf(error_detail,"Nodo: %d,e:%d, r:%d, CPU:%d , GPU:%d \n",i, h_nodes[i].cost,h_cost[i].load(),it_cpu,it_gpu);

			log_error_detail(error_detail);
#endif

			//exit(EXIT_FAILURE);
		}
	}

	return count_error;
}

inline int verify(std::atomic_int *h_cost, int num_of_nodes,
		const char *file_name) {
// Compare to output file
#if PRINT
	printf("Comparing outputs...\n");
#endif
	FILE *fpo = fopen(file_name, "r");
	if (!fpo) {
		printf("Error Reading output file\n");
		exit (EXIT_FAILURE);
	}
#if PRINT
	printf("Reading Output: %s\n", file_name);
#endif

	// the number of nodes in the output
	int num_of_nodes_o = 0, num_edges;
	fscanf(fpo, "%d %d", &num_of_nodes_o, &num_edges);
	if (num_of_nodes != num_of_nodes_o) {
		printf("FAIL: Number of nodes does not match the expected value\n");
		exit (EXIT_FAILURE);
	}

	// cost of nodes in the output
	for (int i = 0; i < num_of_nodes_o; i++) {
		int j, cost;
		fscanf(fpo, "%d %d", &j, &cost);
		if (i != j || h_cost[i].load() * -1 != cost) {
			printf("i %d j %d h_cost * -1 %d cost %d\n", i, j, h_cost[i].load() * -1, cost);
			printf(
					"FAIL: Computed node %d cost (%d != %d) does not match the expected value\n",
					i, h_cost[i].load(), cost);
			exit (EXIT_FAILURE);
		}
	}

	fclose(fpo);
	return 0;
}
#endif
