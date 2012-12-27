/*
 *    Copyright 2012 Thomas Schöps
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
#include "georeferencing_dialog.h"
#include "tool.h"

void doStaticInitializations()
{
	// Register the supported file formats
	FileFormats.registerFormat(new XMLFileFormat());
	FileFormats.registerFormat(new NativeFileFormat());
	FileFormats.registerFormat(new OCAD8FileFormat());
	
	// Load resources
	MapEditorTool::loadPointHandles();
	
	// Register projection templates
	CRSTemplate* temp = new CRSTemplate(QObject::tr("UTM"), "+proj=utm +datum=WGS84 +zone=%1");
	temp->addParam(new CRSTemplate::ZoneParam(QObject::tr("UTM Zone (number north/south, e.g. \"32 N\", \"24 S\")")));
	CRSTemplate::registerCRSTemplate(temp);
	
	temp = new CRSTemplate(QObject::tr("Gauss-Krueger, datum: Potsdam"), "+proj=tmerc +lat_0=0 +lon_0=%1 +k=1.000000 +x_0=3500000 +y_0=0 +ellps=bessel +datum=potsdam +units=m +no_defs");
	temp->addParam(new CRSTemplate::IntRangeParam(QObject::tr("Zone number (1 to 119)"), 1, 119, 3));
	CRSTemplate::registerCRSTemplate(temp);
}
