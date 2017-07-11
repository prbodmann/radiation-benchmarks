/*
 * ConvNet.cpp
 *
 *  Created on: 05/06/2017
 *      Author: fernando
 */

#include "ConvNet.h"

#ifdef GPU
#include "cudaUtil.h"
#endif



void debug_layers(Layer *layer, char *type, int t){
	char temp[200];

#ifdef GPU
	sprintf(temp, "layer_%d_gpu_%s.lay", t++, type);
#else
	sprintf(temp, "layer_%d_cpu_%s.lay", t++, type);
#endif
	FILE *in = fopen(temp, "wb");
	layer->save_layer(in);
	fclose(in);

}

void ConvNet::train(vec2d_t train_x, vec_host train_y, size_t train_size) {

#ifdef GPU
	std::cout << "Training with GPU:" << std::endl;
#else
	std::cout << "Training with CPU:" << std::endl;
#endif
	train_x_ = train_x;
	train_y_ = train_y;
	train_size_ = train_size;
	/*
	 auto add OutputLayer as the last layer.
	 */
	this->add_layer(new OutputLayer(layers.back()->out_depth_));

	/*
	 start training...
	 */
	auto stop = false;
	int iter = 0;
	this->mark.start();
	while (iter < MAX_ITER && !stop) {
		iter++;
		auto err = train_once();
		std::cout << " training cost: " << err << std::endl;
		if (err < END_CONDITION)
			stop = true;
	}
	this->mark.stop();
	std::cout << "Time spent on training " << this->mark << std::endl;
}

void ConvNet::test(vec2d_t test_x, vec_host test_y, size_t test_size) {
//	assert(batch_size > 0);
//	assert(test_size % batch_size == 0);
	test_x_ = test_x;
	test_y_ = test_y;
	test_size_ = test_size;
	int iter = 0;
	int bang = 0;

#ifdef GPU
	std::cout << "Testing with GPU " << std::endl;
#else
	std::cout << "Testing with CPU " << std::endl;
#endif // GPU
	this->mark.start();
	while (iter < test_size_) {
		int result = 0;
//#ifdef GPU // Use GPU
//		result = test_once_batch(iter*batch_size, batch_size);
//		//printf(" Running batch #%d, %d in %d is correct\n", iter, result, batch_size);
//#ifdef CHECK_RESULT     // Check result of batch operations
//		bool check = check_batch_result(batch_size);
//		if (check)
//		printf("  \\__ Results verified.\n");
//#endif // CHECK_RESULT
//#else  // Use CPU
//		if (batch_size == 1)
//		else {
//			std::cout << "Cannot run batch operations with CPU! Abording.."
//					<< std::endl;
//			return;
//		}
//#endif // GPU
		result = test_once(iter) ? 1 : 0;
		bang += result;
		iter++;
	}
	this->mark.stop();
	std::cout << "bang/test_size_: " << (float) bang / test_size_ << std::endl;
	std::cout << "Time spent testing " << this->test_size_ << " samples: "
			<< this->mark << std::endl;
}

//void ConvNet::test(vec2d_t test_x, vec_t test_y, size_t test_size) {
//	test(test_x, test_y, test_size, 1);
//}

void ConvNet::add_layer(Layer* layer) {
	if (!layers.empty())
		this->layers.back()->next = layer;
	this->layers.push_back(layer);
	layer->next = NULL;
}

#ifdef GPU
size_t ConvNet::max_iter(DeviceVector<float> v) {
	size_t i = 0;
	float_t max = v[0];
	std::cout << "v size " << v.size() << "\n";
	for (size_t j = 1; j < v.size(); j++) {
		if (v[j] > max) {
			max = v[j];
			i = j;
		}
	}
	//PEDRO eu usei cudaManaged malloc, entao
	//toda vez que acessa do host a memoria
	//ou em um kernel tem que fazer um cudaDeviceSynchronize
//	cudaError_t ret = cudaDeviceSynchronize();
//	CUDA_CHECK_RETURN(ret);
	return i;
}
#else
size_t ConvNet::max_iter(vec_host v) {
	size_t i = 0;
	float_t max = v[0];
	for (size_t j = 1; j < v.size(); j++) {
		if (v[j] > max) {
			max = v[j];
			i = j;
		}
	}
	return i;
}
#endif

