//
// Created by Moss on 9/20/21.
//


#include "cff.h"
#include <vector>
#include "macros.h"

//int tesselate_curve(std::vector<mka::point>& points, float x0, float y0, float x1, float y1, float x2, float y2, float objspace_flatness_squared, int n)
//{
//    // midpoint
//    float mx = (x0 + 2*x1 + x2)/4;
//    float my = (y0 + 2*y1 + y2)/4;
//    // versus directly drawn line
//    float dx = (x0+x2)/2 - mx;
//    float dy = (y0+y2)/2 - my;
//    if (n > 16) // 65536 segments on one curve better be enough!
//        return 1;
//    if (dx*dx+dy*dy > objspace_flatness_squared) { // half-pixel error allowed... need to be smaller if AA
//        tesselate_curve(points, x0,y0, (x0+x1)/2.0f,(y0+y1)/2.0f, mx,my, objspace_flatness_squared,n+1);
//        tesselate_curve(points, mx,my, (x1+x2)/2.0f,(y1+y2)/2.0f, x2,y2, objspace_flatness_squared,n+1);
//    } else {
//		points.push_back({x2,y2});
//    }
//    return 1;
//}


//void tesselate_cubic(std::vector<mka::point>& points, float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, float objspace_flatness_squared, int n)
//{
//    // @TODO this "flatness" calculation is just made-up nonsense that seems to work well enough
//    float dx0 = x1-x0;
//    float dy0 = y1-y0;
//    float dx1 = x2-x1;
//    float dy1 = y2-y1;
//    float dx2 = x3-x2;
//    float dy2 = y3-y2;
//    float dx = x3-x0;
//    float dy = y3-y0;
//    double longlen = (sqrt(dx0*dx0+dy0*dy0)+sqrt(dx1*dx1+dy1*dy1)+sqrt(dx2*dx2+dy2*dy2));
//    double shortlen = sqrt(dx*dx+dy*dy);
//    float flatness_squared = longlen*longlen-shortlen*shortlen;
//
//    if (n > 16) // 65536 segments on one curve better be enough!
//        return;
//
//    if (flatness_squared > objspace_flatness_squared) {
//        float x01 = (x0+x1)/2;
//        float y01 = (y0+y1)/2;
//        float x12 = (x1+x2)/2;
//        float y12 = (y1+y2)/2;
//        float x23 = (x2+x3)/2;
//        float y23 = (y2+y3)/2;
//
//        float xa = (x01+x12)/2;
//        float ya = (y01+y12)/2;
//        float xb = (x12+x23)/2;
//        float yb = (y12+y23)/2;
//
//        float mx = (xa+xb)/2;
//        float my = (ya+yb)/2;
//
//        tesselate_cubic(points, x0,y0, x01,y01, xa,ya, mx,my, objspace_flatness_squared,n+1);
//        tesselate_cubic(points, mx,my, xb,yb, x23,y23, x3,y3, objspace_flatness_squared,n+1);
//    } else {
//		points.emplace_back(x3, y3);
//    }
//}




void mka::bezier_curve::rasterize(mka::bitmap& out, point& origin) const
{

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
