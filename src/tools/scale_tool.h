/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2014-2016 Kai Pastor
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


#ifndef OPENORIENTEERING_SCALE_TOOL_H
#define OPENORIENTEERING_SCALE_TOOL_H

#include <QObject>

#include "core/map_coord.h"
#include "tools/tool_base.h"

class QAction;
class QPainter;
class QRectF;

namespace OpenOrienteering {

class MapEditorController;
class MapWidget;


/**
 * Tool to scale objects.
 */
class ScaleTool : public MapEditorToolBase
{
Q_OBJECT
public:
	ScaleTool(MapEditorController* editor, QAction* tool_action);
	~ScaleTool() override;
	
protected:
	void initImpl() override;
	
	void updateStatusText() override;
	
	void clickRelease() override;
	void dragStart() override;
	void dragMove() override;
	void dragFinish() override;
	
	bool keyPressEvent(QKeyEvent* event) override;
	bool keyReleaseEvent(QKeyEvent* event) override;

	void drawImpl(QPainter* painter, MapWidget* widget) override;
	
	int updateDirtyRectImpl(QRectF& rect) override;
	void objectSelectionChangedImpl() override;
	
	MapCoordF scaling_center;
	double reference_length = 0;
	double scaling_factor   = 1;
	bool using_scaling_center = true;
};


}  // namespace OpenOrienteering

#endif
