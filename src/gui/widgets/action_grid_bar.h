/*
 *    Copyright 2013 Thomas Schöps
 *    Copyright 2019 Kai Pastor
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


#ifndef OPENORIENTEERING_ACTION_GRID_BAR_H
#define OPENORIENTEERING_ACTION_GRID_BAR_H

#include <vector>

#include <QObject>
#include <QSize>
#include <QString>
#include <QWidget>

class QAction;
class QMenu;
class QResizeEvent;
class QToolButton;


namespace OpenOrienteering {

/**
 * A toolbar with a grid layout, whose button size depends on the ppi.
 * 
 * \todo Update button size on setting changes (not yet needed for mobile UI).
 * \todo Use parameter order col,row / colspan,rowspan to match the common x,y / width,height pattern.
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
	 * Defines possible actual positions of a button.
	 */
	enum ButtonDisplay
	{
		DisplayNormal,    ///< Regular button display
		DisplayOverflow,  ///< Button display in overflow action
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
	ActionGridBar(Direction direction, int height_items, QWidget* parent = nullptr);
	
	/** Returns the number of grid rows. */
	int rowCount() const;
	
	/** Returns the number of grid columns. */
	int columnCount() const;
	
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
	
	/**
	 * Returns the button corresponding to the given action.
	 * 
	 * If the action has not been added to the action bar, this function
	 * returns nullptr.
	 */
	QToolButton* getButtonForAction(const QAction* action) const;
	
	/**
	 * Returns where the given button is displayed.
	 */
	ButtonDisplay buttonDisplay(const QToolButton* button) const;
	
	QSize sizeHint() const override;
	
protected slots:
	void overflowActionClicked();
	
protected:
	void resizeEvent(QResizeEvent* event) override;
	
	struct GridItem
	{
		QAction* action;
		QToolButton* button;
		int id; // sequential id for sorting in overflow item chooser
		int row;
		int col;
		int row_span;
		int col_span;
		bool at_end;
		bool button_hidden = false;
	};
	
	std::vector<GridItem> items;
	std::vector<GridItem*> hidden_items;
	std::vector<ActionGridBar*> include_overflow_from_list;
	QAction* overflow_action;
	QToolButton* overflow_button = nullptr;
	QMenu* overflow_menu;
	Direction direction;
	int next_id = 0;
	int row_count;
	int column_count = 1;
	int button_size_px;
	int margin_size_px;
};


}  // namespace OpenOrienteering
#endif
