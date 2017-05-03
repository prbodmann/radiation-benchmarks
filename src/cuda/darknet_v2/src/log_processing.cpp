/*
 * log_processing.cpp
 *
 *  Created on: 30/04/2017
 *      Author: fernando
 */

#include "log_processing.h"
#include <fstream>
#include <list>
#include <vector>
#include <string>
#include "helpful.h"

#ifdef LOGS
#include "log_helper.h"
#endif

void start_count_app(char *test, char *app) {
#ifdef LOGS
	char test_info[500];
	snprintf(test_info, 500, "gold_file: %s", test);

	start_log_file(app, test_info);
#endif
}

void finish_count_app() {
#ifdef LOGS
	end_log_file();
#endif
}

void saveLayer(network net, int iterator, int n) {

}

void compareLayer(layer l, int i) {

}

char** get_image_filenames(char *img_list_path, int *image_list_size) {
	std::list < std::string > data;
	std::string line;
	std::ifstream img_list_file(img_list_path);
	if (img_list_file.is_open()) {
		while (getline(img_list_file, line)) {
			data.push_back(line);
		}
		img_list_file.close();
	}
	char** array = (char**) malloc(sizeof(char*) * data.size());

	for (std::list<std::string>::const_iterator it = data.begin();
			it != data.end(); ++it) {
		char *temp = (char*) malloc(it->size() * sizeof(char));
		strcpy(temp, it->c_str());
		array[*image_list_size] = temp;
		(*image_list_size)++;
	}

	// use the array

	return array;
}

/**
 * it was adapted from max_index in utils.c line 536image load_image(char *filename, int w, int h, int c);
 * auxiliary funtion for save_gold
 */

inline int get_index(float *a, int n) {
	if (n <= 0)
		return -1;
	int i, max_i = 0;
	float max = a[0];
	for (i = 1; i < n; ++i) {
		if (a[i] > max) {
			max = a[i];
			max_i = i;
		}
	}
	return max_i;
}

/**
 * it was addapted from draw_detections in image.c line 174
 * it saves a gold file for radiation tests
 */
void save_gold(FILE *fp, int w, int h, int num, float thresh, box *boxes,
		float **probs, int classes) {
	int i;

	for (i = 0; i < num; ++i) {
		int class_ = get_index(probs[i], classes);
		float prob = probs[i][class_];
		if (prob > thresh) {
			int width = h * .012;
//			printf("%d: %.0f%%\n", class_, prob * 100);
//			int offset = class_ * 123457 % classes;
//			float red = get_color(2, offset, classes);
//			float green = get_color(1, offset, classes);
//			float blue = get_color(0, offset, classes);
//			float rgb[3];

//			rgb[0] = red;
//			rgb[1] = green;
//			rgb[2] = blue;
			box b = boxes[i];

			float left = (b.x - b.w / 2.) * float(w);
			float right = (b.x + b.w / 2.) * float(w);
			float top = (b.y - b.h / 2.) * float(h);
			float bot = (b.y + b.h / 2.) * float(h);

			if (left < 0)
				left = 0;
			if (right > w - 1)
				right = w - 1;
			if (top < 0)
				top = 0;
			if (bot > h - 1)
				bot = h - 1;

			//will save to fp file in this order
			//class number, left, top, right, bottom, prob (confidence)
			fprintf(fp, "%d;%f;%f;%f;%f;%f\n", class_, left, top, right, bot,
					prob);

		}
	}
}

detection load_gold(Args *arg) {
	detection gold;
	std::list < std::string > data;
	std::string line;
	std::ifstream img_list_file(arg->gold_inout);
	if (img_list_file.is_open()) {
		while (getline(img_list_file, line)) {
			data.push_back(line);
		}
	}

//	reading only header
	std::string header = data.front();
	data.pop_front();
	std::vector < std::string > split_ret = split(header, ';');
//	0       1           2              3              4            5            6
//	thresh; hier_tresh; img_list_size; img_list_path; config_file; config_data; model;weights
	arg->thresh = atof(split_ret[0].c_str());
	arg->hier_thresh = atof(split_ret[1].c_str());
	gold.img_list_size = atoi(split_ret[2].c_str());
	arg->img_list_path = const_cast<char*>(split_ret[3].c_str());
	arg->config_file = const_cast<char*>(split_ret[4].c_str());
	arg->cfg_data = const_cast<char*>(split_ret[5].c_str());
	arg->model = const_cast<char*>(split_ret[6].c_str());
//	char *temp = (char*)malloc(sizeof(char) * split_ret[7].size());
//	strcpy(temp, split_ret[7].c_str());
	arg->weights = const_cast<char*>(split_ret[7].c_str());


	for (std::list<std::string>::const_iterator it = data.begin();
			it != data.end(); ++it) {

	}
	return gold;
}

void delete_detection_var(detection *det) {
	if (det->image_names) {
		int i;
		for (i = 0; i < det->img_list_size; i++) {
			free(det->image_names[i]);
		}
		free(det->image_names);
	}

	if (det->detection_result)
		free(det->detection_result);

}

void clear_boxes_and_probs(box *boxes, float **probs, int n) {
	memset(boxes, 0, sizeof(box) * n);
	for (int i = 0; i < n; i++) {
		memset(probs[i], 0, sizeof(float) * n);
	}
}
