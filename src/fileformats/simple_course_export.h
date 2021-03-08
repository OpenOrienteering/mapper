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

#ifndef OPENORIENTEERING_SIMPLE_COURSE_EXPORT_H
#define OPENORIENTEERING_SIMPLE_COURSE_EXPORT_H

#include <QCoreApplication>
#include <QString>

namespace OpenOrienteering {

class Map;
class PathObject;

/**
 * This class provides utility functions for exporting simple course files.
 * 
 * Simple course files can be created either from a single selected path object,
 * or from the single path object contained in the map's single map part.
 */
class SimpleCourseExport
{
	Q_DECLARE_TR_FUNCTIONS(OpenOrienteering::SimpleCourseExport)
	
public:
	static QString defaultEventName();
	static QString defaultCourseName();
	static int defaultFirstCode() noexcept;
	
	~SimpleCourseExport() = default;
	
	SimpleCourseExport(const Map& map) : map{map} {};
	
	SimpleCourseExport(const SimpleCourseExport&) = default;
	
	
	bool canExport();
	
	bool canExport(const PathObject* object);
	
	const PathObject* findObjectForExport() const;
	
	
	QString eventName() const;
	
	QString courseName() const;
	
	int firstCode() const;
	
	void setProperties(Map& map, const QString& event_name, const QString& course_name, int first_code);
	
	
	QString errorString() const { return error_string; };
	
private:
	const Map& map;
	QString error_string;
	
};


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_SIMPLE_COURSE_EXPORT_H
