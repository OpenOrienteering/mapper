/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2016  Kai Pastor
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


#ifdef QT_PRINTSUPPORT_LIB

#include "print_tool.h"

#include <QMouseEvent>
#include <QPainter>

#include "core/map.h"
#include "core/map_printer.h"
#include "gui/map/map_editor.h"
#include "gui/map/map_widget.h"


namespace OpenOrienteering {

PrintTool::PrintTool(MapEditorController* editor, MapPrinter* map_printer)
: MapEditorTool { editor, Other, nullptr }
, map_printer   { map_printer }
, region        { Unknown }
, dragging      { false }
{
	Q_ASSERT(editor);
	Q_ASSERT(map_printer);
	
	connect(map_printer, &MapPrinter::printAreaChanged, this, &PrintTool::updatePrintArea);
	connect(map_printer, &MapPrinter::pageFormatChanged, this, &PrintTool::updatePrintArea);
	// Page breaks may change upon scale changes.
	connect(map_printer, &MapPrinter::optionsChanged, this, &PrintTool::updatePrintArea);
}

PrintTool::~PrintTool()
{
	// nothing, not inlined
}

void PrintTool::init()
{
	setStatusBarText(tr("<b>Drag</b>: Move the map, the print area or the area's borders. "));
	updatePrintArea();
	
	MapEditorTool::init();
}

const QCursor& PrintTool::getCursor() const
{
	static auto const cursor = QCursor{ Qt::ArrowCursor };
	return cursor;
}

bool PrintTool::mousePressEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget)
{
	if (event->button() == Qt::LeftButton)
	{
		mouseMoved(map_coord, widget);
		dragging = true;
		click_pos = event->pos();
		click_pos_map = map_coord;
		if (region == Inside || region == Outside)
			widget->setCursor(Qt::ClosedHandCursor);
		return true;
	}
	
	if (event->button() == Qt::RightButton)
	{
		return true; // disable context menu
	}
	
	return false;
}

bool PrintTool::mouseMoveEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget)
{
	if (dragging && event->buttons() & Qt::LeftButton)
	{
		if (region == Outside)
		{
			mapWidget()->getMapView()->setPanOffset(event->pos() - click_pos);
		}
		else
		{
			updateDragging(map_coord);
		}
		return true;
	}
	
	mouseMoved(map_coord, widget);
	return false;
}

bool PrintTool::mouseReleaseEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget)
{
	if (dragging && event->button() == Qt::LeftButton)
	{
		if (region == Outside)
		{
			mapWidget()->getMapView()->finishPanning(event->pos() - click_pos);
		}
		else
		{
			updateDragging(map_coord);
		}
		dragging = false;
		region = Unknown; // forces mouseMoved() to update cursor and status
		mouseMoved(map_coord, widget);
		return true;
	}
	
	return false;
}

void PrintTool::draw(QPainter* painter, MapWidget* widget)
{
	painter->save();
	
	QRect view_area = QRect(0, 0, widget->width(), widget->height());
	QRect print_area = widget->mapToViewport(map_printer->getPrintArea()).toRect();
	qreal scale_adjustment = map_printer->getScaleAdjustment();
	QSizeF page_size = widget->mapToViewport(map_printer->getPageFormat().page_rect).size() / scale_adjustment;
	
	// Strongly darken the region outside the print area
	painter->setBrush(QColor(0, 0, 0, 160));
	painter->setPen(Qt::NoPen);
	QPainterPath outside_path;
	outside_path.addRect(view_area);
	outside_path.addRect(view_area.intersected(print_area));
	painter->drawPath(outside_path);
	
	Q_ASSERT(!map_printer->horizontalPagePositions().empty());
	Q_ASSERT(!map_printer->verticalPagePositions().empty());
	QPointF outer_top_left = widget->mapToViewport( QPointF(
	  map_printer->horizontalPagePositions().front(),
	  map_printer->verticalPagePositions().front() ) );
	QPointF outer_bottom_right = widget->mapToViewport( QPointF(
	  map_printer->horizontalPagePositions().back(),
	  map_printer->verticalPagePositions().back() ) );
	outer_bottom_right += QPointF(page_size.width(), page_size.height());
	QRectF outer_rect(outer_top_left, outer_bottom_right);
	
	// Draw red lines for page breaks
	QColor top_left_margin_color(255, 0, 0, 160);
	QColor bottom_right_margin_color(255, 128, 128, 160);
	painter->setBrush(Qt::NoBrush);

	// The relative length of the page dimensions to be actual drawn.
	QSizeF drawing_size(page_size * scale_adjustment);
	if (map_printer->horizontalPagePositions().size() > 1)
	{
		drawing_size.setWidth(drawing_size.width() * 0.9 * (
		  map_printer->horizontalPagePositions()[1] - 
		  map_printer->horizontalPagePositions()[0] ) / 
		  map_printer->getPageFormat().page_rect.width() );
	}
	if (map_printer->verticalPagePositions().size() > 1)
	{
		drawing_size.setHeight(drawing_size.height() * 0.9 * (
		  map_printer->verticalPagePositions()[1] - 
		  map_printer->verticalPagePositions()[0] ) /
		  map_printer->getPageFormat().page_rect.height() );
	}
	
	int h = 0;
	for (auto hpos : map_printer->horizontalPagePositions())
	{
		++h;
		if (h > 100) // Don't visualize too many pages.
			break;
		
		int v = 0;
		for (auto vpos : map_printer->verticalPagePositions())
		{
			++v;
			if (h+v > 100) // Don't visualize too many pages.
				break;
			
			QPointF pos = widget->mapToViewport(MapCoordF(hpos, vpos));
			painter->setPen(top_left_margin_color);
			// Left vertical line
			painter->drawLine(QLineF{pos.x(), pos.y(), pos.x(), pos.y()+drawing_size.height()});
			// Top horizontal line
			painter->drawLine(QLineF{pos.x(), pos.y(), pos.x()+drawing_size.width(), pos.y()});
			
			pos += QPointF(page_size.width(), page_size.height());
			painter->setPen(bottom_right_margin_color);
			// Right vertical line
			painter->drawLine(QLineF{pos.x(), pos.y()-drawing_size.height(), pos.x(), pos.y()});
			// Bottom horizontal line
			painter->drawLine(QLineF{pos.x()-drawing_size.width(), pos.y(), pos.x(), pos.y()});
		}
	}
	
	painter->setPen(top_left_margin_color);
	painter->drawLine(QLineF{outer_rect.left(), outer_rect.top(), outer_rect.left(), outer_rect.bottom()});
	painter->drawLine(QLineF{outer_rect.left(), outer_rect.top(), outer_rect.right(), outer_rect.top()});
	painter->setPen(bottom_right_margin_color);
	painter->drawLine(QLineF{outer_rect.right(), outer_rect.top(), outer_rect.right(), outer_rect.bottom()});
	painter->drawLine(QLineF{outer_rect.left(), outer_rect.bottom(), outer_rect.right(), outer_rect.bottom()});
	
	QRectF print_area_f(print_area);
	QPen marker(Qt::red);
	marker.setWidth(4);
	painter->setPen(marker);
	painter->setOpacity(0.5);
	if (region == Inside)
	{
		painter->drawRect(print_area_f);
	}
	if (region & LeftBorder)
	{
		painter->drawLine(print_area_f.topLeft(), print_area_f.bottomLeft());
	}
	if (region & TopBorder)
	{
		painter->drawLine(print_area_f.topLeft(), print_area_f.topRight());
	}
	if (region & RightBorder)
	{
		painter->drawLine(print_area_f.topRight(), print_area_f.bottomRight());
	}
	if (region & BottomBorder)
	{
		painter->drawLine(print_area_f.bottomLeft(), print_area_f.bottomRight());
	}
	
	painter->restore();
}

