/*
 *    Copyright 2016 Kai Pastor
 *
 *    Some parts taken from file_format_oc*d8{.h,_p.h,cpp} which are
 *    Copyright 2012 Pete Curtis
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

#include "ocd_file_export.h"

#include "fileformats/ocad8_file_format_p.h"


namespace OpenOrienteering {

OcdFileExport::OcdFileExport(const QString& path, const Map* map, const MapView* view)
: Exporter { path, map, view }
{
	// nothing else
}


OcdFileExport::~OcdFileExport() = default;


bool OcdFileExport::exportImplementation()
{
	OCAD8FileExport delegate { path, map, view };
	delegate.setDevice(device());
	auto success = delegate.doExport();
	for (auto&& w : delegate.warnings())
	{
		addWarning(w);
	}
	return success;
}


}  // namespace OpenOrienteering
