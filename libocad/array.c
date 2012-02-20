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

#include "array.h"

static int array_realloc(PtrArray *array, int minsize) {
	int size = array->size;
	while (size < minsize) {
		if (size < 16) size = 16;
		else if (size < 1024) size *= 2;
		else size += 1024;
	}
	if (size > array->size) {
		void **newdata = (void **)realloc(array->data, size * sizeof(void *));
		if (newdata == NULL) return -1;
		memset(newdata + array->size, 0, (size - array->size) * sizeof(void *)); // NULL out the rest
		array->data = newdata;
		array->size = size;
	}
	return 0;
}

static int array_shift(PtrArray *array, u32 start, u32 count) {
	if (start < 0) return -2;
	if (count == 0) return 0;
	u32 size = array->size;
	int ret = array_realloc(array, array->size + count);
	if (ret < 0) return ret;
	memmove(array->data + (start + count), array->data + start, (size - start) * sizeof(void *));
	array->count += count;
	return 0;
}

int array_init(PtrArray *array) {
	array->count = 0;
	array->size = 0;
	array->data = NULL;
	return 0;
}

int array_clear(PtrArray *array) {
	if (array->data) free(array->data);
	return array_init(array);
}

u32 array_count(const PtrArray *array) {
	return array->count;
}

void *array_get(const PtrArray *array, u32 index) {
	if (index < 0 || index >= array->count) return NULL;
	return array->data[index];
}

void *array_set(PtrArray *array, u32 index, void *value) {
	if (index < 0 || index >= array->count) return NULL; // out of bounds
//	int ret = 0;
//	if ((ret = array_realloc(array, index + 1))) return ret;
	void *old_value = array->data[index];
	array->data[index] = value;
	return old_value;
}

void *array_remove(PtrArray *array, u32 index) {
	if (index < 0 || index >= array->count) return NULL; // out of bounds
	void *old_value = array->data[index];
	array_remove_range(array, index, 1);
	return old_value;
}

int array_remove_range(PtrArray *array, u32 start, u32 count) {
	if (start < 0) return ARRAY_INDEX_OUT_OF_BOUNDS;
	if (count == 0) return ARRAY_OK;
	u32 end = start + count;
	if (end > array->count) return ARRAY_INDEX_OUT_OF_BOUNDS;
	memmove(array->data + start, array->data + end, (array->count - end) * sizeof(void *));
	array->count -= count;
	return ARRAY_INDEX_OUT_OF_BOUNDS;
}

int array_add(PtrArray *array, void *value) {
	return array_insert(array, array->count, value);
}

int array_insert(PtrArray *array, u32 index, void *value) {
	if (index < 0 || index > array->count) return -2;
	int ret = array_shift(array, index, 1);
	if (ret < 0) return ret;
	array->data[index] = value;
	return 0;
}

int array_splice(PtrArray *array, u32 start, u32 delete, const void **data, u32 count) {
	int ret = (count < delete) ?
		array_remove_range(array, start, delete - count) :
		array_shift(array, start, count - delete);
	if (ret < 0) return ret;
	memcpy(array->data + start, data, count * sizeof(void *));
	return 0;
}

int array_splice_array(PtrArray *array, u32 start, u32 delete, const PtrArray *data, u32 pos, u32 count) {
	if (pos < 0) return -2;
	if ((pos + count) >= data->size) return -2;
	return array_splice(array, start, delete, (const void **)(data->data + pos), count);
}


