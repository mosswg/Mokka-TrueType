//
// Created by Moss on 9/19/21.
//

#include <fstream>
#include <bitset>
#include "font.h"
#include "letter.h"
#include "macros.h"
#include "buf.h"

namespace mka {
/// OPTIMIZE: binary search
// Taken Verbatim From stb_truetype
	unsigned int find_table(unsigned char *data, unsigned int fontstart, const char *tag) {
		int num_tables = ((data + fontstart + 4)[0] << 8) + (data + fontstart + 4)[1];
		unsigned int tabledir = fontstart + 12;
		for (int i = 0; i < num_tables; ++i) {
			unsigned int loc = tabledir + (16 * i);
			if ((data + loc + 0)[0] == (tag[0]) && (data + loc + 0)[1] == (tag[1]) && (data + loc + 0)[2] == (tag[2]) &&
				(data + loc + 0)[3] == (tag[3]))
				return ((data + loc + 8)[0] << 24) + ((data + loc + 8)[1] << 16) + ((data + loc + 8)[2] << 8) +
					   (data + loc + 8)[3];
		}
		return 0;
	}

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

		unsigned int t;
		int i, numTables;


		this->cff = buf(nullptr, 0);

		this->generate_tables();

		t = find_table(reinterpret_cast<unsigned char *>(this->data), font_start, "maxp");
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

		mka_assert(this->index_map >= 0, -52);

