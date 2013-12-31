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


#include "action_grid_bar.h"

#include <qmath.h>
#include <QApplication>
#include <QGridLayout>
#include <QToolButton>
#include <QAction>
#include <QScreen>
#include <QKeyEvent>
#include <QDebug>

#include "../src/util.h"

// TODO: make configurable as a program setting
const float millimeters_per_button = 10;

ActionGridBar::ActionGridBar(Direction direction, int rows, QWidget* parent)
: QWidget(parent)
{
	this->direction = direction;
	this->rows = rows;
	
	// Will be calculated in resizeEvent()
	cols = 1;
	
	if (direction == Horizontal)
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	else
		setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
}

int ActionGridBar::getRows() const
{
	return rows;
}

int ActionGridBar::getCols() const
{
	return cols;
}

void ActionGridBar::addAction(QAction* action, int row, int col, int row_span, int col_span, bool at_end)
{
	// Determine icon size (important for high-dpi screens).
	// Use a somewhat smaller size than what would cover the whole icon to
	// account for the (assumed) button border.
	int icon_size_pixel = qRound(Util::mmToPixelLogical(millimeters_per_button));
	const int button_icon_size = icon_size_pixel - 8;

	GridItem newItem;
	newItem.action = action;
	newItem.button = new QToolButton();
	newItem.button->setDefaultAction(action);
	newItem.button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	newItem.button->setAutoRaise(true);
	newItem.button->setIconSize(QSize(button_icon_size, button_icon_size));
	newItem.button_hidden = false;
	newItem.row = row;
	newItem.col = col;
	newItem.row_span = row_span;
	newItem.col_span = col_span;
	newItem.at_end = at_end;
	items.push_back(newItem);
}

void ActionGridBar::addActionAtEnd(QAction* action, int row, int col, int row_span, int col_span)
{
	addAction(action, row, col, row_span, col_span, true);
}

QSize ActionGridBar::sizeHint() const
{
	int height_px = Util::mmToPixelLogical(rows * millimeters_per_button);
	if (direction == Horizontal)
		return QSize(100, height_px);
	else
		return QSize(height_px, 100);
}

void ActionGridBar::resizeEvent(QResizeEvent* event)
{
	int length_px = (direction == Horizontal) ? width() : height();
	float length_millimeters = Util::pixelToMMLogical(length_px);
	cols = qMax(1, qFloor(length_millimeters / millimeters_per_button));
	
	delete layout();
	QGridLayout* new_layout = new QGridLayout(this);
	new_layout->setContentsMargins(0, 0, 0, 0);
	new_layout->setSpacing(0);
	for (size_t i = 0, end = items.size(); i < end; ++ i)
	{
		GridItem& item = items[i];
		int resulting_col = item.at_end ? (cols - 1 - item.col) : item.col;
		bool hidden = item.row >= rows || item.col >= cols;
		if (! hidden)
		{
			// Check for collisions with other items
			for (size_t k = 0; k < items.size(); ++ k)
			{
				if (i == k)
					continue;
				GridItem& other = items[k];
				int resulting_col_other = other.at_end ? (cols - 1 - other.col) : other.col;
				if (item.row == other.row && resulting_col == resulting_col_other)
				{
					// Check which item "wins" this spot and which will be hidden
					if (item.at_end == other.at_end)
						qDebug() << "Warning: two items set to same position in ActionGridBar, this case is not handled!";
					if ((item.at_end && resulting_col <= cols / 2)
						|| (! item.at_end && resulting_col > cols / 2))
					{
						hidden = true;
						break;
					}
				}
			}
		}
		if (hidden)
		{
			item.button->hide();
			item.button_hidden = true;
			continue;
		}
		
		if (direction == Horizontal)
		{
			new_layout->addWidget(
				item.button,
				item.row,
				resulting_col,
				qMin(item.row_span, rows - item.row),
				qMin(item.col_span, cols - resulting_col)
			);
		}
		else
		{
			new_layout->addWidget(
				item.button,
				resulting_col,
				item.row,
				qMin(item.col_span, cols - resulting_col),
				qMin(item.row_span, rows - item.row)
			);
		}
		if (item.button_hidden)
		{
			item.button->setVisible(true);
			item.button_hidden = false;
		}
		item.button->updateGeometry();
	}
	
	// Set row/col strech. The first and last row/col acts as margin in case
	// the available area is not a multiple of the button size.
	//int pixel_per_button = qFloor(millimeters_per_button / pixel_to_millimeters);
	if (direction == Horizontal)
	{
		for (int i = 0; i < cols; ++ i)
			new_layout->setColumnStretch(i, 1);
		for (int i = 0; i < rows; ++ i)
			new_layout->setRowStretch(i, 1);
	}
	else
	{
		for (int i = 0; i < cols; ++ i)
			new_layout->setRowStretch(i, 1);
		for (int i = 0; i < rows; ++ i)
			new_layout->setColumnStretch(i, 1);
	}
	
	event->accept();
}
