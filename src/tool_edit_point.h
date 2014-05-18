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


#ifndef _OPENORIENTEERING_EDIT_POINT_TOOL_H_
#define _OPENORIENTEERING_EDIT_POINT_TOOL_H_

#include <QScopedPointer>
#include <QElapsedTimer>

#include "tool_edit.h"

class MapWidget;
class CombinedSymbol;
class PointObject;
class Symbol;
class TextObjectEditorHelper;


/**
 * Standard tool to edit all types of objects.
 */
class EditPointTool : public EditTool
{
Q_OBJECT
public:
	EditPointTool(MapEditorController* editor, QAction* tool_action);
	virtual ~EditPointTool();
	
	/** Returns true if new points shall be added as dash points by default.
	 *  This depends only on the symbol of the selected element. */
	bool addDashPointDefault() const;
	
	virtual bool mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	
	virtual void mouseMove();
	virtual void clickPress();
	virtual void clickRelease();
	
	virtual void dragStart();
	virtual void dragMove();
	virtual void dragFinish();
	
	virtual void focusOutEvent(QFocusEvent* event);
	
public slots:
	void textSelectionChanged(bool text_change);
	
protected:
	virtual bool keyPress(QKeyEvent* event);
	virtual bool keyRelease(QKeyEvent* event);
	
	virtual void initImpl();
	virtual void objectSelectionChangedImpl();
	virtual int updateDirtyRectImpl(QRectF& rect);
	virtual void drawImpl(QPainter* painter, MapWidget* widget);
	
	/**
	 * Hides the method from MapEditorToolBase.
	 * Contains special treatment for text objects.
	 */
	void finishEditing();
	
	/** In addition to the base class implementation, updates the status text. */
	virtual void updatePreviewObjects();
	
	void updateStatusText();
	
	/** Updates hover_point and hover_object. */
	void updateHoverPoint(MapCoordF cursor_pos);
	
	/** Does additional editing setup required after calling startEditing(). */
	void startEditingSetup();
	
	/**
	 * Checks if a single text object is the only selected object and the
	 * cursor hovers over it.
	 */
	bool hoveringOverSingleText();
	
	/** Checks if the cursor hovers over the selection frame. */
	inline bool hoveringOverFrame() const {return hover_point == -1;}
	
	
	/** Measures the time a click takes to decide whether to do selection. */
	QElapsedTimer click_timer;
	
	/** Bounding box of the selection */
	QRectF selection_extent;
	
	/**
	 * Path point index of the hover point if non-negative;
	 * if hovering over the extent rect: -1
	 * if hovering over nothing: -2
	 */
	int hover_point;
	
	/**
	 * Object for hover_point, or the closest object if no hover point,
	 * or NULL.
	 */
	Object* hover_object;
	
	/** Is a box selection in progress? */
	bool box_selection;
	
	QScopedPointer<ObjectMover> object_mover;
	
	// Mouse / key handling
	bool no_more_effect_on_click;
	bool space_pressed;
	
	/**
	 * Offset from cursor position to drag handle of moved element.
	 * When snapping to another element, this offset must be corrected.
	 */
	MapCoordF handle_offset;
	
	/** Text editor tool helper */
	TextObjectEditorHelper* text_editor;
	
	/**
	 * To prevent creating an undo step if text edit mode is entered and
	 * finished, but no text was changed
	 */
	QString old_text;
	/** See old_text */
	int old_horz_alignment;
	/** See old_text */
	int old_vert_alignment;
	
	/**
	 * Maximum number of objects in the selection for which point handles
	 * will still be displayed (and can be edited).
	 */
	static int max_objects_for_handle_display;
};

#endif
