/*
 * DetectionGold.cu
 *
 *  Created on: 28/09/2018
 *      Author: fernando
 */

#include "detection_gold.h"

#include <iterator>
#include "helpful.h"
#include <sstream>
#include <ctime>
#include <iostream>
#include <cmath>

/**
 * Detection Gold class
 */

void DetectionGold::write_gold_header() {
	//	0       1           2              3              4            5            6        7         8
	//	thresh; hier_tresh; img_list_size; img_list_path; config_file; config_data; model;weights;tensor_core;
	std::string gold_header = std::to_string(this->thresh) + ";";
	gold_header += std::to_string(this->hier_thresh) + ";";
	gold_header += std::to_string(this->plist_size) + ";";
	gold_header += this->img_list_path + ";";
	gold_header += this->config_file + ";";
	gold_header += this->cfg_data + ";";
	gold_header += this->model + ";";
	gold_header += this->weights + ";";
	gold_header += std::to_string(int(this->tensor_core_mode)) + ";";

	std::ofstream gold(this->gold_inout, std::ofstream::trunc);
	if (gold.is_open()) {
		gold << gold_header << std::endl;
		gold.close();
	} else {
		std::cerr << "ERROR ON OPENING GOLD OUTPUT FILE\n";
		exit(-1);
	}
}

DetectionGold::DetectionGold(int argc, char **argv, real_t thresh,
		real_t hier_thresh, char *img_list_path, char *config_file,
		char *config_data, char *model, char *weights) {
	char *def = nullptr;
	this->gold_inout = std::string(find_char_arg(argc, argv, "-gold", def));
	this->generate = find_int_arg(argc, argv, "-generate", 0);
	this->network_name = "darknet_v3_";
#if REAL_TYPE == HALF
	this->network_name += "half";
#elif REAL_TYPE == FLOAT
	this->network_name += "single";
#elif REAL_TYPE == DOUBLE
	this->network_name += "double";
#endif

	this->normalized_coordinates = find_int_arg(argc, argv, "-norm_coord", 0);

	this->iterations = find_int_arg(argc, argv, "-iterations", 1);
	this->tensor_core_mode = find_int_arg(argc, argv, "-tensor_cores", 0);
	this->stream_mr = find_int_arg(argc, argv, "-smx_redundancy", 0);
	this->thresh = thresh;
	this->hier_thresh = hier_thresh;
	this->total_errors = 0;

	std::cout << "Radiation test specific info\n";
	std::cout << "Norm. Coord.: " << this->normalized_coordinates << std::endl;
	std::cout << "Tensor cores: " << int(this->tensor_core_mode) << std::endl;
	std::cout << "SMX redundancy: " << this->stream_mr << std::endl;
	std::cout << "Radiation test mode: " << this->generate << std::endl;
	std::cout << "Gold path: " << this->gold_inout << std::endl;

	if (!this->generate) {

		//		Log(std::string gold, int save_layer, int abft, int iterations,
		//				std::string app, unsigned char use_tensor_core_mode)
		this->app_log->start_log(this->gold_inout, 0, 0, this->iterations,
				this->network_name, this->tensor_core_mode);

		//	detection gold;
		std::string line;
		std::ifstream gold_file(this->gold_inout, std::ifstream::in);
		if (gold_file.is_open()) {
			getline(gold_file, line);
		} else {
			std::cerr << "ERROR ON OPENING GOLD FILE\n";
			exit(-1);
		}

		std::vector<std::string> split_ret = split(line, ';');
		//	0       1           2              3              4            5            6      7         8
		//	thresh; hier_tresh; img_list_size; img_list_path; config_file; config_data; model;weights;tensor_core;
		this->thresh = std::stof(split_ret[0]);
		this->hier_thresh = std::stof(split_ret[1]);
		this->plist_size = std::stoi(split_ret[2]);
		this->img_list_path = split_ret[3];
		this->config_file = split_ret[4];
		this->cfg_data = split_ret[5];
		this->model = split_ret[6];
		this->weights = split_ret[7];
		this->tensor_core_mode = std::stoi(split_ret[8]);

		//allocate detector
		this->load_gold_hash(gold_file);

		gold_file.close();
	} else {
		this->img_list_path = std::string(img_list_path);

		//reading the img list path content
		std::ifstream tmp_img_file(this->img_list_path);
		std::copy(std::istream_iterator<std::string>(tmp_img_file),
				std::istream_iterator<std::string>(),
				std::back_inserter(this->gold_img_names));

		this->plist_size = this->gold_img_names.size();
		this->config_file = std::string(config_file);
		this->cfg_data = std::string(config_data);
		this->model = std::string(model);
		this->weights = std::string(weights);

		this->write_gold_header();
		this->iterations = 1;
	}
}

