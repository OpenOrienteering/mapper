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

#include "../map_widget.h"
#include "print_widget.h"


PrintTool::PrintTool(MapEditorController* editor, PrintWidget* print_widget)
: MapEditorTool(editor, Other, NULL),
  print_widget(print_widget),
  dragging(false)
{
	Q_ASSERT(editor != NULL);
	Q_ASSERT(print_widget != NULL);
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
	
	QRectF effective_print_area = print_widget->getEffectivePrintArea();
	QRect page_area = widget->mapToViewport(effective_print_area).toRect();
	
	// Strongly darken the region outside the page area for printers and export
	painter->setBrush(QColor(0, 0, 0, 160));
	painter->setPen(Qt::NoPen);
	QPainterPath outside_path;
	outside_path.addRect(view_area);
	outside_path.addRect(view_area.intersected(page_area));
	painter->drawPath(outside_path);
	
	if (!print_widget->exporterSelected() && !page_area.isEmpty())
	{
		// Determine printable area (page area minus page margins)
		QRect printable_area;
		float margin_top, margin_left, margin_bottom, margin_right;
		print_widget->getMargins(margin_top, margin_left, margin_bottom, margin_right);
		if (effective_print_area.width() > margin_left + margin_right && effective_print_area.height() > margin_top + margin_bottom)
			// FIXME: This does not take into account whether the map is printed in different scale.
			printable_area = widget->mapToViewport(effective_print_area.adjusted(margin_left, margin_top, -margin_right, -margin_bottom)).toRect();
			
		if (printable_area != page_area)
		{
			// Weakly darken the page margins for printers.
			painter->setBrush(QColor(0, 0, 0, 64));
			QPainterPath margin_path;
			margin_path.addRect(page_area);
			margin_path.addRect(page_area.intersected(printable_area));
			painter->drawPath(margin_path);
		}
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
	print_widget->setPrintAreaLeft(print_widget->getPrintAreaLeft() + mouse_pos_map.getX() - click_pos_map.getX());
	print_widget->setPrintAreaTop(print_widget->getPrintAreaTop() + mouse_pos_map.getY() - click_pos_map.getY());
	click_pos_map = mouse_pos_map;
	
	updatePrintArea();
}
