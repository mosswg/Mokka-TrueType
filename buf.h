//
// Created by Moss on 9/20/21.
//

#pragma once
#include <iostream>

namespace mka {
	class buf {
	public:
		unsigned char* data;
		unsigned int size;
		unsigned int cursor;

		buf();

		buf(const void* p, size_t size);

		void seek(unsigned int);

		void skip(unsigned int);

		unsigned char get8();

		unsigned int get(unsigned int);

		buf get_range(unsigned int, unsigned int) const;

		unsigned char peek8() const;

		buf get_subr(unsigned int);

		buf cff_get_index();

		buf cff_index_get(unsigned int);

		void get_ints(unsigned int, unsigned int, unsigned int*);

		buf get_key(unsigned int);

		int next_int();

		void skip_operand();
	};
}

