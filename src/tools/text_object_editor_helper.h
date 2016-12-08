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


#ifndef OPENORIENTEERING_TEXT_OBJECT_EDITOR_HELPER_H
#define OPENORIENTEERING_TEXT_OBJECT_EDITOR_HELPER_H

#include <functional>

#include <QObject>

#include "../tool.h"

QT_BEGIN_NAMESPACE
class QKeyEvent;
class QMouseEvent;
class QPainter;
class QTimer;
QT_END_NAMESPACE

class MapEditorController;
class TextObject;
class TextObjectAlignmentDockWidget;


/**
 * Helper class editing the text of TextObject.
 * 
 * This class is meant to be used by tools (DrawTextTool, EditPointTool) in the
 * following way:
 * 
 *  - When text editing starts, the tool must construct an object of this class.
 *    The constructor takes care of modifying the state of the main window.
 *  - Then the tool must pass all mouse and key events through this object,
 *    call draw() etc. The object will emit a signal for state changes which
 *    need a redraw of the output and another signal when editing is finished.
 *  - When editing is finished, the tool must destroy the object. The destructor
 *    will restore the original state of the main window.
 */
class TextObjectEditorHelper : public QObject
{
Q_OBJECT
public:
	TextObjectEditorHelper(TextObject* text_object, MapEditorController* editor);
	
	~TextObjectEditorHelper() override;
	
	
	/**
	 * Returns the text object which is edited by this object.
	 */
	TextObject* object() const;
	
	
	/**
	 * Sets the position of the selection anchor and of the cursor.
	 * 
	 * The text between the anchor position and the cursor position is the
	 * current selection. Setting both parameters to the same value results
	 * in an empty selection.
	 * 
	 * The reference position for linewise selection is set to the cursor
	 * position.
	 * 
	 * @return True iff there actually was a change.
	 */
	bool setSelection(int anchor, int cursor);
	
	/**
	 * Returns the text of the current selection.
	 */
	QString selectionText() const;
	
	/**
	 * Inserts text in place of the current selection.
	 * 
	 * After this, the selection will be empty, and the cursor will be at the
	 * end of the replacement text.
	 */
	void replaceSelectionText(const QString& replacement);
	
	
	bool mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	bool mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	bool mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	
	bool keyPressEvent(QKeyEvent* event);
	bool keyReleaseEvent(QKeyEvent* event);
	
	void draw(QPainter* painter, MapWidget* widget);
	
	void includeDirtyRect(QRectF& rect) const;
	
signals:
	/**
	 * Emitted when the text object or the selection is modified.
	 * 
	 * This includes movements of the cursor when the selection is empty.
	 */
	void stateChanged();
	
	/**
	 * Emitted when editing is finished.
	 */
	void finished();
	
private slots:
	/**
	 * Claims focus for the editor's main map widget.
	 * 
	 * This is required for proper input event handling.
	 */
	void claimFocus();
	
private:
	/**
	 * Sets the position of the selection anchor, the cursor, and the reference
	 * position for linewise selection.
	 * 
	 * @return True iff there actually was a change.
	 */
	bool setSelection(int anchor, int cursor, int line_position);
	
	/**
	 * Sets the horizontal and vertical alignment of the text object.
	 * 
	 * This methods acts as a slot for messages from TextObjectAlignmentDockWidget.
	 */
	void setTextAlignment(int h_alignment, int v_alignment);
	
	/**
	 * Switches between the text editing cursor and the default cursor.
	 */
	void updateCursor(QWidget* widget, int position);
	
	/**
	 * Adjusts the selection while dragging.
	 */
	void updateDragging(MapCoordF map_coord);
	
	/**
	 * Calls the worker function for the selection rectangle of each line.
	 */
	void foreachLineSelectionRect(std::function<void(const QRectF&)> worker) const;
	
	
	TextObject* text_object;
	MapEditorController* editor;
	TextObjectAlignmentDockWidget* dock_widget;
	int anchor_position;
	int cursor_position;
	int line_selection_position;
	bool dragging;
	bool text_cursor_active;
};


inline
TextObject* TextObjectEditorHelper::object() const
{
	return text_object;
}

inline
bool TextObjectEditorHelper::setSelection(int anchor, int cursor)
{
	return setSelection(anchor, cursor, cursor);
}


#endif
