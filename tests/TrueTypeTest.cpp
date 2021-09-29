#include <iostream>

#define stb


//#ifndef stb



#define SHOW_BIT_MAP 1

#ifndef stb

#include "font.h"
#include "letter.h"

const char* font_file = "res/JetBrainsMonoNL-Medium.ttf";

int main(int argc, char** argv) {
    std::string c;
	float s = 40;

    mka::font font(font_file);

	std::vector<mka::letter> const& letters = font.letters;

	while (true) {
		std::cout << "Enter String: ";

		getline(std::cin, c); // Gets User Input

		if (std::cin.fail()) { // If there was an error we want to flush the cin stream.
			std::cin.clear();
			std::cin.ignore(INT32_MAX, '\n');
		}

		if (c == "qq") {
			break;
		}

		for (char ch: c)
			font.generate_letter(ch, s);

		const char *bits[] = {" ", "ı", "▄", "■", "░", "▒", "▓", "█"};

		int max_height = 0;


		for (char i: c)
			max_height = max_height > letters[i].pixels.h ? max_height : letters[i].pixels.h;




//	int i = 0;
//    for (int j=0; j < max_height; ++j) {
//		for (char ch : c) {
//			if (ch == ' ') {
//				for (; i < font.space_width; i++)
//					std::cout << "  ";
//			}
//			i = 0;
//			if (j <= (max_height - letters[ch].pixels.h)) {
//				for (; i < letters[ch].pixels.w; i++)
//					std::cout << "  ";
//			}
//			for (; i < letters[ch].pixels.w; ++i)
//				std::cout << (bits[letters[ch].pixels[(j - (max_height - letters[ch].pixels.h)) * letters[ch].pixels.w + i] >> 5]) << ' ';
//			std::cout << "  ";
//		}
//        std::cout << std::endl;
//
//    }
//
//	std::cout << std::endl;
//
//	#if SHOW_BIT_MAP == 1
//	for (int j=0; j < letters[c[0]].pixels.h; ++j) {
//		for (int i=0; i < letters[c[0]].pixels.w; ++i)
//			std::cout << +letters[c[0]].pixels[j*letters[c[0]].pixels.w+i] << '\t';
//		std::cout << '\n';
//	}
//	#endif
//	}
	}
    return EXIT_SUCCESS;
}




#else
#include <stdio.h>
#define STB_TRUETYPE_IMPLEMENTATION  // force following include to generate implementation
#include "src/stb_truetype.h"

char ttf_buffer[33554432];

int main(int argc, char **argv)
{
   stbtt_fontinfo font;
   unsigned char *bitmap;
   int w,h,i,j,c = (argc > 1 ? atoi(argv[1]) : 'A'), s = (argc > 2 ? atoi(argv[2]) : 20);

   fread(ttf_buffer, 1, 1<<25, fopen(argc > 3 ? argv[3] : "res/JetBrainsMonoNL-Medium.ttf", "rb"));

    stbtt_InitFont(&font, reinterpret_cast<const unsigned char *>(ttf_buffer), stbtt_GetFontOffsetForIndex(
			reinterpret_cast<const unsigned char *>(ttf_buffer), 0));
   bitmap = stbtt_GetCodepointBitmap(&font, 0,stbtt_ScaleForPixelHeight(&font, s), c, &w, &h, 0,0);

   for (j=0; j < h; ++j) {
      for (i=0; i < w; ++i)
         putchar(" .:ioVM@"[bitmap[j*w+i]>>5]);
      putchar('\n');
   }
   return 0;
}
#endif