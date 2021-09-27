//
// Copyright (c) 2021 Moss Gallagher.
//
#pragma once

#define mka_assert(boolean, ec) if (!(boolean)) { std::cout << "Assert " << (ec) << " Failed. Exiting..." << std::endl; exit(ec); }

#define mka_assert_unreachable(i) std::cout << "Unreachable Code Executed. Exiting..." << std::endl; exit(-100 * (i))

#define get_ushort(p) ((unsigned short)((p)[0]<<8) | (p)[1])
#define get_short(p)   ((short)(((p)[0]<<8) | (p)[1]))
#define get_ulong(p)  (unsigned long)(((p)[0]<<24) | ((p)[1]<<16) | ((p)[2]<<8) | (p)[3])
#define get_long(p)    ((long)(((p)[0]<<24) | ((p)[1]<<16) | ((p)[2]<<8) | (p)[3]))
#define get_byte(p)     (* (unsigned char *) (p))
#define get_char(p)     (* (char *) (p))

