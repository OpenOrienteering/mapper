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

#include "course_export.h"

#include <QDebug>
#include <QRegularExpression>

#include "core/map.h"
#include "core/map_part.h"
#include "core/objects/object.h"
#include "core/objects/text_object.h"


namespace OpenOrienteering {

// static
QString CourseExport::defaultEventName()
{
	return tr("Unnamed event");
}

// static
QString CourseExport::defaultCourseName()
{
	return tr("Unnamed course");
}

// static
int CourseExport::defaultFirstCode() noexcept
{
	return 31;
}

bool CourseExport::canExport()
{
	if (map.getNumSelectedObjects() < 1)
	{
		error_string = tr("No control points or path object selected.");
		return false;
	}
	return true;
}

const PathObject* CourseExport::findPathObjectForExport() const
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

bool CourseExport::isPathExportMode() const
{
	if (findPathObjectForExport())
	{
		return true;
	}
	return false;
}

void addControl(std::vector<ControlPoint>& controls, Object* object, const QString& code)
{
	if (object)
	{
		controls.push_back(ControlPoint(code, object->asPoint()->getCoordF()));
	}
}

void addControl(std::vector<ControlPoint>& controls, const MapCoord& coord, const QString& code)
{
	controls.push_back(ControlPoint(code, MapCoordF(coord)));
}

QString removeTrailingZerosAndDots(QString s)
{
	return s.replace(QRegularExpression(QStringLiteral(R"((\.)?0+$)")), QStringLiteral(""));
}

const std::vector<ControlPoint> CourseExport::getControlsForExport()
{
	const PathObject* path_object = findPathObjectForExport();
	if (path_object)
	{
		return addControlsByPath(path_object);
	}

	std::vector<Object*> control_objects;
	std::set<Object*> control_code_objects;
	if (map.getNumSelectedObjects() < 1)
	{
		return {};
	}
	for (auto* object : map.selectedObjects())
	{
		if (object->getType() == Object::Text)
		{
			control_code_objects.insert(object);
		}
		else if (object->getType() == Object::Point)
		{
			control_objects.push_back(object);
		}
	}
	if (control_objects.empty())
	{
		error_string = tr("No control points selected.");
		return {};
	}
	if (control_code_objects.empty())
	{
		error_string = tr("No control codes selected.");
		return {};
	}
	qDebug() << "Found control points:" << control_objects.size() << "control codes:" << control_code_objects.size();
	auto controls = addControlsWithCodes(control_objects, control_code_objects);
	sortControls(controls);
	return controls;
}

const std::vector<ControlPoint> CourseExport::addControlsByPath(const PathObject* path_object)
{
	auto next = [](auto current) {
		return current + (current->isCurveStart() ? 3 : 1);
	};

	Q_ASSERT(path_object);
	std::vector<MapCoord> coords = path_object->getRawCoordinateVector();

	std::vector<ControlPoint> controls;
	addControl(controls, coords.front(), QStringLiteral("S1"));
	auto code_number = firstCode();
	for (auto current = next(coords.begin()); current != coords.end() - 1; current = next(current))
	{
		auto const name = QString::number(code_number);
		addControl(controls, *current, name);
		++code_number;
	}
	addControl(controls, coords.back(), QStringLiteral("F1"));
	return controls;
}

const std::vector<ControlPoint> CourseExport::addControlsWithCodes(const std::vector<Object*>& control_objects, std::set<Object*>& control_code_objects)
{
	std::vector<ControlPoint> controls;
	QString start_symbol = removeTrailingZerosAndDots(startSymbol());
	QString finish_symbol = removeTrailingZerosAndDots(finishSymbol());
	for (auto* object : control_objects)
	{
		QString symbol = removeTrailingZerosAndDots(object->getSymbol()->getNumberAsString());
		if (symbol == start_symbol)
		{
			qDebug() << "Found start:" << object->asPoint()->getCoordF();
			addControl(controls, object, QStringLiteral("S1"));
			continue;
		}
		else if (symbol == finish_symbol)
		{
			qDebug() << "Found finish:" << object->asPoint()->getCoordF();
			addControl(controls, object, QStringLiteral("F1"));
			continue;
		}
		auto* control_code_object = getNearestObject(object, control_code_objects);
		if (control_code_object)
		{
			qDebug() << "Found control point:" << object->asPoint()->getCoordF() << "control code:" << control_code_object->asText()->getText();
			control_code_objects.erase(control_code_object);
			auto code = control_code_object->asText()->getText();
			addControl(controls, object, code);
		}
	}
	return controls;
}

// static
Object* CourseExport::getNearestObject(const Object* object, const std::set<Object*>& objects)
{
	Object* nearest_object = nullptr;
	double nearest_distance = 0;
	for (auto* other_object : objects)
	{
		if (other_object != object)
		{
			auto distance = getObjectCoord(object).distanceTo(getObjectCoord(other_object));
			if (!nearest_object || distance < nearest_distance)
			{
				nearest_object = other_object;
				nearest_distance = distance;
			}
		}
	}
	return nearest_object;
}

// static
MapCoordF CourseExport::getObjectCoord(const Object* object)
{
	if (object)
	{
		switch (object->getType())
		{
			case Object::Point:
				return object->asPoint()->getCoordF();
			case Object::Text:
				return object->asText()->getAnchorCoordF();
			default:
				break;
		}
	}
	return MapCoordF();
}

// static
void CourseExport::sortControls(std::vector<ControlPoint>& controls)
{
	std::sort(controls.begin(), controls.end(), [](const ControlPoint& a, const ControlPoint& b) {
		if (a.isStart() || b.isFinish())
		{
			return true;
		}
		if (a.isFinish() || b.isStart())
		{
			return false;
		}
		return a.code() < b.code();
	});
}


QString CourseExport::eventName() const
{
	auto name = map.property("course-export-event-name").toString();
	if (name.isEmpty())
		name = defaultEventName();
	return name;
}

QString CourseExport::courseName() const
{
	auto name = map.property("course-export-course-name").toString();
	if (name.isEmpty())
		name = defaultCourseName();
	return name;
}

QString CourseExport::startSymbol() const
{
	return map.property("course-export-start-symbol").toString();
}

QString CourseExport::finishSymbol() const
{
	return map.property("course-export-finish-symbol").toString();
}

int CourseExport::firstCode() const
{
	auto code = map.property("course-export-first-code").toInt();
	return code > 0 ? code : defaultFirstCode();
}

void CourseExport::setProperties(Map& map, const QString& event_name, const QString& course_name, const QString& start_symbol, const QString& finish_symbol, int first_code)
{
	map.setProperty("course-export-event-name", event_name);
	map.setProperty("course-export-course-name", course_name);
	map.setProperty("course-export-start-symbol", start_symbol);
	map.setProperty("course-export-finish-symbol", finish_symbol);
	map.setProperty("course-export-first-code", first_code);
}


}  // namespace OpenOrienteering