bool operator!=(const box& a, const box& b) {
	real_t x_diff = std::abs((a.x - b.x));
	real_t y_diff = std::abs((a.y - b.y));
	real_t w_diff = std::abs((a.w - b.w));
	real_t h_diff = std::abs((a.h - b.h));

	return (x_diff > THRESHOLD_ERROR || y_diff > THRESHOLD_ERROR
			|| w_diff > THRESHOLD_ERROR || h_diff > THRESHOLD_ERROR);
}

bool operator!=(const std::tuple<real_t, real_t, real_t, real_t> f,
		std::tuple<real_t, real_t, real_t, real_t> g) {

	real_t x1_diff = std::abs(std::get<0>(f) - std::get<0>(g));
	real_t x2_diff = std::abs(std::get<1>(f) - std::get<1>(g));
	real_t x3_diff = std::abs(std::get<2>(f) - std::get<2>(g));
	real_t x4_diff = std::abs(std::get<3>(f) - std::get<3>(g));

	return (x1_diff > THRESHOLD_ERROR_INTEGER
			|| x2_diff > THRESHOLD_ERROR_INTEGER
			|| x3_diff > THRESHOLD_ERROR_INTEGER
			|| x4_diff > THRESHOLD_ERROR_INTEGER);
}

int DetectionGold::compare_line(real_t g_objectness, real_t f_objectness,
		int g_sort_class, int f_sort_class, const box& g_box, const box& f_box,
		const std::string& img, int nb, int classes, const Detection& g_det,
		detection f_det, int img_w, int img_h) {

	real_t objs_diff = std::abs(g_objectness - f_objectness);
	int sortc_diff = std::abs(g_sort_class - f_sort_class);
	std::tuple<real_t, real_t, real_t, real_t> g_box_tuple;
	std::tuple<real_t, real_t, real_t, real_t> f_box_tuple;

	std::ostringstream error_info("");
	error_info.precision(STORE_PRECISION);
	int error_count = 0;

	if (this->normalized_coordinates) {
		real_t left = std::max((g_box.x - g_box.w / 2.) * img_w, 0.0);
		real_t right = std::min((g_box.x + g_box.w / 2.) * img_w, img_w - 1.0);
		real_t top = std::max((g_box.y - g_box.h / 2.) * img_h, 0.0);
		real_t bot = std::min((g_box.y + g_box.h / 2.) * img_h, img_h - 1.0);
		g_box_tuple = {left, right, top, bot};

		left = std::max((f_box.x - f_box.w / 2.) * img_w, 0.0);
		right = std::min((f_box.x + f_box.w / 2.) * img_w, img_w - 1.0);
		top = std::max((f_box.y - f_box.h / 2.) * img_h, 0.0);
		bot = std::min((f_box.y + f_box.h / 2.) * img_h, img_h - 1.0);
		f_box_tuple = {left, right, top, bot};

		if ((objs_diff > THRESHOLD_ERROR) || (sortc_diff > THRESHOLD_ERROR)
				|| (g_box_tuple != f_box_tuple)) {
			error_count++;
		}
//		if (left < 0)	left = 0;
//		if (right > im.w - 1)	right = im.w - 1;
//		if (top < 0)	top = 0;
//		if (bot > im.h - 1)	bot = im.h - 1;

	} else {
		if ((objs_diff > THRESHOLD_ERROR) || (sortc_diff > THRESHOLD_ERROR)
				|| (g_box != f_box)) {
			error_count++;
		}
	}

	if (error_count > 0) {
		error_info << "img: " << img << " detection: " << nb << " x_e: "
				<< g_box.x << " x_r: " << f_box.x << " y_e: " << g_box.y
				<< " y_r: " << f_box.y << " h_e: " << g_box.h << " h_r: "
				<< f_box.h << " w_e: " << g_box.w << " w_r: " << f_box.w
				<< " objectness_e: " << g_objectness << " objectness_r: "
				<< f_objectness << " sort_class_e: " << g_sort_class
				<< " sort_class_r: " << f_sort_class << " img_w: " << img_w
				<< " img_h: " << img_h;
		this->app_log->log_error_info(error_info.str());
	}

	for (int cl = 0; cl < classes; ++cl) {
		real_t g_prob = g_det.prob[cl];
		real_t f_prob = f_det.prob[cl];
		real_t prob_diff = std::abs(g_prob - f_prob);
		if ((g_prob >= this->thresh || f_prob >= this->thresh)
				&& prob_diff > THRESHOLD_ERROR) {
			std::ostringstream error_info("");
			error_info.precision(STORE_PRECISION);
			error_info << "img: " << img << " detection: " << nb << " class: "
					<< cl << " prob_e: " << g_prob << " prob_r: " << f_prob;
			this->app_log->log_error_info(error_info.str());
			error_count++;
		}
	}
	return error_count;
}

