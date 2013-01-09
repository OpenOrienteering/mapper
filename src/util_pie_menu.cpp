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


#include "util_pie_menu.h"

#include <cassert>

#include <qmath.h>
#include <QAction>
#include <QApplication>
#include <QDesktopWidget>
#include <QMouseEvent>
#include <QPainter>

#include "tool.h"

PieMenu::PieMenu(QWidget* parent, int action_count, int icon_size)
: QWidget(parent, Qt::Popup | Qt::FramelessWindowHint),	// NOTE: use Qt::Window for debugging to avoid mouse grab
   icon_size(icon_size),
   hover_item(-1)
{
	icon_border_outer = qRound(0.2 * icon_size);
	icon_border_inner = qRound(0.7 * icon_size);
	inner_radius = qRound(0.5 * icon_size);
	setSize(action_count);
	
	setCursor(QCursor(Qt::ArrowCursor));
	setAttribute(Qt::WA_OpaquePaintEvent);
	setAttribute(Qt::WA_ShowWithoutActivating);
	setAutoFillBackground(false);
	setMouseTracking(true);
}

void PieMenu::setSize(int action_count)
{
	assert(action_count >= 3);
	actions.resize(action_count, NULL);
	
	float half_angle = 2*M_PI / (2 * action_count);
	QPolygon inner_mask(action_count);
	float inner_corner_radius = inner_radius / cos(half_angle);
	QPolygon outer_mask(action_count);
	float outer_corner_radius = (inner_radius + icon_border_inner + icon_size + icon_border_outer) / cos(half_angle);
	total_radius = qCeil(outer_corner_radius);
	
	for (int i = 0; i < action_count; ++i)
	{
		inner_mask.setPoint(i, getPoint(inner_corner_radius, (1 + 2*i) * half_angle));
		outer_mask.setPoint(i, getPoint(outer_corner_radius, (1 + 2*i) * half_angle));
	}
	
	outer_mask = outer_mask.subtracted(inner_mask);
	setMask(outer_mask);
}

QPoint PieMenu::getPoint(float radius, float angle)
{
	return QPoint(qRound(total_radius + radius * -sin(angle)), qRound(total_radius + radius * -cos(angle)));
}
QPolygon PieMenu::itemArea(int index)
{
	float half_angle = 2*M_PI / (2 * actions.size());
	float inner_corner_radius = inner_radius / cos(half_angle);
	float outer_corner_radius = (inner_radius + icon_border_inner + icon_size + icon_border_outer) / cos(half_angle);
	
	QPolygon result(4);
	result.setPoint(0, getPoint(inner_corner_radius, (2*index - 1) * half_angle));
	result.setPoint(1, getPoint(inner_corner_radius, (2*index + 1) * half_angle));
	result.setPoint(2, getPoint(outer_corner_radius, (2*index + 1) * half_angle));
	result.setPoint(3, getPoint(outer_corner_radius, (2*index - 1) * half_angle));
	return result;
}

void PieMenu::setAction(int index, QAction* action)
{
	actions[index] = action;
	
	if (isVisible())
		update();
}

void PieMenu::clear()
{
	for (size_t i = 0, end = actions.size(); i < end; ++i)
		actions[i] = NULL;
	
	if (isVisible())
		update();
}

bool PieMenu::isEmpty() const
{
	for (size_t i = 0, end = actions.size(); i < end; ++i)
	{
		if (actions[i] != NULL)
			return false;
	}
	return true;
}

void PieMenu::popup(const QPoint pos)
{
	QPoint cursor_pos = QCursor::pos();
	QRect screen_rect = qApp->desktop()->availableGeometry(cursor_pos);
	
	if (cursor_pos.x() > screen_rect.right() - total_radius)
		cursor_pos.setX(screen_rect.right() - total_radius);
	else if (cursor_pos.x() < total_radius)
		cursor_pos.setX(total_radius);
	
	if (cursor_pos.y() > screen_rect.bottom() - total_radius)
		cursor_pos.setY(screen_rect.bottom() - total_radius);
	else if (cursor_pos.y() < total_radius)
		cursor_pos.setY(total_radius);
	
	setGeometry(pos.x() - total_radius, pos.y() - total_radius, 2 * total_radius, 2 * total_radius);
	show();
	
	mouse_moved = false;
	click_pos = pos;
	hover_item = -1;
}

void PieMenu::mousePressEvent(QMouseEvent* event)
{
	if (event->button() != Qt::LeftButton && event->button() != Qt::RightButton)
	{
		event->ignore();
		return;
	}
	
	findHoverItem(event->pos());
	if (hover_item >= 0 && actions[hover_item] != NULL && actions[hover_item]->isEnabled())
		actions[hover_item]->activate(QAction::Trigger);
	
    event->accept();
	hide();
}

void PieMenu::mouseMoveEvent(QMouseEvent* event)
{
	findHoverItem(event->pos());
	
	if (!mouse_moved && (QCursor::pos() - click_pos).manhattanLength() >= QApplication::startDragDistance())
		mouse_moved = true;
}

void PieMenu::mouseReleaseEvent(QMouseEvent* event)
{
	event->accept();
	if (!mouse_moved)
		return;
	
	mousePressEvent(event);
}

void PieMenu::paintEvent(QPaintEvent* event)
{
	// Draw on the widget
	QPainter painter;
	painter.begin(this);
	painter.setClipRect(event->rect());
	
	// Background color
	painter.fillRect(rect(), QColor(qRgb(220, 220, 220)));
	
	// Items
	float half_angle = 2*(M_PI) / (float)(2 * actions.size());
	float icon_radius = inner_radius + icon_border_inner + 0.5f * icon_size;
	painter.setPen(Qt::NoPen);
	for (int i = 0, end = (int)actions.size(); i < end; ++i)
	{
		bool highlight = actions[i] != NULL && actions[i]->isEnabled() && i == hover_item;
		
		if (actions[i] == NULL || !actions[i]->isEnabled() || actions[i]->isChecked() || highlight)
		{
			// Draw item background
			if (highlight)
				painter.setBrush(QBrush(MapEditorTool::active_color));
			else if (actions[i] != NULL && actions[i]->isChecked())
				painter.setBrush(QBrush(qRgb(240, 240, 240)));
			else
				painter.setBrush(Qt::darkGray);
			
			QPolygon area = itemArea(i);
			painter.drawConvexPolygon(area);
		}
		if (actions[i] == NULL)
			continue;
		
		// Draw icon
		QPixmap pixmap = actions[i]->icon().pixmap(icon_size, actions[i]->isEnabled() ? (highlight ? QIcon::Active : QIcon::Normal) : QIcon::Disabled);
		QPoint midpoint = QPoint(total_radius + icon_radius * -sin(2*i * half_angle), total_radius + icon_radius * -cos(2*i * half_angle));
		painter.drawPixmap(QPointF(midpoint.x() - 0.5f * pixmap.width(), midpoint.y() - 0.5f * pixmap.height()), pixmap);
	}
	
	painter.end();
}

void PieMenu::findHoverItem(const QPoint& pos)
{
	for (size_t i = 0, end = actions.size(); i < end; ++i)
	{
		QPolygon area = itemArea(i);
		if (area.containsPoint(pos, Qt::WindingFill))
		{
			setHoverItem(i);
			return;
		}
	}
	setHoverItem(-1);
}

void PieMenu::setHoverItem(int index)
{
	if (index != hover_item)
	{
		hover_item = index;
		if (hover_item >= 0 && actions[hover_item] != NULL && actions[hover_item]->isEnabled())
			actions[hover_item]->activate(QAction::Hover);
		update();
	}
}
