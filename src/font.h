//
// Created by Moss on 9/19/21.
//
#pragma once
#include <string>
#include <iostream>
#include <unordered_map>
#include "cff.h"
#include "letter.h"


namespace mka{
	class letter;

	class font {
	public:

		explicit font(const std::string &file_path);

		bool generate_tables();

		~font();

		int find_glyph_index(int unicode_codepoint) const;

		void generate_letter_bitmap(float, float, int);

		void generate_letter(char, float);

		void generate_letter(char, float, float);

		letter& get_letter(char);

		letter& get_letter(char, float);

		const letter& get_letter(char) const;

		unsigned int get_glyph_offset(int glyph_index) const;

		float scale_for_pixel_height(float height) const;

		float scale_for_pixel_width(float width) const;

		int get_table(const std::string& tag) const
		{
			return this->tables.at(tag);
		}


	public:
		static const int num_letters = 256;                        				// number of letters in the letters letter

		std::vector<letter> letters;											// Bits of the Letters
		unsigned char *data{};													// Data of the font
		long font_size;															// offset of start of font
		std::unordered_map<std::string, int> tables;							// table locations as offset from start of .ttf

	private:
		int font_start;															// The offset from data to where the font data starts

		int numGlyphs;                                            				// number of glyphs, needed for range checking

		unsigned int index_map;                                					// a cmap mapping for our chosen character encoding
		int indexToLocFormat;                                    				// format needed to map from glyph index to glyph
	};

}