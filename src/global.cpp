/*
 *    Copyright 2012, 2013 Thomas Schöps
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

#include <mapper_config.h>

#include "file_format_native.h"
#include "file_format_ocad8.h"
#include "file_format_xml.h"
#include "georeferencing.h"
#include "tool.h"

void registerProjectionTemplates()
{
	/*
	 * CRSTemplate(
	 *     id,
	 *     name,
	 *     coordinates name,
	 *     specification string)
	 * 
	 * The id must be unique and different from "Local".
	 */
	
	// UTM
	CRSTemplate* temp = new CRSTemplate(
		"UTM",
		QObject::tr("UTM", "UTM coordinate reference system"),
		QObject::tr("UTM coordinates"),
		"+proj=utm +datum=WGS84 +zone=%1");
	temp->addParam(new CRSTemplate::ZoneParam(QObject::tr("UTM Zone (number north/south, e.g. \"32 N\", \"24 S\")")));
	CRSTemplate::registerCRSTemplate(temp);
	
	// Gauss-Krueger
	temp = new CRSTemplate(
		"Gauss-Krueger, datum: Potsdam",
		QObject::tr("Gauss-Krueger, datum: Potsdam", "Gauss-Krueger coordinate reference system"),
		QObject::tr("Gauss-Krueger coordinates"),
		"+proj=tmerc +lat_0=0 +lon_0=%1 +k=1.000000 +x_0=3500000 +y_0=0 +ellps=bessel +datum=potsdam +units=m +no_defs");
	temp->addParam(new CRSTemplate::IntRangeParam(QObject::tr("Zone number (1 to 119)", "Zone number for Gauss-Krueger coordinates"), 1, 119, 3));
	CRSTemplate::registerCRSTemplate(temp);
}

void doStaticInitializations()
{
	// Register the supported file formats
	FileFormats.registerFormat(new XMLFileFormat());
	FileFormats.registerFormat(new NativeFileFormat());
	FileFormats.registerFormat(new OCAD8FileFormat());
	
	// Load resources
	MapEditorTool::loadPointHandles();
	
	// Register projection templates
	registerProjectionTemplates();
}
