/*
 *    Copyright 2012, 2013, 2014 Thomas Schöps
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


#include "global.h"

#include "mapper_config.h" // IWYU pragma: keep

#include "fileformats/file_format_registry.h"
#include "fileformats/native_file_format.h"
#include "fileformats/xml_file_format.h"
#include "fileformats/ocd_file_format.h"
#include "gdal/ogr_file_format.h"


namespace OpenOrienteering {

void doStaticInitializations()
{
	static bool call_once = true;
	if (call_once)
	{
		// Register the supported file formats
		FileFormats.registerFormat(new XMLFileFormat());
#ifndef MAPPER_BIG_ENDIAN
		FileFormats.registerFormat(new OcdFileFormat());
#endif
#ifdef MAPPER_USE_GDAL
		FileFormats.registerFormat(new OgrFileFormat());
#endif
#ifndef MAPPER_BIG_ENDIAN
#ifndef NO_NATIVE_FILE_FORMAT
		FileFormats.registerFormat(new NativeFileFormat()); // TODO: Remove before release 1.0
#endif
#endif
		call_once = false;
	}
}


}  // namespace OpenOrienteering
