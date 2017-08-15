/*
 * abft.c
 *
 *  Created on: 13/08/2017
 *      Author: fernando
 */

#include "abft.h"
#include <stdlib.h>
#include <string.h>


void init_error_return(ErrorReturn *e) {
	e->err_detected_size = MAXPOOL_N;
	e->error_detected = (unsigned long long*) malloc(
			e->err_detected_size * sizeof(unsigned long long));
}

void free_error_return(ErrorReturn *e) {
	if (e->error_detected)
		free(e->error_detected);
	e->error_detected = NULL;
}

void reset_error_return(ErrorReturn *e) {
	memset(e->error_detected, 0,
			sizeof(unsigned long long) * e->err_detected_size);
}
