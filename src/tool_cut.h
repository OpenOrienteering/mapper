/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2015 Kai Pastor
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


#ifndef _OPENORIENTEERING_TOOL_CUT_H_
#define _OPENORIENTEERING_TOOL_CUT_H_

#include <QScopedPointer>

#include "core/path_coord.h"
#include "tool.h"
#include "tool_edit.h"

class DrawPathTool;
class PathObject;
class MapRenderables;

/**
 * A tool to cut objects into smaller pieces.
 * 
 * May cut lines and areas.
 * 
 * \todo This tool has some similarities with EditPointTool and maybe should use
 *       the same base class (EditTool).
 */
class CutTool : public MapEditorTool
{
public:
	using HoverFlag  = EditTool::HoverFlag;
	using HoverState = EditTool::HoverState;
	
Q_OBJECT
public:
	CutTool(MapEditorController* editor, QAction* tool_action);
	virtual ~CutTool();
	
	virtual void init();
	virtual const QCursor& getCursor() const;
	
	virtual bool mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseDoubleClickEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual void leaveEvent(QEvent* event);
	
	virtual bool keyPressEvent(QKeyEvent* event);
	virtual bool keyReleaseEvent(QKeyEvent* event);
	virtual void focusOutEvent(QFocusEvent* event);
	
	virtual void draw(QPainter* painter, MapWidget* widget);
	
public slots:
	void objectSelectionChanged();
	void pathDirtyRectChanged(const QRectF& rect);
	void pathAborted();
	void pathFinished(PathObject* split_path);
	
protected:
	/**
	 * Splits the given path object, removing the section between begin and end.
	 * 
	 * This may remove the object from the map and add new objects instead.
	 */
	void splitLine(PathObject* object, std::size_t part_index, qreal begin, qreal end) const;
	
	/**
	 * Splits the path object at the given position.
	 * 
	 * This may remove the object from the map and add new objects instead.
	 */
	void splitLine(PathObject* object, const PathCoord& split_pos) const;
	
	/**
	 * Replaces the given object in the map with the replacement objects.
	 * 
	 * Creates the neccessary undo steps. If replacement is empty, the object is
	 * deleted without replacement.
	 * 
	 * @todo Consider moving this to a more general class (Map, MapPart).
	 */
	void replaceObject(PathObject* object, const std::vector<PathObject*>& replacement) const;
	
	void updateStatusText();
	void updatePreviewObjects();
	void deletePreviewPath();
	void updateDirtyRect(const QRectF* path_rect = nullptr) const;
	void updateDragging(MapCoordF cursor_pos_map, MapWidget* widget);
	void updateHoverState(QPointF cursor_pos_screen, MapWidget* widget);
	bool findEditPoint(PathCoord& out_edit_point, PathObject*& out_edit_object, MapCoordF cursor_pos_map, int with_type, int without_type, MapWidget* widget);
	
	void startCuttingArea(const PathCoord& coord, MapWidget* widget);
	
	// Mouse handling
	QPoint click_pos;
	MapCoordF click_pos_map;
	QPoint cur_pos;
	MapCoordF cur_pos_map;
	bool dragging;
	
	HoverState hover_state;
	Object* hover_object;
	MapCoordVector::size_type hover_point;
	PathObject* edit_object;
	
	// For removing segments from lines
	quint32 drag_part_index; // PathPartVector::size_type
	float drag_start_len;
	float drag_end_len;
	bool drag_forward;		// true if [drag_start_len; drag_end_len] is the drag range, else [drag_end_len; drag_start_len]
	bool dragging_on_line;
	
	// For cutting areas
	bool cutting_area;
	DrawPathTool* path_tool;
	MapWidget* edit_widget;
	
	// Preview objects for dragging
	PathObject* preview_path;
	QScopedPointer<MapRenderables> renderables;
};

#endif
