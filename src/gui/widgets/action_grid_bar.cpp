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

#include <QtMath>
#include <QAction>
#include <QGridLayout>
#include <QMenu>
#include <QResizeEvent>
#include <QToolButton>

#include "settings.h"
#include "gui/util_gui.h"


namespace OpenOrienteering {

ActionGridBar::ActionGridBar(Direction direction, int rows, QWidget* parent)
: QWidget(parent)
{
	this->direction = direction;
	this->rows = rows;
	next_id = 0;
	
	// Will be calculated in resizeEvent()
	cols = 1;
	
	if (direction == Horizontal)
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	else
		setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
	
	// Create overflow action
	overflow_action = new QAction(QIcon(QString::fromLatin1(":/images/three-dots.png")), tr("Show remaining items"), this);
 	connect(overflow_action, &QAction::triggered, this, &ActionGridBar::overflowActionClicked);
	overflow_button = nullptr;
	overflow_menu = new QMenu(this);
	include_overflow_from_list.push_back(this);
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
	QSize icon_size = getIconSize(row_span, col_span);
	
	// Ensure that the icon of the given action is big enough. If not, scale it up.
	// NOTE: Here, row_span == col_span is assumed.
	QIcon icon = action->icon();
	QPixmap pixmap = icon.pixmap(icon_size, QIcon::Normal, QIcon::Off);
	if (! pixmap.isNull() && pixmap.width() < icon_size.width())
	{
		pixmap = pixmap.scaled(icon_size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
		icon.addPixmap(pixmap);
		action->setIcon(icon);
	}

	// Add the item
	GridItem newItem;
	newItem.id = next_id ++;
	newItem.action = action;
	newItem.button = new QToolButton();
	newItem.button->setDefaultAction(action);
	newItem.button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	newItem.button->setAutoRaise(true);
	newItem.button->setIconSize(icon_size);
	newItem.button_hidden = false;
	newItem.row = row;
	newItem.col = col;
	newItem.row_span = row_span;
	newItem.col_span = col_span;
	newItem.at_end = at_end;
	items.push_back(newItem);
	
	// If this is the overflow action, remember the button.
	if (action == overflow_action)
		overflow_button = newItem.button;
}

void ActionGridBar::addActionAtEnd(QAction* action, int row, int col, int row_span, int col_span)
{
	addAction(action, row, col, row_span, col_span, true);
}

QSize ActionGridBar::getIconSize(int row_span, int col_span) const
{
	int icon_size_pixel_row = qRound(row_span * Util::mmToPixelLogical(Settings::getInstance().getSettingCached(Settings::ActionGridBar_ButtonSizeMM).toFloat()));
	int icon_size_pixel_col = qRound(col_span * Util::mmToPixelLogical(Settings::getInstance().getSettingCached(Settings::ActionGridBar_ButtonSizeMM).toFloat()));
	const int button_icon_size_row = icon_size_pixel_row - 12;
	const int button_icon_size_col = icon_size_pixel_col - 12;
	if (direction == Horizontal)
		return QSize(button_icon_size_row, button_icon_size_col);
	else
		return QSize(button_icon_size_col, button_icon_size_row);
}

QAction* ActionGridBar::getOverflowAction() const
{
	return overflow_action;
}

void ActionGridBar::setToUseOverflowActionFrom(ActionGridBar* other_bar)
{
	other_bar->include_overflow_from_list.push_back(this);
}

QToolButton* ActionGridBar::getButtonForAction(QAction* action)
{
	for (auto& item : items)
	{
		if (item.action == action)
			return item.button_hidden ? nullptr : item.button;
	}
	return nullptr;
}

QSize ActionGridBar::sizeHint() const
{
	int height_px = Util::mmToPixelLogical(rows * Settings::getInstance().getSettingCached(Settings::ActionGridBar_ButtonSizeMM).toFloat());
	if (direction == Horizontal)
		return QSize(100, height_px);
	else
		return QSize(height_px, 100);
}

bool ActionGridBar::compareItemPtrId(ActionGridBar::GridItem* a, ActionGridBar::GridItem* b)
{
	return a->id < b->id;
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
	float length_millimeters = Util::pixelToMMLogical(length_px);
	cols = qMax(1, qFloor(length_millimeters / Settings::getInstance().getSettingCached(Settings::ActionGridBar_ButtonSizeMM).toFloat()));
	
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
						qDebug("Warning: two items set to same position in ActionGridBar, this case is not handled!");
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
			hidden_items.push_back(&item);
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
	
	overflow_action->setEnabled(! hidden_items.empty());
	std::sort(hidden_items.begin(), hidden_items.end(), compareItemPtrId);
	
	event->accept();
}


}  // namespace OpenOrienteering
