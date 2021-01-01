/*
 *    Copyright 2020 Kai Pastor
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

#ifndef OPENORIENTEERING_GDAL_FILE_H
#define OPENORIENTEERING_GDAL_FILE_H

class QByteArray;

namespace OpenOrienteering {


/**
 * Utility functions which use GDAL's VSI-aware file API.
 * 
 * Paths must be passed as, and are returned as, UTF-8.
 * 
 * \see https://gdal.org/user/virtual_file_systems.html
 */
namespace GdalFile {

/**
 * Checks if a file exists.
 */
bool exists(const QByteArray& filepath);

/**
 * Checks if a path is regarded as relative.
 */
bool isRelative(const QByteArray& filepath);

/**
 * Checks if a path is an existing directory.
 */
bool isDir(const QByteArray& filepath);

/**
 * Creates a directory.
 */
bool mkdir(const QByteArray& filepath);


/**
 * GDAL-based implementation of Template::tryToFindRelativeTemplateFile().
 * 
 * Returns an absolute path when the given template path identifies an existing
 * file relative to the map path, or an empty value otherwise.
 */
QByteArray tryToFindRelativeTemplateFile(const QByteArray& template_path, const QByteArray& map_path);


}   // namespace GdalUtil

}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_GDAL_FILE_H
