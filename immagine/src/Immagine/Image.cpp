#pragma once

#include "Immagine/Image.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "Immagine/Disjoint_Set.h"

#include <map>

using namespace std;

namespace immagine
{
	// Helper Functions
	inline static size_t
	_image_red_idx(const Image& image)
	{
		size_t index = image.width * image.height * 0;
		return index;
	}

	inline static size_t
	_image_green_idx(const Image& image)
	{
		size_t index = image.width * image.height * 1;
		return index;
	}

	inline static size_t
	_image_blue_idx(const Image& image)
	{
		size_t index = image.width * image.height * 2;
		return index;
	}

	inline static size_t
	_image_alpha_idx(const Image& image)
	{
		size_t index = image.width * image.height * 3;
		return index;
	}
	
	inline static Image
	_image_data_fork(unsigned char* data, uint32_t width, uint32_t height, uint8_t channels)
	{
		Image self = image_new(width, height, channels);

		size_t j = 0;
		size_t size = width * height * channels;
		for (uint8_t c = 0; c < channels; ++c)
			for (size_t i = c; i < size; i += channels)
				self.data[j++] = data[i];

		return self;
	}

	inline static Image
	_image_data_join(const Image& image)
	{
		Image self = image_new(image.width, image.height, image.channels);

		size_t j = 0;
		size_t size = image.width * image.height * image.channels;
		for (uint8_t c = 0; c < image.channels; ++c)
			for (size_t i = c; i < size; i += image.channels)
				self.data[i] = image.data[j++];

		return self;
	}

	inline static Image
	_image_resize_nearest_neighbour(const Image& image, uint32_t width, uint32_t height)
	{
		Image self = image_new(width, height, image.channels);

		float width_ratio = (float)image.width / (float)width;
		float height_ratio = (float)image.height / (float)height;

		for (uint8_t k = 0; k < image.channels; ++k)
			for (size_t i = 0; i < height; ++i)
				for (size_t j = 0; j < width; ++j)
					self(i, j, k) = image(size_t(floor(i * height_ratio)), size_t(floor(j * width_ratio)), k);

		return self;
	}

	inline static Image
	_image_resize_bilinear(const Image& image, uint32_t width, uint32_t height)
	{
		Image self = image_new(width, height, image.channels);

		float width_ratio = (float)image.width / (float)width;
		float height_ratio = (float)image.height / (float)height;

		for (uint8_t k = 0; k < image.channels; ++k)
			for (size_t i = 0; i < height - 1; ++i)
				for (size_t j = 0; j < width - 1; ++j)
				{
					size_t src_i = (size_t)(i * height_ratio);
					size_t src_j = (size_t)(j * width_ratio);

					float x_dist = (width_ratio  * j) - src_j;
					float y_dist = (height_ratio * i) - src_i;

					uint8_t p0 = image(src_i, src_j, k);
					uint8_t p1 = (src_j + 1 < image.width) ? image(src_i, src_j + 1, k) : 0;
					uint8_t p2 = (src_i + 1 < image.height) ? image(src_i + 1, src_j, k) : 0;
					uint8_t p3 = ((src_i + 1 < image.height) && (src_j + 1 < image.width)) ? image(src_i + 1, src_j + 1, k) : 0;

					float lp0 = p0 * (1 - x_dist) + p1 * x_dist;
					float lp1 = p2 * (1 - x_dist) + p3 * x_dist;

					uint32_t color = (uint32_t)(lp0 * (1 - y_dist) + lp1 * y_dist);
					self(i, j, k) = (color < BLACK) ? BLACK : (color > WHITE) ? WHITE : color;
				}

		return self;
	}


	// API
	Image
	image_new(uint32_t width, uint32_t height, uint8_t channels)
	{
		Image self{};

		self.width = width;
		self.height = height;
		self.channels = channels;
		self.data = (uint8_t*)::malloc(width * height * channels * sizeof(uint8_t));

		::memset(self.data, 0, width * height * channels);

		return self;
	}

	Image
	image_clone(const Image& image)
	{
		Image self = image_new(image.width, image.height, image.channels);

		::memcpy(self.data, image.data, image.width * image.height * image.channels * sizeof(uint8_t));

		return self;
	}

	Image
	image_from_ptr(const void* data, uint32_t width, uint32_t height, uint8_t channels)
	{
		Image self = image_new(width, height, channels);

		::memcpy(self.data, data, width * height * channels * sizeof(uint8_t));

		return self;
	}

	void
	image_free(Image& self)
	{
		if (self.data) 
			::free(self.data);
		self.data = nullptr;
	}