		unsigned short format = get_ushort(font_data + this->index_map + 0);
		if (format == 0) { // apple byte encoding
			int bytes = get_ushort(font_data + index_map + 2);
			if (unicode_codepoint < bytes - 6)
				return get_byte(font_data + index_map + 6 + unicode_codepoint);
			return 0;
		} else if (format == 6) {
			int first = get_ushort(font_data + index_map + 6);
			unsigned int count = get_ushort(font_data + index_map + 8);
			if ((unsigned int) unicode_codepoint >= first && (unsigned int) unicode_codepoint < first + count)
				return get_ushort(font_data + index_map + 10 + (unicode_codepoint - first) * 2);
			return 0;
		} else if (format == 2) {
			mka_assert_unreachable(-1); // @TODO: high-byte mapping for japanese/chinese/korean
			return 0;
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

	/**
	 * We're making assumptions based on how many points there are. This is generally bad but in this case it should be fine.
	 *
	 * @param edges - Vector that will be appended to.
	 * @param points - Points that make up the edge that we're adding.
	 *
	 */
	void mka::font::close_shape(std::vector<bezier_curve>& edges, const std::vector<point>& points) {
		edges.emplace_back(points);
	}

	static int close_shape(bezier_curve *vertices, int num_vertices, int was_off, int start_off,
						   int sx, int sy, int scx, int scy, int cx, int cy)
	{
		if (start_off) {
			if (was_off) {
				vertices[++num_vertices][0].flags = 2;
				vertices[num_vertices][0].x = (cx+scx)>>1;
				vertices[num_vertices][0].y = (cy+scy)>>1;
				vertices[num_vertices][1].x = cx;
				vertices[num_vertices][1].y = cy;
			}
			vertices[++num_vertices][0].flags = 2;
			vertices[num_vertices][0].x = sx;
			vertices[num_vertices][0].y = sy;
			vertices[num_vertices][1].x = scx;
			vertices[num_vertices][1].y = scy;
		} else {
			if (was_off) {
				vertices[++num_vertices][0].flags = 2;
				vertices[num_vertices][0].x = sx;
				vertices[num_vertices][0].y = sy;
				vertices[num_vertices][1].x = cx;
				vertices[num_vertices][1].y = cy;
			}
			else {
				vertices[++num_vertices][0].flags = 1;
				vertices[num_vertices][0].x = sx;
				vertices[num_vertices][0].y = sy;
				vertices[num_vertices][1].x = 0;
				vertices[num_vertices][1].y = 0;
			}
		}
		return num_vertices;
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


	// This is a function that has been taken almost verbatim from stbtt. This will be 
	// changed later but this is the part I need to redesign at the moment.
	int mka::font::GetGlyphShape(int glyph_index, mka::bezier_curve **pvertices)
	{
		short numberOfContours;
		unsigned char *endPtsOfContours;
		unsigned char *data = this->data;
		bezier_curve *vertices=0;
		int num_vertices=0;
		int g = this->get_glyph_offset(glyph_index);

		*pvertices = NULL;

		if (g < 0) return 0;

		numberOfContours = get_short(data + g);

		if (numberOfContours > 0) {
			unsigned char flags=0,flagcount;
			int ins, i,j=0,m,n, next_move, was_off=0, off, start_off=0;
			int x,y,cx,cy,sx,sy, scx,scy;
			unsigned char *points;
			endPtsOfContours = (data + g + 10);
			ins = get_ushort(data + g + 10 + numberOfContours * 2);
			points = data + g + 10 + numberOfContours * 2 + 2 + ins;

			n = 1+get_ushort(endPtsOfContours + numberOfContours*2-2);

			m = n + 2*numberOfContours;  // a loose bound on how many vertices we might need
			vertices = new bezier_curve[m];
			if (vertices == nullptr)
				return 0;


			next_move = 0;
			flagcount=0;

			// in first pass, we load uninterpreted data into the allocated array
			// above, shifted to the end of the array so we won't overwrite it when
			// we create our final data starting from the front

			off = m - n; // starting offset for uninterpreted data, regardless of how m ends up being calculated

			// first load flags

			for (i=0; i < n; ++i) {
				if (flagcount == 0) {
					flags = *points++;
					if (flags & 8)
						flagcount = *points++;
				} else
					--flagcount;
				vertices[off+i][0].flags = flags;
			}

			// now load x coordinates
			x=0;
			for (i=0; i < n; ++i) {
				flags = vertices[off+i][0].flags;
				if (flags & 2) {
					short dx = *points++;
					x += (flags & 16) ? dx : -dx; // ???
				} else {
					if (!(flags & 16)) {
						x = x + (short) (points[0]*256 + points[1]);
						points += 2;
					}
				}
				vertices[off+i][0].x = (short) x;
			}

			// now load y coordinates
			y=0;
			for (i=0; i < n; ++i) {
				flags = vertices[off+i][0].flags;
				if (flags & 4) {
					short dy = *points++;
					y += (flags & 32) ? dy : -dy; // ???
				} else {
					if (!(flags & 32)) {
						y = y + (short) (points[0]*256 + points[1]);
						points += 2;
					}
				}
				vertices[off+i][0].y = (short) y;
			}

			// now convert them to our format
			num_vertices=0;
			sx = sy = cx = cy = scx = scy = 0;
			for (i=0; i < n; ++i) {
				flags = vertices[off+i][0].flags;
				x     = (short) vertices[off+i][0].x;
				y     = (short) vertices[off+i][0].y;

				if (next_move == i) {
					if (i != 0) {
						num_vertices = ::mka::close_shape(vertices, num_vertices, was_off, start_off, sx, sy, scx, scy, cx, cy);
					}

					// now start the new one
					start_off = !(flags & 1);
					if (start_off) {
						// if we start off with an off-curve point, then when we need to find a point on the curve
						// where we can start, and we need to save some state for when we wraparound.
						scx = x;
						scy = y;
						if (!(vertices[off+i+1][0].flags & 1)) {
							// next point is also a curve point, so interpolate an on-point curve
							sx = (x + (int) vertices[off+i+1][0].x) >> 1;
							sy = (y + (int) vertices[off+i+1][0].y) >> 1;
						} else {
							// otherwise just use the next point as our start point
							sx = (int) vertices[off+i+1][0].x;
							sy = (int) vertices[off+i+1][0].y;
							++i; // we're using point i+1 as the starting point, so skip it
						}
					} else {
						sx = x;
						sy = y;
					}
					vertices[++num_vertices][0].flags = 0;
					vertices[num_vertices][0].x = sx;
					vertices[num_vertices][0].y = sy;
					vertices[num_vertices][1].x = 0;
					vertices[num_vertices][1].y = 0;
					was_off = 0;
					next_move = 1 + get_ushort(endPtsOfContours+j*2);
					++j;
				} else {
					if (!(flags & 1)) { // if it's a curve
						if (was_off) { // two off-curve control points in a row means interpolate an on-curve midpoint
							vertices[++num_vertices][0].flags = 2;
							vertices[num_vertices][0].x = (cx+x)/2.0;
							vertices[num_vertices][0].y = (cy+y)/2.0;
							vertices[num_vertices][1].x = cx;
							vertices[num_vertices][1].y = cy;
						}
						cx = x;
						cy = y;
						was_off = 1;
					} else {
						if (was_off) {
							vertices[++num_vertices][0].flags = 2;
							vertices[num_vertices][0].x = x;
							vertices[num_vertices][0].y = y;
							vertices[num_vertices][1].x = cx;
							vertices[num_vertices][1].y = cy;
						}
						else {
							vertices[++num_vertices][0].flags = 1;
									vertices[num_vertices][0].x = x;
									vertices[num_vertices][0].y = y;
									vertices[num_vertices][1].x = 0;
									vertices[num_vertices][1].y = 0;
						}
						was_off = 0;
					}
				}
			}
			num_vertices = ::mka::close_shape(vertices, num_vertices, was_off, start_off, sx,sy,scx,scy,cx,cy);
		} else if (numberOfContours < 0) {
			// Compound shapes.
			int more = 1;
			unsigned char *comp = data + g + 10;
			num_vertices = 0;
			vertices = 0;
			while (more) {
				unsigned short flags, gidx;
				int comp_num_verts = 0, i;
				bezier_curve *comp_verts = 0, *tmp = 0;
				float mtx[6] = {1,0,0,1,0,0}, m, n;

				flags = get_short(comp); comp+=2;
				gidx = get_short(comp); comp+=2;

				if (flags & 2) { // XY values
					if (flags & 1) { // shorts
						mtx[4] = get_short(comp); comp+=2;
						mtx[5] = get_short(comp); comp+=2;
					} else {
						mtx[4] = get_char(comp); comp+=1;
						mtx[5] = get_char(comp); comp+=1;
					}
				}
				else {
					// @TODO handle matching point
					mka_assert_unreachable(-55);
				}
				if (flags & (1<<3)) { // WE_HAVE_A_SCALE
					mtx[0] = mtx[3] = get_short(comp)/16384.0f; comp+=2;
					mtx[1] = mtx[2] = 0;
				} else if (flags & (1<<6)) { // WE_HAVE_AN_X_AND_YSCALE
					mtx[0] = get_short(comp)/16384.0f; comp+=2;
					mtx[1] = mtx[2] = 0;
					mtx[3] = get_short(comp)/16384.0f; comp+=2;
				} else if (flags & (1<<7)) { // WE_HAVE_A_TWO_BY_TWO
					mtx[0] = get_short(comp)/16384.0f; comp+=2;
					mtx[1] = get_short(comp)/16384.0f; comp+=2;
					mtx[2] = get_short(comp)/16384.0f; comp+=2;
					mtx[3] = get_short(comp)/16384.0f; comp+=2;
				}

				// Find transformation scales.
				m = (float) sqrt(mtx[0]*mtx[0] + mtx[1]*mtx[1]);
				n = (float) sqrt(mtx[2]*mtx[2] + mtx[3]*mtx[3]);

				// Get indexed glyph.
				comp_num_verts = this->GetGlyphShape(gidx, &comp_verts);
				if (comp_num_verts > 0) {
					// Transform vertices.
					for (i = 0; i < comp_num_verts; ++i) {
						bezier_curve* v = &comp_verts[i];
						short x,y;
						x=v->points[0].x; y=v->points[0].y;
						v->points[0].x = (short)(m * (mtx[0]*x + mtx[2]*y + mtx[4]));
						v->points[0].y = (short)(n * (mtx[1]*x + mtx[3]*y + mtx[5]));
						x=v->points[1].x; y=v->points[0].y;
						v->points[1].x = (short)(m * (mtx[0]*x + mtx[2]*y + mtx[4]));
						v->points[1].y = (short)(n * (mtx[1]*x + mtx[3]*y + mtx[5]));
					}
					// Append vertices.
					tmp = (bezier_curve*)malloc((num_vertices+comp_num_verts)*sizeof(point));
					if (!tmp) {
						if (vertices) free(vertices);
						if (comp_verts) free(comp_verts);
						return 0;
					}
					if (num_vertices > 0 && vertices) memcpy(tmp, vertices, num_vertices*sizeof(point));
					memcpy(tmp+num_vertices, comp_verts, comp_num_verts*sizeof(point));
					if (vertices) free(vertices);
					vertices = tmp;
					free(comp_verts);
					num_vertices += comp_num_verts;
				}
				// More components ?
				more = flags & (1<<5);
			}
		} else {
			// numberOfCounters == 0, do nothing
		}

		*pvertices = vertices;
		return num_vertices;
	}


	void font::get_codepoint_bitmap_subpixel(float scale_x, float scale_y, float shift_x, float shift_y, int codepoint) {
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

		gbm.rasterize_edges();

		for (mka::bezier_curve& e : this->letters[codepoint].edges) {
			std::cout << "type: ";
			switch (e.points.size()) {
				case 1:
					std::cout << "move with 1 points";
					break;
				case 2:
					std::cout << "line with 2 points at (" << e.points[0].x << ", " << e.points[0].y << ") and (" << e.points[1].x << ", " << e.points[1].y << ")";
					break;
				case 3:
					std::cout << "linear with 3 points";
					break;
				default:
					std::cout << "cubic with " << e.points.size() << " points";
					break;
			}
			std::cout << std::endl;
		}
	}

	void font::generate_letter(char codepoint, float scale) {
		if (scale == letters[codepoint].scale.x && scale == letters[codepoint].scale.y)
			return;

		float f_scale_y = this->scale_for_pixel_height((float) scale);
		float f_scale_x = this->scale_for_pixel_width((float) scale);

		this->get_codepoint_bitmap_subpixel(f_scale_x, f_scale_y, 0.0f, 0.0f, codepoint);
	}

	void font::generate_letter(char codepoint, float scale_x, float scale_y) {
		if (scale_x == letters[codepoint].scale.x && scale_y == letters[codepoint].scale.y)
			return;

		int width;
		int height;
		int xoff;
		int yoff;


		float f_scale_x = this->scale_for_pixel_width((float) scale_x);
		float f_scale_y = this->scale_for_pixel_height((float) scale_y);

		this->get_codepoint_bitmap_subpixel(f_scale_x, f_scale_y, 0.0f, 0.0f, codepoint);
	}


	mka::letter& font::get_letter(char letter) {
		if (letters[letter].pixels.w == 0 && letters[letter].pixels.h == 0) {
			this->generate_letter(letter, 20);
		}

		return letters[letter];
	}

	mka::letter& font::get_letter(char letter, float scale) {
		if (letters[letter].pixels.w == 0 && letters[letter].pixels.h == 0) {
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
