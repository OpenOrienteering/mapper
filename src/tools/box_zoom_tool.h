/*
 *    Copyright 2025 Andreas Bertheussen
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


#ifndef OPENORIENTEERING_TOOL_BOX_ZOOM_H
#define OPENORIENTEERING_TOOL_BOX_ZOOM_H

#include <QObject>
#include <QString>

#include "core/map_coord.h"
#include "tools/tool_base.h"

class QAction;
class QPainter;

namespace OpenOrienteering {

class MapEditorController;


/**
 * Tool to pan the map.
 */
class BoxZoomTool : public MapEditorToolBase
{
	Q_OBJECT
public:
	BoxZoomTool(MapEditorController* editor, QAction* tool_action);
	~BoxZoomTool() override;

protected:
	void updateStatusText() override;
	void objectSelectionChangedImpl() override;
	void clickPress() override;
	void dragStart() override;
	void dragMove() override;
	void dragFinish() override;
	void dragCanceled() override;

	void drawImpl(QPainter* painter, MapWidget* widget) override;
	int updateDirtyRectImpl(QRectF& rect) override;
	MapCoordF selection_start_point, selection_end_point;
	MapCoordF selection_top_left;
	MapCoordF selection_bottom_right;
	QRectF selection_rectangle;
};


}  // namespace OpenOrienteering

#endif
