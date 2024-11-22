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

#ifndef OPENORIENTEERING_IOF_COURSE_EXPORT_H
#define OPENORIENTEERING_IOF_COURSE_EXPORT_H

#include <vector>

#include "fileformats/file_import_export.h"

class QString;
class QXmlStreamWriter;

namespace OpenOrienteering {

class LatLon;
class Map;
class MapView;
class MapCoordF;
class ControlPoint;
class PathObject;
class CourseExport;


/**
 * This class generates IOF Interface Standard 3.0 course files.
 * 
 * This export handles a single path object and outputs placemarks for start
 * (S1), finish (F1), and controls in between. Event name, course name, and
 * the code number of the first control are taken from transient map properties
 * in collaboration with the SimpleCourseExport class.
 */
class IofCourseExport : public Exporter
{
public:
	static QString formatDescription();
	static QString filenameExtension();
	
	~IofCourseExport();
	
	IofCourseExport(const QString& path, const Map* map, const MapView* view);
	
protected:
	bool exportImplementation() override;
	
	//void writeXml(const PathObject& object);
	void writeXml(const std::vector<ControlPoint>& controls);
	
	//void writeControls(const std::vector<MapCoord>& coords);
	void writeControls(const std::vector<ControlPoint>& controls);
	
	//void writeCourse(const std::vector<MapCoord>& coords);
	void writeCourse(const std::vector<ControlPoint>& controls);
	
	void writeControl(const MapCoordF& coord, const QString& id);
	
	void writeCourseControl(const QString& type, const QString& id);
	
	void writePosition(const LatLon& latlon);
	
private:
	QXmlStreamWriter* xml = nullptr;
	CourseExport* course = nullptr;
};


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_IOF_COURSE_EXPORT_H
