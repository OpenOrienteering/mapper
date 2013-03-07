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

#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "types.h"

typedef
struct _Path {
	u32 count;
	u8 *flags;
	double *coords;
}
Path;


typedef
struct _Transform {
	double m00;
	double m01;
	double m02;
	double m10;
	double m11;
	double m12;
}
Transform;


/** Allocates a path with room for a given number of points. The first parameter can be
 *  non-null for a preallocated Path structure, or NULL to create a new structure on the
 *  heap.
 */
int path_alloc(Path **ppath, u32 npts);


/** Transforms all the points in a path according to the given matrix.
 */
void path_map(Path *path, const Transform *mx);


/** Transforms all the points in a path according to the inverse of the given matrix. Returns
 *  TRUE if the path was mapped, FALSE if the mapping failed due to a noninvertible matrix.
 */
bool path_unmap(Path *path, const Transform *mx);


/** Resets the matrix to the identity matrix.
 */
void matrix_clear(Transform *matrix);


/** Translates the matrix by a given amount.
 */
void matrix_translate(Transform *matrix, double dx, double dy);


/** Rotates the matrix counterclockwise around the origin by the given angle, in degrees.
 */
void matrix_rotate(Transform *matrix, double angle);


/** Rotates the matrix counterclockwise around the given point by the given angle, in degrees.
 */
void matrix_rotate_point(Transform *matrix, double x, double y, double angle);


/** Scales the matrix by the given amount on each axis.
 */
void matrix_scale(Transform *matrix, double sx, double sy);


/** Maps a contiguous set of points from one array to another. The output array may overlap, or
 *  be the same, as the input array.
 */
void matrix_map(const Transform *matrix, const double *pts, double *opts, int count);


/** Maps a contiguous set of points from one array to another, using the inverse of the provided
 *  matrix. The output array may overlap, or be the same, as the input array. If the matrix is
 *  singular, FALSE is returned and no points are changed; otherwise all points will be mapped
 *  and TRUE is returned.
 */
bool matrix_unmap(const Transform *matrix, const double *pts, double *opts, int count);


/** Replaces the destination matrix with the inverse of the source matrix. Returns TRUE if successful,
 *  FALSE if the inversion failed because the source transform is singular.
 */
bool matrix_invert(Transform *dest, const Transform *src);

void matrix_dump(const Transform *mx);

#endif

