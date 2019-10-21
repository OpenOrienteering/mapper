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
#include "libocad.h"
#include "geometry.h"

// Internal path structure

#define MAX_PATH_POINTS 32768
#define F_PATH_CONTROL 1
#define F_PATH_CORNER 2
#define F_PATH_DASH 4
#define F_PATH_MOVE 8
#define F_PATH_NO_STROKE 16

typedef
struct _IntPath {
	u32 count;
	s32 x[MAX_PATH_POINTS];
	s32 y[MAX_PATH_POINTS];
	u8 f[MAX_PATH_POINTS];
}
IntPath;

/** Reverses a section of a u8 array. The caller is responsible for sending a valid range.
 */
static void reverse_u8(u8 *arr, u32 start, u32 count) {
	u32 i = start, j = start + count - 1;
	while (i < j) {
		u8 s = arr[i];
		arr[i] = arr[j];
		arr[j] = s;
		i++; j--;
	}
}


/** Reverses a section of an s32 array. The caller is responsible for sending a valid range.
 */
static void reverse_s32(s32 *arr, u32 start, u32 count) {
	u32 i = start, j = start + count - 1;
	while (i < j) {
		s32 s = arr[i];
		arr[i] = arr[j];
		arr[j] = s;
		i++; j--;
	}
}

/** Returns the signed area of a subsection of an IntPath, in map units.
 */
static s64 ocad_internal_path_area(IntPath *path, u32 start, u32 count) {
	s64 a = 0LL;
	u32 last = start + count - 1;
	s32 lx = path->x[start + count - 1], ly = path->y[last];
	int i;
	for (i = 0; i < count; i++) {
		s32 x = path->x[start + i], y = path->y[start + i];
		a = a + lx * y - ly * x;
		lx = x; ly = y;
	}
	return a;
}

/** Returns the orientation of a subsection of an IntPath, either 1 or -1.
 *  If the path is empty, 0 is returned.
 */
static s8 ocad_internal_path_area_sign(IntPath *path, u32 start, u32 count) {
	s64 area = ocad_internal_path_area(path, start, count);
	if (area < 0) return -1;
	if (area > 0) return 1;
	return 0;
}

/** Reverses a section of an IntPath
 */
static void ocad_internal_path_reverse(IntPath *path, u32 start, u32 count) {
	reverse_s32(path->x, start, count);
	reverse_s32(path->y, start, count);
	reverse_u8(path->f, start, count);
}


/** Converts a contiguous set of OCADPoints to an IntPath. The point values are copied, flags are
 *  adjusted, and any sections marked as holes are given the correct orientation. The result is
 *  a path that is suitable for use in PostScript or PDF. (The even-odd winding rule should be assumed)
 *
 */
static int ocad_path_to_internal(IntPath *path, u32 npts, const OCADPoint *pts) {
#define F_PATH_HOLE 0x80 // not publicly accessible, temporary and will be removed before the method returns
	u32 i;
	u32 hole_idx;
	u32 j;
	s8 sign;
	path->count = npts;
	for (i = 0; i < npts; i++) {
		u8 f;
		s32 x = pts[i].x, y = pts[i].y;
		path->x[i] = (x >> 8);
		path->y[i] = (y >> 8);
		f = 0;
		if (x & (PX_CTL1 | PX_CTL2)) f |= F_PATH_CONTROL;
        if (y & PY_CORNER) f |= F_PATH_CORNER;
		if (y & PY_DASH) f |= F_PATH_DASH;
		if (y & PY_HOLE) f |= F_PATH_HOLE;
		// F_PATH_NO_STROKE isn't used except for left and right paths on a line object
		path->f[i] = f;
	}
	// Check for holes. If we encounter a section of the path with the hole flag,
	// we need to see if its orientation matches the main segment. If it's the
	// same, then E/O would fail to make it a hole.
	hole_idx = 0;
	sign = 0;
	for (j = 0; j < npts; j++) {
		if (path->f[j] & F_PATH_HOLE) {
			path->f[j] &= ~F_PATH_HOLE;
			if (hole_idx == 0) {
				// This is the main part of the path; find its area
				sign = ocad_internal_path_area_sign(path, 0, j);
			}
			else {
				// We've reached the end of a hole segment
				s8 hole_sign = ocad_internal_path_area_sign(path, hole_idx, j - hole_idx);
				if (sign == hole_sign) ocad_internal_path_reverse(path, hole_idx, j - hole_idx);
			}
			hole_idx = j;
			path->f[j] |= F_PATH_MOVE;
		}
	}
	return 0;
}

