/*
 *    Copyright 2020-2021 Kai Pastor
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

#include "kml_course_export.h"

#include <QByteArray>
#include <QIODevice>
#include <QString>

#include "mapper_config.h"
#include "core/georeferencing.h"
#include "core/latlon.h"
#include "core/map.h"
#include "core/map_coord.h"
#include "core/objects/object.h"
#include "fileformats/simple_course_export.h"


namespace OpenOrienteering {

// static
QString KmlCourseExport::formatDescription()
{
	return OpenOrienteering::ImportExport::tr("KML");
}

// static
QString KmlCourseExport::filenameExtension()
{
	return QStringLiteral("kml");
}


KmlCourseExport::~KmlCourseExport() = default;

KmlCourseExport::KmlCourseExport(const QString& path, const Map* map, const MapView* view)
: Exporter(path, map, view)
{}


// override
bool KmlCourseExport::exportImplementation()
{
	auto course_export = SimpleCourseExport(*map);
	auto const* const object = course_export.findObjectForExport();
	if (!course_export.canExport(object))
	{
		addWarning(course_export.errorString());
		return false;
	}
	
	writeKml(*object);
	return true;
}


void KmlCourseExport::writeKml(const PathObject& object)
{
	auto* device = this->device();
	device->write(
	  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	  "<!-- Generator: OpenOrienteering Mapper " APP_VERSION " -->\n"
	  "<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n"
	  "<Document>\n"
	  " <name>Course</name>\n"
	  " <Folder>\n"
	  "  <name>Course 1</name>\n"
	  "  <open>1</open>\n"
	);
	writeKmlPlacemarks(object.getRawCoordinateVector());
	device->write(
	  " </Folder>\n"
	  "</Document>\n"
	  "</kml>\n"
	);
	
}

void KmlCourseExport::writeKmlPlacemarks(const std::vector<MapCoord>& coords)
{
	auto next = [](auto current) {
		return current + (current->isCurveStart() ? 3 : 1);
	};
	
	writeKmlPlacemark(coords.front(), "S1", "Start");
	auto code_number = 1;
	for (auto current = next(coords.begin()); current != coords.end() - 1; current = next(current))
	{
		auto const name = QByteArray::number(code_number);
		writeKmlPlacemark(*current, name, QByteArray("Control " + name));
		++code_number;
	}
	writeKmlPlacemark(coords.back(), "F1", "Finish");
}

void KmlCourseExport::writeKmlPlacemark(const MapCoord& coord, const char* name, const char* description)
{
	auto* device = this->device();
	device->write(
	  "  <Placemark>\n"
	  "   <name>"
	);
	device->write(name);
	device->write("</name>\n"
	  "   <description>"
	);
	device->write(description);
	device->write("</description>\n"
	  "   <Point>\n"
	  "    <coordinates>"
	);
	writeCoordinates(map->getGeoreferencing().toGeographicCoords(MapCoordF(coord)));
	device->write(
	  "</coordinates>\n"
	  "   </Point>\n"
	  "  </Placemark>\n"
	);
}

void KmlCourseExport::writeCoordinates(const LatLon& latlon)
{
	auto* device = this->device();
	device->write(QByteArray::number(latlon.longitude(), 'f', 7));
	device->write(",");
	device->write(QByteArray::number(latlon.latitude(), 'f', 7));
	device->write(",0");
}


}  // namespace OpenOrienteering
