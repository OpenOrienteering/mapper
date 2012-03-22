/*
 *    Copyright 2012 Peter Curtis
 *
 *    This file is part of libocad.
 *
 *    libocad is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    libocad is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with libocad.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>

#include "libocad.h"

char *my_strdup(const char *s) {
	int size = strlen(s) + 1;
	char *p = malloc(size);
	if (p) {
		memcpy(p, s, size);
	}
	return p;
}
int my_round (double x) {
	int i = (int) x;
	if (x >= 0.0) {
		return ((x-i) >= 0.5) ? (i + 1) : (i);
	} else {
		return (-x+i >= 0.5) ? (i - 1) : (i);
	}
}

const s32 *ocad_point(s32 *buf, const OCADPoint *pt) {
	buf[0] = pt->x >> 8;
	buf[1] = pt->y >> 8;
	buf[2] = (pt->x & 0xff) | (pt->y & 0xff) << 8;
	return buf;
}

const s32 *ocad_point2(s32 **pbuf, const OCADPoint *pt) {
	s32 *buf = *pbuf;
	ocad_point(buf, pt);
	*pbuf = buf + 3;
	return buf;
}


const char *ocad_str(char *buf, const str *ostr) {
	int n = *ostr;
	memcpy(buf, ostr + 1, n);
	buf[n] = 0;
	return buf;
}

const char *ocad_str2(char **pbuf, const str *ostr) {
	char *buf = *pbuf;
	int n = *ostr;
	memcpy(buf, ostr + 1, n);
	buf[n] = 0;
	*pbuf = buf + (n + 1);
	return buf;
}

void ocad_to_real(s32 *from, double *to, u32 count) {
	u32 n = count * 2;
	u32 i;
	for (i = 0; i < n; i += 2) {
		double x = from[i], y = from[i + 1];
		to[i] = x; to[i + 1] = y;
	}
}

void dump_bytes(u8 *base, u32 size) {
	u32 i;
	for (i = 0; i < size; i++) {
		if (i % 16 == 0) fprintf(stderr, "%06x:", i);
		fprintf(stderr, " %02x", base[i]);
		if (i %16 == 15) fprintf(stderr, "\n");
	}
	if (size % 16 != 15) fprintf(stderr, "\n");
}
