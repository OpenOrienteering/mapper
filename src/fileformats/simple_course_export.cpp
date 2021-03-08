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

#include "simple_course_export.h"

#include <QVariant>

#include "core/map.h"
#include "core/map_part.h"
#include "core/objects/object.h"


namespace OpenOrienteering {


// static
QString SimpleCourseExport::defaultEventName()
{
	return tr("Unnamed event");
}

// static
QString SimpleCourseExport::defaultCourseName()
{
	return tr("Unnamed course");
}

// static
int SimpleCourseExport::defaultFirstCode() noexcept
{
	return 31;
}


bool SimpleCourseExport::canExport()
{
	return canExport(findObjectForExport());
}

bool SimpleCourseExport::canExport(const PathObject* object)
{
	if (!object)
	{
		error_string = tr("For this course export, a single line object must be selected.");
		return false;
	}
	return true;
}


const PathObject* SimpleCourseExport::findObjectForExport() const
{
	const PathObject* path_object = nullptr;
	if (map.getNumSelectedObjects() == 1
	    && map.getFirstSelectedObject()->getType() == Object::Path)
	{
		path_object = static_cast<PathObject const*>(map.getFirstSelectedObject());
	}
	else if (map.getNumParts() == 1
	         && map.getPart(0)->getNumObjects() == 1
	         && map.getPart(0)->getObject(0)->getType() == Object::Path)
	{
		path_object = static_cast<PathObject const*>(map.getPart(0)->getObject(0));
	}
	return (path_object && path_object->parts().size() == 1) ? path_object : nullptr;
}


QString SimpleCourseExport::eventName() const
{
	auto name = map.property("simple-course-event-name").toString();
	if (name.isEmpty())
		name = defaultEventName();
	return name;
}

QString SimpleCourseExport::courseName() const
{
	auto name = map.property("simple-course-course-name").toString();
	if (name.isEmpty())
		name = defaultCourseName();
	return name;
}

int SimpleCourseExport::firstCode() const
{
	auto code = map.property("simple-course-first-code").toInt();
	return code > 0 ? code : defaultFirstCode();
}

void SimpleCourseExport::setProperties(Map& map, const QString& event_name, const QString& course_name, int first_code)
{
	map.setProperty("simple-course-event-name", event_name);
	map.setProperty("simple-course-course-name", course_name);
	map.setProperty("simple-course-first-code", first_code);
}


}  // namespace OpenOrienteering