size_t ConvNet::max_iter(float v[], size_t size) {
	size_t i = 0;
	float_t max = v[0]; //std::cout<< " raw output: "<<v[0]<<" ";
	for (size_t j = 1; j < size; j++) {
		//std::cout<< v[j] << " ";
		if (v[j] > max) {
			max = v[j];
			i = j;
		}
	}
	//std::cout<<std::endl;
	return i;
}

bool ConvNet::test_once_random() {
	int test_x_index = uniform_rand(0, test_size_ - 1);
	return test_once(test_x_index);
}

bool ConvNet::test_once(int test_x_index) {
	layers[0]->input_ = test_x_[test_x_index];
	for (auto layer : layers) {
		layer->forward();
		if (layer->next != nullptr) {
			layer->next->input_ = layer->output_;
		}
	}
	return (int) test_y_[test_x_index] == (int) max_iter(layers.back()->output_);
}


float_t ConvNet::train_once() {
	float_t err = 0;
	int iter = 0;
		//DEBUG:
		std::ofstream debugFile;
#ifdef GPU
		debugFile.open ("GPUdebugFile.txt");
#else //CPU
		debugFile.open ("CPUdebugFile.txt");
#endif
	//
	int test = 0;
	while (iter < M) {
		auto train_x_index = iter % train_size_;
		iter++;
		//std::cout << " train_x_index: " << train_x_index << std::endl; //debug
		//auto train_x_index = uniform_rand(0, train_size_ - 1);
		layers[0]->input_ = train_x_[train_x_index];
		layers.back()->exp_y = (int) train_y_[train_x_index];
		/*
		 Start forward feeding.
		 */
		int debugIter = 0;
		for (auto layer : layers) {

			//debug
			debugFile << "iter: "<<iter << " DEBUG layer" << debugIter << "->output: ";
			for(int i = 0; i < layer->output_.size(); i++){
				debugFile << layer->output_[i] << ", ";
			}
			debugFile << "\n";

			//std::cout << "layer w h d: " << layer->out_width_ << std::endl; //debug
			layer->forward();
			if (layer->next != nullptr) {
				layer->next->input_ = layer->output_;
			}

			debugIter++;
		}
		err += layers.back()->err;
		/*
		 back propgation
		 */
//		if(test++ > 2) break;
		for (auto i = layers.rbegin(); i != layers.rend(); i++) {
			(*i)->back_prop();
		}

	}

	debugFile.close();
	//end DEBUG

	return err / M;
}

void ConvNet::load_weights(std::string path) {
	FILE *in = fopen(path.c_str(), "rb");
//	std::ifstream in(path, std::ios::in | std::ios::binary | std::ios::ate);
	if (in != NULL) {
		for (auto i = layers.begin(); i != layers.end(); i++) {

			(*i)->load_layer(in);
		}
		fclose(in);
	} else {
		error("FAILED TO OPEN FILE " + path);
	}
}

void ConvNet::save_weights(std::string path) {
	FILE *fout = fopen(path.c_str(), "wb");
//	std::ofstream fout(path, std::ios::out | std::ios::binary);
	if (fout != NULL) {
		for (auto i = layers.begin(); i != layers.end(); i++) {
			(*i)->save_layer(fout);
		}
		fclose(fout);
	} else {
		error("FAILED TO OPEN FILE " + path);
	}

}