	Image
	image_load(const char* file_path)
	{
		int width, height, channels;
		unsigned char* data = stbi_load(file_path, &width, &height, &channels, 0);

		assert(data != NULL && "Can't open the image");

		Image self = _image_data_fork(data, width, height, channels);

		STBI_FREE(data);

		return self;
	}

	bool
	image_save(const char* file_path, const Image& image, IMAGE_FORMAT format)
	{
		Image self = _image_data_join(image);

		bool result;
		switch (format)
		{
		case IMAGE_FORMAT::BMP:
			result = stbi_write_bmp(file_path, int(self.width), int(self.height), int(self.channels), self.data);
			break;

		case IMAGE_FORMAT::PNG:
			result = stbi_write_png(file_path, int(self.width), int(self.height), int(self.channels), self.data, 0);
			break;

		case IMAGE_FORMAT::JPEG:
			result = stbi_write_jpg(file_path, int(self.width), int(self.height), int(self.channels), self.data, 0);
			break;

		default:
			assert(false && "Unsupported image format\n");
			result = false;
			break;
		}

		image_free(self);

		return result;
	}

	Image
	image_red_channel(const Image& image)
	{
		assert(image.channels >= 3 && "image is grayscale with 8-bit depth");

		Image self = image_new(image.width, image.height, 1);

		size_t j = _image_red_idx(image);
		size_t size = image.width * image.height;

		for (size_t i = 0; i < size; ++i, ++j)
			self.data[i] = image.data[j];

		return self;
	}

	Image
	image_green_channel(const Image& image)
	{
		assert(image.channels >= 3 && "image is grayscale with 8-bit depth");

		Image self = image_new(image.width, image.height, 1);

		size_t j = _image_green_idx(image);
		size_t size = image.width * image.height;

		for (size_t i = 0; i < size; ++i, ++j)
			self.data[i] = image.data[j];

		return self;
	}

	Image
	image_blue_channel(const Image& image)
	{
		assert(image.channels >= 3 && "image dosen't have blue channel");

		Image self = image_new(image.width, image.height, 1);

		size_t j = _image_blue_idx(image);
		size_t size = image.width * image.height;

		for (size_t i = 0; i < size; ++i, ++j)
			self.data[i] = image.data[j];

		return self;
	}

	Image
	image_alpha_channel(const Image& image)
	{
		assert(image.channels == 4 && "image dosen't have an alpha channel");

		Image self = image_new(image.width, image.height, 1);

		size_t j = _image_alpha_idx(image);
		size_t size = image.width * image.height;

		for (size_t i = 0; i < size; ++i, ++j)
			self.data[i] = image.data[j];

		return self;
	}

	Image
	image_grayscale(const Image& image)
	{
		// Image is already grayscale image
		if (image.channels == 1) {
			Image self = image_clone(image);
			return self;
		}

		Image self = image_new(image.width, image.height, 1);

		size_t r = _image_red_idx(image);
		size_t g = _image_green_idx(image);
		size_t b = _image_blue_idx(image);
		size_t size = image.width * image.height;

		for(size_t i = 0; i < size; ++i, ++r, ++g, ++b)
			self.data[i] = uint8_t(float((image.data[r]) + (image.data[g]) + (image.data[b])) / 3.0f);

		return self;
	}

	Image
	image_flip_horizontally(const Image& image)
	{
		Image self = image_new(image.width, image.height, image.channels);

		for (uint8_t k = 0; k < image.channels; ++k)
			for (size_t i = 0; i < image.height; ++i)
				for (size_t j = 0; j < image.width; ++j)
					self(i, j, k) = image(i, image.width - 1 - j, k);

		return self;
	}

	Image
	image_flip_vertically(const Image& image)
	{
		Image self = image_new(image.width, image.height, image.channels);

		for (uint8_t k = 0; k < image.channels; ++k)
			for (size_t i = 0; i < image.height; ++i)
				for (size_t j = 0; j < image.width; ++j)
					self(i, j, k) = image(image.height - 1 - i, j, k);

		return self;
	}

	Image
	image_mirror(const Image& image)
	{
		return image_flip_horizontally(image);
	}

	Image
	image_rotate_right(const Image& image)
	{
		Image self = image_new(image.height, image.width, image.channels);

		for (uint8_t k = 0; k < image.channels; ++k)
			for (size_t i = 0; i < image.height; ++i)
				for (size_t j = 0; j < image.width; ++j)
					self(j, image.height - 1 - i, k) = image(i, j, k);

		return self;
	}

	Image
	image_rotate_left(const Image& image)
	{
		Image self = image_new(image.height, image.width, image.channels);

		for (uint8_t k = 0; k < image.channels; ++k)
			for (size_t i = 0; i < image.height; ++i)
				for (size_t j = 0; j < image.width; ++j)
					self(image.width - 1 - j, i, k) = image(i, j, k);

		return self;
	}

