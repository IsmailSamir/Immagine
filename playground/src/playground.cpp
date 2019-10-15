#include <iostream>

#include "Immagine/Image.h"
#include "Immagine/Point_Processing.h"
#include "Immagine/Convolution.h"
#include "Immagine/Disjoint_Set.h"
#include "Immagine/Utilities.h"

#include <chrono>
#include <string>

#include <map>
#include <vector>
#include <algorithm>
#include <random>
#include <vld.h>

using namespace std::chrono;
using namespace immagine;
using namespace std;

inline static Image
color_image(uint32_t*** vec, size_t width, size_t height)
{
	Image self = image_new(width, height, 3);

	std::map<uint8_t, std::vector<std::pair<size_t, size_t>>> map;

	for (size_t i = 0; i < self.height; ++i)
		for (size_t j = 0; j < self.width; ++j)
			if(vec[i][j][0] > 0)
				map[vec[i][j][0]].push_back(make_pair(i, j));

	std::vector<uint8_t> color(3);
	color[0] = 255;
	color[1] = 150;
	color[2] = 50;
	for (auto it = map.begin(); it != map.end(); ++it) {
		for (auto pair : it->second) {
			self(pair.first, pair.second, 0) = color[0];
			self(pair.first, pair.second, 1) = color[1];
			self(pair.first, pair.second, 2) = color[2];
		}

		color[0] = uint8_t(std::rand() % 255); // red component of color
		color[1] = uint8_t(std::rand() % 255); // green component of color
		color[2] = uint8_t(std::rand() % 255); // blue component of color
	}
	return self;
}

int
main(int argc, char** argv)
{
	string file_path = string(IMAGE_DIR) + string("/images/7.jpg");
	Image image = image_load(file_path.c_str());
	
	Image img1 = image_grayscale(image);
	Image img2 = image_binarize(img1);

	auto start = high_resolution_clock::now();

	uint32_t*** img3 = image_connected_component(img2);

	auto stop = high_resolution_clock::now();
	auto duration = duration_cast<seconds>(stop - start);
	printf("Time = %lld  seconds\n", duration.count());

	Image result = color_image(img3, image.width, image.height);

	string out_path = string(IMAGE_DIR) + string("/images/result.bmp");
	image_save(out_path.c_str(), result, IMAGE_FORMAT::BMP);

	array3d_free(img3, image.width, image.height, 1);
	image_free(image);
	image_free(img1);
	image_free(img2);
	image_free(result);
	return 0;
}