/** Iterates over an IntPath. For each point in the given path, the callback is called and passed 
 *  the user_object, the segment type, and the segment endpoint.
 */
static bool ocad_internal_path_iterate(IntPath *path, IntPathCallback callback, void *user_object) {
	s32 pt[6]; int n = 0; bool first = TRUE; u32 i;
	for (i = 0; i < path->count; i++) {
		pt[n++] = path->x[i];
		pt[n++] = path->y[i];
		if (path->f[i] & F_PATH_CONTROL) continue;
		if (n == 2) {
			if (first) callback(user_object, MoveTo, pt); 
			else callback(user_object, SegmentLineTo, pt);
		}
		else if (n == 6) {
			if (first) return FALSE; else callback(user_object, CurveTo, pt);
		}
		first = FALSE; n = 0;
	}
	return TRUE;
}

/** Iterates over an OCADPath by first converting it to an IntPath, and then calling 
 *  ocad_internal_path_iterate.
 */
bool ocad_path_iterate(u32 npts, const OCADPoint *pts, IntPathCallback callback, void *param) {
	IntPath path;
	ocad_path_to_internal(&path, npts, pts);
	return ocad_internal_path_iterate(&path, callback, param);
}

/** Returns the signed area of a subsection of path, in map units.
 */
s64 ocad_path_area(s32 *path, u32 start, u32 count) {
	int i;
	s64 a = 0LL;
	s32 *p = path + 3 * (start + count - 1);
	s32 lx = p[0], ly = p[1];
	p = path + 3 * start;
	for (i = 0; i < count; i++) {
		s32 x = p[0], y = p[1];
		a = a + lx * y - ly * x;
		lx = x; ly = y; p += 3;
	}
	return a;
}

/** Applies a matrix transformation to a collection of contiguous OCADPoints. Typically one or both of the
 *  point arrays would be part of an OCADPath. All flags within the OCADPoints are preserved; only the
 *  positions are modified. The arrays may point to the same location in order to map a set of points 
 *  in-place.
 */
void ocad_path_map(const Transform *matrix, const OCADPoint *pts, OCADPoint *opts, u32 npts) {
	double p[2];
	const OCADPoint *pt = pts;
	u32 i;
	for (i = 0; i < npts; i++) {
		int x, y;
		p[0] = pt->x >> 8;
		p[1] = pt->y >> 8;
		matrix_map(matrix, p, p, 1);
		x = (int)my_round(p[0]);
		y = (int)my_round(p[1]);
		opts[i].x = x << 8 | (pt->x & 0xff);
		opts[i].y = y << 8 | (pt->y & 0xff);
		pt++;
	}
}

/** Applies the inverse of a matrix transformation to a collection of contiguous OCADPoints. This is done
 *  by inverting the matrix and then calling ocad_path_map. Returns TRUE if the call was successful, or
 *  FALSE if the supplied matrix was not invertible.
 */
bool ocad_path_unmap(const Transform *matrix, const OCADPoint *pts, OCADPoint *opts, u32 npts) {
    Transform imx;
	if (!matrix_invert(&imx, matrix)) return FALSE;
	ocad_path_map(&imx, pts, opts, npts);
	return TRUE;
}

