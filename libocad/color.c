#include <math.h>
#include "libocad.h"

static float ocad_color_scale_rgbf(int value) {
	float v = 0.005f * value;
	if (v < 0) v = 0;
	if (v > 1) v = 1;
	return v;
}

static int ocad_color_scale_rgb(int value) {
	float v = ocad_color_scale_rgbf(value);
	return (int)round(255 * v);
}

int ocad_color_count(OCADFile *pfile) {
	if (!pfile->header) return -1;
	return pfile->header->ncolors;
}

OCADColor *ocad_color_at(OCADFile *pfile, int index) {
	if (!pfile->header) return NULL;
	if (index < 0 || index >= pfile->header->ncolors) return NULL;
	return (pfile->colors + index);
}

OCADColor *ocad_color(OCADFile *pfile, u8 number) {
	if (!pfile->header) return NULL;
	for (int i = 0; i < pfile->header->ncolors; i++) {
		if (pfile->colors[i].number == number) return (pfile->colors + i);
	}
	return NULL;
}

void ocad_color_to_rgb(const OCADColor *c, int *arr) {
	arr[0] = ocad_color_scale_rgb(200 - c->cyan - c->black);
	arr[1] = ocad_color_scale_rgb(200 - c->magenta - c->black);
	arr[2] = ocad_color_scale_rgb(200 - c->yellow - c->black);
}

void ocad_color_to_rgbf(const OCADColor *c, float *arr) {
	arr[0] = ocad_color_scale_rgbf(200 - c->cyan - c->black);
	arr[1] = ocad_color_scale_rgbf(200 - c->magenta - c->black);
	arr[2] = ocad_color_scale_rgbf(200 - c->yellow - c->black);
}
