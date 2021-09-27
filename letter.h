//
// Copyright (c) 2021 Moss Gallagher.
//

#pragma once
#include <cmath>
#include <cstring>
#include <utility>
#include <vector>
#include "font.h"
#include "buf.h"
#include "cff.h"


namespace mka {
	class font;

	class letter{
	public:
		char ch;
		int min_x, min_y;
		point scale;
		std::vector<bezier_curve> edges;
		bitmap pixels{};

	private:
		font* parent_font;


	public:
		explicit letter(font* parent_font) : parent_font(parent_font) {
			this->ch = '\0';
		}

		letter(font* parent, char ch) : parent_font(parent), ch(ch) {
			letter::generate_edges();
		}


		//letter(font& parent, std::vector<edge> edges) : parent_font(parent), edges(std::move(edges)) {}

		void generate_edges();

		void rasterize_edges();

		void calculate_bounding_box();
	};
}