/** Stores the smallest bounding rectangle that contains all the provided OCADPoints into rect.
 *  All flags within the OCADRect are set to 0. If npts is 0, then rect is not modified and
 *  FALSE is returned; otherwise, it is modified and TRUE is returned.
 */
bool ocad_path_bounds_rect(OCADRect *rect, u32 npts, const OCADPoint *pts) {
	s32 p[4];
	if (!ocad_path_bounds(p, npts, pts)) return FALSE;
	rect->min.x = p[0] << 8; rect->min.y = p[1] << 8;
	rect->max.x = p[2] << 8; rect->max.y = p[3] << 8;
	return TRUE;
}

/** Stores the smallest bounding rectangle that contains all the provided OCADPoints into rect,
 *  as an array of four integers in the order (min-x, min-y, max-x, max-y). If npts is 0, then
 *  the array is not modified and FALSE is returned; otherwise, it is modified and TRUE is 
 *  returned. 
 */
bool ocad_path_bounds(s32 *rect, u32 npts, const OCADPoint *pts) {
	int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
	u32 i;
	if (npts == 0) return FALSE;
	for (i = 0; i < npts; i++) {
		int x = pts[i].x >> 8, y = pts[i].y >> 8;
		if (i == 0) {
			x1 = x2 = x;
			y1 = y2 = y;
		}
		else {
			if (x < x1) x1 = x;
			if (y < y1) y1 = y;
			if (x > x2) x2 = x;
			if (y > y2) y2 = y;
		}
	}
	rect[0] = x1; rect[1] = y1;
	rect[2] = x2; rect[3] = y2;
	return TRUE;
}

/** Grows the boundaries of an OCADRect by the given amount, in map units. The flags in the
 *  OCADRect are unaffected. This function always returns TRUE.
 */
bool ocad_rect_grow(OCADRect *prect, s32 amount) {
	s32 amt = amount << 8;
	prect->min.x -= amt;
	prect->min.y -= amt;
	prect->max.x += amt;
	prect->max.y += amt;
	return TRUE;
}

/** Calculates the intersection of r2 with r1. If the result is empty, r1 is unchanged and FALSE
 *  is returned. Otherwise, the result is stored in r1 and TRUE is returned.
 */
bool ocad_rect_intersects(const OCADRect *r1, const OCADRect *r2) {
	s32 x1 = r1->min.x & ~0xff, y1 = r1->min.y & ~0xff;
	s32 x2 = r1->max.x & ~0xff, y2 = r1->max.y & ~0xff;
	s32 x3 = r2->min.x & ~0xff, y3 = r2->min.y & ~0xff;
	s32 x4 = r2->max.x & ~0xff, y4 = r2->max.y & ~0xff;
	if (x2 < x3 || x4 < x1) return FALSE;
	if (y2 < y3 || y4 < y1) return FALSE;
	return TRUE;
}

/** Merges the second rectangle into the first. The flags in the
 *  OCADRect are unaffected. This function always returns TRUE.
 */
void ocad_rect_union(OCADRect *r1, const OCADRect *r2) {
	s32 x1 = r1->min.x & ~0xff, y1 = r1->min.y & ~0xff;
	s32 x2 = r1->max.x & ~0xff, y2 = r1->max.y & ~0xff;
	s32 x3 = r2->min.x & ~0xff, y3 = r2->min.y & ~0xff;
	s32 x4 = r2->max.x & ~0xff, y4 = r2->max.y & ~0xff;
	if (x3 < x1) x1 = x3;
	if (y3 < y1) y1 = y3;
	if (x4 > x2) x2 = x4;
	if (y4 > y2) y2 = y4;
	r1->min.x = (r1->min.x & 0xff) | (x1 << 8);
	r1->max.y = (r1->min.y & 0xff) | (y1 << 8);
	r1->min.x = (r1->max.x & 0xff) | (x2 << 8);
	r1->max.y = (r1->max.y & 0xff) | (y2 << 8);
}



