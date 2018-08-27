/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2013-2017 Kai Pastor
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


#ifndef OPENORIENTEERING_EDIT_LINE_TOOL_H
#define OPENORIENTEERING_EDIT_LINE_TOOL_H

#include <QElapsedTimer>
#include <QObject>
#include <QRectF>
#include <QScopedPointer>

#include "core/map_coord.h"
#include "tools/edit_tool.h"

class QAction;
class QKeyEvent;
class QMouseEvent;
class QPainter;
class QRectF;

namespace OpenOrienteering {

class MapEditorController;
class MapRenderables;
class MapWidget;
class ObjectMover;
class PathObject;


/**
 * A tool to edit lines of PathObjects.
 */
class EditLineTool : public EditTool
{
Q_OBJECT
public:
	EditLineTool(MapEditorController* editor, QAction* tool_action);
	~EditLineTool() override;
	
	bool mousePressEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget) override;
	bool mouseMoveEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget) override;
	bool mouseReleaseEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget) override;
	
	void mouseMove() override;
	void clickPress() override;
	void clickRelease() override;
	
	void dragStart() override;
	void dragMove() override;
	void dragFinish() override;
	void dragCanceled() override;
	
protected:
	bool keyPress(QKeyEvent* event) override;
	bool keyRelease(QKeyEvent* event) override;
	
	void initImpl() override;
	void objectSelectionChangedImpl() override;
	int updateDirtyRectImpl(QRectF& rect) override;
	void drawImpl(QPainter* painter, MapWidget* widget) override;
	
	/** In addition to the base class implementation, updates the status text. */
	void updatePreviewObjects() override;
	
	/** Deletes the highlight object if it exists and correctly removes its renderables. */
	void deleteHighlightObject();
	
	void updateStatusText() override;
	
	/** Recalculates hover_line. */
	void updateHoverState(const MapCoordF& cursor_pos);
	
	bool hoveringOverFrame() const;
	
private:
	/** Measures the time a click takes to decide whether to do selection. */
	QElapsedTimer click_timer;
	
	/** Bounding box of the selection */
	QRectF selection_extent;
	
	
	/**
	 * Provides general information on what is hovered over.
	 */
	HoverState hover_state = OverNothing;
	
	/**
	 * Object which is hovered over (if any).
	 */
	PathObject* hover_object = nullptr;
	
	/**
	 * Coordinate identifying the hover_object's line which is hovered over.
	 */
	MapCoordVector::size_type hover_line;
	
	/**
	 * An object created for the current hover_line.
	 */
	PathObject* highlight_object = nullptr;
	
	
	/** Is a box selection in progress? */
	bool box_selection = false;
	
	bool waiting_for_mouse_release = false;
	
	/**
	 * Offset from cursor position to drag handle of moved element.
	 * When snapping to another element, this offset must be corrected.
	 */
	MapCoordF handle_offset;
	
	QScopedPointer<ObjectMover> object_mover;
	QScopedPointer<MapRenderables> highlight_renderables;
};



// ### EditLineTool inline code ###

inline
bool EditLineTool::hoveringOverFrame() const
{
	return hover_state == OverFrame;
}


}  // namespace OpenOrienteering

#endif
