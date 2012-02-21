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

#include <string.h>

#include "libocad.h"

#define F_TEMPL "\ts%d\tx%d\ty%d\ta%lg\tu%lg\tv%lg\td%d\tp%d\tt%d\to%d"

int ocad_to_background(OCADBackground *bg, OCADTemplate *templ) {
	char tmp[1024];
	char *p = strchr(templ->str, 9);
	if (p == NULL) return -1;
	int sz = p - templ->str;
	memcpy(tmp, templ->str, sz); tmp[sz] = 0;
	bg->filename = (char *)my_strdup(tmp);
	if (10 == sscanf(p, F_TEMPL,
		&(bg->s), &(bg->trnx), &(bg->trny), &(bg->angle), &(bg->sclx), &(bg->scly),
		&(bg->dimming), &(bg->p), (int *)&(bg->transparent), &(bg->o))) {
		return 0;
	}
	return -1;
}

int ocad_template_size_background(OCADBackground *bg) {
	char tmp[1024];
	snprintf(tmp, 1024, "%s" F_TEMPL,
		bg->filename, bg->s, bg->trnx, bg->trny, bg->angle, bg->sclx, bg->scly,
		bg->dimming, bg->p, bg->transparent, bg->o);
	return strlen(tmp);
}

void ocad_background_to_string(char *buf, int size, OCADBackground *bg) {
	snprintf(buf, size, "%s" F_TEMPL,
		bg->filename, bg->s, bg->trnx, bg->trny, bg->angle, bg->sclx, bg->scly,
		bg->dimming, bg->p, bg->transparent, bg->o);
}




OCADTemplateIndex *ocad_template_index_first(OCADFile *pfile) {
	if (!pfile->header) return NULL;
	dword offs = pfile->header->otemplidx;
	if (offs == 0) return NULL;
	return (OCADTemplateIndex *)(pfile->buffer + offs);
}

OCADTemplateIndex *ocad_template_index_next(OCADFile *pfile, OCADTemplateIndex *current) {
	if (!pfile->header || !current) return NULL;
	dword offs = current->next;
	if (offs == 0) return NULL;
	return (OCADTemplateIndex *)(pfile->buffer + offs);
}

OCADTemplateEntry *ocad_template_entry_at(OCADFile *pfile, OCADTemplateIndex *current, int index) {
	if (!pfile->header || !current) return NULL;
	if (index < 0 || index >= 256) return NULL;
	return &(current->entry[index]);
}

OCADTemplateEntry *ocad_template_entry_new(OCADFile *pfile, u32 size) {
	if (!pfile->header) return NULL;
	OCADTemplateEntry *empty = NULL; // holder for first empty (size=0) index entry, if needed
	OCADTemplateIndex *idx;
	for (idx = ocad_template_index_first(pfile); idx != NULL; idx = ocad_template_index_next(pfile, idx)) {
		int i;
		for (i = 0; i < 256; i++) {
			OCADTemplateEntry *entry = &(idx->entry[i]);
			if (entry->type == 0) {
				if (entry->size == 0 && !empty) empty = entry;
				else if (entry->size >= size) return entry;
			}
		}
	}

	if (!empty) {
			// We don't have any suitable entries, but we do have an empty slot.
			return NULL; // FIXME
	}
	return NULL;
}

int ocad_template_remove(OCADFile *pfile, OCADTemplateEntry *entry) {
	entry->type = 0;
	return 0;
}

bool ocad_template_entry_iterate(OCADFile *pfile, OCADTemplateEntryCallback callback, void *param) {
	OCADTemplateIndex *idx;
	for (idx = ocad_template_index_first(pfile); idx != NULL; idx = ocad_template_index_next(pfile, idx)) {
		int i;
		for (i = 0; i < 256; i++) {
			OCADTemplateEntry *entry = &(idx->entry[i]);
			if (entry->type != 0) {
				if (!callback(param, pfile, entry)) return FALSE;
			}
		}
	}
	return TRUE;
}

OCADTemplate *ocad_template_at(OCADFile *pfile, OCADTemplateIndex *current, int index) {
	OCADTemplateEntry *entry = ocad_template_entry_at(pfile, current, index);
	if (entry == NULL) return NULL;
	dword offs = entry->ptr;
	if (offs == 0) return NULL;
	return (OCADTemplate *)(pfile->buffer + offs);
}

OCADTemplate *ocad_template(OCADFile *pfile, OCADTemplateEntry *entry) {
	if (!pfile->header) return NULL;
	if (entry == NULL || entry->ptr == 0) return NULL;
	return (OCADTemplate *)(pfile->buffer + entry->ptr);
}

int ocad_template_add_background(OCADFile *pfile, OCADBackground *bg) {
	int size = ocad_template_size_background(bg);
	OCADTemplateEntry *entry = ocad_template_entry_new(pfile, size);
	if (entry == NULL) return -1;
	OCADTemplate *templ = ocad_template(pfile, entry);
	ocad_background_to_string(templ->str, size, bg);
	entry->type = 8;
	entry->res1 = 0;
	entry->res2 = 0;
	return 0;
}
