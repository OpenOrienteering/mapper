/*
 *    Copyright 2013 Thomas Schöps
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


#include "touch_cursor.h"

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>

#include "map_widget.h"


// TODO: convert these to settings
const float touch_pos_offset_mm = 10;
const float control_ring_radius_mm = 5;

TouchCursor::TouchCursor(MapWidget* map_widget)
: visible(false)
, left_button_pressed(false)
, last_pressed_button(NoButton)
, map_widget(map_widget)
{
	// nothing
}

void TouchCursor::mousePressEvent(QMouseEvent* event)
{
	if (event->button() != Qt::LeftButton)
		return;
	last_touch_pos = event->pos();
	
	ControlID control_id = NoButton;
	if (!visible || !touchedControl(event->pos(), &control_id))
	{
		// Jump tp position
		QPoint cursor_pos = event->pos() - QPoint(touchPosOffsetPx(), 0);
		last_cursor_pos = cursor_pos;
		cursor_coord = map_widget->viewportToMapF(cursor_pos);
		visible = true;
		
		// TODO: limit redraw area
		map_widget->update();
		
		*event = QMouseEvent(
			QEvent::MouseMove, cursor_pos,
			Qt::NoButton, event->buttons() & ~Qt::LeftButton, event->modifiers());
		last_pressed_button = NoButton;
	}
	else if (control_id == LeftButton)
	{
		*event = QMouseEvent(
			QEvent::MouseButtonPress, map_widget->mapToViewport(cursor_coord),
			event->button(), event->buttons(), event->modifiers());
		left_button_pressed = true;
		last_pressed_button = LeftButton;
	}
}

void TouchCursor::mouseMoveEvent(QMouseEvent* event)
{
	if (!(event->buttons() & Qt::LeftButton))
		return;
	
	QPointF cursor_pos;
	if (last_pressed_button == LeftButton)
		cursor_pos = last_cursor_pos + (event->pos() - last_touch_pos);
	else
		cursor_pos = event->pos() - QPoint(touchPosOffsetPx(), 0);
	last_touch_pos = event->pos();
	last_cursor_pos = cursor_pos;
	cursor_coord = map_widget->viewportToMapF(cursor_pos);
		
	*event = QMouseEvent(
		QEvent::MouseMove, cursor_pos,
		left_button_pressed ? event->button() : Qt::NoButton,
		left_button_pressed ? event->buttons() : (event->buttons() & ~Qt::LeftButton),
		event->modifiers());
	
	// TODO: limit redraw area
	map_widget->update();
}

bool TouchCursor::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() != Qt::LeftButton)
		return true;
	
	if (left_button_pressed)
	{
		*event = QMouseEvent(
			QEvent::MouseButtonRelease, map_widget->mapToViewport(cursor_coord),
			event->button(), event->buttons(), event->modifiers());
		left_button_pressed = false;
		return true;
	}
	else
		return false;
}

bool TouchCursor::mouseDoubleClickEvent(QMouseEvent* event)
{
	if (last_pressed_button == LeftButton)
	{
		*event = QMouseEvent(
			QEvent::MouseButtonDblClick, map_widget->mapToViewport(cursor_coord),
			event->button(), event->buttons(), event->modifiers());
		return true;
	}
	else
		return false;
}

void TouchCursor::paint(QPainter* painter)
{
	if (!visible)
		return;
	
	QPointF cursor_pos = map_widget->mapToViewport(cursor_coord);
	
	// Draw cursor
	QPixmap cursor_pixmap = map_widget->cursor().pixmap();
	if (!cursor_pixmap.isNull())
		painter->drawPixmap(cursor_pos - map_widget->cursor().hotSpot(), cursor_pixmap);
	else
	{
		// TODO: better standard "cursor"?
		painter->setPen(Qt::gray);
		painter->setBrush(Qt::NoBrush);
		painter->drawLine(cursor_pos - QPointF(2, 0), cursor_pos + QPointF(2, 0));
		painter->drawLine(cursor_pos - QPointF(0, 2), cursor_pos + QPointF(0, 2));
	}
	
	// Draw move handle / left button
	painter->setPen(Qt::gray);
	painter->setBrush(Qt::NoBrush);
	painter->drawEllipse(cursor_pos + QPointF(touchPosOffsetPx(), 0), controlRingRadiusPx(), controlRingRadiusPx());
}

bool TouchCursor::touchedControl(QPoint pos, TouchCursor::ControlID* out_id)
{
	QPointF cursor_pos = map_widget->mapToViewport(cursor_coord);
	QPointF control_ring_center = cursor_pos + QPointF(touchPosOffsetPx(), 0);
	
	QPointF dist_to_center = pos - control_ring_center;
	if (dist_to_center.x()*dist_to_center.x() + dist_to_center.y()*dist_to_center.y() < controlRingRadiusPx()*controlRingRadiusPx())
	{
		*out_id = LeftButton;
		return true;
	}
	
	return false;
}

float TouchCursor::touchPosOffsetPx() const
{
	float pixel_to_millimeters = 25.4f / (QApplication::primaryScreen()->logicalDotsPerInch());
	return touch_pos_offset_mm / pixel_to_millimeters;
}

float TouchCursor::controlRingRadiusPx() const
{
	float pixel_to_millimeters = 25.4f / (QApplication::primaryScreen()->logicalDotsPerInch());
	return control_ring_radius_mm / pixel_to_millimeters;
}
