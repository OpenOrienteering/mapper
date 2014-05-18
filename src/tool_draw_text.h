/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2014 Kai Pastor
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


#ifndef _OPENORIENTEERING_DRAW_TEXT_H_
#define _OPENORIENTEERING_DRAW_TEXT_H_

#include <QDockWidget>

#include "tool.h"

QT_BEGIN_NAMESPACE
class QPushButton;
QT_END_NAMESPACE

class TextObject;
struct TextObjectLineInfo;
class TextObjectEditorHelper;
class TextObjectAlignmentDockWidget;
class MapRenderables;
class Symbol;

/**
 * Tool to draw text objects.
 */
class DrawTextTool : public MapEditorTool
{
Q_OBJECT
public:
	DrawTextTool(MapEditorController* editor, QAction* tool_action);
	virtual ~DrawTextTool();
	
	virtual void init();
	virtual QCursor* getCursor() {return cursor;}
	
	virtual bool mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	void leaveEvent(QEvent* event);
	
	virtual bool keyPressEvent(QKeyEvent* event);
	virtual bool keyReleaseEvent(QKeyEvent* event);
	
	virtual void draw(QPainter* painter, MapWidget* widget);
	
	static QCursor* cursor;
	
protected slots:
	void setDrawingSymbol(Symbol* symbol);
	
	void selectionChanged(bool text_change);
	
protected:
	void updateDirtyRect();
	void updateStatusText();
	
	void updatePreviewText();
	void deletePreviewText();
	void setPreviewLetter();
	void abortEditing();
	void finishEditing();
	
	Symbol* drawing_symbol;
	
	QPoint click_pos;
	MapCoordF click_pos_map;
	QPoint cur_pos;
	MapCoordF cur_pos_map;
	bool dragging;
	
	TextObject* preview_text;
	TextObjectEditorHelper* text_editor;
	
	QScopedPointer<MapRenderables> renderables;
};

/**
 * Widget which is shown while the DrawTextTool is active, allowing the set
 * the text alignment.
 */
class TextObjectAlignmentDockWidget : public QDockWidget
{
Q_OBJECT
public:
	TextObjectAlignmentDockWidget(TextObject* object, int horz_default, int vert_default, TextObjectEditorHelper* text_editor, QWidget* parent);
	virtual QSize sizeHint() const {return QSize(10, 10);}
	
	virtual bool event(QEvent* event);
	virtual void keyPressEvent(QKeyEvent* event);
	virtual void keyReleaseEvent(QKeyEvent* event);
	
signals:
	void alignmentChanged(int horz, int vert);
	
public slots:
	void horzClicked(int index);
	void vertClicked(int index);
	
private:
	void addHorzButton(int index, const QString& icon_path, int horz_default);
	void addVertButton(int index, const QString& icon_path, int vert_default);
	void emitAlignmentChanged();
	
	QPushButton* horz_buttons[3];
	QPushButton* vert_buttons[4];
	int horz_index;
	int vert_index;
	
	TextObject* object;
	TextObjectEditorHelper* text_editor;
};

#endif
