//
// Created by Moss on 9/19/21.
//

#include <fstream>
#include <bitset>
#include "font.h"
#include "letter.h"
#include "macros.h"

namespace mka {
	bool mka::font::generate_tables() {
		std::string table_tag;

		int num_tables = ((this->data + this->font_start + 4)[0] << 8) + (this->data + this->font_start + 4)[1];
		unsigned int tabledir = this->font_start + 12;
		for (int i = 0; i < num_tables; ++i) {
			unsigned int loc = tabledir + (16 * i);

			table_tag = std::string() + (char)(data + loc + 0)[0] + (char)(data + loc + 0)[1] + (char)(data + loc + 0)[2] + (char)(data + loc + 0)[3];

			tables.emplace(table_tag, ((data + loc + 8)[0] << 24) + ((data + loc + 8)[1] << 16) + ((data + loc + 8)[2] << 8) +
					   (data + loc + 8)[3]);
		}

		// Check if required tables exist
		if (this->tables.at("cmap") && this->tables.at("head") && this->tables.at("hhea") && this->tables.at("hmtx") && this->tables.at("glyf") && this->tables.at("loca")) {
			return true;
		} else {
			std::cout << "ERR: File Missing Required Headers (only ttf fonts are currently supported)" << std::endl;
			return false;
		}
	}

	font::font(const std::string &file_path) {
		std::ifstream font_file(file_path, std::ios_base::in | std::ios_base::binary | std::ios::ate);

		letters = std::vector<mka::letter>(num_letters, mka::letter(this));

		if (!font_file.is_open()) {
			std::cout << "File Was Not Found" << std::endl;
			exit(-1);
		}

		this->font_size = font_file.tellg();

		this->data = new unsigned char[this->font_size];

		this->font_start = 0;

		font_file.seekg(0, std::ios::beg);

		font_file.read(reinterpret_cast<char *>(this->data), this->font_size);

		if (font_file) {
			std::cout << "Font Read Successfully With Size of " << round((double)this->font_size/128.0) / 8.0 << "kb" << std::endl;
		} else {
			std::cout << "Error Reading Font: " << font_file.rdstate() << std::endl;
			exit(-2);
		}

		// Initialization
		int i, numTables;

		this->generate_tables();

		unsigned int t = get_table("maxp");

		if (t)
			this->numGlyphs = (this->data + t + 4)[0] * 256 + (this->data + t + 4)[1];
		else
			this->numGlyphs = 0xffff;

		this->tables.emplace("svg", -1);

		// find a cmap encoding table we understand *now* to avoid searching
		// later. (todo: could make this installable)
		// the same regardless of glyph.
		numTables = get_ushort(this->data + this->get_table("cmap") + 2);
		this->index_map = 0;
		for (i = 0; i < numTables; ++i) {
			unsigned int encoding_record = this->get_table("cmap") + 4 + 8 * i;
			// find an encoding we understand:
			unsigned short encoding_type = get_ushort(this->data + encoding_record);
			if (encoding_type >= platform_id::UNICODE && encoding_type <= platform_id::MICROSOFT) {
				this->index_map = this->get_table("cmap") + get_ulong(this->data + encoding_record + 4);
				break;
			}
		}

		if (this->index_map == 0)
			return;

		this->indexToLocFormat = get_ushort((this->data) + (this->get_table("head")) + 50);

		font_file.close();
	}

	font::~font() {
		delete data;
		letters.clear();
	}


	int font::find_glyph_index(int unicode_codepoint) const {
		unsigned char *font_data = this->data;

		unsigned short format = get_ushort(font_data + this->index_map + 0);
		if (format == 0) { // apple byte encoding
			int bytes = get_ushort(font_data + index_map + 2);
			if (unicode_codepoint < bytes - 6)
				return get_byte(font_data + index_map + 6 + unicode_codepoint);
			return 0;
		} else if (format == 6) {
			int first = get_ushort(font_data + index_map + 6);
			unsigned int count = get_ushort(font_data + index_map + 8);
			if (unicode_codepoint >= first && (unsigned int) unicode_codepoint < first + count)
				return get_ushort(font_data + index_map + 10 + (unicode_codepoint - first) * 2);
			return 0;
		} else if (format == 2) {
			mka_assert_unreachable(-1); // @TODO: high-byte mapping for japanese/chinese/korean
		} else if (format == 4) { // standard mapping for windows fonts: binary search collection of ranges
			unsigned short segcount = get_ushort(font_data + index_map + 6) >> 1;
			unsigned short searchRange = get_ushort(font_data + index_map + 8) >> 1;
			unsigned short entrySelector = get_ushort(font_data + index_map + 10);
			unsigned short rangeShift = get_ushort(font_data + index_map + 12) >> 1;

			// do a binary search of the segments
			unsigned int endCount = index_map + 14;
			unsigned int search = endCount;

			if (unicode_codepoint > 0xffff)
				return 0;

			// they lie from endCount .. endCount + segCount
			// but searchRange is the nearest power of two, so...
			if (unicode_codepoint >= get_ushort(font_data + search + rangeShift * 2))
				search += rangeShift * 2;

			// now decrement to bias correctly to find smallest
			search -= 2;
			while (entrySelector) {
				unsigned short end;
				searchRange >>= 1;
				end = get_ushort(font_data + search + searchRange * 2);
				if (unicode_codepoint > end)
					search += searchRange * 2;
				--entrySelector;
			}
			search += 2;

			{
				unsigned short offset, start, last;
				auto item = (unsigned short) ((search - endCount) >> 1);

				start = get_ushort(font_data + index_map + 14 + segcount * 2 + 2 + 2 * item);
				last = get_ushort(font_data + endCount + 2 * item);
				if (unicode_codepoint < start || unicode_codepoint > last)
					return 0;

				offset = get_ushort(font_data + index_map + 14 + segcount * 6 + 2 + 2 * item);
				if (offset == 0)
					return (unsigned short) (unicode_codepoint +
											 get_short(font_data + index_map + 14 + segcount * 4 + 2 + 2 * item));

				return get_ushort(
						font_data + offset + (unicode_codepoint - start) * 2 + index_map + 14 + segcount * 6 + 2 +
						2 * item);
			}
		} else if (format == 12 || format == 13) {
			unsigned int ngroups = get_ulong(font_data + index_map + 12);
			int low, high;
			low = 0;
			high = (int) ngroups;
			// Binary search the right group.
			while (low < high) {
				int mid = low + ((high - low) >> 1); // rounds down, so low <= mid < high
				unsigned int start_char = get_ulong(font_data + index_map + 16 + mid * 12);
				unsigned int end_char = get_ulong(font_data + index_map + 16 + mid * 12 + 4);
				if ((unsigned int) unicode_codepoint < start_char)
					high = mid;
				else if ((unsigned int) unicode_codepoint > end_char)
					low = mid + 1;
				else {
					unsigned int start_glyph = get_ulong(font_data + index_map + 16 + mid * 12 + 8);
					if (format == 12)
						return start_glyph + unicode_codepoint - start_char;
					else // format == 13
						return start_glyph;
				}
			}
			return 0; // not found
		}
		mka_assert_unreachable(-2);
		return 0;
	}

