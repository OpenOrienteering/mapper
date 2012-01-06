/*
 *    Copyright 2011 Thomas Schöps
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


#include "symbol.h"

#include <QFile>

#include "util.h"

Symbol::Symbol() : name(""), description(""), is_helper_symbol(false)
{
	for (int i = 0; i < number_components; ++i)
		number[i] = -1;
}

void Symbol::save(QFile* file, Map* map)
{
	saveString(file, name);
	for (int i = 0; i < number_components; ++i)
		file->write((const char*)&number[i], sizeof(int));
	saveString(file, description);
	file->write((const char*)&is_helper_symbol, sizeof(bool));
	
	saveImpl(file, map);
}
void Symbol::load(QFile* file, Map* map)
{
	loadString(file, name);
	for (int i = 0; i < number_components; ++i)
		file->read((char*)&number[i], sizeof(int));
	loadString(file, description);
	file->read((char*)&is_helper_symbol, sizeof(bool));
	
	loadImpl(file, map);
}

#include "symbol.moc"
