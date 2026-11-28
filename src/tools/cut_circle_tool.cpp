/*
 *    Copyright 2026 Matthias Kühlewein
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

#include "cut_circle_tool.h"

#include <Qt>
#include <QtGlobal>
#include <QCursor>
#include <QGuiApplication>
#include <QPoint>
#include <QString>

#include "settings.h"
#include "core/map.h"
#include "core/map_part.h"
#include "core/objects/object.h"
#include "core/symbols/point_symbol.h"
#include "core/symbols/symbol.h"
#include "gui/map/map_widget.h"
#include "undo/object_undo.h"
#include "undo/undo.h"

namespace OpenOrienteering {


CutCircleTool::CutCircleTool(MapEditorController* editor, QAction* tool_action)
 : MapEditorToolBase { scaledToScreen(QCursor{ QString::fromLatin1(":/images/cursor-cut.png"), 11, 11 }), Other, editor, tool_action }
 , user_click_tolerance { Settings::getInstance().getSetting(Settings::MapEditor_ClickToleranceMM).toDouble() / 10.0}
 , first_point_element { -1 }
{
	// nothing
}


CutCircleTool::~CutCircleTool()
{
	// nothing
}


void CutCircleTool::initImpl()
{
	updateDirtyRect();
	updateStatusText();
}


// This function contains translations. Keep it close to the top of the file so
// that line numbers remain stable here when changing other parts of the file.
void CutCircleTool::updateStatusText()
{
	QString text;

	if (first_point_element == -1)
		text = tr("<b>Click</b> on the point circle: Create first cut point. <b>Click</b> on a gap in the point circle: Remove gap. ");
	else
		text = tr("<b>Click</b> somewhere else on the point circle: Create gap. ");
	
	setStatusBarText(text);
}

void CutCircleTool::objectSelectionChangedImpl()
{
	updateDirtyRect();
	deactivate();
}


void CutCircleTool::clickPress()
{
	if (map()->getNumSelectedObjects() != 1)
	{
		deactivate();
		return;
	}
	const auto point = map()->getFirstSelectedObject();
	int angle;
	int point_element_index = first_point_element;
	if (getCircleAngle(point, angle, point_element_index))
	{
		// second click:
		if (first_point_element != -1)
		{
			first_point_element = -1;
			if (angle != first_angle)
			{
				// adding the gap
				auto new_point = point->duplicate();
				new_point->asPoint()->getCutCircle().addGap(first_angle, angle);
				replaceObject(point, new_point);
				updateDirtyRect();
			}
			updateStatusText();
			return;
		}
		if (point->asPoint()->getCutCircle().isAngleInAnyGap(angle))
		{
			// removing the gap
			auto new_point = point->duplicate();
			new_point->asPoint()->getCutCircle().deleteGap(angle);
			replaceObject(point, new_point);
			updateDirtyRect();
		}
		else	// first click
		{
			first_angle = angle;
			first_point_element = point_element_index;
			updateStatusText();
		}
		return;
	}
}

bool CutCircleTool::getCircleAngle(Object* obj, int& angle, int& point_element_index) const
{
	auto point = static_cast<PointObject*>(obj);
	auto symbol = obj->getSymbol()->asPoint();
	if (point_element_index < 0)	// search matching circle
	{
		// use base part first
		if (symbol->isCircle())
		{
			if (checkCircleAngle(point->getCoordF(), symbol, (symbol->isRotatable() ? point->getRotation() : 0.0), angle))
			{
				point_element_index = 0;
				return true;
			}
		}
		// else use sub elements
		for (int i = 0; i < symbol->getNumElements(); ++i)
		{
			auto element_symbol = symbol->getElementSymbol(i);
			if (element_symbol->getType() == Symbol::Point && element_symbol->asPoint()->isCircle())
			{
				Q_ASSERT(symbol->getElementObject(i)->getType() == Object::Point);
				
				auto element_object = symbol->getElementObject(i)->asPoint();
				auto element_coords = element_object->getCoordF();
				double rotation = 0.0;
				if (symbol->isRotatable())
				{
					rotation = point->getRotation();
					element_coords.rotate(-rotation);
				}
				if (checkCircleAngle(point->getCoordF() + element_coords, element_symbol->asPoint(), rotation, angle))
				{
					point_element_index = i + 1;
					return true;
				}
			}
		}
		return false;
	}
	
	if (point_element_index == 0) // use base part again
	{
		return checkCircleAngle(point->getCoordF(), symbol, (symbol->isRotatable() ? point->getRotation() : 0.0), angle);
	}
	
	// use previously clicked sub element again
	auto element_symbol = symbol->getElementSymbol(point_element_index - 1);
	auto element_object = symbol->getElementObject(point_element_index - 1)->asPoint();
	auto element_coords = element_object->getCoordF();
	double rotation = 0.0;
	if (symbol->isRotatable())
	{
		rotation = point->getRotation();
		element_coords.rotate(-rotation);
	}
	return checkCircleAngle(point->getCoordF() + element_coords, element_symbol->asPoint(), rotation, angle);
}

bool CutCircleTool::checkCircleAngle(const MapCoordF coords, const PointSymbol* symbol, double rotation, int& angle) const
{
	const double radius = (symbol->getInnerRadius() + symbol->getOuterWidth() / 2) / 1000.0;
	const double click_tolerance = qMax(symbol->getOuterWidth() / 2000.0, user_click_tolerance);
	if (qAbs(click_pos_map.distanceTo(coords) - radius) < click_tolerance)
	{
		auto normalized_click_pos = click_pos_map - coords;
		auto temp = qRadiansToDegrees(-normalized_click_pos.angle() - rotation);
		if (temp < 0)
			temp += 360;
		angle = (int)(temp * 160);
		return true;
	}
	return false;
}


void CutCircleTool::replaceObject(Object* object, Object* new_object) const
{
	auto map = this->map();
	auto map_part = map->getCurrentPart();
	
	auto add_step = new AddObjectsUndoStep(map);
	add_step->addObject(map_part->findObjectIndex(object), object);
	map->removeObjectFromSelection(object, false);
	map->releaseObject(object);
	
	auto delete_step = new DeleteObjectsUndoStep(map);
	map->addObject(new_object);
	map->addObjectToSelection(new_object, false);
	delete_step->addObject(map_part->findObjectIndex(new_object));
	
	auto undo_step = new CombinedUndoStep(map);
	undo_step->push(add_step);
	undo_step->push(delete_step);
	map->push(undo_step);
	
	map->emitSelectionChanged();
}


}  // namespace OpenOrienteering
