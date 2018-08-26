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


#ifndef OPENORIENTEERING_PIE_MENU_H
#define OPENORIENTEERING_PIE_MENU_H

#include <cmath>

#include <QtGlobal>
#include <QHash>
#include <QObject>
#include <QPoint>
#include <QPolygon>
#include <QWidget>

class QAction;
class QActionEvent;
class QHideEvent;
class QMouseEvent;
class QPaintEvent;

namespace OpenOrienteering {


/** 
 * Displays a pie menu.
 * 
 * This class has an API and behavior similar to QMenu,
 * but is neither a subclass nor a full replacement.
 */
class PieMenu : public QWidget
{
Q_OBJECT
public:
	/** Stores the geometry of a particular item in the PieMenu. */
	struct ItemGeometry
	{
		/** The area covered by the item. */
		QPolygon area;
		
		/** The position of the center of the item. */
		QPoint   icon_pos;
	};
	
	/** Constructs a pie menu with default settings. */
	PieMenu(QWidget* parent = nullptr);
	
	/** Returns the minimum number of actions in the menu. */
	int minimumActionCount() const;
	
	/** Sets the minimum number of actions in the menu (must be at least three). */
	void setMinimumActionCount(int count);
	
	/** Returns the current number of visible actions. */
	int visibleActionCount() const;
	
	/** Removes all actions from the menu.
	 *  Other than QMenu::clear(), this will not immediately delete any action,
	 *  even if owned by this object. (Actions owned by this object will be
	 *  deleted when this objected is deleted.)
	 *  @see QMenu::clear() */
	void clear();
	
	/** Returns true if there are no visible actions in the menu,
	 *  false otherwise.
	 *  @see QMenu::isEmpty */
	bool isEmpty() const;
	
	/** Returns the icon size (single dimension). */
	int iconSize() const;
	
	/** Sets the icon size (single dimension).
	 *  @param icon_size the new size (at least 1) */
	void setIconSize(int icon_size);
	
	/** Returns the action at pos.
	 *  Returns nullptr if there is no action at this place. */
	QAction* actionAt(const QPoint& pos) const;
	
	/** Returns the geometry of the given action.
	 *  The action must be enabled and visible.
	 *  Otherwise the geometry will be empty.
	 *  @see QMenu::actionGeometry */
	ItemGeometry actionGeometry(QAction* action) const;
	
	/** Returns the currently highlighted action.
	 *  Returns nullptr if no action is highlighted.
	 *  @see QMenu::activeAction */
	QAction* activeAction() const;
	
	/** Sets the currently highlighted action.
	 *  The action must be enabled and visible.
	 *  Otherwise there will be no highlighted action.
	 *  @see QMenu::setActiveAction */
	void setActiveAction(QAction* action);
	
	/** Shows the menu at the given absolute position.
	 *  @see QMenu::popup */
	void popup(const QPoint& pos);
	
signals:
	/** This signal is emitted just before the menu is shown.
	 *  @see QMenu::aboutToShow */
	void aboutToShow();
	
	/** This signal is emitted just before the menu is hidden.
	 *  @see QMenu::aboutToHide */
	void aboutToHide();
	
	/** This signal is emitted when a menu item is highlighted.
	 *  @see QMenu::hovered */
	void hovered(QAction* action);
	
	/** This signal is emitted when a menu item's action is triggered.
	 *  @see QMenu::triggered */
	void triggered(QAction* action);
	
protected:
	void actionEvent(QActionEvent* event) override;
	void hideEvent(QHideEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void paintEvent(QPaintEvent* event) override;
	
	void updateCachedState();
	
	QPoint getPoint(double radius, double angle) const;
	
	int minimum_action_count;
	int icon_size;
	QAction* active_action;
	
	bool actions_changed;
	bool clicked;
	int total_radius;
	QPolygon outer_border;
	QPolygon inner_border;
	QHash< QAction*, ItemGeometry > geometries;
};


// ### PieMenu inline code ###

inline
QPoint PieMenu::getPoint(double radius, double angle) const
{
	return QPoint(total_radius + qRound(radius * -sin(angle)), total_radius + qRound(radius * -cos(angle)));
}


}  // namespace OpenOrienteering

#endif
