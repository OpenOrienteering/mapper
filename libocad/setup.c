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

bool ocad_setup_world_matrix(OCADFile *pfile, Transform *matrix) {
	if (!pfile || !pfile->setup) return FALSE;
	OCADSetup *setup = pfile->setup;
	//if (!setup->realcoord) return FALSE;
	matrix_clear(matrix);
	double s = 72000 / 25.4 / setup->scale;
	matrix_rotate(matrix, setup->angle);
	matrix_scale(matrix, s, s);
	matrix_translate(matrix, setup->offsetx, setup->offsety);
	//matrix_dump(matrix);
	return TRUE;
}

bool ocad_setup_paper_matrix(OCADFile *pfile, Transform *matrix) {
	if (!pfile || !pfile->setup) return FALSE;
	matrix_clear(matrix);
	double s = 72.0 / 2540;
	matrix_scale(matrix, s, s);
	return TRUE;
}
