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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "geometry.h"

// For some reason M_* constants aren't defined in c99 mode (-D__STRICT_ANSI__) under MinGW
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int path_alloc(Path **ppath, u32 npts) {
	int err = -1;
	Path *p = *ppath;
	if (p == NULL) {
		p = (Path *)malloc(sizeof(Path));
		if (p == NULL) { err = -1; goto path_alloc_0; }
	}
	memset(p, 0, sizeof(Path));
	p->count = npts;
	p->coords = (double *)malloc(2 * npts * sizeof(double));
	if (p->coords == NULL) { err = -1; goto path_alloc_1; }
	p->flags = (u8 *)malloc(npts * sizeof(u8));
	if (p->flags == NULL) { err = -1; goto path_alloc_2; }
	*ppath = p;
	return 0;

path_alloc_2:
	free(p->coords); p->coords = NULL;
path_alloc_1:
	free(p); p = NULL;
path_alloc_0:
	return err;
}


void path_map(Path *path, const Transform *mx) {
	matrix_map(mx, path->coords, path->coords, path->count);
}

bool path_unmap(Path *path, const Transform *mx) {
	return matrix_unmap(mx, path->coords, path->coords, path->count);
}


void matrix_clear(Transform *matrix) {
	matrix->m00 = matrix->m11 = 1;
	matrix->m01 = matrix->m10 = 0;
	matrix->m02 = matrix->m12 = 0;
}

void matrix_translate(Transform *matrix, double dx, double dy) {
	matrix->m02 += dx;
	matrix->m12 += dy;
}

void matrix_rotate(Transform *matrix, double angle) {
	if (angle == 0.0) return;
	double a = M_PI * angle / 180.0, c = cos(a), s = sin(a);
	double n00 = matrix->m00 * c - matrix->m01 * s;
	double n01 = matrix->m00 * s + matrix->m01 * c;
	double n10 = matrix->m10 * c - matrix->m11 * s;
	double n11 = matrix->m10 * s + matrix->m11 * c;
	matrix->m00 = n00; matrix->m01 = n01;
	matrix->m10 = n10; matrix->m11 = n11;
}

void matrix_rotate_point(Transform *matrix, double x, double y, double angle) {
	matrix_translate(matrix, -x, -y);
	matrix_rotate(matrix, angle);
	matrix_translate(matrix, x, y);
}

void matrix_scale(Transform *matrix, double sx, double sy) {
	matrix->m00 *= sx; matrix->m01 *= sx; matrix->m02 *= sx;
	matrix->m10 *= sy; matrix->m11 *= sy; matrix->m12 *= sy;
}

void matrix_map(const Transform *matrix, const double *pts, double *opts, int count) {
	int n = 2 * count;
	if (opts > pts) {
		int i;
		for (i = n - 2; i >= 0; i -= 2) {
			double x = pts[i], y = pts[i + 1];
			opts[i] = matrix->m00 * x + matrix->m01 * y + matrix->m02;
			opts[i + 1] = matrix->m10 * x + matrix->m11 * y + matrix->m12;
		}
	}
	else {
		int i;
		for (i = 0; i < n; i += 2) {
			double x = pts[i], y = pts[i + 1];
			opts[i] = matrix->m00 * x + matrix->m01 * y + matrix->m02;
			opts[i + 1] = matrix->m10 * x + matrix->m11 * y + matrix->m12;
		}
	}
}

bool matrix_unmap(const Transform *matrix, const double *pts, double *opts, int count) {
	// FIXME: do this more efficiently?
    Transform imx;
	if (!matrix_invert(&imx, matrix)) return FALSE;
	matrix_map(&imx, pts, opts, count);
	return TRUE;
}

bool matrix_invert(Transform *dest, const Transform *src) {
	double det = src->m00 * src->m11 - src->m01 * src->m10;
	if (fabs(det) < 1e-7) return FALSE;
	dest->m00 = src->m11 / det;
	dest->m01 = -src->m01 / det;
	dest->m02 = (src->m01 * src->m12 - src->m11 * src->m02) / det;
	dest->m10 = src->m00 / det;
	dest->m11 = -src->m11 / det;
	dest->m12 = (src->m00 * src->m12 - src->m10 * src->m02) / det;
	return FALSE;
}

void matrix_dump(const Transform *mx) {
	fprintf(stderr, "m00=%g m01=%g m02=%g : m10=%g m11=%g m12=%g\n",
		mx->m00, mx->m01, mx->m02, mx->m10, mx->m11, mx->m12);
}