void PrintTool::updatePrintArea()
{
	// The print area visualization is updated by redrawing the whole map.
	// TODO: Replace with a more explicit way of marking the whole map area as dirty.
	editor->getMap()->setDrawingBoundingBox(QRectF(-1000000, -1000000, 2000000, 2000000), 0);
}

void PrintTool::updateDragging(const MapCoordF& mouse_pos_map)
{
	QPointF delta = QPointF(mouse_pos_map - click_pos_map);
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
			Q_ASSERT(false); // Handled outside.
		case Unknown:
			; // Nothing
	}
	
	if (area.left() < area.right() && area.top() < area.bottom())
	{
		map_printer->setPrintArea(area);
		click_pos_map = mouse_pos_map;
	}
}

void PrintTool::mouseMoved(const MapCoordF& mouse_pos_map, MapWidget* widget)
{
	Q_ASSERT(!dragging); // No change while dragging!
	
	static const qreal margin_width = 16.0;
	static const qreal outer_margin = 8.0;
	
	QRectF print_area = widget->mapToViewport(map_printer->getPrintArea());
	print_area.adjust(-outer_margin, -outer_margin, outer_margin, outer_margin);
	QPointF mouse_pos = widget->mapToViewport(mouse_pos_map);
	
	int new_region = Outside;
	if (print_area.contains(mouse_pos))
	{
		new_region = Inside;
		
		if (mouse_pos.rx() < print_area.left() + margin_width)
		{
			new_region |= LeftBorder;
		}
		else if (mouse_pos.rx() > print_area.right() - margin_width)
		{
			new_region |=  RightBorder;
		}
		
		if (mouse_pos.ry() < print_area.top() + margin_width)
		{
			new_region |= TopBorder;
		}
		else if (mouse_pos.ry() > print_area.bottom() - margin_width)
		{
			new_region |= BottomBorder;
		}
	}
	
	if (new_region != region)
	{
		region = InteractionRegion(new_region);
		
		switch (region)
		{
			case Inside:
				setStatusBarText(tr("<b>Drag</b>: Move the print area. "));
				widget->setCursor(Qt::OpenHandCursor);
				break;
			case Outside:
				setStatusBarText(tr("<b>Drag</b>: Move the map. "));
				widget->setCursor(Qt::ArrowCursor);
				break;
			case LeftBorder:
			case RightBorder:
				setStatusBarText(tr("<b>Drag</b>: Move the print area's border. "));
				widget->setCursor(Qt::SizeHorCursor);
				break;
			case TopBorder:
			case BottomBorder:
				setStatusBarText(tr("<b>Drag</b>: Move the print area's border. "));
				widget->setCursor(Qt::SizeVerCursor);
				break;
			case TopLeftCorner:
			case BottomRightCorner:
				setStatusBarText(tr("<b>Drag</b>: Move the print area's borders. "));
				widget->setCursor(Qt::SizeFDiagCursor);
				break;
			case TopRightCorner:
			case BottomLeftCorner:
				setStatusBarText(tr("<b>Drag</b>: Move the print area's borders. "));
				widget->setCursor(Qt::SizeBDiagCursor);
				break;
			case Unknown:
				setStatusBarText(tr("<b>Drag</b>: Move the map, the print area or the area's borders. "));
				widget->setCursor(Qt::ArrowCursor);
		}
		
		updatePrintArea();
	}
}


}  // namespace OpenOrienteering

#endif  // QT_PRINTSUPPORT_LIB
