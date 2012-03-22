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

#include "libocad.h"

OCADSymbolIndex *ocad_symidx_first(OCADFile *pfile) {
	if (!pfile || !pfile->header) return NULL;
	dword offs = pfile->header->osymidx;
	if (offs == 0) return NULL;
	return (OCADSymbolIndex *)(pfile->buffer + offs);
}

OCADSymbolIndex *ocad_symidx_next(OCADFile *pfile, OCADSymbolIndex *current) {
	if (!pfile || !pfile->header || !current) return NULL;
	dword offs = current->next;
	if (offs == 0) return NULL;
	return (OCADSymbolIndex *)(pfile->buffer + offs);
}

int ocad_symbol_count(OCADFile *pfile) {
	if (!pfile || !pfile->header) return -1;
	int count = 0;
	OCADSymbolIndex *idx;
	for (idx = ocad_symidx_first(pfile); idx != NULL; idx = ocad_symidx_next(pfile, idx)) {
		int i;
		for (i = 0; i < 256; i++) {
			OCADSymbol *sym = ocad_symbol_at(pfile, idx, i);
			if (sym != NULL) count++;
		}
	}
	return count;
}

OCADSymbol *ocad_symbol_at(OCADFile *pfile, OCADSymbolIndex *current, int index) {
	if (!pfile || !pfile->header || !current) return NULL;
	if (index < 0 || index >= 256) return NULL;
	dword offs = current->entry[index].ptr;
	if (offs == 0) return NULL;
	return (OCADSymbol *)(pfile->buffer + offs);
}

OCADSymbol *ocad_symbol(OCADFile *pfile, word number) {
	OCADSymbolIndex *idx;
	for (idx = ocad_symidx_first(pfile); idx != NULL; idx = ocad_symidx_next(pfile, idx)) {
		int i;
		for (i = 0; i < 256; i++) {
			OCADSymbol *sym = ocad_symbol_at(pfile, idx, i);
			if (sym != NULL && sym->number == number) return sym;
		}
	}
	return NULL;
}

bool ocad_symbol_iterate(OCADFile *pfile, OCADSymbolCallback callback, void *param) {
	OCADSymbolIndex *idx;
	for (idx = ocad_symidx_first(pfile); idx != NULL; idx = ocad_symidx_next(pfile, idx)) {
		int i;
		for (i = 0; i < 256; i++) {
			OCADSymbol *sym = ocad_symbol_at(pfile, idx, i);
			if (sym != NULL) {
				if (!callback(param, pfile, sym)) return FALSE;
			}
		}
	}
	return TRUE;
}

bool ocad_symbol_uses_color(const OCADSymbol *symbol, s16 number) {
	if (number < 0 || number > 256) return FALSE;
	return (symbol->colors[number / 8] & (0x1 << (number % 8)));
}


bool ocad_symbol_element_iterate(s16 ngrp, OCADPoint *pts, OCADSymbolElementCallback callback, void *param) {
	OCADPoint *p = pts, *end = pts + ngrp;
	while (p < end) {
		OCADSymbolElement *elt = (OCADSymbolElement *)p;
		if (!callback(param, elt)) return FALSE;
		p += (2 + elt->npts);
	}
	return TRUE;
}

bool ocad_symbol_elements_use_color(s16 ngrp, OCADPoint *pts, s16 number) {
	OCADPoint *p = pts, *end = pts + ngrp;
	while (p < end) {
		OCADSymbolElement *elt = (OCADSymbolElement *)p;
		if (elt->color == number) return TRUE;
		p += (2 + elt->npts);
	}
	return FALSE;
}
