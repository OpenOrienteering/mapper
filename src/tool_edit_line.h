/*
 *    Copyright 2012 Thomas Schöps
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


#ifndef _OPENORIENTEERING_EDIT_LINE_TOOL_H_
#define _OPENORIENTEERING_EDIT_LINE_TOOL_H_

#include <QScopedPointer>

#include "tool.h"
#include "tool_helpers.h"
#include "tool_edit.h"

class MapWidget;
class CombinedSymbol;
class PointObject;
class PathObject;
class Symbol;


/// Tool to edit lines of path objects
class EditLineTool : public EditTool
{
Q_OBJECT
public:
	EditLineTool(MapEditorController* editor, QAction* tool_button, SymbolWidget* symbol_widget);
	virtual ~EditLineTool();
	
    virtual void mouseMove();
	virtual void clickRelease();
	
    virtual void dragStart();
    virtual void dragMove();
    virtual void dragFinish();
	
protected:
    virtual bool keyPress(QKeyEvent* event);
    virtual bool keyRelease(QKeyEvent* event);
	
    virtual void initImpl();
    virtual void objectSelectionChangedImpl();
    virtual int updateDirtyRectImpl(QRectF& rect);
	virtual void drawImpl(QPainter* painter, MapWidget* widget);
	
	/// In addition to the base class implementation, updates the status text.
    virtual void updatePreviewObjects();
	
	void updateStatusText();
	void updateHoverLine(MapCoordF cursor_pos);
	void toggleAngleHelper();
	
	inline bool hoveringOverFrame() const {return hover_line == -1;}
	
	/// Bounding box of the selection
	QRectF selection_extent;
	
	/// Path point index of the hover line's starting point if non-negative;
	/// if hovering over the extent rect: -1
	/// if hovering over nothing: -2
	int hover_line;
	
	/// Object for hover_line, or NULL in case of no hover line.
	PathObject* hover_object;
	
	/// Object made of the extracted hover line
	PathObject* highlight_object;
	
	/// Is a box selection in progress?
	bool box_selection;
	
	/// Offset from cursor position to drag handle of moved element.
	/// When snapping to another element, this offset must be corrected.
	MapCoordF handle_offset;
	
	QScopedPointer<ObjectMover> object_mover;
	QScopedPointer<MapRenderables> highlight_renderables;
	
	static int max_objects_for_handle_display;
};

#endif
