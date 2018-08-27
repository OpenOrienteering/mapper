/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2014 Thomas Schöps, Kai Pastor
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


#include "pie_menu.h"

#include <QtMath>
#include <QAction>
#include <QApplication>
#include <QDesktopWidget>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOptionMenuItem>

#include "settings.h"
#include "util/backports.h"


namespace OpenOrienteering {

PieMenu::PieMenu(QWidget* parent)
: QWidget(parent, Qt::Popup | Qt::FramelessWindowHint),	// NOTE: use Qt::Window for debugging to avoid mouse grab
   minimum_action_count(3),
   icon_size(24),
   active_action(nullptr),
   actions_changed(true),
   clicked(false)
{
	setCursor(QCursor(Qt::ArrowCursor));
	setAttribute(Qt::WA_OpaquePaintEvent);
	setAttribute(Qt::WA_ShowWithoutActivating);
	setAutoFillBackground(false);
	setMouseTracking(true);
	
	auto scale = Settings::getInstance().getSetting(Settings::General_PixelsPerInch).toReal() / 96.0;
	if (scale > 1.5)
	{
		icon_size = qRound(icon_size * scale);
	}
	
	updateCachedState();
}

int PieMenu::minimumActionCount() const
{
	return minimum_action_count;
}

void PieMenu::setMinimumActionCount(int count)
{
	if (count >= 3 && count != minimum_action_count)
	{
		minimum_action_count = count;
		actions_changed = true;
		update();
	}
}

int PieMenu::visibleActionCount() const
{
	const auto actions = this->actions();
	return int(std::count_if(actions.begin(), actions.end(), [](const auto* action) {
		return action->isVisible() && !action->isSeparator();
	}));
}

bool PieMenu::isEmpty() const
{
	return visibleActionCount() == 0;
}

void PieMenu::clear()
{
	const auto actions = this->actions();
	for (auto action : actions)
		removeAction(action);
}

int PieMenu::iconSize() const
{
	return icon_size;
}

void PieMenu::setIconSize(int icon_size)
{
	if (icon_size > 0)
	{
		this->icon_size = icon_size;
		actions_changed = true;
		update();
	}
}

QAction* PieMenu::actionAt(const QPoint& pos) const
{
	const auto actions = this->actions();
	for (auto action : actions)
	{
		if (geometries.contains(action) &&
		    geometries[action].area.containsPoint(pos, Qt::WindingFill))
		{
			return action;
		}
	}
	return nullptr;
}

PieMenu::ItemGeometry PieMenu::actionGeometry(QAction* action) const
{
	ItemGeometry geometry;
	if (geometries.contains(action))
		geometry = geometries[action];
	
	return geometry;
}

QAction* PieMenu::activeAction() const
{
	return active_action;
}

void PieMenu::setActiveAction(QAction* action)
{
	QAction* const prev_action = active_action;
	active_action = (action && action->isEnabled() && action->isVisible() && !action->isSeparator()) ? action : nullptr;
	if (isVisible())
	{
		if (active_action && active_action != prev_action)
		{
			active_action->activate(QAction::Hover);
			emit hovered(active_action);
			active_action->showStatusText(0);
		}
		else if (prev_action && !active_action)
		{
			QString empty_string;
			QStatusTipEvent e(empty_string);
			QApplication::sendEvent(parent(), &e);
		}
		update();
	}
}

void PieMenu::popup(const QPoint& pos)
{
	updateCachedState(); // We need the current total_radius.
	
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
	
	clicked = false;
	active_action = nullptr;
	
	emit aboutToShow();
	show();
}

void PieMenu::actionEvent(QActionEvent* event)
{
	if (event->type() == QEvent::ActionRemoved)
	{
		QAction* const action = event->action();
		geometries.remove(action);
		if (action == active_action)
			setActiveAction(nullptr);
	}
	
	actions_changed = true;
	update();
	QWidget::actionEvent(event);
}

void PieMenu::hideEvent(QHideEvent* event)
{
	if (!event->spontaneous())
	{
		emit aboutToHide();
		setActiveAction(nullptr);
		QString empty_string;
		QStatusTipEvent e(empty_string);
		QApplication::sendEvent(parent(), &e);
	}
	QWidget::hideEvent(event);
}

void PieMenu::mouseMoveEvent(QMouseEvent* event)
{
	setActiveAction(actionAt(event->pos()));
	event->accept();
}

void PieMenu::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton)
	{
		if (mask().contains(event->pos()))
		{
			clicked = true;
			setActiveAction(actionAt(event->pos()));
		}
		else
		{
			hide();
		}
		event->accept();
		return;
	}
	
	QWidget::mousePressEvent(event);
}