	unsigned int font::get_glyph_offset(int glyph_index) const {
		unsigned int g1, g2;

		if (glyph_index >= this->numGlyphs) return -1; // glyph index out of range
		if (this->indexToLocFormat >= 2) return -1; // unknown index->glyph map format

		if (this->indexToLocFormat == 0) {
			g1 = this->get_table("glyf") + get_ushort(this->data + this->get_table("loca") + glyph_index * 2) * 2;
			g2 = this->get_table("glyf") + get_ushort(this->data + this->get_table("loca") + glyph_index * 2 + 2) * 2;
			} else {
			g1 = this->get_table("glyf") + get_ulong(this->data + this->get_table("loca") + glyph_index * 4);
			g2 = this->get_table("glyf") + get_ulong(this->data + this->get_table("loca") + glyph_index * 4 + 4);
		}

		return (g1 == g2) ? -1 : g1; // if length is 0, return -1
	}


	void font::generate_letter_bitmap(float scale_x, float scale_y, int codepoint) {
		mka::letter& gbm = this->letters[codepoint];
		gbm.ch = (char)codepoint;

		if (scale_x == 0) scale_x = scale_y;
		if (scale_y == 0) {
			if (scale_x == 0) {
				return;
			}
			scale_y = scale_x;
		}
		gbm.scale = {scale_x, scale_y};

		gbm.generate_edges();

		for (auto const& e : gbm.edges) {
			std::cout << "curve with type: ";
			switch (e.points.size()) {
				case 1:
					std::cout << "move\t";
					break;
				case 2:
					std::cout << "line\t";
					break;
				case 3:
				default:
					std::cout << "curve\t";
					break;
			}
			e.print_points();
			std::cout << std::endl;
		}

		gbm.rasterize_edges();
	}

	void font::generate_letter(char codepoint, float scale) {
		if (scale == letters[codepoint].scale.x && scale == letters[codepoint].scale.y)
			return;

		float f_scale_y = this->scale_for_pixel_height((float) scale);
		float f_scale_x = this->scale_for_pixel_width((float) scale);

		this->generate_letter_bitmap(f_scale_x, f_scale_y, codepoint);
	}

	void font::generate_letter(char codepoint, float scale_x, float scale_y) {
		if (scale_x == letters[codepoint].scale.x && scale_y == letters[codepoint].scale.y)
			return;

		float f_scale_x = this->scale_for_pixel_width((float) scale_x);
		float f_scale_y = this->scale_for_pixel_height((float) scale_y);

		this->generate_letter_bitmap(f_scale_x, f_scale_y, codepoint);
	}


	mka::letter& font::get_letter(char letter) {
		if (letters[letter].get_size().is_zero()) {
			this->generate_letter(letter, 20);
		}

		return letters[letter];
	}

	mka::letter& font::get_letter(char letter, float scale) {
		if (letters[letter].get_size().is_zero()) {
			this->generate_letter(letter, scale);
		} else {
			if (scale != letters[letter].scale.x) {
				this->generate_letter(letter, scale);
			}
		}

		return letters[letter];
	}


	const letter& font::get_letter(char letter) const {
		return letters[letter];
	}

	float font::scale_for_pixel_height(float height) const {
		int fheight = get_short(this->data + this->tables.at("hhea") + 4) - get_short(this->data + this->tables.at("hhea") + 6);
		return (height / (float)fheight);
	}

	float font::scale_for_pixel_width(float width) const {
		int fwidth = get_short(this->data + this->tables.at("hhea") + 10) - get_short(this->data + this->tables.at("hhea") + 12);
		return (width / (float)fwidth);
	}

}
