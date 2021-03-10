/*
 *    Copyright 2013-2020 Kai Pastor
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


#include "tags_widget.h"

#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QTableWidget>
#include <QToolButton>
#include <QVBoxLayout>

#include "core/map.h"
#include "core/objects/object.h"
#include "gui/main_window.h"
#include "gui/util_gui.h"
#include "gui/map/map_editor.h"
#include "undo/object_undo.h"


namespace OpenOrienteering {

TagsWidget::TagsWidget(Map* map, MapView* main_view, MapEditorController* controller, QWidget* parent)
 : QWidget(parent),
  map(map), 
  main_view(main_view), 
  controller(controller)
{
	react_to_changes = false;
	
	auto layout = new QVBoxLayout();
	layout->setMargin(0);
	
	tags_table = new QTableWidget(1, 2);
	tags_table->setEditTriggers(QAbstractItemView::AllEditTriggers);
	tags_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	tags_table->setSelectionMode(QAbstractItemView::SingleSelection);
	tags_table->setHorizontalHeaderLabels(QStringList() << tr("Key") << tr("Value"));
	tags_table->verticalHeader()->setVisible(false);
	
	auto header_view = tags_table->horizontalHeader();
	header_view->setSectionResizeMode(0, QHeaderView::Stretch);
	header_view->setSectionResizeMode(1, QHeaderView::Stretch);
	header_view->setSectionsClickable(false);
	
	layout->addWidget(tags_table);
	
	auto help_button = Util::ToolButton::create(QIcon(QString::fromLatin1(":/images/help.png")), tr("Help"));
	help_button->setAutoRaise(true);
	
	auto all_buttons_layout = new QHBoxLayout();
	QStyleOption style_option(QStyleOption::Version, QStyleOption::SO_DockWidget);
	all_buttons_layout->setContentsMargins(
		style()->pixelMetric(QStyle::PM_LayoutLeftMargin, &style_option) / 2,
		style()->pixelMetric(QStyle::PM_LayoutLeftMargin, &style_option) / 2,
		style()->pixelMetric(QStyle::PM_LayoutRightMargin, &style_option) / 2,
		style()->pixelMetric(QStyle::PM_LayoutBottomMargin, &style_option) / 2
	);
	all_buttons_layout->addWidget(new QLabel(QString::fromLatin1("   ")), 1);
	all_buttons_layout->addWidget(help_button);
	
	layout->addLayout(all_buttons_layout);
	
	setLayout(layout);
	
	connect(tags_table, &QTableWidget::cellChanged, this, &TagsWidget::cellChange);
	
	connect(map, &Map::objectSelectionChanged, this, &TagsWidget::objectTagsChanged);
	connect(map, &Map::selectedObjectEdited, this, &TagsWidget::objectTagsChanged);
	
	react_to_changes = true;
	objectTagsChanged();
}

TagsWidget::~TagsWidget() = default;


// slot
void TagsWidget::showHelp()
{
	Util::showHelp(controller->getWindow(), "tag_editor.html");
}

void TagsWidget::setupLastRow()
{
	const int row = tags_table->rowCount() - 1;
	tags_table->setItem(row, 0, new QTableWidgetItem());
	auto value_item = new QTableWidgetItem();
	value_item->setFlags(value_item->flags() & ~Qt::ItemIsEnabled);
	tags_table->setItem(row, 1, value_item);
}

void TagsWidget::createUndoStep(Object* object)
{
	Q_ASSERT(object);
	
	auto undo_step = new ObjectTagsUndoStep(map);
	undo_step->addObject(map->getCurrentPart()->findObjectIndex(object));
	map->push(undo_step);
}

// slot
void TagsWidget::objectTagsChanged()
{
	if (!react_to_changes)
		return;
	
	react_to_changes = false;
	
	int row = 0;
	const Object* object = map->getFirstSelectedObject();
	if (object)
	{
		auto const& tags = object->tags();
		tags_table->clearContents();
		tags_table->setRowCount(tags.size() + 1);
		for (auto const& tag : tags)
		{
			tags_table->setItem(row, 0, new QTableWidgetItem(tag.key));
			tags_table->item(row, 0)->setData(Qt::UserRole, tag.key);
			tags_table->setItem(row, 1, new QTableWidgetItem(tag.value));
			++row;
		}
		tags_table->sortItems(0);
		setupLastRow();
	}
	else
	{
		tags_table->setRowCount(0);
	}
	
	react_to_changes = true;
}

// slot
void TagsWidget::cellChange(int row, int column)
{
	auto object = map->getFirstSelectedObject();
	if (!react_to_changes || !object)
		return;
	
	react_to_changes = false;
	const QString key = tags_table->item(row, 0)->text().trimmed();
	const QString value = tags_table->item(row, 1)->text();
	
	if (column == 1 && key.isEmpty())
	{
		// Shall not happen
		qWarning("Empty key for modified tag value!");
	}
	else if (column == 1)
	{
		// Value edited: update the tag
		createUndoStep(object);
		object->setTag(key, value);
		if (row + 1 == tags_table->rowCount())
		{
			// Append new row, jump to key
			tags_table->setRowCount(row + 2);
			setupLastRow();
		}
		
		if (!tags_table->item(row + 1, 1)->flags().testFlag(Qt::ItemIsEnabled))
		{
			// Jump to key
			tags_table->setCurrentCell(row + 1, 0);
		}
		else
		{
			// Jump to value
			tags_table->setCurrentCell(row + 1, 1);
		}
	}
	else if (column == 0)
	{
		const QString old_key = tags_table->item(row, 0)->data(Qt::UserRole).toString();
		if (old_key == key && !key.isEmpty())
		{
			// Key edited but not changed: do nothing
		}
		else if (!old_key.isEmpty() && key.isEmpty())
		{
			// Key deleted: remove tag
			createUndoStep(object);
			object->removeTag(old_key);
			tags_table->removeRow(row);
			if (row > 0)
			{
				// Jump to previous row
				tags_table->setCurrentCell(row - 1, 1);
			}
			else
			{
				// Reset current row
				tags_table->item(row, 1)->setText({});
				auto value_item = tags_table->item(row, 1);
				value_item->setFlags(value_item->flags() & ~Qt::ItemIsEnabled);
			}
		}
		else if (!key.isEmpty())
		{
			// Key edited: update the tags
			if (object->tags().contains(key))
			{
				QMessageBox::critical(window(), tr("Key exists"),
				  tr("The key \"%1\" already exists and must not be used twice.").arg(key)
				);
				tags_table->item(row, column)->setText(old_key);
				QTimer::singleShot(0, tags_table, [tags_table = this->tags_table, row, column] {
					tags_table->setCurrentCell(row, column);
				});
			}
			else
			{
				if (value.isEmpty() && old_key.isEmpty())
				{
					// New key, empty value - don't insert yet
					tags_table->item(row, 0)->setData(Qt::UserRole, QLatin1String("ABOUT TO INSERT NEW VALUE"));
					tags_table->setCurrentCell(row, 1);
				}
				else
				{
					// New key with value - update tags
					createUndoStep(object);
					object->removeTag(old_key);
					object->setTag(key, value);
				}
				tags_table->item(row, 0)->setData(Qt::UserRole, key);
				if (tags_table->rowCount() == row + 1)
				{
					auto value_item = tags_table->item(row, 1);
					value_item->setFlags(value_item->flags() | Qt::ItemIsEnabled);
				}
				tags_table->setCurrentCell(row, 1);
			}
		}
	}
	
	react_to_changes = true;
}


}  // namespace OpenOrienteering
