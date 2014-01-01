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
class QMenu;
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
	 * After constructions, add items and either insert the overflow action
	 * with addAction(getOverflowAction(), ...) or set another ActionGridBar
	 * to include the overflow items.
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
	void addAction(QAction* action, int row, int col, int row_span = 1, int col_span = 1, bool at_end = false);
	
	/** Adds an action to the grid, starting from the opposite direction. */
	void addActionAtEnd(QAction* action, int row, int col, int row_span = 1, int col_span = 1);
	
	/** Returns the size of the button icons. */
	QSize getIconSize(int row_span = 1, int col_span = 1) const;
	
	/** Returns the overflow action (to be inserted into the action bar with addAction()).
	 *  The overflow action is enabled if there are items which do not fit into
	 *  the action bar. On click, it shows a list of those actions. */
	QAction* getOverflowAction() const;
	
	/** Configures this bar to put its overflow actions into another bar. */
	void setToUseOverflowActionFrom(ActionGridBar* other_bar);
	
	virtual QSize sizeHint() const;
	
protected slots:
	void overflowActionClicked();
	
protected:
	struct GridItem
	{
		int id; // sequential id for sorting in overflow item chooser
		int row;
		int col;
		int row_span;
		int col_span;
		bool at_end;
		QAction* action;
		QToolButton* button;
		bool button_hidden;
	};
	
	static bool compareItemPtrId(GridItem* a, GridItem* b);
	virtual void resizeEvent(QResizeEvent* event);
	
	Direction direction;
	int rows;
	int cols;
	std::vector< GridItem > items;
	int next_id;
	QAction* overflow_action;
	QToolButton* overflow_button;
	QMenu* overflow_menu;
	std::vector< GridItem* > hidden_items;
	std::vector< ActionGridBar* > include_overflow_from_list;
};

#endif
