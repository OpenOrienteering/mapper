/*
 *    Copyright 2013 Kai Pastor
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

#include "../main_window.h"
#include "../../map.h"
#include "../../map_editor.h"
#include "../../map_undo.h"
#include "../../object.h"
#include "../../util.h"


TagsWidget::TagsWidget(Map* map, MapView* main_view, MapEditorController* controller, QWidget* parent)
 : QWidget(parent),
  map(map), 
  main_view(main_view), 
  controller(controller)
{
	react_to_changes = false;
	
	QBoxLayout* layout = new QVBoxLayout();
	layout->setMargin(0);
	
	tags_table = new QTableWidget(1, 2);
	tags_table->setEditTriggers(QAbstractItemView::AllEditTriggers);
	tags_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	tags_table->setSelectionMode(QAbstractItemView::SingleSelection);
	tags_table->setHorizontalHeaderLabels(QStringList() << tr("Key") << tr("Value"));
	tags_table->verticalHeader()->setVisible(false);
	
	QHeaderView* header_view = tags_table->horizontalHeader();
	header_view->setSectionResizeMode(0, QHeaderView::Stretch);
	header_view->setSectionResizeMode(1, QHeaderView::Stretch);
	header_view->setSectionsClickable(false);
	
	layout->addWidget(tags_table);
	
	QToolButton* help_button = newToolButton(QIcon(":/images/help.png"), tr("Help"));
	help_button->setAutoRaise(true);
	
	QBoxLayout* all_buttons_layout = new QHBoxLayout();
	QStyleOption style_option(QStyleOption::Version, QStyleOption::SO_DockWidget);
	all_buttons_layout->setContentsMargins(
		style()->pixelMetric(QStyle::PM_LayoutLeftMargin, &style_option) / 2,
		style()->pixelMetric(QStyle::PM_LayoutLeftMargin, &style_option) / 2,
		style()->pixelMetric(QStyle::PM_LayoutRightMargin, &style_option) / 2,
		style()->pixelMetric(QStyle::PM_LayoutBottomMargin, &style_option) / 2
	);
	all_buttons_layout->addWidget(new QLabel("   "), 1);
	all_buttons_layout->addWidget(help_button);
	
	layout->addLayout(all_buttons_layout);
	
	setLayout(layout);
	
	connect(tags_table, SIGNAL(cellChanged(int,int)), this, SLOT(cellChange(int,int)));
	
	connect(map, SIGNAL(objectSelectionChanged()), this, SLOT(objectTagsChanged()));
	connect(map, SIGNAL(selectedObjectEdited()), this, SLOT(objectTagsChanged()));
	
	react_to_changes = true;
	objectTagsChanged();
}

TagsWidget::~TagsWidget()
{
	; // Nothing
}

QToolButton* TagsWidget::newToolButton(const QIcon& icon, const QString& text)
{
	QToolButton* button = new QToolButton();
	button->setToolButtonStyle(Qt::ToolButtonIconOnly);
	button->setToolTip(text);
	button->setIcon(icon);
	button->setText(text);
	button->setWhatsThis("<a href=\"templates.html#setup\">See more</a>");
	return button;
}

// slot
void TagsWidget::showHelp()
{
	Util::showHelp(controller->getWindow(), "tag_editor.html");
}

void TagsWidget::setupLastRow()
{
	const int row = tags_table->rowCount() - 1;
	tags_table->setItem(row, 0, new QTableWidgetItem());
	QTableWidgetItem* value_item = new QTableWidgetItem();
	value_item->setFlags(value_item->flags() & ~Qt::ItemIsEnabled);
	tags_table->setItem(row, 1, value_item);
}

void TagsWidget::createUndoStep(Object* object)
{
	Q_ASSERT(object);
	
	Object* undo_duplicate = object->duplicate();
	undo_duplicate->setMap(map);
	
	ReplaceObjectsUndoStep* undo_step = new ReplaceObjectsUndoStep(map);
	undo_step->addObject(object, undo_duplicate);
	
	map->objectUndoManager().push(undo_step);
}

// slot
void TagsWidget::objectTagsChanged()
{
	if (!react_to_changes)
		return;
	
	react_to_changes = false;
	
	int row = 0;
	Object* object = map->getFirstSelectedObject();
	if (object)
	{
		const Object::Tags& tags = object->tags();
		tags_table->setRowCount(tags.size() + 1);
		for (Object::Tags::const_iterator tag = tags.constBegin(), end = tags.constEnd(); tag != end; ++tag, ++row)
		{
			tags_table->setItem(row, 0, new QTableWidgetItem(tag.key()));
			tags_table->item(row, 0)->setData(Qt::UserRole, tag.key());
			tags_table->setItem(row, 1, new QTableWidgetItem(tag.value()));
		}
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
	Object* object = map->getFirstSelectedObject();
	if (!react_to_changes || !object)
		return;
	
	react_to_changes = false;
	const QString key = tags_table->item(row, 0)->text().trimmed();
	const QString value = tags_table->item(row, 1)->text();
	if (column == 1)
	{
		// Value edited
		createUndoStep(object);
		object->setTag(key, value);
		if (row == tags_table->rowCount())
		{
			tags_table->setRowCount(row + 1);
			setupLastRow();
		}
	}
	else if (column == 0)
	{
		const QString old_key = tags_table->item(row, 0)->data(Qt::UserRole).toString();
		if (old_key == key && key.length() > 0)
		{
			// value edited
			createUndoStep(object);
			object->setTag(key, value);
			if (row == tags_table->rowCount())
			{
				tags_table->setRowCount(row + 1);
				setupLastRow();
			}
		}
		else if (old_key.length() > 0 && key.length() == 0)
		{
			// key deleted
			createUndoStep(object);
			object->removeTag(old_key);
			tags_table->removeRow(row);
			if (row > 0)
			{
				tags_table->setCurrentCell(row - 1, 1);
			}
		}
		else if (key.length() > 0)
		{
			// key edited
			if (object->tags().contains(key))
			{
				QMessageBox::critical(window(), tr("Key exists"),
				  tr("The key \"%1\" already exists and must not be used twice.").arg(key)
				);
				tags_table->item(row, column)->setText(old_key);
			}
			else
			{
				createUndoStep(object);
				object->removeTag(old_key);
				object->setTag(key, value);
				tags_table->item(row, 0)->setData(Qt::UserRole, key);
				if (tags_table->rowCount() == row + 1)
				{
					QTableWidgetItem* value_item = tags_table->item(row, 1);
					value_item->setFlags(value_item->flags() | Qt::ItemIsEnabled);
					tags_table->setRowCount(row + 2);
					setupLastRow();
				}
			}
		}
	}
	
	react_to_changes = true;
}