int DetectionGold::cmp(detection* found_dets, int nboxes, int img_index,
		int classes, int img_w, int img_h) {
	std::string img = this->gold_img_names[img_index];
	std::vector<Detection> gold_dets = this->gold_hash_var[img];

	int error_count = 0;

	for (int nb = 0; nb < nboxes; nb++) {
		Detection g_det = gold_dets[nb];
		detection f_det = found_dets[nb];

		box g_box = g_det.bbox;
		box f_box = f_det.bbox;

		real_t g_objectness = g_det.objectness;
		real_t f_objectness = f_det.objectness;

		int g_sort_class = g_det.sort_class;
		int f_sort_class = f_det.sort_class;

		error_count = this->compare_line(g_objectness, f_objectness,
				g_sort_class, f_sort_class, g_box, f_box, img, nb, classes,
				g_det, f_det, img_w, img_h);
	}
	this->total_errors += error_count;
	if (this->total_errors > MAX_ERROR_COUNT) {
		this->app_log->update_error_count(this->total_errors);
		exit(-1);
	}
	this->app_log->update_error_count(error_count);

	return error_count;
}

int DetectionGold::run(detection *dets, int nboxes, int img_index, int classes,
		int img_w, int img_h) {
	int ret = 0;
	// To generate function
	//std::string img, detection* dets, int nboxes, int classes, int l_coord
	if (this->generate) {

		std::ofstream gold_file(this->gold_inout, std::ofstream::app);
		if (!gold_file.is_open()) {
			std::cerr << "ERROR ON OPENING GOLD FILE\n";
			exit(-1);
		}
		this->gen(dets, nboxes, img_index, gold_file, classes);
		gold_file.close();
	} else {
		// To compare function
		//detection is allways nboxes size
		ret = this->cmp(dets, nboxes, img_index, classes, img_w, img_h);
	}
	return ret;
}

std::ostringstream DetectionGold::generate_gold_line(int bb, detection det,
		const box& b, detection* dets) {
	std::ostringstream box_str("");
	box_str.precision(STORE_PRECISION);

	//TODO: To be implemented
	if (this->normalized_coordinates) {
		real_t left = std::max((g_box.x - g_box.w / 2.) * img_w, 0.0);
		real_t right = std::min((g_box.x + g_box.w / 2.) * img_w, img_w - 1.0);
		real_t top = std::max((g_box.y - g_box.h / 2.) * img_h, 0.0);
		real_t bot = std::min((g_box.y + g_box.h / 2.) * img_h, img_h - 1.0);
	}

	box_str << dets[bb].objectness << ";" << det.sort_class << ";" << b.x << ";"
			<< b.y << ";" << b.w << ";" << b.h << ";" << det.classes << ";"
			<< std::endl;
	for (int cl = 0; cl < det.classes; cl++) {
		real_t prob = det.prob[cl];
		box_str << prob << ";";
	}
	return box_str;
}

