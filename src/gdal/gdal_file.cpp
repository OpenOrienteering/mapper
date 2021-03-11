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

#include "gdal_file.h"

#include <cpl_conv.h>
#include <cpl_vsi.h>
#include <gdal.h>

#include <QByteArray>
#include <QString>


namespace OpenOrienteering {

namespace GdalFile {

bool exists(const QByteArray& filepath)
{
	VSIStatBufL stat_buf;
	return VSIStatExL(filepath, &stat_buf, VSI_STAT_EXISTS_FLAG) == 0;
}

bool isRelative(const QByteArray& filepath)
{
	return CPLIsFilenameRelative(filepath) == TRUE;
}


bool isDir(const QByteArray& filepath)
{
	VSIStatBufL stat_buf;
	return VSIStatExL(filepath, &stat_buf, VSI_STAT_EXISTS_FLAG | VSI_STAT_NATURE_FLAG) == 0
	       && VSI_ISDIR(stat_buf.st_mode);
}

bool mkdir(const QByteArray& filepath)
{
	// 0777 would be right for POSIX mkdir with umask, but we
	// cannot rely on umask for all paths supported by VSIMkdir.
	return VSIMkdir(filepath, 0755) == 0;
}


QByteArray tryToFindRelativeTemplateFile(const QByteArray& template_path, const QByteArray& map_path)
{
	QByteArray filepath = map_path + '/' + template_path;
	if (!exists(filepath))
		filepath = QByteArray(CPLGetDirname(map_path)) + '/' + template_path;
	if (!exists(filepath))
		filepath.clear();
	return filepath;
}

}   // namespace GdalUtil

}  // namespace OpenOrienteering
