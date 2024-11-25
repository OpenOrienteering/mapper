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

#include <Qt>
#include <QDateTime>
#include <QLatin1String>
#include <QString>
#include <QXmlStreamWriter>

#include "mapper_config.h"
#include "core/georeferencing.h"
#include "core/latlon.h"
#include "core/map.h"
#include "core/map_coord.h"
#include "fileformats/course_export.h"
#include "util/xml_stream_util.h"


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
	auto course_export = CourseExport(*map);
	std::vector<ControlPoint> controls = course_export.getControlsForExport();

	if (controls.empty())
	{
		addWarning(course_export.errorString());
		return false;
	}
	
	course = &course_export;
	
	QXmlStreamWriter writer(device());
	writer.setAutoFormatting(true);
	xml = &writer;
	xml->writeStartDocument();
	writeKml(controls);
	xml = nullptr;
	
	course = nullptr;
	return true;
}


void KmlCourseExport::writeKml(const std::vector<ControlPoint>& controls)
{
	auto const stamp = QDateTime::currentDateTime();
	xml->writeDefaultNamespace(QLatin1String("http://www.opengis.net/kml/2.2"));
	xml->writeComment(QLatin1String("Generator: OpenOrienteering Mapper " APP_VERSION));
	xml->writeComment(QLatin1String("Created:   ") + stamp.toString(Qt::ISODate));
	
	XmlElementWriter kml(*xml, QLatin1String("kml"));
	{
		XmlElementWriter document(*xml, QLatin1String("Document"));
		xml->writeTextElement(QLatin1String("name"), course->eventName());
		{
			XmlElementWriter folder(*xml, QLatin1String("Folder"));
			xml->writeTextElement(QLatin1String("name"), course->courseName());
			xml->writeTextElement(QLatin1String("open"), QLatin1String("1"));
			writeKmlPlacemarks(controls);
		}
	}
}

void KmlCourseExport::writeKmlPlacemarks(const std::vector<ControlPoint>& controls)
{
	for (auto& control : controls)
	{
		QString description;
		if (control.isStart())
		{
			description = QLatin1String("Start");
		}
		else if (control.isFinish())
		{
			description = QLatin1String("Finish");
		}
		writeKmlPlacemark(control.coord(), control.code(), description);
	}
}

void KmlCourseExport::writeKmlPlacemark(const MapCoordF& coord, const QString& name, const QString& description)
{
	XmlElementWriter placemark(*xml, QLatin1String("Placemark"));
	xml->writeTextElement(QLatin1String("name"), name);
	xml->writeTextElement(QLatin1String("description"), description);
	{
		XmlElementWriter point(*xml, QLatin1String("Point"));
		writeCoordinates(map->getGeoreferencing().toGeographicCoords(coord));
	}
}

void KmlCourseExport::writeCoordinates(const LatLon& latlon)
{
	XmlElementWriter coordinates(*xml, QLatin1String("coordinates"));
	xml->writeCharacters(QString::number(latlon.longitude(), 'f', 7));
	xml->writeCharacters(QLatin1String(","));
	xml->writeCharacters(QString::number(latlon.latitude(), 'f', 7));
	xml->writeCharacters(QLatin1String(",0"));
}


}  // namespace OpenOrienteering
