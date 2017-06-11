/*
 * ConvNet.h
 *
 *  Created on: May 26, 2017
 *      Author: carol
 */

#ifndef CONVNET_H_
#define CONVNET_H_

//#pragma once
#include "Util.h"
#include "ConvolutionalLayer.h"
#include "MNISTParser.h"
#include "Layer.h"
#include "MaxpoolingLayer.h"
#include "OutputLayer.h"
#include "FullyConnectedLayer.h"

//namespace convnet {
//
#define MAX_ITER 2  // maximum training iterations
#define M 10 // training sample counts in each iteration
#define END_CONDITION 1e-3

class ConvNet {
public:
	void train(vec2d_t train_x, vec_t train_y, size_t train_size);
	void test(vec2d_t test_x, vec_t test_y, size_t test_size, int batch_size);
	void test(vec2d_t test_x, vec_t test_y, size_t test_size);

	void add_layer(Layer* layer);

private:
	size_t max_iter(vec_t v);
	size_t max_iter(float v[], size_t size);
	bool test_once_random();

	bool test_once(int test_x_index);

	int test_once_random_batch(int batch_size);
	int test_once_batch(int test_x_index, int batch_size);

	bool check_batch_result(int batch_size);

	float_t train_once();

	std::vector<Layer*> layers;

	size_t train_size_;
	vec2d_t train_x_;
	vec_t train_y_;

	size_t test_size_;
	vec2d_t test_x_;
	vec_t test_y_;

};

//#undef MAX_ITER
//#undef M

//} /* namespace convnet */

#endif /* CONVNET_H_ */