void PieMenu::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton)
	{
		if (active_action)
		{
			Q_ASSERT(active_action->isEnabled());
			Q_ASSERT(active_action->isVisible());
			Q_ASSERT(!active_action->isSeparator());
			active_action->activate(QAction::Trigger);
			emit triggered(active_action);
			hide();
		}
		else if (clicked && !mask().contains(event->pos()))
		{
			hide();
		}
		
		clicked = false;
		event->accept();
		return;
	}
	
	QWidget::mouseReleaseEvent(event);
}

void PieMenu::paintEvent(QPaintEvent* event)
{
	updateCachedState();
	
	QStyleOptionMenuItem option;
	option.initFrom(this);
	QPalette& palette = option.palette;
	palette.setCurrentColorGroup(QPalette::Active);
	
	// Draw on the widget
	QPainter painter(this);
	painter.setClipRect(event->rect());
	painter.setRenderHint(QPainter::Antialiasing, true);
	
	// Background
	QPen pen(palette.color(QPalette::Dark));
	pen.setWidth(1);
	painter.setPen(pen);
	painter.setBrush(palette.brush(QPalette::Button));
	painter.drawConvexPolygon(outer_border);
	painter.setBrush(Qt::NoBrush);
	painter.drawConvexPolygon(inner_border);
	
	// Items
	const auto actions = this->actions();
	for (auto action : actions)
	{
		if (!geometries.contains(action))
			continue;
		
		QPolygon area = geometries[action].area;
		QIcon::Mode mode = QIcon::Normal;
		if (action->isChecked())
		{
			mode = QIcon::Selected;
			QPen pen(palette.color(QPalette::Dark));
			pen.setWidth(1);
			painter.setPen(pen);
			painter.setBrush(palette.color(QPalette::Button).darker(120));
			painter.setOpacity(1.0);
			painter.drawConvexPolygon(area);
		}
		else if (action->isEnabled() && action == active_action)
		{
			mode = QIcon::Active;
			QPen pen(palette.color(QPalette::Dark));
			pen.setWidth(1);
			painter.setPen(pen);
			painter.setBrush(clicked ? palette.color(QPalette::Button).darker(120) : palette.color(QPalette::Button).lighter(120));
			painter.setOpacity(1.0);
			painter.drawConvexPolygon(area);
		}
		else if (!action->isEnabled())
		{
			mode = QIcon::Disabled;
			painter.setOpacity(0.4);
		}
		else
		{
			painter.setOpacity(1.0);
		}
		
		// Draw icon
		QPixmap pixmap = action->icon().pixmap(icon_size, mode);
		QPoint midpoint = geometries[action].icon_pos;
		painter.drawPixmap(QPoint(midpoint.x() - pixmap.width() / 2, midpoint.y() - pixmap.height() / 2), pixmap);
	}
}

void PieMenu::updateCachedState()
{
	if (actions_changed)
	{
		int action_count = qMax(minimum_action_count, visibleActionCount());
		outer_border.resize(action_count);
		inner_border.resize(action_count);
		
		double half_angle = M_PI / action_count;
		double inner_radius = 0.5 * icon_size;
		double inner_corner_radius = inner_radius / cos(half_angle);
		
		double icon_padding_inner = 0.7 * icon_size;
		double icon_radius = inner_radius + icon_padding_inner + 0.5 * icon_size;
		
		double icon_padding_outer = 0.2 * icon_size;
		double outer_corner_radius = (inner_radius + icon_padding_inner + icon_size + icon_padding_outer) / cos(half_angle);
		
		total_radius = qCeil(outer_corner_radius);
		
		// The total shape
		for (int i = 0; i < action_count; ++i)
		{
			double angle = (2*i + 1) * half_angle;
			inner_border.setPoint(i, getPoint(inner_corner_radius, angle));
			outer_border.setPoint(i, getPoint(outer_corner_radius, angle));
		}
		
		setMask(outer_border.subtracted(inner_border));
		
		// The items
		int i = 0;
		const auto actions = this->actions();
		for (auto action : actions)
		{
			if (action->isVisible() && !action->isSeparator())
			{
				ItemGeometry geometry;
				
				geometry.area.resize(4);
				double angle = (2*i - 1) * half_angle;
				geometry.area.setPoint(0, getPoint(inner_corner_radius, angle));
				geometry.area.setPoint(1, inner_border.point(i));
				geometry.area.setPoint(2, outer_border.point(i));
				geometry.area.setPoint(3, getPoint(outer_corner_radius, angle));
				
				angle = 2*i * half_angle;
				geometry.icon_pos = getPoint(icon_radius, angle);
				
				geometries[action] = geometry;
				++i;
			}
		}
		
		actions_changed = false;
	}
}


}  // namespace OpenOrienteering
