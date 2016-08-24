/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2014, 2016 Kai Pastor
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


#ifndef OPENORIENTEERING_TOOL_ROTATE_H
#define OPENORIENTEERING_TOOL_ROTATE_H

#include "tool_base.h"

#include <QScopedPointer>

class ConstrainAngleToolHelper;


/**
 * Tool to rotate objects.
 */
class RotateTool : public MapEditorToolBase
{
	Q_OBJECT
	
public:
	RotateTool(MapEditorController* editor, QAction* tool_action);
	~RotateTool() override;
	
protected:
	void initImpl() override;
	
	void clickRelease() override;
	void dragStart() override;
	void dragMove() override;
	void dragFinish() override;
	bool keyPress(QKeyEvent* event) override;
	bool keyRelease(QKeyEvent* event) override;
	
	void drawImpl(QPainter* painter, MapWidget* widget) override;
	
	int updateDirtyRectImpl(QRectF& rect) override;
	void objectSelectionChangedImpl() override;
	
	void updateStatusText() override;
	
	QScopedPointer<ConstrainAngleToolHelper> angle_helper;
	QPointF constrained_pos;
	MapCoordF constrained_pos_map;
	
	// Mouse handling
	MapCoordF rotation_center;
	bool rotation_center_set;
	bool rotating;
	double old_rotation;
	double original_rotation;
};

#endif
