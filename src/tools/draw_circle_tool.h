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


#ifndef OPENORIENTEERING_DRAW_CIRCLE_H
#define OPENORIENTEERING_DRAW_CIRCLE_H

#include <QObject>
#include <QPoint>
#include <QPointer>

#include "core/map_coord.h"
#include "tools/draw_line_and_area_tool.h"

class QAction;
class QCursor;
class QKeyEvent;
class QMouseEvent;
class QPainter;

namespace OpenOrienteering {

class KeyButtonBar;
class MapEditorController;
class MapWidget;


/** Tool to draw circles and ellipses. */
class DrawCircleTool : public DrawLineAndAreaTool
{
Q_OBJECT
public:
	DrawCircleTool(MapEditorController* editor, QAction* tool_action, bool is_helper_tool);
	~DrawCircleTool() override;
	
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
	
	void updateCircle();
	void setDirtyRect();
	void updateStatusText();
	
	QPoint click_pos;
	MapCoordF circle_start_pos_map;
	QPoint cur_pos;
	MapCoordF cur_pos_map;
	MapCoordF opposite_pos_map;		// position on cirlce/ellipse opposite to click_pos_map
	bool dragging          = false;
	bool start_from_center = false;
	bool first_point_set   = false;
	bool second_point_set  = false;
	QPointer<KeyButtonBar> key_button_bar;
};


}  // namespace OpenOrienteering

#endif
