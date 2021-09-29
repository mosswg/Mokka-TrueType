//
// Copyright (c) 2021 Moss Gallagher.
//

#pragma once
#include <cmath>
#include <cstring>
#include <utility>
#include <vector>
#include "font.h"
#include "cff.h"


namespace mka {
	class font;

	class letter{
	public:
		char ch;
		point scale;
		std::vector<bezier_curve> edges;
		bitmap pixels{};

	protected:
		font* parent_font{};



	public:

		explicit letter(font* parent_font) : parent_font(parent_font) {
			this->ch = '\0';
		}

		letter(font* parent, char ch) : ch(ch), parent_font(parent) {}

		point get_size() const {
			return {this->pixels.size.x * scale.x, this->pixels.size.y * this->scale.y};
		}

		void generate_edges();

		void rasterize_edges();

		void calculate_bounding_box();
	};
}



