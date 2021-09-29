//
// Copyright (c) 2021 Moss Gallagher.
//

#include "src/font.h"
#include "src/cff.h"
#include "src/letter.h"


int main() {

	mka::font a("res/JetBrainsMonoNL-Medium.ttf");

	mka::bezier_curve* result;

	int num_verts = a.GetGlyphShape(a.find_glyph_index('A'), &result);

	mka::letter& lA = a.get_letter('a', 20);

	const char *bits[] = {" ", "ı", "▄", "■", "░", "▒", "▓", "█"};

	mka::point size = lA.get_size();

	std::cout << size.to_string() << std::endl;

	for (int j = size.x - 1; j >= 0; j--) {
		for (int i = 0; i < size.y; i++)
			std::cout << bits[lA.pixels[j * (int)size.x + i] >> 5] << ' ';
		std::cout << '\n';
	}


//	const int width = 150;
//	const int height = 150;
//
//	mka::bezier_curve test_curve({{0,     0},
//								  {0,     height},
//								  {width, height},
//								  {width, 0},
//								  {0,     0},
//								  {width, 0}});
//
//	mka::move m({0, 0});
//
//	mka::point origin = {0, 0};
//
//	mka::bitmap bitmap(width, height);
//
//	test_curve.rasterize(bitmap, origin);
//
//	m.rasterize(bitmap, origin);
//
//	const char *bits[] = {" ", "ı", "▄", "■", "░", "▒", "▓", "█"};
//
//
//	for (int j = height - 1; j >= 0; j--) {
//		for (int i = 0; i < width; i++)
//			std::cout << bits[bitmap[j * width + i] >> 5] << bits[bitmap[j * width + i] >> 5];
//		std::cout << '\n';
//	}
}