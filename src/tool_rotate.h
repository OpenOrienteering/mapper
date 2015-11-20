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


#ifndef _OPENORIENTEERING_TOOL_ROTATE_H_
#define _OPENORIENTEERING_TOOL_ROTATE_H_

#include "tool_base.h"

#include <QScopedPointer>

class ConstrainAngleToolHelper;

/** Tool to rotate objects. */
class RotateTool : public MapEditorToolBase
{
Q_OBJECT
public:
	RotateTool(MapEditorController* editor, QAction* tool_action);
	virtual ~RotateTool();
	
	virtual void drawImpl(QPainter* painter, MapWidget* widget);
	
	virtual bool keyPress(QKeyEvent* event);
	virtual bool keyRelease(QKeyEvent* event);
	
protected:
	virtual void initImpl();
	virtual int updateDirtyRectImpl(QRectF& rect);
	virtual void updateStatusText();
	virtual void objectSelectionChangedImpl();
	
	virtual void clickRelease();
	virtual void dragStart();
	virtual void dragMove();
	virtual void dragFinish();
	
	QScopedPointer<ConstrainAngleToolHelper> angle_helper;
	QPointF constrained_pos;
	MapCoordF constrained_pos_map;
	
	// Mouse handling
	bool rotation_center_set;
	MapCoordF rotation_center;
	bool rotating;
	double old_rotation;
	double original_rotation;
};

#endif
