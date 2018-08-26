/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2016 Kai Pastor
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


#ifndef OPENORIENTEERING_CUT_TOOL_H
#define OPENORIENTEERING_CUT_TOOL_H

#include <vector>

#include <QObject>
#include <QRectF>
#include <QScopedPointer>

#include "core/map_coord.h"
#include "core/path_coord.h"
#include "core/objects/object.h"
#include "tools/edit_tool.h"
#include "tools/tool_base.h"

class QAction;
class QEvent;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QPainter;
class QRectF;

namespace OpenOrienteering {

class DrawPathTool;
class MapEditorController;
class MapRenderables;
class MapWidget;


/**
 * A tool to cut (split) lines and areas into smaller pieces.
 */
class CutTool : public MapEditorToolBase
{
Q_OBJECT
public:
	using length_type = PathCoord::length_type;
	using HoverFlag   = EditTool::HoverFlag;
	using HoverState  = EditTool::HoverState;
	
	CutTool(MapEditorController* editor, QAction* tool_action);
	~CutTool() override;
	
protected:
	void initImpl() override;
	
	void updateStatusText() override;
	
	void objectSelectionChangedImpl() override;
	
	// MapEditorTool input event handlers
	bool mousePressEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget) override;
	bool mouseMoveEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget) override;
	bool mouseReleaseEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget) override;
	bool mouseDoubleClickEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget) override;
	void leaveEvent(QEvent* event) override;
	void focusOutEvent(QFocusEvent* event) override;
	
	// MapEditorToolBase input event handlers
	void mouseMove() override;
	void clickPress() override;
	void clickRelease() override;
	void dragStart() override;
	void dragMove() override;
	void dragFinish() override;
	bool keyPress(QKeyEvent* event) override;
	bool keyRelease(QKeyEvent* event) override;
	
	// Functions for splitting lines
	void startCuttingLine(const ObjectPathCoord& point);
	void updateCuttingLine(const MapCoordF& cursor_pos);
	void finishCuttingLine();
	
	// Functions for splitting areas
	bool startCuttingArea(const ObjectPathCoord& point);
	void abortCuttingArea();
	void finishCuttingArea(PathObject* split_path);
	
	// Drawing
	void updatePreviewObjects() override;
	void deletePreviewObject();
	int updateDirtyRectImpl(QRectF& rect) override;
	void drawImpl(QPainter* painter, MapWidget* widget) override;
	
	// State 
	void updateHoverState(const MapCoordF& cursor_pos);
	ObjectPathCoord findEditPoint(const MapCoordF& cursor_pos_map, int with_type, int without_type) const;
	
	/**
	 * Replaces the given object in the map with the replacement objects.
	 * 
	 * Creates the neccessary undo steps. If replacement is empty, the object is
	 * deleted without replacement.
	 * 
	 * @todo Consider moving this to a more general class (Map, MapPart).
	 */
	void replaceObject(Object* object, const std::vector<PathObject*>& replacement) const;
	
	// Basic state
	bool waiting_for_mouse_release = false;
	HoverState  hover_state = HoverFlag::OverNothing;
	PathObject* hover_object = nullptr;
	MapCoordVector::size_type hover_point = 0;
	
	// The object which is about to be split
	PathObject* edit_object = nullptr;
	
	// State for splitting lines
	PathPartVector::size_type drag_part_index;
	length_type drag_start_len;
	length_type drag_end_len;
	bool reverse_drag; // if true, the effective drag range is [drag_end_len; drag_start_len]
	
	// State for cutting areas
	DrawPathTool* path_tool = nullptr;
	QRectF path_tool_rect;
	
	// Preview objects for dragging
	PathObject* preview_path = nullptr;
	/**
	 * The renderables member in MapEditorToolBase contains the selection at the moment,
	 * but the path of cutting a line needs to be drawn separately.
	 * \todo Rewrite renderables handling in MapEditorToolBase so that we can remove it here.
	 */
	QScopedPointer<MapRenderables> renderables;
};


}  // namespace OpenOrienteering

#endif
