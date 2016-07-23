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


#ifndef OPENORIENTEERING_DRAW_TEXT_H
#define OPENORIENTEERING_DRAW_TEXT_H

#include "tool.h"


class TextObject;
class TextObjectEditorHelper;
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
	virtual const QCursor& getCursor() const;
	
	virtual bool mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	void leaveEvent(QEvent* event);
	
	virtual bool keyPressEvent(QKeyEvent* event);
	virtual bool keyReleaseEvent(QKeyEvent* event);
	
	virtual void draw(QPainter* painter, MapWidget* widget);
	
	virtual void finishEditing();
	
protected slots:
	void setDrawingSymbol(const Symbol* symbol);
	
	void selectionChanged(bool text_change);
	
protected:
	void updateDirtyRect();
	void updateStatusText();
	
	void updatePreviewText();
	void deletePreviewText();
	void setPreviewLetter();
	void abortEditing();
	
	const Symbol* drawing_symbol;
	
	QPoint click_pos;
	MapCoordF click_pos_map;
	QPoint cur_pos;
	MapCoordF cur_pos_map;
	bool dragging;
	
	TextObject* preview_text;
	TextObjectEditorHelper* text_editor;
	
	QScopedPointer<MapRenderables> renderables;
};

#endif
