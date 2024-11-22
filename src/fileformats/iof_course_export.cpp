/*
 *    Copyright 2021 Kai Pastor
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

#include "iof_course_export.h"

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
QString IofCourseExport::formatDescription()
{
	return OpenOrienteering::ImportExport::tr("IOF Data Standard 3.0");
}

// static
QString IofCourseExport::filenameExtension()
{
	return QStringLiteral("xml");
}


IofCourseExport::~IofCourseExport() = default;

IofCourseExport::IofCourseExport(const QString& path, const Map* map, const MapView* view)
: Exporter(path, map, view)
{}


bool IofCourseExport::exportImplementation()
{
	CourseExport course_export(*map);
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
	writeXml(controls);
	xml = nullptr;
	
	course = nullptr;
	return true;
}


void IofCourseExport::writeXml(const std::vector<ControlPoint>& controls)
{
	auto const stamp = QDateTime::currentDateTime();
	xml->writeDefaultNamespace(QLatin1String("http://www.orienteering.org/datastandard/3.0"));
	
	XmlElementWriter course_data(*xml, QLatin1String("CourseData"));
	course_data.writeAttribute(QLatin1String("iofVersion"), QLatin1String("3.0"));
	course_data.writeAttribute(QLatin1String("creator"), QLatin1String("OpenOrienteering Mapper " APP_VERSION));
	course_data.writeAttribute(QLatin1String("createTime"), stamp.toString(Qt::ISODate));
	{
		XmlElementWriter event(*xml, QLatin1String("Event"));
		xml->writeTextElement(QLatin1String("Name"), course->eventName());
	}
	{
		XmlElementWriter event(*xml, QLatin1String("RaceCourseData"));
		writeControls(controls);
		writeCourse(controls);
	}
}

void IofCourseExport::writeControls(const std::vector<ControlPoint>& controls)
{
	for (auto& control : controls)
	{
		writeControl(control.coord(), control.code());
	}
}

void IofCourseExport::writeCourse(const std::vector<ControlPoint>& controls)
{
	XmlElementWriter event(*xml, QLatin1String("Course"));
	xml->writeTextElement(QLatin1String("Name"), course->courseName());
	for (auto& control : controls)
	{
		QString type = QLatin1String("Control");
		if (control.isStart())
		{
			type = QLatin1String("Start");
		}
		else if (control.isFinish())
		{
			type = QLatin1String("Finish");
		}
		writeCourseControl(type, control.code());
	}
}

void IofCourseExport::writeControl(const MapCoordF& coord, const QString& id)
{
	XmlElementWriter control(*xml, QLatin1String("Control"));
	xml->writeTextElement(QLatin1String("Id"), id);
	writePosition(map->getGeoreferencing().toGeographicCoords(coord));
}

void IofCourseExport::writeCourseControl(const QString& type, const QString& id)
{
	XmlElementWriter course_control(*xml, QLatin1String("CourseControl"));
	course_control.writeAttribute(QLatin1String("type"), type);
	xml->writeTextElement(QLatin1String("Control"), id);
}

void IofCourseExport::writePosition(const LatLon& latlon)
{
	XmlElementWriter position(*xml, QLatin1String("Position"));
	position.writeAttribute(QLatin1String("lng"), latlon.longitude(), 7);
	position.writeAttribute(QLatin1String("lat"), latlon.latitude(), 7);
}


}  // namespace OpenOrienteering
