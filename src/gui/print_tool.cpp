/*
 *    Copyright 2012, 2013 Thomas Schöps, Kai Pastor
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
  region(Unknown),
  dragging(false)
{
	Q_ASSERT(editor != NULL);
	Q_ASSERT(map_printer != NULL);
	
	connect(map_printer, SIGNAL(printAreaChanged(QRectF)), this, SLOT(updatePrintArea()));
	connect(map_printer, SIGNAL(pageFormatChanged(MapPrinterPageFormat)), this, SLOT(updatePrintArea()));
	// Page breaks may change upon scale changes.
	connect(map_printer, SIGNAL(optionsChanged(MapPrinterOptions)), this, SLOT(updatePrintArea()));
}

void PrintTool::init()
{
	setStatusBarText(tr("<b>Drag</b> to move the print area or its borders."));
	updatePrintArea();
}

QCursor* PrintTool::getCursor()
{
	static QCursor cursor(Qt::ArrowCursor);
	return &cursor;
}

bool PrintTool::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (region == Unknown)
		mouseMoved(map_coord, widget);
	
	if (event->button() == Qt::LeftButton && region != Outside)
	{
		dragging = true;
		click_pos_map = map_coord;
		if (region == Inside)
			widget->setCursor(Qt::ClosedHandCursor);
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
	
	mouseMoved(map_coord, widget);
	return false;
}

bool PrintTool::mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (event->button() == Qt::LeftButton && dragging)
	{
		updateDragging(map_coord);
		dragging = false;
		mouseMoved(map_coord, widget);
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
	QPointF delta = (mouse_pos_map - click_pos_map).toQPointF();
	QRectF area = map_printer->getPrintArea();
	switch (region)
	{
		case Inside:
			area.moveTopLeft(area.topLeft() + delta);
			break;
		case LeftBorder:
			area.setLeft(area.left() + delta.rx());
			break;
		case TopLeftCorner:
			area.setTopLeft(area.topLeft() + delta);
			break;
		case TopBorder:
			area.setTop(area.top() + delta.ry());
			break;
		case TopRightCorner:
			area.setTopRight(area.topRight() + delta);
			break;
		case RightBorder:
			area.setRight(area.right() + delta.rx());
			break;
		case BottomRightCorner:
			area.setBottomRight(area.bottomRight() + delta);
			break;
		case BottomBorder:
			area.setBottom(area.bottom() + delta.ry());
			break;
		case BottomLeftCorner:
			area.setBottomLeft(area.bottomLeft() + delta);
			break;
		case Outside:
		case Unknown:
			; // Nothing
	}
	
	if (area.left() < area.right() && area.top() < area.bottom())
	{
		map_printer->setPrintArea(area);
		click_pos_map = mouse_pos_map;
	}
}

void PrintTool::mouseMoved(MapCoordF mouse_pos_map, MapWidget* widget)
{
	Q_ASSERT(!dragging); // No change while dragging!
	
	static const qreal margin_width = 10.0;
	static const qreal outer_margin = 3.0;
	
	QRectF print_area = widget->mapToViewport(map_printer->getPrintArea());
	print_area.adjust(-outer_margin, -outer_margin, outer_margin, outer_margin);
	QPointF mouse_pos = widget->mapToViewport(mouse_pos_map);
	if (!print_area.contains(mouse_pos))
	{
		region = Outside;
	}
	else
	{
		region = Inside;
		
		if (mouse_pos.rx() < print_area.left() + margin_width)
		{
			region = (InteractionRegion)(region | LeftBorder);
		}
		else if (mouse_pos.rx() > print_area.right() - margin_width)
		{
			region = (InteractionRegion)(region | RightBorder);
		}
		
		if (mouse_pos.ry() < print_area.top() + margin_width)
		{
			region = (InteractionRegion)(region | TopBorder);
		}
		else if (mouse_pos.ry() > print_area.bottom() - margin_width)
		{
			region = (InteractionRegion)(region | BottomBorder);
		}
	}
	
	switch (region)
	{
		case Inside:
			widget->setCursor(Qt::OpenHandCursor);
			break;
		case LeftBorder:
		case RightBorder:
			widget->setCursor(Qt::SizeHorCursor);
			break;
		case TopBorder:
		case BottomBorder:
			widget->setCursor(Qt::SizeVerCursor);
			break;
		case TopLeftCorner:
		case BottomRightCorner:
			widget->setCursor(Qt::SizeFDiagCursor);
			break;
		case TopRightCorner:
		case BottomLeftCorner:
			widget->setCursor(Qt::SizeBDiagCursor);
			break;
		case Outside:
		case Unknown:
			widget->setCursor(Qt::ArrowCursor);
	}
}
