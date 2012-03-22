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
#include "array.h"

static dword ocad_alloc_object(u32 npts) {
	int size = 0x20 + 8 * npts;
	size++;
	return 0; // FIXME
}

static bool ocad_object_copy(OCADObject *dest, const OCADObject *src) {
	u32 ssize = ocad_object_size(src);
	u32 dsize = ocad_object_size(dest);
	if (dsize < ssize) return FALSE;
	memcpy(dest, src, ssize);
	memset(dest + ssize, 0, dsize - ssize);
	return TRUE;
}



OCADObjectIndex *ocad_objidx_first(OCADFile *pfile) {
	if (!pfile->header) return NULL;
	dword offs = pfile->header->oobjidx;
	if (offs == 0) return NULL;
	return (OCADObjectIndex *)(pfile->buffer + offs);
}

OCADObjectIndex *ocad_objidx_next(OCADFile *pfile, OCADObjectIndex *current) {
	if (!pfile->header || !current) return NULL;
	dword offs = current->next;
	if (offs == 0) return NULL;
	return (OCADObjectIndex *)(pfile->buffer + offs);
}

OCADObjectEntry *ocad_object_entry_at(OCADFile *pfile, OCADObjectIndex *current, int index) {
	if (!pfile->header || !current) return NULL;
	if (index < 0 || index >= 256) return NULL;
	return &(current->entry[index]);
}

OCADObjectEntry *ocad_object_entry_new(OCADFile *pfile, u32 npts) {
	if (!pfile->header) return NULL;
	if (npts == 0) return NULL;
	OCADObjectEntry *empty = NULL; // holder for first empty (npts=0) index entry, if needed
	OCADObjectIndex *idx;
	for (idx = ocad_objidx_first(pfile); idx != NULL; idx = ocad_objidx_next(pfile, idx)) {
		int i;
		for (i = 0; i < 256; i++) {
			OCADObjectEntry *entry = &(idx->entry[i]);
			if (entry->symbol == 0) {
				if (entry->npts == 0 && !empty) empty = entry;
				else if (entry->npts >= npts) return entry;
			}
		}
	}

	if (!empty) {
		// We don't have any empty entries - need to create a new one!
		return NULL; // FIXME
	}

	// There exists an empty index entry, with symbol=0 and npts=0. We can allocate a new object and fill it
	dword offs = ocad_alloc_object(npts);
	if (offs == 0) return NULL; // no memory
	empty->ptr = offs;
	empty->npts = npts;
	// symbol, min, and max still need to be updated by the caller!
	return empty;
}

void ocad_object_entry_refresh(OCADFile *pfile, OCADObjectEntry *entry, OCADObject *object) {
	OCADRect *prect = &(entry->rect);
	if (!ocad_path_bounds_rect(prect, object->npts, object->pts)) {
		memset(prect, 0, sizeof(OCADRect)); // Object with no points get a zeroed bounds rect
	}
	OCADSymbol *symbol = ocad_symbol(pfile, object->symbol);
	if (symbol && symbol->extent > 0) {
		ocad_rect_grow(prect, symbol->extent);
	}
	entry->symbol = object->symbol;
}


int ocad_object_remove(OCADFile *pfile, OCADObjectEntry *entry) {
	if (entry == NULL) return -1;
	entry->symbol = 0;
	dword offs = entry->ptr;
	if (offs != 0) {
		OCADObject *obj = (OCADObject *)(pfile->buffer + offs);
		obj->symbol = 0;
	}
	return 0;
}

bool ocad_object_entry_iterate(OCADFile *pfile, OCADObjectEntryCallback callback, void *param) {
	OCADObjectIndex *idx;
	for (idx = ocad_objidx_first(pfile); idx != NULL; idx = ocad_objidx_next(pfile, idx)) {
		int i;
		for (i = 0; i < 256; i++) {
			OCADObjectEntry *entry = ocad_object_entry_at(pfile, idx, i);
			if (entry != NULL && entry->ptr && entry->symbol) {
				if (!callback(param, pfile, entry)) return FALSE;
			}
		}
	}
	return TRUE;
}

OCADObject *ocad_object_at(OCADFile *pfile, OCADObjectIndex *current, int index) {
	OCADObjectEntry *entry = ocad_object_entry_at(pfile, current, index);
	if (entry == NULL) return NULL;
	if (entry->symbol == 0) return NULL;
	dword offs = entry->ptr;
	if (offs == 0) return NULL;
	//fprintf(stderr, "offs=%x\n", offs);
	return (OCADObject *)(pfile->buffer + offs);
}

OCADObject *ocad_object(OCADFile *pfile, OCADObjectEntry *entry) {
	if (!pfile->header) return NULL;
	if (entry == NULL || entry->ptr == 0) return NULL;
	if (entry->symbol == 0) return NULL;
	return (OCADObject *)(pfile->buffer + entry->ptr);
}

bool ocad_object_iterate(OCADFile *pfile, OCADObjectCallback callback, void *param) {
	OCADObjectIndex *idx;
	for (idx = ocad_objidx_first(pfile); idx != NULL; idx = ocad_objidx_next(pfile, idx)) {
		int i;
		for (i = 0; i < 256; i++) {
			OCADObject *obj = ocad_object_at(pfile, idx, i);
			if (obj != NULL) {
				if (!callback(param, pfile, obj)) return FALSE;
			}
		}
	}
	return TRUE;
}

u32 ocad_object_size(const OCADObject *object) {
	if (object == NULL) return 0;
	return ocad_object_size_npts(object->npts + object->ntext);
}


u32 ocad_object_size_npts(u32 npts) {
	return 0x20 + 8 * npts;
}

OCADObject *ocad_object_alloc(const OCADObject *source) {
	int size = ocad_object_size_npts(OCAD_MAX_OBJECT_PTS);
	OCADObject *obj = (OCADObject *)malloc(size);
	if (obj == NULL) return NULL;
	if (source != NULL) {
		int ssize = ocad_object_size(source);
		memcpy(obj, source, ssize);
		memset(obj + ssize, 0, size - ssize);
	}
	else {
		memset(obj, 0, size);
	}
	return obj;
}


OCADObject *ocad_object_add(OCADFile *file, const OCADObject *object) {
	OCADObjectEntry *entry = ocad_object_entry_new(file, object->npts + object->ntext);
	if (entry == NULL) return NULL;
	OCADObject *dest = ocad_object(file, entry);
	if (!ocad_object_copy(dest, object)) return NULL;
	ocad_object_entry_refresh(file, entry, dest);
	return dest;
}

