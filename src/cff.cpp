//
// Created by Moss on 9/20/21.
//


#include "cff.h"
#include <vector>
#include "macros.h"


void mka::bezier_curve::rasterize(mka::bitmap& out, point& origin, point const& scale) const
{
	if (points.empty())
		return;

	double width = fabs(points.back().x - points.front().x); 	// This is not accurate because the endpoints are not necessarily the widest point,
	double height = fabs(points.back().y - points.front().y);	// but it gives a good estimation for our purposes
	double size = width + height;
	double increment = 1/(bezier_curve::ACCURACY * size);

	point current;
	point current_pixel;
	double value_from_dist_to_pixel;
	double min_dist_to_pixel;
	double dist_to_nearest_pixel;

	for (double i = 0; i <= 1; i+=increment) {
		current = this->calculate_at_position(i);

		if (round(current.x+origin.x) > out.w || round(current.x + origin.x) < 0) {
			continue; // So we don't wrap around edges.
		}
		if (round(current.y+origin.y) > out.h || round(current.y+origin.y) < 0) {
			continue; // So we don't write to out of bounds memory.
		}

		if (round(current.x) != current_pixel.x || round(current.y) != current_pixel.y || i > (increment-1)) {
			value_from_dist_to_pixel = (1.0/(min_dist_to_pixel+1)) * 255;

			//std::cout << "Writing " << value_from_dist_to_pixel << " to (" << current_pixel.x+origin.x << ", " << current_pixel.y+origin.y << ")\n";

			unsigned char& curr_char = out[(int)(((current_pixel.y+origin.y) * out.w) + (current_pixel.x+origin.x))];
			curr_char = value_from_dist_to_pixel > curr_char ? (int)round(value_from_dist_to_pixel) : curr_char;
			min_dist_to_pixel = 10;
		}
		current_pixel.x = (int)round(current.x);
		current_pixel.y = (int)round(current.y);

		dist_to_nearest_pixel = (fabs(current.x - (double)current_pixel.x) + fabs(current.y - double(current_pixel.y)));


		min_dist_to_pixel = dist_to_nearest_pixel < min_dist_to_pixel ? dist_to_nearest_pixel : min_dist_to_pixel;
	}
	origin = current_pixel;
}

mka::point mka::bezier_curve::calculate_value_from_points(std::vector<mka::point> const& points, double t, int start_idx, int end_idx) {
	if (start_idx == end_idx) {
		return points[start_idx];
	}

	return (calculate_value_from_points(points, t, start_idx, end_idx-1) * (1-t)) + (calculate_value_from_points(points, t, start_idx+1, end_idx) * t);
}

mka::point mka::bezier_curve::calculate_at_position(double t) const {
	return calculate_value_from_points(this->points, t, 0, points.size()-1);
}
