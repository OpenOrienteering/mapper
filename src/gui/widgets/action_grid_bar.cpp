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


#include "action_grid_bar.h"

#include <algorithm>

#include <QtGlobal>
#include <QAction>
#include <QGridLayout>
#include <QIcon>
#include <QLayout>
#include <QMenu>
#include <QPoint>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QToolButton>
#include <QVariant>

#include "settings.h"
#include "gui/util_gui.h"


namespace OpenOrienteering {

ActionGridBar::ActionGridBar(Direction direction, int rows, QWidget* parent)
: QWidget(parent)
, direction(direction)
, row_count(rows)
, button_size_px(qRound(Util::mmToPixelPhysical(Settings::getInstance().getSetting(Settings::ActionGridBar_ButtonSizeMM).toReal())))
, margin_size_px(button_size_px / 4)
{
	if (direction == Horizontal)
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	else
		setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
	
	// Create overflow action
	overflow_action = new QAction(QIcon(QString::fromLatin1(":/images/three-dots.png")), tr("Show remaining items"), this);
 	connect(overflow_action, &QAction::triggered, this, &ActionGridBar::overflowActionClicked);
	overflow_menu = new QMenu(this);
	include_overflow_from_list.push_back(this);
}

int ActionGridBar::rowCount() const
{
	return row_count;
}

int ActionGridBar::columnCount() const
{
	return column_count;
}

void ActionGridBar::addAction(QAction* action, int row, int col, int row_span, int col_span, bool at_end)
{
	auto* button = new QToolButton();
	button->setDefaultAction(action);
	button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	button->setAutoRaise(true);
	button->setIconSize(getIconSize(row_span, col_span));
	
	// Add the item
	items.push_back({action, button, next_id++, row, col, row_span, col_span, at_end});
	
	// If this is the overflow action, remember the button.
	if (action == overflow_action)
		overflow_button = button;
}

void ActionGridBar::addActionAtEnd(QAction* action, int row, int col, int row_span, int col_span)
{
	addAction(action, row, col, row_span, col_span, true);
}

QSize ActionGridBar::getIconSize(int row_span, int col_span) const
{
	auto size = QSize{col_span * button_size_px - margin_size_px, row_span * button_size_px - margin_size_px};
	if (direction == Vertical)
		size.transpose();
	return size;
}

QAction* ActionGridBar::getOverflowAction() const
{
	return overflow_action;
}

void ActionGridBar::setToUseOverflowActionFrom(ActionGridBar* other_bar)
{
	other_bar->include_overflow_from_list.push_back(this);
}

QToolButton* ActionGridBar::getButtonForAction(const QAction* action) const
{
	for (auto& item : items)
	{
		if (item.action == action)
			return item.button;
	}
	return nullptr;
}

ActionGridBar::ButtonDisplay ActionGridBar::buttonDisplay(const QToolButton* button) const
{
	for (auto& item : items)
	{
		if (item.button == button)
			return item.button_hidden ? DisplayOverflow : DisplayNormal;
	}
	Q_UNREACHABLE();
	return DisplayNormal;
}

QSize ActionGridBar::sizeHint() const
{
	auto extent = row_count * button_size_px;
	return {extent, extent};
}

void ActionGridBar::overflowActionClicked()
{
	overflow_menu->clear();
	for (const auto* source_bar : include_overflow_from_list)
	{
		for (const auto* hidden_item : source_bar->hidden_items)
			overflow_menu->addAction(hidden_item->action);
	}
	if (overflow_button)
		overflow_menu->popup(overflow_button->mapToGlobal(QPoint(0, overflow_button->height())));
	else
		overflow_menu->popup(mapToGlobal(QPoint(0, height())));
}

void ActionGridBar::resizeEvent(QResizeEvent* event)
{
	hidden_items.clear();
	
	int length_px = (direction == Horizontal) ? width() : height();
	column_count = qMax(1, length_px / button_size_px);
	
	delete layout();
	auto* new_layout = new QGridLayout(this);
	new_layout->setContentsMargins(0, 0, 0, 0);
	new_layout->setSpacing(0);
	for (auto& item : items)
	{
		int resulting_col = item.at_end ? (column_count - 1 - item.col) : item.col;
		bool hidden = item.row >= row_count || item.col >= column_count;
		if (! hidden)
		{
			// Check for collisions with other items
			for (auto& other : items)
			{
				if (&item == &other)
					continue;
				int resulting_col_other = other.at_end ? (column_count - 1 - other.col) : other.col;
				if (item.row == other.row && resulting_col == resulting_col_other)
				{
					// Check which item "wins" this spot and which will be hidden
					if (item.at_end == other.at_end)
						qDebug("Warning: two items set to same position in ActionGridBar, this case is not handled!");
					if ((item.at_end && resulting_col <= column_count / 2)
					    || (! item.at_end && resulting_col > column_count / 2))
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
			hidden_items.push_back(&item);
			continue;
		}
		
		if (direction == Horizontal)
		{
			new_layout->addWidget(
				item.button,
				item.row,
				resulting_col,
				qMin(item.row_span, row_count - item.row),
				qMin(item.col_span, column_count - resulting_col)
			);
		}
		else
		{
			new_layout->addWidget(
				item.button,
				resulting_col,
				item.row,
				qMin(item.col_span, column_count - resulting_col),
				qMin(item.row_span, row_count - item.row)
			);
		}
		if (item.button_hidden)
		{
			item.button->setVisible(true);
			item.button_hidden = false;
		}
		item.button->updateGeometry();
	}
	
	// Set row/col stretch. The first and last row/col acts as margin in case
	// the available area is not a multiple of the button size.
	if (direction == Horizontal)
	{
		for (int i = 0; i < column_count; ++ i)
			new_layout->setColumnStretch(i, 1);
		for (int i = 0; i < row_count; ++ i)
			new_layout->setRowStretch(i, 1);
	}
	else
	{
		for (int i = 0; i < column_count; ++ i)
			new_layout->setRowStretch(i, 1);
		for (int i = 0; i < row_count; ++ i)
			new_layout->setColumnStretch(i, 1);
	}
	
	overflow_action->setEnabled(! hidden_items.empty());
	std::sort(hidden_items.begin(), hidden_items.end(), [](GridItem* a, GridItem* b) {
		return a->id < b->id;
	});
	
	event->accept();
}


}  // namespace OpenOrienteering
