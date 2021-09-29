#pragma once

#include <iostream>
#include <cmath>
#include <cstring>
#include <vector>
#include <string>

enum platform_id {
    UNICODE   =0,
    MAC       =1,
    ISO       =2,
    MICROSOFT =3
};

namespace mka {
	class point {
	public:
		double x, y;
		unsigned char flags;

	public:

		point() {
			x = 0, y = 0;
			flags = 0;
		}

		explicit point(unsigned char flags) {
			this->x = 0, this->y = 0;
			this->flags = flags;
		}


		point(double x, double y, unsigned char flags = 0) {
			this->x = x;
			this->y = y;
			this->flags = flags;
		}

		point operator+(mka::point& add) const {
			return {this->x + add.x, this->y + add.y};
		}

		point operator+(const mka::point& add) const {
			return {this->x + add.x, this->y + add.y};
		}

		point& operator+=(const mka::point& add) {
			this->x += add.x;
			this->y += add.y;
			return *this;
		}


		point operator/(double const& divisor) const {
			return {this->x/divisor, this->y/divisor};
		}

		point& operator/=(point const& divisor) {
			this->x /= divisor.x;
			this->y /= divisor.y;
			return *this;
		}

		point operator*(double const& multiplier) const {
			return {this->x*multiplier, this->y*multiplier};
		}

		point& operator*=(double const& multiplier) {
			this->x *= multiplier;
			this->y *= multiplier;
			return *this;
		}

		point operator*(point const& multiplier) const {
			return {this->x*multiplier.x, this->y*multiplier.y};
		}

		double sq() const {
			return this->x * this->y;
		}

		std::string to_string() const
		{
			return std::string("(") + std::to_string(this->x) + ", " + std::to_string(this->y) + ")";
		}

		bool is_zero() const {return x == 0 && y == 0;}



	};

	class bitmap {
	public:
		point size;
		unsigned char* data;

	public:
		~bitmap() {
			if (this->data != nullptr) {
				free(this->data);
				this->data = nullptr;
			}
		}
		bitmap() {
			this->size = {0, 0};
			this->data = nullptr;
		}

		bitmap(int width, int height) {
			this->size = {(double)width, (double)height};
			this->data = new unsigned char[width * height];
			for (int i = 0; i < width * height; i++) {
				this->data[i] = 0;
			}
		}


		unsigned char& operator[](int pos) const {
			return data[pos];
		}
	};


	class bezier_curve {
	public:
		std::vector<mka::point> points;

		static const int ACCURACY = 100;

	public:
		explicit bezier_curve(std::vector<point> const& points) {
			this->points = points;
		}

		bezier_curve() {
			this->points = std::vector<point>(1);
		}

		static mka::point calculate_value_from_points(std::vector<mka::point> const& points, double pos, int start_idx, int end_idx);

		mka::point calculate_at_position(double x) const;

		void rasterize(bitmap& out, point& origin) const;

		point& operator[](int idx) {
			return points[idx];
		}
	};


	class move : public bezier_curve {
	public:
		explicit move(point p) : bezier_curve({p}) {}
	};

	template <class T> class vec2 {
	public:
		T x,y;

	public:
		vec2() {
			this->x = 0;
			this->y = 0;
		};

		vec2(T x, T y) {
			this->x = x;
			this->y = y;
		}
	};


}

void sort_edges_ins_sort(std::vector<mka::bezier_curve>&, int);

void setvertex(mka::bezier_curve& v, char type, int x, int y, int cx, int cy);

void tesselate_cubic(std::vector<mka::point>& points, float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, float objspace_flatness_squared, int n);

int tesselate_curve(std::vector<mka::point>& points, float x0, float y0, float x1, float y1, float x2, float y2, float objspace_flatness_squared, int n);

