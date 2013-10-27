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

#include "../src/util.h"

// TODO: make configurable as a program setting
const float millimeters_per_button = 8;

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

void ActionGridBar::addAction(QAction* action, int row, int col, int row_span, int col_span)
{
	GridItem newItem;
	newItem.action = action;
	newItem.button = new QToolButton();
	newItem.button->setDefaultAction(action);
	newItem.button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	newItem.button_hidden = false;
	newItem.row = row;
	newItem.col = col;
	newItem.row_span = row_span;
	newItem.col_span = col_span;
	items.push_back(newItem);
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
	float length_millimeters = Util::mmToPixelLogical(length_px);
	cols = qMax(1, qFloor(length_millimeters / millimeters_per_button));
	
	delete layout();
	QGridLayout* newLayout = new QGridLayout(this);
	newLayout->setContentsMargins(0, 0, 0, 0);
	newLayout->setSpacing(0);
	for (size_t i = 0, end = items.size(); i < end; ++ i)
	{
		GridItem& item = items[i];
		if (item.row >= rows || item.col >= cols)
		{
			item.button->hide();
			item.button_hidden = true;
			continue;
		}
		
		if (direction == Horizontal)
		{
			newLayout->addWidget(
				item.button,
				item.row,
				item.col,
				qMin(item.row_span, rows - item.row),
				qMin(item.col_span, cols - item.col)
			);
		}
		else
		{
			newLayout->addWidget(
				item.button,
				item.col,
				item.row,
				qMin(item.col_span, cols - item.col),
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
			newLayout->setColumnStretch(i, 1);
		for (int i = 0; i < rows; ++ i)
			newLayout->setRowStretch(i, 1);
	}
	else
	{
		for (int i = 0; i < cols; ++ i)
			newLayout->setRowStretch(i, 1);
		for (int i = 0; i < rows; ++ i)
			newLayout->setColumnStretch(i, 1);
	}
	
	event->accept();
}