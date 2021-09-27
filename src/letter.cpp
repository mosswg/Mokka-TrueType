//
// Copyright (c) 2021 Moss Gallagher.
//
#include <vector>
#include "cff.h"
#include "letter.h"
#include "macros.h"

void mka::letter::calculate_bounding_box() {
	if (edges.empty())
		return;

	this->min_y = INT32_MAX;
	this->min_x = INT32_MAX;
	double max_y = 0;
	double max_x = 0;
	for (const bezier_curve& e : edges) {
		/// FIXME: This is a Hack
		if (e.points.size() < 2) {
			continue;
		}

//		std::cout << "y: " << e.points[0].y << ", " << e.points[1].y << "\tx: " << e.points[0].x << ", " << e.points[1].x << std::endl;

		// max y
		if (e.points[1].y > max_y)
			max_y = e.points[1].y;

		if (e.points[0].y > max_y) {
			max_y = e.points[0].y;
		}

		// min y
		if (e.points[0].y != 0 && floor(e.points[0].y) < min_y) {
			min_y = floor(e.points[0].y);
		}

		// min x
		if (e.points[0].x != 0 && floor(e.points[0].x) < min_x) {
			min_x = floor(e.points[0].x);
		}

		// max x
		if (e.points[1].x > max_x)
			max_x = e.points[1].x;
	}

	this->pixels.h = (int)(ceil(max_y) - this->min_y);
	this->pixels.w = (int)(ceil(max_x) - this->min_x);


	std::cout << "h: " << this->pixels.h << "\tmin: " << min_y << "\tmax: " << max_y << std::endl;
	std::cout << "w: " << this->pixels.w << "\tmin: " << min_x << "\tmax: " << max_x << std::endl;
}

