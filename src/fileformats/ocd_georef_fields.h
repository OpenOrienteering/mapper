/*
 *    Copyright 2018 Libor Pecháček
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

#ifndef OPENORIENTEERING_OCD_GEOREF_FIELDS_H
#define OPENORIENTEERING_OCD_GEOREF_FIELDS_H

#include <functional>

#include <QString>

#include "core/georeferencing.h"

namespace OpenOrienteering {

namespace OcdGeoref {

/**
 * Fill in provided georef with data from OCD type 1039 string.
 * @param georef Reference to a Georeferencing object.
 * @param param_string Full OCD type 1039 string.
 * @param warning_handler Function to handle import warnings.
 */
void setupGeorefFromString(Georeferencing& georef,
                           const QString& param_string,
                           const std::function<void (const QString&)>& warning_handler);

}  // namespace OcdGeoref

}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_OCD_GEOREF_FIELDS_H
