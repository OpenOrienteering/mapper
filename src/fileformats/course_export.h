/*
 *    Copyright 2024 Semyon Yakimov
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

#ifndef OPENORIENTEERING_COURSE_EXPORT_H
#define OPENORIENTEERING_COURSE_EXPORT_H

#include <QCoreApplication>
#include <QString>
#include <set>
#include "core/map_coord.h"

namespace OpenOrienteering {

class Map;
class Object;
class PathObject;

class ControlPoint
{
public:
	ControlPoint() = default;
	ControlPoint(const QString& code, const MapCoordF& coord) : _code{code}, _coord{coord} {}

	bool isStart() const { return _code.startsWith(QStringLiteral("S")); }
	bool isFinish() const { return _code.startsWith(QStringLiteral("F")); }

	QString code() const { return _code; }
	MapCoordF coord() const { return _coord; }

private:
	QString _code;
	MapCoordF _coord;
};

/**
 * This class provides utility functions for exporting course files.
 * 
 */
class CourseExport
{
	Q_DECLARE_TR_FUNCTIONS(OpenOrienteering::CourseExport)
	
public:
	static QString defaultEventName();
	static QString defaultCourseName();
	static int defaultFirstCode() noexcept;
	
	~CourseExport() = default;
	
	CourseExport(const Map& map) : map{map} {};
	
	CourseExport(const CourseExport&) = default;

	bool canExport();

	bool isPathExportMode() const;

	const std::vector<ControlPoint> getControlsForExport();
	
	QString eventName() const;
	
	QString courseName() const;

	QString startSymbol() const;

	QString finishSymbol() const;
	
	int firstCode() const;
	
	void setProperties(Map& map, const QString& event_name, const QString& course_name, const QString& start_symbol, const QString& finish_symbol, int first_code);
	
	QString errorString() const { return error_string; };
	
private:
	static Object* getNearestObject(const Object* object, const std::set<Object*>& objects);
	static MapCoordF getObjectCoord(const Object* object);
	static void sortControls(std::vector<ControlPoint>& controls);

	const PathObject* findPathObjectForExport() const;
	/**
	 * Add controls, start and finish points to the course by a single path.
	 */
	const std::vector<ControlPoint> addControlsByPath(const PathObject* path_object);
	const std::vector<ControlPoint> addControlsWithCodes(const std::vector<Object*>& control_objects, std::set<Object*>& control_code_objects);

	const Map& map;
	QString error_string;
};


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_COURSE_EXPORT_H