//void ConvNet::test(vec2d_t test_x, vec_t test_y, size_t test_size,
//		int batch_size) {
//	assert(batch_size > 0);
//	assert(test_size % batch_size == 0);
//	test_x_ = test_x, test_y_ = test_y, test_size_ = test_size;
//	int iter = 0;
//	int bang = 0;
//
//#ifdef GPU
//	std::cout << "Testing with batch size of " << batch_size << std::endl;
//#else
//	std::cout << "Testing with CPU " << std::endl;
//#endif // GPU
//	while (iter < test_size_ / batch_size) {
//		int result = 0;
//#ifdef GPU // Use GPU
//		result = test_once_batch(iter*batch_size, batch_size);
//		//printf(" Running batch #%d, %d in %d is correct\n", iter, result, batch_size);
//#ifdef CHECK_RESULT     // Check result of batch operations
//		bool check = check_batch_result(batch_size);
//		if (check)
//		printf("  \\__ Results verified.\n");
//#endif // CHECK_RESULT
//#else  // Use CPU
//		if (batch_size == 1)
//			result = test_once(iter) ? 1 : 0;
//		else {
//			std::cout << "Cannot run batch operations with CPU! Abording.."
//					<< std::endl;
//			return;
//		}
//#endif // GPU
//		bang += result;
//		iter++;
//	}
//	std::cout << "bang/test_size_: " << (float) bang / test_size_ << std::endl;
//}
//int ConvNet::test_once_random_batch(int batch_size) {
//	int test_x_index = uniform_rand(0, test_size_ - batch_size);
//	return test_once_batch(test_x_index, batch_size);
//}
//
//int ConvNet::test_once_batch(int test_x_index, int batch_size) {
//	//std::cout<<"test_x_index: "<<test_x_index<<std::endl;
//	layers.back()->exp_y_batch.resize(batch_size);
//	// concatenate input vectors into one vector
//	for (int s = 0; s < batch_size; s++) {
//		layers[0]->input_batch_.insert(layers[0]->input_batch_.end(),
//				test_x_[test_x_index + s].begin(),
//				test_x_[test_x_index + s].end());
//		layers.back()->exp_y_batch[s] = test_y_[test_x_index + s];
//	}
//
//	for (auto layer : layers) {
//		layer->forward_batch(batch_size);
//		if (layer->next != nullptr) {
//			layer->next->input_batch_ = layer->output_batch_;
//		}
//	}
//
//	// collect batch results
//	int count = 0;
//	int outlables = layers.back()->in_depth_;
//	for (int sample = 0; sample < batch_size; sample++)
//		if ((int) test_y_[test_x_index + sample]
//				== (int) max_iter(
//						&layers.back()->output_batch_[0] + sample * outlables,
//						outlables))
//			count++;
//	return count;
//}
//return true;

//bool ConvNet::check_batch_result(int batch_size) {
//	bool all_correct = true;
//	for (int sample = 0; sample < batch_size; sample++) {
//		int each_input_size = layers[0]->in_height_ * layers[0]->in_width_;
//		vec_t this_input = vec_t(
//				layers[0]->input_batch_.begin() + sample * each_input_size,
//				layers[0]->input_batch_.begin()
//						+ (sample + 1) * each_input_size);
//		layers[0]->input_ = this_input;
//		for (auto layer : layers) {
//			layer->forward();
//			if (layer->next != nullptr) {
//				layer->next->input_ = layer->output_;
//			}
//		}
//		vec_t output_batch = layers.back()->output_batch_;
//		vec_t this_output = layers.back()->output_;
//		int out_depth = layers.back()->in_depth_;
//		for (int out = 0; out < out_depth; out++) {
//			//printf("     Checking result of batch #%d out #%d...\n", batch, out);
//			float err = fabs(
//					this_output[out] - output_batch[out + sample * out_depth]);
//			if (err > 5e-3) {
//				printf(
//						"   !!==Wrong output. Sample: #%d, Out: #%d, should be: %f, batch result: %f\n",
//						sample, out, this_output[out],
//						output_batch[out + sample * out_depth]);
//				all_correct = false;
//			}
//		}
//		int outlables = layers.back()->in_depth_;
//		int cpu_result = (int) max_iter(layers.back()->output_);
//		int gpu_result = (int) max_iter(
//				&layers.back()->output_batch_[0] + sample * outlables,
//				outlables);
//		if (cpu_result != gpu_result)
//			printf("result #%d mismatch: CPU output: %d, GPU output: %d\n",
//					sample, cpu_result, gpu_result);
//	}
//	return all_correct;
//}
