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


#ifndef _OPENORIENTEERING_PIE_MENU_H_
#define _OPENORIENTEERING_PIE_MENU_H_

#include <QWidget>

/** Displays a pie menu. */
class PieMenu : public QWidget
{
Q_OBJECT
public:
	/** Constructs a pie menu with the given initial number of (empty) items
	 *  and given dimensions. */
	PieMenu(QWidget* parent, int action_count, int icon_size);
	
	/** Sets the number of items in the menu (must be at least three). */
	void setSize(int action_count);
	
	/** Replaces the action in place index with the new action.
	 *  Index zero is at the top, the order is counter-clockwise. */
	void setAction(int index, QAction* action);
	
	/** Removes all actions from the menu, but leaves the number of
	 *  menu items unchanged. */
	void clear();
	
	/** Returns whether all actions are set to NULL. */
	bool isEmpty() const;
	
	/** Shows the menu at the given absolute position */
	void popup(const QPoint pos);
	
protected:
	virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void paintEvent(QPaintEvent* event);
	
	QPoint getPoint(float radius, float angle);
	QPolygon itemArea(int index);
	
	void findHoverItem(const QPoint& pos);
	void setHoverItem(int index);
	
	int icon_size;
	int icon_border_inner;
	int icon_border_outer;
	int inner_radius;
	int total_radius;
	
	bool mouse_moved;
	QPoint click_pos;
	int hover_item;
	std::vector< QAction* > actions;
};

#endif
