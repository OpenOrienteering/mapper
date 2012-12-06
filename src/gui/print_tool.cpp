/*
 *    Copyright 2012 Thomas Schöps
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

// #include <limits>

#if QT_VERSION < 0x050000
#include <QtGui>
#else
#include <QtWidgets>
#endif

#include "../map_widget.h"
#include "print_widget.h"


PrintTool::PrintTool(MapEditorController* editor, PrintWidget* print_widget)
: MapEditorTool(editor, Other, NULL),
  print_widget(print_widget),
  dragging(false)
{
}

void PrintTool::init()
{
	setStatusBarText(tr("<b>Drag</b> to move the print area"));
	updatePrintArea();
}

QCursor* PrintTool::getCursor()
{
	static QCursor cursor(Qt::SizeAllCursor);
	return &cursor;
}

bool PrintTool::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (event->button() != Qt::LeftButton)
		return false;
	
	dragging = true;
	click_pos_map = map_coord;
	return true;
}

bool PrintTool::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (!(event->buttons() & Qt::LeftButton) || !dragging)
		return false;
	
	updateDragging(map_coord);
	return true;
}

bool PrintTool::mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (event->button() != Qt::LeftButton)
		return false;
	
	updateDragging(map_coord);
	dragging = false;
	return true;
}

void PrintTool::draw(QPainter* painter, MapWidget* widget)
{
	QRectF effective_print_area = print_widget->getEffectivePrintArea();
	QRect outer_rect = widget->mapToViewport(effective_print_area).toRect();
	QRect view_rect = QRect(0, 0, widget->width(), widget->height());
	
	painter->setPen(Qt::NoPen);
	painter->setBrush(QColor(0, 0, 0, 160));
	drawBetweenRects(painter, view_rect, outer_rect);
	
	if (!print_widget->exporterSelected())
	{
		float margin_top, margin_left, margin_bottom, margin_right;
		print_widget->getMargins(margin_top, margin_left, margin_bottom, margin_right);
		if (margin_top > 0 || margin_left > 0 || margin_bottom > 0 || margin_right > 0)
		{
			QRect inner_rect = widget->mapToViewport(effective_print_area.adjusted(margin_left, margin_top, -margin_right, -margin_bottom)).toRect();
			
			painter->setBrush(QColor(0, 0, 0, 64));
			if (effective_print_area.width() < margin_left + margin_right || effective_print_area.height() < margin_top + margin_bottom)
				painter->drawRect(outer_rect);
			else
				drawBetweenRects(painter, outer_rect, inner_rect);
		}
	}
	
	/*painter->setBrush(QColor(255, 255, 0, 128));
	painter->drawRect(QRect(outer_rect.topLeft(), outer_rect.bottomRight() - QPoint(1, 1)));
	painter->setBrush(QColor(0, 255, 0, 128));
	painter->drawRect(QRect(inner_rect.topLeft(), inner_rect.bottomRight() - QPoint(1, 1)));*/
	
	/*QRect rect = widget->mapToViewport(this->widget->getEffectivePrintArea()).toRect();

	painter->setPen(active_color);
	painter->drawRect(QRect(rect.topLeft(), rect.bottomRight() - QPoint(1, 1)));
	painter->setPen(qRgb(255, 255, 255));
	painter->drawRect(QRect(rect.topLeft() + QPoint(1, 1), rect.bottomRight() - QPoint(2, 2)));*/
}

void PrintTool::drawBetweenRects(QPainter* painter, const QRect &outer, const QRect &inner) const
{
	if (outer.isEmpty())
		return;
	
	QRect clipped_inner = outer.intersected(inner);
	if (clipped_inner.isEmpty())
	{
		painter->drawRect(outer);
	}
	else
	{
		if (outer.left() < clipped_inner.left())
			painter->drawRect(QRect(outer.left(), outer.top(), clipped_inner.left() - outer.left(), outer.height()));
		if (outer.right() > clipped_inner.right())
			painter->drawRect(QRect(clipped_inner.left() + clipped_inner.width(), outer.top(), outer.right() - clipped_inner.right(), outer.height()));
		if (outer.top() < clipped_inner.top())
			painter->drawRect(QRect(clipped_inner.left(), outer.top(), clipped_inner.width(), clipped_inner.top() - outer.top()));
		if (outer.bottom() > clipped_inner.bottom())
			painter->drawRect(QRect(clipped_inner.left(), clipped_inner.bottom(), clipped_inner.width(), outer.top() + outer.height() - clipped_inner.bottom()));
	}
}

void PrintTool::updatePrintArea()
{
	editor->getMap()->setDrawingBoundingBox(QRectF(-1000000, -1000000, 2000000, 2000000), 0);
	
	//editor->getMap()->setDrawingBoundingBox(widget->getEffectivePrintArea(), 1);
}

void PrintTool::updateDragging(MapCoordF mouse_pos_map)
{
	print_widget->setPrintAreaLeft(print_widget->getPrintAreaLeft() + mouse_pos_map.getX() - click_pos_map.getX());
	print_widget->setPrintAreaTop(print_widget->getPrintAreaTop() + mouse_pos_map.getY() - click_pos_map.getY());
	click_pos_map = mouse_pos_map;
	
	updatePrintArea();
}
