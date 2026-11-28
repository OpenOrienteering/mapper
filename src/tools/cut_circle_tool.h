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

#ifndef OPENORIENTEERING_CUT_CIRCLE_TOOL_H
#define OPENORIENTEERING_CUT_CIRCLE_TOOL_H

#include <QObject>

#include "tools/edit_tool.h"
#include "tools/tool_base.h"

class QAction;

namespace OpenOrienteering {

class MapEditorController;
class MapWidget;
class Object;


/**
 * A tool to cut gaps in point circles.
 */
class CutCircleTool : public MapEditorToolBase
{
Q_OBJECT
public:
	CutCircleTool(MapEditorController* editor, QAction* tool_action);
	~CutCircleTool() override;
	
protected:
	void initImpl() override;
	
	void updateStatusText() override;
	
	void objectSelectionChangedImpl() override;
	
	// MapEditorToolBase input event handlers
	void clickPress() override;
	
	bool getCircleAngle(Object* obj, int& angle, int& point_element_index) const;
	bool checkCircleAngle(const MapCoordF coords, const PointSymbol* symbol, double rotation, int& angle) const;
	
	
	/**
	 * Replaces the given object in the map with the replacement object.
	 * 
	 * Creates the necessary undo steps. If replacement is empty, the object is
	 * deleted without replacement.
	 * 
	 * @todo Consider moving this to a more general class (Map, MapPart).
	 */
	void replaceObject(Object* object, Object* new_object) const;
	
	
	const double user_click_tolerance;
	int first_angle;
	int first_point_element;
};

}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_CUT_CIRCLE_TOOL_H
