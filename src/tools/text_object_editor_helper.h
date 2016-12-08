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

#include <QObject>

#include "../tool.h"

QT_BEGIN_NAMESPACE
class QMouseEvent;
class QKeyEvent;
class QPainter;
class QTimer;
QT_END_NAMESPACE

class MapEditorController;
class TextObject;
class TextObjectAlignmentDockWidget;


/**
 * Helper class to enable text editing (using the DrawTextTool and the EditTool).
 * 
 * To use it, after constructing an instance for the TextObject to edit, you must
 * pass through all mouse and key events to it and call draw() & includeDirtyRect().
 * If mousePressEvent() or mouseReleaseEvent() returns false, editing is finished.
 */
class TextObjectEditorHelper : public QObject
{
Q_OBJECT
public:
	TextObjectEditorHelper(TextObject* object, MapEditorController* editor);
	~TextObjectEditorHelper();
	
	bool mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	bool mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	bool mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	
	bool keyPressEvent(QKeyEvent* event);
	bool keyReleaseEvent(QKeyEvent* event);
	
	void draw(QPainter* painter, MapWidget* widget);
	void includeDirtyRect(QRectF& rect);
	
	inline void setSelection(int start, int end) {selection_start = start; selection_end = end; click_position = start;}
	inline TextObject* getObject() const {return object;}
	
public slots:
	void alignmentChanged(int horz, int vert);
	void setFocus();
	
signals:
	/// Emitted when a user action changes the selection (not called by setSelection()), or the text alignment. If the text is also changed, text_change is true.
	void selectionChanged(bool text_change);
	
private:
	void insertText(QString text);
	void updateDragging(MapCoordF map_coord);
	bool getNextLinesSelectionRect(int& line, QRectF& out);
	
	bool dragging;
	int click_position;
	int selection_start;
	int selection_end;
	TextObject* object;
	MapEditorController* editor;
	TextObjectAlignmentDockWidget* dock_widget;
	QCursor original_cursor;
	bool original_cursor_retrieved;
	QTimer* timer;
};


#endif