void DetectionGold::gen(detection *dets, int nboxes, int img_index,
		std::ofstream& gold_file, int classes) {
	//first write the image string name
	std::string img = this->gold_img_names[img_index];

	gold_file << img << ";" << nboxes << ";" << std::endl;

	for (int bb = 0; bb < nboxes; bb++) {
		detection det = dets[bb];
		box b = det.bbox;

		std::ostringstream box_str = this->generate_gold_line(bb, det, b, dets);
		gold_file << box_str.str() << std::endl;
	}

}

void DetectionGold::load_gold_hash(std::ifstream& gold_file) {
//allocate detector
	this->gold_img_names = std::vector<std::string>(this->plist_size);
	std::string line;

	for (int i = 0; i < this->plist_size; i++) {

		//	gold_file << img << ";" << nboxes << ";" << std::endl;
		getline(gold_file, line);
		std::vector<std::string> splited_line = split(line, ';');

		// Set each img_name path
		this->gold_img_names[i] = splited_line[0];

		// Probarray creation

		int nboxes = std::stoi(splited_line[1]);

		std::vector<Detection> detections(nboxes, Detection());

		for (int bb = 0; bb < nboxes; ++bb) {

			// Getting bb box
			box b;
			getline(gold_file, line);

			splited_line = split(line, ';');
			real_t objectness = std::stof(splited_line[0]);
			int sort_class = std::stoi(splited_line[1]);
			b.x = std::stof(splited_line[2]);
			b.y = std::stof(splited_line[3]);
			b.w = std::stof(splited_line[4]);
			b.h = std::stof(splited_line[5]);
			int classes = std::stoi(splited_line[6]);

			// Getting the probabilities
			std::vector<real_t> probs(classes, 0.0);

			if (getline(gold_file, line)) {
				splited_line = split(line, ';');

				for (auto cl = 0; cl < classes; cl++) {
					probs[cl] = std::stof(splited_line[cl]);
				}
			}
			detections[bb] = Detection(classes, nboxes, sort_class, objectness,
					probs, b);
		}

		this->gold_hash_var.put(this->gold_img_names[i], detections);
	}

}

DetectionGold::~DetectionGold() {
	if (!this->generate) {
#ifdef LOGS
		end_log_file();
#endif
		delete this->app_log;
	}

}
//
//void DetectionGold::start_iteration() {
//	if (!this->generate) {
//#ifdef LOGS
//		start_iteration();
//#endif
//	}
//}
//
//void DetectionGold::end_iteration() {
//	if (!this->generate) {
//#ifdef LOGS
//		end_iteration();
//#endif
//	}
//
//	this->current_iteration++;
//}

//void DetectionGold::start_log(std::string gold, int save_layer, int abft,
//		int iterations, std::string app, unsigned char use_tensor_core_mode) {
//#ifdef LOGS
//	std::string test_info = std::string("gold_file: ") + gold;
//
//	test_info += " save_layer: " + std::to_string(save_layer) + " abft_type: ";
//
//	test_info += std::string(ABFT_TYPES[abft]) + " iterations: "
//	+ std::to_string(iterations);
//
//	test_info += " tensor_core_mode: "
//	+ std::to_string(int(use_tensor_core_mode));
//
//	set_iter_interval_print(10);
//
//	start_log_file(const_cast<char*>(app.c_str()),
//			const_cast<char*>(test_info.c_str()));
//#endif
//}

//void DetectionGold::update_timestamp_app() {
//#ifdef LOGS
//	update_timestamp();
//#endif
//}

//void DetectionGold::log_error_info(std::string error_detail) {
//#ifdef LOGS
//	log_error_detail(const_cast<char*>(error_detail.c_str()));
//#endif
//}

//void DetectionGold::update_error_count(long error_count) {
//#ifdef LOGS
//	if(error_count)
//	log_error_count(error_count);
//#endif
//}
