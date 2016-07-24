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


#ifndef OPENORIENTEERING_DRAW_TEXT_TOOL_H
#define OPENORIENTEERING_DRAW_TEXT_TOOL_H

#include "tool_base.h"

#include <memory>

#include "renderable.h"


class TextObject;
class TextObjectEditorHelper;
class MapRenderables;
class Symbol;

/**
 * Tool to draw text objects.
 */
class DrawTextTool : public MapEditorToolBase
{
Q_OBJECT
public:
	DrawTextTool(MapEditorController* editor, QAction* tool_action);
	virtual ~DrawTextTool();
	
protected:
	virtual void initImpl();
	
	virtual bool mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	void leaveEvent(QEvent* event);
	
	virtual bool keyPressEvent(QKeyEvent* event);
	virtual bool keyReleaseEvent(QKeyEvent* event);
	
	virtual void drawImpl(QPainter* painter, MapWidget* widget);
	
	void startEditing();
	virtual void finishEditing();
	
	void setDrawingSymbol(const Symbol* symbol);
	
	void selectionChanged(bool text_change);
	
	virtual int updateDirtyRectImpl(QRectF& rect);
	void updateStatusText();
	
	void updatePreviewText();
	void setPreviewLetter();
	void abortEditing();
	
	const Symbol* drawing_symbol;
	
	
	
	MapRenderables renderables;
	std::unique_ptr<TextObject, MapRenderables::ObjectDeleter> preview_text;
	std::unique_ptr<TextObjectEditorHelper> text_editor;
	
	virtual void mouseMove();
	virtual void clickPress();
	virtual void clickRelease();
	virtual void dragMove();
	virtual void dragFinish();
	virtual bool keyPress(QKeyEvent* event);
	virtual bool keyRelease(QKeyEvent* event);
	virtual void objectSelectionChangedImpl();
};

#endif
