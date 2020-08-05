/*
 *    Copyright 2012, 2013 Thomas Schöps
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


#ifndef OPENORIENTEERING_MAP_EDITOR_ACTIVITY_H
#define OPENORIENTEERING_MAP_EDITOR_ACTIVITY_H

#include <QObject>

class QPainter;

namespace OpenOrienteering {

class MapWidget;


/**
 * Represents a complex editing activity, e.g. template position adjustment.
 * 
 * Only one activity can be active at a time.
 * This is for example used to close the template adjustment window when
 * selecting an edit tool.
 * It can also be used to paint activity-specific graphics onto the map.
 */
class MapEditorActivity : public QObject
{
Q_OBJECT
public:
	~MapEditorActivity() override;
	
	/**
	 * Starts this activity, setting up the editor as needed.
	 * 
	 * This function is called by MapEditorController::setEditorActivity()
	 * after the previous activity was properly destroyed.
	 * 
	 * The constructor may run long before the activity becomes active.
	 * So the main work of initialization and setting up the editor for
	 * the activity must be done here.
	 */
	virtual void init();
	
	/**
	 * All dynamic drawings must be drawn here using the given painter.
	 * 
	 * Drawing is only possible in the area specified
	 * by calling map->setActivityBoundingBox().
	 */
	virtual void draw(QPainter* painter, MapWidget* widget);
	
};


}  // namespace OpenOrienteering

#endif
