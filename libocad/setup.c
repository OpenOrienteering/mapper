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
