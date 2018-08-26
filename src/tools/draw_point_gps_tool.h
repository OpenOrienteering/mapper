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


#ifndef OPENORIENTEERING_DRAW_POINT_GPS_H
#define OPENORIENTEERING_DRAW_POINT_GPS_H

#include <QObject>
#include <QPointer>
#include <QScopedPointer>

#include "tools/tool_base.h"

class QAction;
class QKeyEvent;
class QLabel;
class QPainter;
class QRectF;

namespace OpenOrienteering {

class GPSDisplay;
class MapCoordF;
class MapEditorController;
class MapRenderables;
class MapWidget;
class PointObject;
class Symbol;


/**
 * Tool to draw a PointObject at the GPS position.
 */
class DrawPointGPSTool : public MapEditorToolBase
{
Q_OBJECT
public:
	DrawPointGPSTool(GPSDisplay* gps_display, MapEditorController* editor, QAction* tool_action);
	~DrawPointGPSTool() override;
	
public slots:
	void newGPSPosition(const MapCoordF& coord, float accuracy);
	
protected slots:
	void activeSymbolChanged(const Symbol* symbol);
	void symbolDeleted(int pos, const Symbol* old_symbol);
	
protected:
	void initImpl() override;
	int updateDirtyRectImpl(QRectF& rect) override;
	void drawImpl(QPainter* painter, MapWidget* widget) override;
	void updateStatusText() override;
	void objectSelectionChangedImpl() override;
	
	void clickRelease() override;
	bool keyPress(QKeyEvent* event) override;
	
	double x_sum;
	double y_sum;
	double weights_sum;
	
	const Symbol* last_used_symbol = nullptr;
	PointObject* preview_object = nullptr;
	QScopedPointer<MapRenderables> renderables;
	QPointer<QLabel> help_label;
};


}  // namespace OpenOrienteering
#endif