	Image
	image_resize(const Image & image, uint32_t width, uint32_t height, INTERPOLATION_METHOD method)
	{
		switch (method)
		{
		case INTERPOLATION_METHOD::NEAREST_NEIGHBOUR:
			return _image_resize_nearest_neighbour(image, width, height);

		case INTERPOLATION_METHOD::BILINEAR:
			return _image_resize_bilinear(image, width, height);

		default:
			assert(false && "Unreachable state");
			return Image();
		}
	}

	Image
	image_crop(const Image & image, const Rectangle& rect)
	{
		Image self = image_new(rect.width, rect.height, image.channels);

		for (uint8_t k = 0; k < self.channels; ++k)
			for (size_t i = rect.y; i < rect.y + rect.height; ++i)
				for (size_t j = rect.x; j < rect.x + rect.width; ++j)
					self(i - rect.y, j - rect.x, k) = image(i, j, k);

		return self;
	}

	Image
	image_pad(const Image& image, uint32_t expand_w, uint32_t expand_h, uint8_t val)
	{
		Image self = image_new(image.width + 2 * expand_w, image.height + 2 * expand_h, image.channels);

		::memset(self.data, val, self.width * self.height * self.channels);

		for (uint8_t k = 0; k < self.channels; ++k)
			for (size_t i = expand_h; i < self.height - expand_h; ++i)
				for (size_t j = expand_w; j < self.width - expand_w; ++j)
					self(i, j, k) = image(i - expand_h, j - expand_w, k);

		return self;
	}

	Image
	image_binarize(const Image& image)
	{
		Image self = image_new(image.width, image.height, image.channels);

		size_t size = image.width * image.height * image.channels;
		size_t sum = 0;
		for (size_t i = 0; i < size; ++i)
			sum += image.data[i];

		uint8_t threshold = ((float)sum / (float)size + 0.5f);
		for (size_t i = 0; i < size; ++i)
			self.data[i] = (image.data[i] > threshold) ? WHITE: BLACK;

		return self;
	}

	struct Label_Info{
		uint8_t left;
		uint8_t above;
		uint8_t count;
	};

	inline static Label_Info
	_neighbouring_labels(const Image& image, size_t i, size_t j)
	{
		Label_Info labels{};

		// Pixel is not on top edge of image
		if (i > 0)
		{
			//It's a labelled pixel
			if (image(i - 1, j) > 0) 
			{
				labels.above = image(i - 1, j);
				labels.count++;
			}
		}

		// Pixel is not on left edge of image
		if (j > 0)
		{
			//It's a labelled pixel
			if (image(i, j - 1) > 0)
			{
				labels.left = image(i, j - 1);
				labels.count++;
			}
		}

		return labels;
	}

	Image
	image_connected_component(const Image & image)
	{
		Image self = image_new(image.width, image.height, image.channels);

		Disjoint_Set uf = disjoint_set_new();
		uint8_t current_label = 1;

		//1st Pass : label image and record label equivalences
		for (size_t i = 0; i < image.height; ++i) {
			for (size_t j = 0; j < image.width; ++j) {

				//background
				if (image(i, j) == 0) {
					self(i, j) = 0;
					continue;
				}

				Label_Info labels = _neighbouring_labels(self, i, j);
				
				//If no neighbouring foreground pixels, new label->use current_label
				if (labels.count == 0)
				{
					self(i, j) = current_label;	
					//record label in disjoint set
					disjoint_set_make(uf, current_label);
					++current_label;
				}
				else
				{
					uint8_t smallest_label = (labels.left < labels.above) ? labels.left : labels.above;
					self(i, j) = smallest_label;

					//# More than one type of label in component
					if (labels.count > 1 && labels.left != labels.above)
					{
						//add equivalence class
						disjoint_set_union(uf, smallest_label, labels.left);
						disjoint_set_union(uf, smallest_label, labels.above);
					}
				}
			}
		}

		std::map<uint8_t, uint8_t> final_labels;
		uint8_t new_label_number = 1;

		//2nd Pass: replace labels with their root labels
		for (size_t i = 0; i < self.height; ++i) {
			for (size_t j = 0; j < self.width; ++j) {

				if (self(i, j) > 0)
				{
					uint8_t new_label = disjoint_set_find(uf, self(i, j));
					self(i, j) = new_label;

					if (final_labels.find(new_label) == final_labels.end())
					{
						final_labels[new_label] = new_label_number;
						++new_label_number;
					}
				}
			}
		}

		for (size_t i = 0; i < self.height; ++i) {
			for (size_t j = 0; j < self.width; ++j) {
				if (self(i, j) > 0)
					self(i, j) = final_labels[self(i, j)];
			}
		}

		return self;
	}
}