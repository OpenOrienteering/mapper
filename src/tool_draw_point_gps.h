/*
 *    Copyright 2014 Thomas Schöps
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


#ifndef _OPENORIENTEERING_DRAW_POINT_GPS_H_
#define _OPENORIENTEERING_DRAW_POINT_GPS_H_

#include <QScopedPointer>

#include "tool_base.h"

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE
class GPSDisplay;
class MapWidget;
class PointObject;
class Symbol;
class SymbolWidget;

/**
 * Tool to draw a PointObject at the GPS position.
 */
class DrawPointGPSTool : public MapEditorToolBase
{
Q_OBJECT
public:
	DrawPointGPSTool(GPSDisplay* gps_display, MapEditorController* editor, QAction* tool_action);
	virtual ~DrawPointGPSTool();
	
public slots:
	void newGPSPosition(MapCoordF coord, float accuracy);
	
protected slots:
	void activeSymbolChanged(Symbol* symbol);
	void symbolDeleted(int pos, Symbol* old_symbol);
	
protected:
	virtual void initImpl();
	virtual int updateDirtyRectImpl(QRectF& rect);
	virtual void drawImpl(QPainter* painter, MapWidget* widget);
	virtual void updateStatusText();
	virtual void objectSelectionChangedImpl();
	
	virtual void clickRelease();
	virtual bool keyPress(QKeyEvent* event);
	
	double x_sum;
	double y_sum;
	double weights_sum;
	
	Symbol* last_used_symbol;
	PointObject* preview_object;
	QScopedPointer<MapRenderables> renderables;
	QLabel* help_label;
};

#endif
