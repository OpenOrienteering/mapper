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


#ifndef _OPENORIENTEERING_ACTION_GRID_BAR_H_
#define _OPENORIENTEERING_ACTION_GRID_BAR_H_

#include <QWidget>

QT_BEGIN_NAMESPACE
class QToolButton;
QT_END_NAMESPACE

/**
 * A toolbar with a grid layout, whose button size depends on the ppi.
 */
class ActionGridBar : public QWidget
{
Q_OBJECT
public:
	enum Direction
	{
		Horizontal = 0,
		Vertical
	};
	
	/**
	 * Constructs a new ActionGridBar.
	 * 
	 * @param direction Direction of the toolbar.
	 * @param height_items Number of rows in the direction opposite to the main
	 *    toolbar direction.
	 */
	ActionGridBar(Direction direction, int height_items, QWidget* parent = NULL);
	
	/** Returns the number of grid rows. */
	int getRows() const;
	
	/** Returns the number of grid columns. */
	int getCols() const;
	
	/** Adds an action to the grid. */
	void addAction(QAction* action, int row, int col, int row_span = 1, int col_span = 1);
	
	virtual QSize sizeHint() const;
	
protected:
	struct GridItem
	{
		int row;
		int col;
		int row_span;
		int col_span;
		QAction* action;
		QToolButton* button;
		bool button_hidden;
	};
	
	virtual void resizeEvent(QResizeEvent* event);
	
	Direction direction;
	int rows;
	int cols;
	std::vector< GridItem > items;
};

#endif
