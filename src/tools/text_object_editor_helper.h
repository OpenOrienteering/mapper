/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2017 Kai Pastor
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

#include <Qt>
#include <QObject>
#include <QString>

class QEvent;
class QInputMethodEvent;
class QKeyEvent;
class QMouseEvent;
class QPainter;
class QRectF;
class QVariant;
class QWidget;

namespace OpenOrienteering {

class MapCoordF;
class MapEditorController;
class MapWidget;
class TextObject;
class TextObjectAlignmentDockWidget;


/**
 * Helper class for editing the text of a TextObject.
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
	/**
	 * Indicates arguments which must not be nullptr.
	 * \todo Use the Guideline Support Library
	 */
	template <typename T>
	using not_null = T;
	
	
	/**
	 * Actions which may be performed on batch edit start or end.
	 */
	enum BatchEditAction
	{
		NoInputMethodAction   = 0x00,  ///< On end, emit state change signal, but don't notify the input method.
		UpdateInputProperties = 0x01,  ///< On end, update commonly changed input properties.
		UpdateAllProperties   = 0x02,  ///< On end, update all input properties.
		ResetInputMethod      = 0x03,  ///< On end, reset input method (alternative to updating).
		CommitPreedit         = 0x10,  ///< On start, commit the preedit string.
	};
	
	/**
	 * Establishes a scoped batch edit context and handles state change signals.
	 * 
	 * Batch edits are sequences of possibly nested operations which change the
	 * state which is observable by outside objects.
	 * Scoped objects of this class take care of sending messages to observers
	 * only at the beginning and end of a complex editing operation.
	 * This helps to properly deal with the input method state.
	 */
	class BatchEdit
	{
	public:
		/**
		 * Constructor. 
		 * 
		 * Only the constructor of the outermost scope calls
		 * TextObjectEditorHelper::commitPreedit() if CommitPreedit is set.
		 */
		BatchEdit(not_null<TextObjectEditorHelper*> editor, int actions = CommitPreedit | UpdateInputProperties);
		
		BatchEdit(const BatchEdit&) = delete;
		
		/**
		 * Destructor.
		 * 
		 * The given actions have no effected in a nested batch edit context.
		 * Only the destructor of the outermost scope calls 
		 * TextObjectEditorHelper::commitStateChange().
		 */
		~BatchEdit();
		
		BatchEdit& operator=(const BatchEdit&) = delete;
		
	private:
		TextObjectEditorHelper* const editor;
	};
	
	
	
	TextObjectEditorHelper(not_null<TextObject*> text_object, not_null<MapEditorController*> editor);
	
	~TextObjectEditorHelper() override;
	
	
	/**
	 * Returns the text object which is edited by this object.
	 */
	TextObject* object() const;
	

private slots:	
	/**
	 * Claims focus for the editor's main map widget.
	 * 
	 * Focus on the map widget is required for proper input event handling.
	 * This function also activates the input method.
	 * 
	 * However no action is performed if the application is not in active state.
	 */
	void claimFocus();
	
	
protected:
	/**
	 * Returns true when the input method is composing input.
	 */
	bool isPreediting() const;
	
	/**
	 * Applies the current preedit string.
	 * 
	 * Normally not called directly but during construction of a BatchEdit object.
	 */
	void commitPreedit();
	
	/**
	 * Emits state change signals and updates the input method.
	 * 
	 * Normally not called directly but during destruction of a BatchEdit object.
	 */
	void commitStateChange();
	
	/**
	 * Updates the object's text from the current pristine text, cursor,
	 * and preedit string.
	 */
	void updateDisplayText();
	
	
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
	 * Must be called in a batch edit context.
	 * 
	 * @return True iff there actually was a change.
	 */
	bool setSelection(int anchor, int cursor);
	
	/**
	 * Sets the position of the selection anchor, the cursor, and the reference
	 * position for linewise selection.
	 * 
	 * Must be called in a batch edit context.
	 * 
	 * @return True iff there actually was a change.
	 */
	bool setSelection(int anchor, int cursor, int line_position);
	
	/**
	 * Returns the text of the current selection.
	 */
	QString selectionText() const;

	/**
	 * Inserts text in place of the current selection.
	 * 
	 * After this, the selection will be empty, and the cursor will be placed
	 * at the end of the replacement text.
	 * 
	 * Must be called in a batch edit context.
	 */
	void replaceSelectionText(const QString& replacement);
	
	
	/**
	 * Returns the absolute position where the current "block" starts (in the input method sense).
	 */
	int blockStart() const;
	
	/**
	 * Returns the absolute position where the current "block" ends (in the input method sense).
	 */
	int blockEnd() const;
	
	
public:
	QVariant inputMethodQuery(Qt::InputMethodQuery property, const QVariant& argument) const;
	bool inputMethodEvent(QInputMethodEvent* event);
	
	bool mousePressEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget);
	bool mouseMoveEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget);
	bool mouseReleaseEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget);
	
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
	
private:
	/**
	 * May send a mouse event to the input context.
	 * 
	 * Returns true when the event is processed by the input context.
	 * 
	 * \see QWidgetTextControl::sendMouseEventToInputContext
	 */
	bool sendMouseEventToInputContext(QEvent* event, const MapCoordF& map_coord);
	
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
	void updateDragging(const MapCoordF& map_coord);
	
	/**
	 * Returns the area occupied by the text cursor.
	 */
	QRectF cursorRectangle() const;
	
	/**
	 * Calls the worker function for the given range's rectangle of each line.
	 */
	void foreachLineRect(int begin, int end, const std::function<void(const QRectF&)>& worker) const;
	
	TextObject* text_object;
	MapEditorController* editor;
	TextObjectAlignmentDockWidget* dock_widget;
	QString pristine_text;
	QString preedit_string;
	mutable int block_start;
	int anchor_position;
	int cursor_position;
	int line_selection_position;
	int preedit_cursor;
	int batch_editing;
	int state_change_action;
	bool dirty;
	bool dragging;
	bool text_cursor_active;
	bool update_input_method_enabled;
	
	friend class BatchEdit;
};



inline
TextObject* TextObjectEditorHelper::object() const
{
	return text_object;
}


}  // namespace OpenOrienteering

#endif