void mka::letter::generate_edges() {
	// From https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6glyf.html

	/*
	 * 	If set, the point is on the curve;
	 *	Otherwise, it is off the curve.
	 */
	const char ON_CURVE = (1 << 0);

	/*
	 *	If set, the corresponding x-coordinate is 1 byte long;
	 *	Otherwise, the corresponding x-coordinate is 2 bytes long
	 */
	const char X_SHORT_VECTOR = (1 << 1);


	/*
	 * 	If set, the corresponding y-coordinate is 1 byte long;
	 *	Otherwise, the corresponding y-coordinate is 2 bytes long
	 */
	const char Y_SHORT_VECTOR = (1 << 2);

	/*
	 * If set, the next byte specifies the number of additional times this set of flags is to be repeated.
	 * In this way, the number of flags listed can be smaller than the number of points in a character.
	 */
	const char REPEAT = (1 << 3);

	/*
	 * This flag has one of two meanings, depending on how the x-Short Vector flag is set.
	 * If the x-Short Vector bit is set, this bit describes the sign of the value, with a value of 1 equalling positive and a zero value negative.
	 * If the x-short Vector bit is not set, and this bit is set, then the current x-coordinate is the same as the previous x-coordinate.
	 * If the x-short Vector bit is not set, and this bit is not set, the current x-coordinate is a signed 16-bit delta vector. In this case, the delta vector is the change in x
	 */
	const char POSITIVE_X_SHORT_VECTOR = (1 << 4);


	/*
	 * This flag has one of two meanings, depending on how the y-Short Vector flag is set.
	 * If the y-Short Vector bit is set, this bit describes the sign of the value, with a value of 1 equalling positive and a zero value negative.
	 * If the y-short Vector bit is not set, and this bit is set, then the current y-coordinate is the same as the previous y-coordinate.
	 * If the y-short Vector bit is not set, and this bit is not set, the current y-coordinate is a signed 16-bit delta vector. In this case, the delta vector is the change in y
	 */
	const char POSITIVE_Y_SHORT_VECTOR = (1 << 5);

	/*
	 * The rest of the bit should be set to zero.
	 */




	short numberOfContours;
	unsigned char *endPtsOfContours;
	int num_vertices;
	unsigned int glyph_offset = parent_font->get_glyph_offset(this->ch);

	if (glyph_offset < 0) return;

	numberOfContours = get_short(parent_font->data + glyph_offset);

	std::cout << "min x: " << get_short(parent_font->data + glyph_offset + 2);
	std::cout << "min y: " << get_short(parent_font->data + glyph_offset + 4);
	std::cout << "max x: " << get_short(parent_font->data + glyph_offset + 6);
	std::cout << "max y: " << get_short(parent_font->data + glyph_offset + 8);

	std::vector<mka::point> uninterpreted_data{};

	if (numberOfContours > 0) {
		unsigned char flags = 0, flagcount;
		int instructions_length;
		int i;
		int n;

		std::vector<mka::point> current_points;
		unsigned char *points;
		endPtsOfContours = (parent_font->data + glyph_offset + 10);
		instructions_length = get_ushort(parent_font->data + glyph_offset + 10 + numberOfContours * 2);
		points = parent_font->data + glyph_offset + 10 + numberOfContours * 2 + 2 + instructions_length;

		n = 1 + get_ushort(endPtsOfContours + numberOfContours * 2 - 2);

		//m = n + 2 * numberOfContours;  // a loose bound on how many vertices we might need

		flagcount = 0;

		// in first pass, we load uninterpreted font_data into the vector
		for (i = 0; i < n; ++i) {
			if (flagcount == 0) { // If we're not repeating the current flags...
				flags = *points++; // Get the value from points and increment the pointer
				if (flags & REPEAT) // If the repeat flag is set...
					flagcount = *points++; // Set the flag_count variable to represent how many times this flag should be repeated in the uninterpreted data set
			} else {
				--flagcount; // Decrement flag_count if we're repeating.
			}

			uninterpreted_data.emplace_back(flags); // Emplacing because then we don't need to count then allocate and then count again.
		}

		// Note: x values are relative to previous values. This is why we increment x instead of just setting it. The same is true for y values.
		current_points.emplace_back(0,0);
		for (i = 0; i < n; ++i) {
			flags = uninterpreted_data[i].flags; // Read the flag of the uninterpreted data at the index of i
			if (flags & X_SHORT_VECTOR) {	// If the vector's x is 1 byte long...
				short data_x = *points++;	// Set data x to the value at points and increment points pointer
				/*
				 * If the x short vector and the positive x short vector flags are set
				 * then x should be positive otherwise negative
				 */
				current_points[0].x += (flags & POSITIVE_X_SHORT_VECTOR) ? data_x : -data_x;
			} else { // If the vector's x is 2 bytes long...
				if (!(flags & POSITIVE_X_SHORT_VECTOR)) { // And the positive x short vector flag is set...
					/*
					 * The current x-coordinate is a signed 16-bit delta vector and
					 * The delta vector is the change in x, so
					 * x should be x plus the next two byte of points combined.
					 */
					current_points[0].x = current_points[0].x + (short)((points[0] << 8) | points[1]);

					points += 2; // Increment points by two since we got two byte from it.

					/* Note:
					 * We shouldn't get this by incrementing the pointer because assignments are left to right,
					 * and we would need to put the first byte last. This would be very confusing.
					*/

				}
			}
			/*
			 * If the positive x short vector is not set then the current x is the same as the previous x, so we simply don't set it.
			 */


			uninterpreted_data[i].x = current_points[0].x;
		}

		// now load y coordinates
		current_points[0].y = 0;
		for (i = 0; i < n; ++i) {
			flags = uninterpreted_data[i].flags; // Read the flag of the uninterpreted data at the index of i
			if (flags & Y_SHORT_VECTOR) {	// If the vector's y is 1 byte long...
				short data_y = *points++;	// Set data y to the value at points and increment points pointer
				/*
				 * If the y short vector and the positive y short vector flags are set
				 * then y should be positive otherwise negative
				 */
				current_points[0].y += (flags & POSITIVE_Y_SHORT_VECTOR) ? data_y : -data_y;
			}
			else { 												// If the vector's y is 2 bytes long...
				if (!(flags & POSITIVE_Y_SHORT_VECTOR)) { 		// And the positive y short vector flag is set...
					/*
					 * The current y-coordinate is a signed 16-bit delta vector and
					 * The delta vector is the change in x, so
					 * it will be the current y position plus the next two byte of points combined.
					 */
					current_points[0].y = current_points[0].y + (short)((points[0]<<8) | points[1]);

					points += 2;
				}
			}
			uninterpreted_data[i].y = current_points[0].y;
		}

		current_points.clear();

		short next_move = 0;
		int j = 0;
		// Convert the previously gotten data to a format we can use.
		for (i = 0; i < uninterpreted_data.size(); i++) {
			flags = uninterpreted_data[i].flags;
			current_points.push_back(uninterpreted_data[i] * this->scale); // Read the point that we previously retrieved.


			if (next_move == i) {
				if (i != 0) {
					edges.emplace_back(current_points);
					current_points.clear();
				}


				next_move = 1 + get_ushort(endPtsOfContours+j*2); // Get next_move using j as an offset and add 1 to it.
				++j; // inc j
			}

			if (flags & ON_CURVE) { // If the current point is on the curve (First point, middle, or end point)...
				if (current_points.size() != 1) { // and it's not the first point of the curve (middle, or end)...
					if (uninterpreted_data[i+1].flags & ON_CURVE) { // and the next point is not on the curve (end point)...
						edges.emplace_back(current_points); // Create the edge with the gotten points and...
						current_points.clear(); // Clear the current points vector so that next time through the loop we have a fresh set.
					}
				}
			}
		}
	}
	else if (numberOfContours < 0) { /// FIXME: Compound Glyphs read as having no edges.
		// Compound shapes.
		int more = 1;
		unsigned char *comp = parent_font->data + g + 10;
		num_vertices = 0;
		while (more) {
			unsigned short flags, gidx;
			int comp_num_verts = 0, i;
			std::vector<bezier_curve> comp_edges, tmp;
			comp_edges = edges;
			float mtx[6] = {1, 0, 0, 1, 0, 0}, m, n;

			flags = get_short(comp);
			comp += 2;
			gidx = get_short(comp);
			comp += 2;

			if (flags & 2) { // XY values
				if (flags & 1) { // shorts
					mtx[4] = get_short(comp);
					comp += 2;
					mtx[5] = get_short(comp);
					comp += 2;
				} else {
					mtx[4] = get_char(comp);
					comp += 1;
					mtx[5] = get_char(comp);
					comp += 1;
				}
			} else {
				// @TODO handle matching point
				mka_assert_unreachable(-3);
			}
			if (flags & (1 << 3)) { // WE_HAVE_A_SCALE
				mtx[0] = mtx[3] = get_short(comp) / 16384.0f;
				comp += 2;
				mtx[1] = mtx[2] = 0;
			} else if (flags & (1 << 6)) { // WE_HAVE_AN_X_AND_YSCALE
				mtx[0] = get_short(comp) / 16384.0f;
				comp += 2;
				mtx[1] = mtx[2] = 0;
				mtx[3] = get_short(comp) / 16384.0f;
				comp += 2;
			} else if (flags & (1 << 7)) { // WE_HAVE_A_TWO_BY_TWO
				mtx[0] = get_short(comp) / 16384.0f;
				comp += 2;
				mtx[1] = get_short(comp) / 16384.0f;
				comp += 2;
				mtx[2] = get_short(comp) / 16384.0f;
				comp += 2;
				mtx[3] = get_short(comp) / 16384.0f;
				comp += 2;
			}

			// Find transformation scales.
			m = (float) sqrtf(mtx[0] * mtx[0] + mtx[1] * mtx[1]);
			n = (float) sqrtf(mtx[2] * mtx[2] + mtx[3] * mtx[3]);

			// Get indexed glyph.
			//comp_num_verts = mka::letter::get_edges_from_vertices(gidx, comp_edges);
			if (comp_num_verts > 0) {
				// Transform vertices.
				for (i = 0; i < comp_num_verts; ++i) {
					bezier_curve *v = &comp_edges[i];
					point p;
					p = v->points[0];
					v->points[0] = {(m * (mtx[0] * p.x + mtx[2] * p.y + mtx[4])), (n * (mtx[1] * p.x + mtx[3] * p.y + mtx[5]))};
					p = v->points[1];
					v->points[1] = {(m * (mtx[0] * p.x + mtx[2] * p.y + mtx[4])), (n * (mtx[1] * p.x + mtx[3] * p.y + mtx[5]))};
				}
				// Append vertices.
				if (num_vertices > 0) {
					for (bezier_curve& v : edges) {
						tmp.push_back(v);
					}
				}
				for (bezier_curve& v : comp_edges) {
					tmp.push_back(v);
				}
				edges = tmp;
			}
			// More components ?
			more = flags & (1 << 5);
		}
	} else {
		// numberOfCounters == 0, do nothing
	}
}

