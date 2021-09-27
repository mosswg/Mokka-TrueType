//
// Created by Moss on 9/19/21.
//
#pragma once
#include <string>
#include <iostream>
#include "buf.h"
#include "cff.h"
#include "letter.h"


namespace mka{
	class letter;

	class font {
	public:
		static void close_shape(std::vector<bezier_curve>& edges, const std::vector<point>& points);

		explicit font(const std::string &file_path);

		bool generate_tables();

		~font();

		int find_glyph_index(int unicode_codepoint) const;

		void get_codepoint_bitmap_subpixel(float, float, float, float, int);

		void generate_letter(char, float);

		void generate_letter(char, float, float);

		int GetGlyphShape(int glyph_index, mka::bezier_curve **pvertices);

		letter& get_letter(char);

		letter& get_letter(char, float);

		const letter& get_letter(char) const;

		unsigned int get_glyph_offset(int glyph_index) const;

		void get_glyph_bitmap_box_subpixel(int glyph, float scale_x, float scale_y, float shift_x, float shift_y, int *ix0,
										   int *iy0,
										   int *ix1, int *iy1) const;

		mka::buf cid_get_glyph_subrs(int glyph_index);

		float scale_for_pixel_height(float height) const;

		float scale_for_pixel_width(float width) const;

		void print_ints() const {
			std::cout << this->index_map << "i   " << this->loca << "l  " << this->head << "h  " << this->glyf << "g  "
					  << this->hhea << "h1  " << this->hmtx << "h2  " << this->kern << "k  " << this->gpos << "g  "
					  << this->svg << "s  " << std::endl;
		}

		void print_buf_sizes() const {
			std::cout << this->cff.size << "cff   " << this->charstrings.size << "cstr  " << this->gsubrs.size
					  << "gsubrs  " << this->subrs.size << "sbr  " << this->fontdicts.size << "fd  "
					  << this->fdselect.size << "fdsel  " << std::endl;
		}

	public:
		static const int num_letters = 256;                        				// number of letters in the letters letter

		std::vector<letter> letters;											// Bits of the Letters
		unsigned char *data{};													// Data of the font
		long font_size;															// offset of start of font

	private:
		int font_start;															// The offset from data to where the font data starts

		int numGlyphs;                                            				// number of glyphs, needed for range checking

		unsigned int loca, head, glyf, hhea, hmtx, kern, gpos, svg, cmap;    	// table locations as offset from start of .ttf
		unsigned int index_map;                                					// a cmap mapping for our chosen character encoding
		int indexToLocFormat;                                    				// format needed to map from glyph index to glyph

		buf cff;																// cff font data
		buf charstrings;														// the charstring index
		buf gsubrs;																// global charstring subroutines index
		buf subrs;																// private charstring subroutines index
		buf fontdicts;															// array of font dicts
		buf fdselect;															// map from glyph to fontdict
	};

}