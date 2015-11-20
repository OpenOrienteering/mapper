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


#ifndef _OPENORIENTEERING_DRAW_POINT_H_
#define _OPENORIENTEERING_DRAW_POINT_H_

#include <QScopedPointer>

#include "tool_base.h"

class MapWidget;
class PointObject;
class Symbol;
class SymbolWidget;
class KeyButtonBar;

/**
 * Tool to draw PointObjects.
 */
class DrawPointTool : public MapEditorToolBase
{
Q_OBJECT
public:
	DrawPointTool(MapEditorController* editor, QAction* tool_action);
	virtual ~DrawPointTool();
	
	virtual void leaveEvent(QEvent* event);
	
protected slots:
	void activeSymbolChanged(Symbol* symbol);
	void symbolDeleted(int pos, Symbol* old_symbol);
	
protected:
	/**
	 * Checks if the user dragged the mouse away a certain minimum distance from
	 * the click point and if yes, returns the drag angle, otherwise returns 0.
	 */
	float calculateRotation(QPointF mouse_pos, MapCoordF mouse_pos_map);

	virtual void initImpl();
	virtual int updateDirtyRectImpl(QRectF& rect);
	virtual void drawImpl(QPainter* painter, MapWidget* widget);
	virtual void updateStatusText();
	virtual void objectSelectionChangedImpl();
	
	virtual void clickPress();
	virtual void clickRelease();
	virtual void mouseMove();
	virtual void dragStart();
	virtual void dragMove();
	virtual void dragFinish();
	virtual bool keyPress(QKeyEvent* event);
	virtual bool keyRelease(QKeyEvent* event);
	
	bool rotating;
	Symbol* last_used_symbol;
	PointObject* preview_object;
	QScopedPointer<MapRenderables> renderables;
	KeyButtonBar* key_button_bar;
};

#endif
