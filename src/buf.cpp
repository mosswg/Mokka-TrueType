//
// Created by Moss on 9/20/21.
//
// Inspired by stb_truetype
//
#include <cstdlib>
#include "buf.h"
#include "macros.h"


mka::buf::buf() {
	data = nullptr;
	size = 0;
	cursor = 0;
}

mka::buf::buf(const void* p, size_t size)
{
    mka_assert(size < 0x40000000, -3);
    this->data = (unsigned char*) p;
    this->size = (int) size;
    this->cursor = 0;
}

void mka::buf::seek(unsigned int o)
{
    mka_assert(!(o > this->size || o < 0), -4);
    this->cursor = ((o > this->size || o < 0) ? this->size : o);
}

void mka::buf::skip(unsigned int n) {
    this->seek(this->cursor + n);
}

unsigned char mka::buf::get8()
{
    if (this->cursor >= this->size)
        return 0;
    return this->data[this->cursor++];
}

unsigned int mka::buf::get(unsigned int n)
{
	unsigned int v = 0;
    int i;
    mka_assert(n >= 1 && n <= 4, -6);
    for (i = 0; i < n; i++)
        v = (v << 8) | this->get8();
    return v;
}

mka::buf mka::buf::get_range(unsigned int start, unsigned int end) const
{
    mka::buf r = mka::buf(nullptr, 0);
    if (start < 0 || end < 0 || start > this->size || end > this->size - start) return r;
    r.data = this->data + start;
    r.size = end;
    return r;
}



unsigned char mka::buf::peek8() const
{
    if (this->cursor >= this->size)
        return 0;
    return this->data[this->cursor];
}


mka::buf mka::buf::get_subr(unsigned int n)
{

	this->seek(0);
	int count = this->get(2);
    int bias = 107;
    if (count >= 33900)
        bias = 32768;
    else if (count >= 1240)
        bias = 1131;
    n += bias;
    if (n < 0 || n >= count)
        return {};
    return this->cff_index_get(n);
}

mka::buf mka::buf::cff_get_index()
{
	unsigned int count, start, offsize;
    start = this->cursor;
    count = this->get(2);
    if (count) {
        offsize = this->get8();
        mka_assert(offsize >= 1 && offsize <= 4, -7);
        this->skip(offsize * count);
        this->skip(this->get(offsize) - 1);
    }
    return this->get_range(start, this->cursor - start);
}

mka::buf mka::buf::cff_index_get(unsigned int i)
{
	unsigned int count, offsize, start, end;
	this->seek(0);
	count = this->get(2);
	offsize = this->get8();
	mka_assert(i >= 0 && i < count, -6);
	mka_assert(offsize >= 1 && offsize <= 4, -7);
	this->skip(i*offsize);
	start = this->get(offsize);
	end = this->get(offsize);
	return this->get_range(2+(count+1)*offsize+start, end - start);
}
mka::buf mka::buf::get_key(unsigned int key)
{
	this->seek(0);
	while (this->cursor < this->size) {
		int start = this->cursor, end, op;
		while (this->peek8() >= 28)
			this->skip_operand();
		end = this->cursor;
		op = this->get8();
		if (op == 12)  op = this->get8() | 0x100;
		if (op == key) return this->get_range(start, end-start);
	}
	return this->get_range(0, 0);
}

void mka::buf::get_ints(unsigned int key, unsigned int outcount, unsigned int *out) {
	int i;
	mka::buf operands = this->get_key(key);
	for (i = 0; i < outcount && operands.cursor < operands.size; i++)
		out[i] = operands.next_int();
}

int mka::buf::next_int()
{
	int b0 = this->get8();
	if (b0 >= 32 && b0 <= 246)       return b0 - 139;
	else if (b0 >= 247 && b0 <= 250) return (b0 - 247)*256 + this->get8() + 108;
	else if (b0 >= 251 && b0 <= 254) return -(b0 - 251)*256 - this->get8() - 108;
	else if (b0 == 28)               return this->get(2);
	else if (b0 == 29)               return this->get(4);

	mka_assert_unreachable(-4);
}

void mka::buf::skip_operand() {
	int v, b0 = this->peek8();
	mka_assert(b0 >= 28, -8);
	if (b0 == 30) {
		this->skip(1);
		while (this->cursor < this->size) {
			v = this->get8();
			if ((v & 0xF) == 0xF || (v >> 4) == 0xF)
				break;
		}
	} else {
		this->next_int();
	}
}
