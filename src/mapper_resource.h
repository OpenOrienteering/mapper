/*
 *    Copyright 2012-2017 Kai Pastor
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


#ifndef OPENORIENTEERING_MAPPER_RESOURCE_H
#define OPENORIENTEERING_MAPPER_RESOURCE_H

namespace OpenOrienteering {


namespace MapperResource {

/**
 * Initializes QDir::searchPaths for Mapper resource prefixes.
 * 
 * This function registers the prefixes "data" and "doc" with paths leading
 * to possible locations in the build dir (development build only), application
 * bundle (Android, macOS, Windows), system (Linux) and Qt resource system.
 * Thus resource lookup may either iterate explicitly over
 * QDir::searchPaths("PREFIX"), or directly access resources using 
 * QFile("PREFIX:name").
 */
void setSeachPaths();

}  // namespace MapperResource


}  // namespace OpenOrienteering


#endif
