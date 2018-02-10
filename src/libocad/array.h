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

#ifndef ARRAY_H
#define ARRAY_H

#include "types.h"

/** A return code indicating that an array access used an out-of-bounds index. Not all array function
 *  will return this value; see the individual documentation for each function.
 */
#define ARRAY_INDEX_OUT_OF_BOUNDS -2

/** A return code indicating the system ran out of memory attempting to reallocate an array's data. 
 *  Generally this should be considered a fatal error.
 */
#define ARRAY_OUT_OF_MEMORY -1

/** A return code indicating a successful array operation.
 */
#define ARRAY_OK 0


/** This type represents a growable array of void pointers (void*).
 */
typedef
struct _PtrArray {
	u32 count;
	u32 size;
	void **data;
}
PtrArray;


/** Initializes a existing pointer array with an size of zero. Since the caller is responsible for 
 *  allocating the array, this function always returns ARRAY_OK.
 */
int array_init(PtrArray *array);

/** Removes all items from a pointer array and deallocates any storage associated with it. Always 
 *  returns ARRAY_OK.
 */
int array_clear(PtrArray *array);

/** Returns the number of items in this array.
 */
u32 array_count(const PtrArray *array);

/** Returns the pointer at a given position in the array. If the index is out of bounds,
 *  a nullptr pointer is returned.
 */
void *array_get(const PtrArray *array, u32 index);

/** Sets the pointer at a given position in the array and returns the former pointer at that location.
 *  If the index is out of bounds, the provided pointer is not stored, and nullptr is returned. The caller
 *  is responsible for owning the previous pointer, and determining whether to free it.
 */
void *array_set(PtrArray *array, u32 index, void *value);

/** Appends the given value to the end of the array. The array size is increased by one. Returns 
 *  ARRAY_OK or ARRAY_OUT_OF_MEMORY.
 */
int array_add(PtrArray *array, void *value);

/** Inserts the given value at a certain position in the array. The array size is increased by one. Returns 
 *  ARRAY_OK, ARRAY_INDEX_OUT_OF_BOUNDS, or ARRAY_OUT_OF_MEMORY.
 */
int array_insert(PtrArray *array, u32 index, void *value);

/** Removes the value at the given index from the array and returns the former pointer at that location.
 *  If the index is out of bounds, the array is unchanged and nullptr is returned. The caller is responsible
 *  for owning the previous pointer, and determining whether to free it.
 */
void *array_remove(PtrArray *array, u32 index);

/** Removes a range of indexes from the array. If any boundary condition is violated or the count is negative,
 *  the array is unchanged and ARRAY_INDEX_OUT_OF_BOUNDS is returned. A count of 0 is allowed and always 
 *  returns ARRAY_OK provided the index is in a valid range. The deleted pointers are not provided to the caller.
 */
int array_remove_range(PtrArray *array, u32 index, u32 count);

/** Combines a removal and insertion operation, replacing some subsection of the array with a sequence of 
 *  values (possibly of a different size). If any boundary condition is violated, or either the deletion or
 *  insertion count is negative, the array is unchanged and ARRAY_INDEX_OUT_OF_BOUNDS is returned. As in
 *  array_remove, counts of zero are allowed, so this function can serve to both insert and remove items
 *  from the array, but is more efficient when both operations must be done. 
 *	The function will return ARRAY_OUT_OF_MEMORY if it is unable to allocate memory for a growing array,
 *  and ARRAY_OK if the change was successful.
 */
int array_splice(PtrArray *array, u32 start, u32 del, const void **data, u32 count);

/** This function is like array_splice, but copies the inserted pointers from a another array defined by
 *  pos and count. The array defined in data is left unchanged.
 */
int array_splice_array(PtrArray *array, u32 start, u32 del, const PtrArray *data, u32 pos, u32 count);

#endif
