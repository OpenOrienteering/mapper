/*
 *    Copyright 2012 Thomas Schöps
 *    
 *    This file is part of OpenOrienteering.
 * 
 *    OpenOrienteering is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 * 
 *    OpenOrienteering is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 * 
 *    You should have received a copy of the GNU General Public License
 *    along with OpenOrienteering.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "matrix.h"

#include <QIODevice>

void Matrix::save(QIODevice* file)
{
	file->write((const char*)&n, sizeof(int));
	file->write((const char*)&m, sizeof(int));
	file->write((const char*)d, n*m * sizeof(double));
}
void Matrix::load(QIODevice* file)
{
	int new_n, new_m;
	file->read((char*)&new_n, sizeof(int));
	file->read((char*)&new_m, sizeof(int));
	
	setSize(new_n, new_m);
	file->read((char*)d, n*m * sizeof(double));
}
