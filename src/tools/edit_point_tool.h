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


#ifndef OPENORIENTEERING_EDIT_POINT_TOOL_H
#define OPENORIENTEERING_EDIT_POINT_TOOL_H

#include <QElapsedTimer>
#include <QScopedPointer>

#include <Qt>
#include <QObject>
#include <QPointer>
#include <QRectF>
#include <QString>
#include <QVariant>

#include "core/map_coord.h"
#include "tools/edit_tool.h"

class QAction;
class QFocusEvent;
class QInputMethodEvent;
class QKeyEvent;
class QMouseEvent;
class QPainter;
class QRectF;
class QToolButton;

namespace OpenOrienteering {

class MapEditorController;
class MapWidget;
class Object;
class ObjectMover;
class TextObjectEditorHelper;


/**
 * Standard tool to edit all types of objects.
 * 
 * \todo See tool_edit_line.cpp for a number of todos.
 */
class EditPointTool : public EditTool
{
Q_OBJECT
public:
	EditPointTool(MapEditorController* editor, QAction* tool_action);
	~EditPointTool() override;
	
	/**
	 * Returns true if new points shall be added as dash points by default.
	 * 
	 * This depends only on the symbol of the selected element.
	 * 
	 * \todo Test/fix for combined symbols.
	 */
	bool addDashPointDefault() const;
	
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
	
	void focusOutEvent(QFocusEvent* event) override;
	
	/**
	 * Contains special treatment for text objects.
	 */
	void finishEditing() override;
	
protected:
	bool keyPress(QKeyEvent* event) override;
	bool keyRelease(QKeyEvent* event) override;
	bool inputMethodEvent(QInputMethodEvent* event) override;
	QVariant inputMethodQuery(Qt::InputMethodQuery property, const QVariant& argument) const override;
	
	void initImpl() override;
	void objectSelectionChangedImpl() override;
	int updateDirtyRectImpl(QRectF& rect) override;
	void drawImpl(QPainter* painter, MapWidget* widget) override;
	
	/** In addition to the base class implementation, updates the status text. */
	void updatePreviewObjects() override;
	
	void updateStatusText() override;
	
	/** 
	 * Updates hover_state, hover_object and hover_point.
	 */
	void updateHoverState(const MapCoordF& cursor_pos);
	
	/**
	 * Sets up the angle tool helper for the object currently hovered over.
	 * 
	 * The object must be of type Object::Path.
	 */
	void setupAngleHelperFromHoverObject();
	
	/** Does additional editing setup required after calling startEditing(). */
	void startEditingSetup();
	
	/**
	 * Checks if a single text object is the only selected object and the
	 * cursor hovers over it.
	 */
	bool hoveringOverSingleText() const;
	
	/**
	 * Checks if cursor hovers over a curve handle.
	 */
	bool hoveringOverCurveHandle() const;
	
	/**
	 * Checks if the cursor hovers over the selection frame.
	 */
	bool hoveringOverFrame() const;
	
	/**
	 * Checks if a the opposite handle should be moved synchronously.
	 */
	bool moveOppositeHandle() const;
	
	
	QPointer<QToolButton> dash_points_button;
	
	
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
	Object* hover_object = nullptr;
	
	/**
	 * Index of the object's coordinate which is hovered over.
	 */
	MapCoordVector::size_type hover_point = 0;
	
	
	/** Is a box selection in progress? */
	bool box_selection = false;
	
	QScopedPointer<ObjectMover> object_mover;
	
	// Mouse / key handling
	bool waiting_for_mouse_release = false;
	bool switch_dash_points = false;
	
	/**
	 * Offset from cursor position to drag handle of moved element.
	 * When snapping to another element, this offset must be corrected.
	 */
	MapCoordF handle_offset;
	
	/** Text editor tool helper */
	TextObjectEditorHelper* text_editor = nullptr;
	
	/**
	 * To prevent creating an undo step if text edit mode is entered and
	 * finished, but no text was changed
	 */
	QString old_text;
	/** See old_text */
	int old_horz_alignment;
	/** See old_text */
	int old_vert_alignment;
};



// ### EditPointTool inline code ###

inline
bool EditPointTool::hoveringOverFrame() const
{
	return hover_state == OverFrame;
}


}  // namespace OpenOrienteering

#endif
