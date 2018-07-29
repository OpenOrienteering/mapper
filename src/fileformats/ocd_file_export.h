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

#ifndef OPENORIENTEERING_OCD_FILE_EXPORT_H
#define OPENORIENTEERING_OCD_FILE_EXPORT_H

#include <QCoreApplication>
#include <QString>

#include "fileformats/file_import_export.h"

namespace OpenOrienteering {

class Map;
class MapView;


/**
 * An exporter for OCD files.
 */
class OcdFileExport : public Exporter
{
	Q_DECLARE_TR_FUNCTIONS(OpenOrienteering::OcdFileExport)
	
public:
	OcdFileExport(const QString& path, const Map* map, const MapView* view);
	
	~OcdFileExport() override;
	
protected:
	/**
	 * Exports an OCD file.
	 * 
	 * For now, this simply uses the OCAD8FileExport class.
	 */
	bool exportImplementation() override;
	
};


}  // namespace OpenOrienteering

#endif
