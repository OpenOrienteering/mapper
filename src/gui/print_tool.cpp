/*
 *    Copyright 2012 Thomas Schöps, Kai Pastor
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


#include "print_tool.h"

#include <QMouseEvent>
#include <QPainter>

#include "../core/map_printer.h"
#include "../map_widget.h"
#include "print_widget.h"


PrintTool::PrintTool(MapEditorController* editor, MapPrinter* map_printer)
: MapEditorTool(editor, Other, NULL),
  map_printer(map_printer),
  dragging(false)
{
	Q_ASSERT(editor != NULL);
	Q_ASSERT(map_printer != NULL);
	
	connect(map_printer, SIGNAL(printAreaChanged(QRectF)), this, SLOT(updatePrintArea()));
	connect(map_printer, SIGNAL(pageFormatChanged(MapPrinterPageFormat)), this, SLOT(updatePrintArea()));
	connect(map_printer, SIGNAL(optionsChanged(MapPrinterOptions)), this, SLOT(updatePrintArea()));
}

void PrintTool::init()
{
	setStatusBarText(tr("<b>Drag</b> to move the print area"));
	updatePrintArea();
}

QCursor* PrintTool::getCursor()
{
	static QCursor open_hand_cursor(Qt::OpenHandCursor);
	static QCursor closed_hand_cursor(Qt::ClosedHandCursor);
	return dragging ? &closed_hand_cursor : &open_hand_cursor;
}

bool PrintTool::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (event->button() == Qt::LeftButton)
	{
		dragging = true;
		click_pos_map = map_coord;
		widget->setCursor(*getCursor());
		return true;
	}
	
	return false;
}

bool PrintTool::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if ((event->buttons() & Qt::LeftButton) && dragging)
	{
		updateDragging(map_coord);
		return true;
	}
	
	return false;
}

bool PrintTool::mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (event->button() == Qt::LeftButton)
	{
		updateDragging(map_coord);
		dragging = false;
		widget->setCursor(*getCursor());
		return true;
	}
	
	return false;
}

void PrintTool::draw(QPainter* painter, MapWidget* widget)
{
	QRect view_area = QRect(0, 0, widget->width(), widget->height());
	QRect print_area = widget->mapToViewport(map_printer->getPrintArea()).toRect();
	
	// Strongly darken the region outside the print area
	painter->setBrush(QColor(0, 0, 0, 160));
	painter->setPen(Qt::NoPen);
	QPainterPath outside_path;
	outside_path.addRect(view_area);
	outside_path.addRect(view_area.intersected(print_area));
	painter->drawPath(outside_path);

	// Draw red lines for page breaks
	painter->setPen(QColor(255, 0, 0, 160));
	painter->setBrush(Qt::NoBrush);
	bool first_item = true;
	Q_FOREACH(qreal hpos, map_printer->horizontalPagePositions())
	{
		if (first_item) { first_item = false; continue; }
		QPointF view_pos = widget->mapToViewport(MapCoordF(hpos, 0));
		painter->drawLine(view_pos.x(), 0, view_pos.x(), widget->height());
	}
	first_item = true;
	Q_FOREACH(qreal vpos, map_printer->verticalPagePositions())
	{
		if (first_item) { first_item = false; continue; }
		QPointF view_pos = widget->mapToViewport(MapCoordF(0, vpos));
		painter->drawLine(0, view_pos.y(), widget->width(), view_pos.y());
	}
}

void PrintTool::updatePrintArea()
{
	// The print area visualization is updated by redrawing the whole map.
	// TODO: Replace with a more explicit way of marking the whole map area as dirty.
	editor->getMap()->setDrawingBoundingBox(QRectF(-1000000, -1000000, 2000000, 2000000), 0);
}

void PrintTool::updateDragging(MapCoordF mouse_pos_map)
{
	QRectF area = map_printer->getPrintArea();
	area.moveLeft(area.left() + mouse_pos_map.getX() - click_pos_map.getX());
	area.moveTop(area.top() + mouse_pos_map.getY() - click_pos_map.getY());
	map_printer->setPrintArea(area);
	click_pos_map = mouse_pos_map;
}