void mka::letter::rasterize_edges()
{
	calculate_bounding_box();

	this->pixels.data = new unsigned char[pixels.w * pixels.h];
	for (int i = 0; i < pixels.w * pixels.h; i++)
		this->pixels[i] = 0;

	point origin = {0, 0};

	for (mka::bezier_curve const& e : edges) {
		e.rasterize(this->pixels, origin);
	}

	edges.clear();
}

//std::vector<mka::bezier_curve> mka::letter::get_edges_from_vertices(const std::vector<vertex>& vertices, float objspace_flatness, int **contour_lengths, int *num_contours)
//{
//	std::vector<bezier_curve> edges{};
//	int num_points = 0;
//	bool error = false;
//
//	float objspace_flatness_squared = objspace_flatness * objspace_flatness;
//	int i,n=0,start=0;
//
//	// count how many "moves" there are to get the contour count
//	for (i=0; i < vertices.size(); ++i)
//		if (vertices[i].type == vmove)
//			++n;
//
//	*num_contours = n;
//	if (n == 0) return edges;
//
//	*contour_lengths = (int *) malloc(sizeof(**contour_lengths) * n);
//
//	if (*contour_lengths == nullptr) {
//		*num_contours = 0;
//		return edges;
//	}
//
//
//	// make two passes through the points so we don't need to realloc
//	point current_point;
//	num_points = 0;
//	n= -1;
//	for (i=0; i < vertices.size(); ++i) {
//		switch (vertices[i].type) {
//			case vmove:
//				// start the next contour
//				if (n >= 0)
//					(*contour_lengths)[n] = num_points - start;
//				++n;
//				start = num_points;
//
//				current_point = vertices[i].p0;
//				edges.push_back(mka::move(current_point));
//				break;
//			case vline:
//				current_point = vertices[i].p0;
//				edges.push_back(mka::edge::line(current_point, vertices[++i].p0));
//				break;
//			case vcurve:
//				edges.emplace_back(mka::edge::linear_curve(current_point,
//														   vertices[i].p1,
//														   vertices[i].p0));
//				current_point = vertices[i].p0;
//				break;
//			case vcubic:
//				edges.emplace_back(mka::edge::cubic_curve(current_point,
//														  vertices[i].p1,
//														  vertices[i].p2,
//														  vertices[i].p0));
//				current_point = vertices[i].p0;
//				break;
//		}
//	}
//	(*contour_lengths)[n] = num_points - start;
//	return edges;
//}
