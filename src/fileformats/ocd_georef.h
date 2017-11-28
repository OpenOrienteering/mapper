/*
 *    Copyright 2017 Libor Pecháček
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

#ifndef OCD_GEOREF_H
#define OCD_GEOREF_H

#include <vector>

#include <QString>

#include "core/georeferencing.h"

class OcdGeoref
{
public:
	static Georeferencing georefFromString(const QString& param_string,
		                                       std::list<QString>& warnings);
protected:
	static void applyGridAndZone(Georeferencing& georef,
	                             const QString& combined_grid_zone,
	                             std::list<QString>& warnings);
};

#endif // OCD_GEOREF_H
