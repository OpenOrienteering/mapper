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
 * Represents a type of editing activity, e.g. template position adjustment.
 * Only one activity can be active at a time.
 * 
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
	 * All initializations apart from setting variables like the activity object
	 * should be done here instead of in the constructor, as at the time init()
	 * is called, the old activity was properly destroyed
	 * (including resetting the activity drawing).
	 */
	virtual void init();
	
	/**
	 * All dynamic drawings must be drawn here using the given painter.
	 * Drawing is only possible in the area specified
	 * by calling map->setActivityBoundingBox().
	 */
	virtual void draw(QPainter* painter, MapWidget* widget);
	
};


}  // namespace OpenOrienteering

#endif
