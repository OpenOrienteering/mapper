/*
 *    Copyright 2014 Thomas Schöps
 *    Copyright 2014, 2015, 2017 Kai Pastor
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


#ifndef OPENORIENTEERING_DRAW_FREEHAND_TOOL_H
#define OPENORIENTEERING_DRAW_FREEHAND_TOOL_H

#include <cstddef>
#include <vector>

#include <QtGlobal>
#include <QObject>
#include <QPoint>

#include "core/map_coord.h"
#include "tools/draw_line_and_area_tool.h"

class QAction;
class QCursor;
class QKeyEvent;
class QMouseEvent;
class QPainter;

namespace OpenOrienteering {

class MapEditorController;
class MapWidget;


/** Tool for free-hand drawing. */
class DrawFreehandTool : public DrawLineAndAreaTool
{
Q_OBJECT
public:
	DrawFreehandTool(MapEditorController* editor, QAction* tool_action, bool is_helper_tool);
	~DrawFreehandTool() override;
	
	void init() override;
	const QCursor& getCursor() const override;
	
	bool mousePressEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget) override;
	bool mouseMoveEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget) override;
	bool mouseReleaseEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget) override;
	
	bool keyPressEvent(QKeyEvent* event) override;
	
	void draw(QPainter* painter, MapWidget* widget) override;
	
protected:
	void finishDrawing() override;
	void abortDrawing() override;
	
	void updatePath();
	void setDirtyRect();
	void updateStatusText();
	
private:
	void checkLineSegment(std::size_t first, std::size_t last);
	
	std::vector<bool> point_mask;
	qreal split_distance_sq;
	
	QPoint last_pos;
	QPoint cur_pos;
	MapCoordF cur_pos_map;
};


}  // namespace OpenOrienteering

#endif
