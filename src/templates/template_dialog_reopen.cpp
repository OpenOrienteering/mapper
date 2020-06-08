/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2016 Kai Pastor
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


#include "template_dialog_reopen.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>

#include "core/map.h"
#include "template.h"
#include "gui/util_gui.h"


namespace OpenOrienteering {

ReopenTemplateDialog::ReopenTemplateDialog(QWidget* parent, Map* map, const QString& map_directory)
: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
, map(map)
, map_directory(map_directory)
{
	setWindowTitle(tr("Reopen template"));
	
	QLabel* description = new QLabel(tr("Drag items from the left list to the desired spot in the right list to reload them."));
	
	QLabel* closed_template_list_label = Util::Headline::create(tr("Closed templates:"));
	closed_template_list = new QListWidget();
	updateClosedTemplateList();
	clear_button = new QPushButton(tr("Clear list"));
	clear_button->setEnabled(map->getNumClosedTemplates() > 0);
	
	QLabel* open_template_list_label = Util::Headline::create(tr("Active templates:"));
	open_template_list = new OpenTemplateList(this);
	for (int i = map->getNumTemplates() - 1; i >= 0; --i)
	{
		Template* temp = map->getTemplate(i);
		QListWidgetItem* item = new QListWidgetItem(temp->getTemplateFilename());
		item->setToolTip(temp->getTemplatePath());
		open_template_list->addItem(item);
	}
	QListWidgetItem* item = new QListWidgetItem(tr("- Map -"));
	item->setData(Qt::UserRole, true);
	open_template_list->insertItem(map->getNumTemplates() - map->getFirstFrontTemplate(), item);
	
	closed_template_list->setSelectionMode(QAbstractItemView::SingleSelection);
	closed_template_list->setDragDropMode(QAbstractItemView::DragOnly);
	closed_template_list->setDefaultDropAction(Qt::MoveAction);
	open_template_list->setDragDropMode(QAbstractItemView::DropOnly);
	open_template_list->setDropIndicatorShown(true);
	open_template_list->setDragDropOverwriteMode(false);
	
	QDialogButtonBox* button_box = new QDialogButtonBox(QDialogButtonBox::Close);
	
	int row = 0;
	QGridLayout* layout = new QGridLayout();
	layout->addWidget(description, row++, 0, 1, 2);
	layout->setRowMinimumHeight(row++, 16);
	layout->addWidget(closed_template_list_label, row, 0);
	layout->addWidget(open_template_list_label, row++, 1);
	layout->addWidget(closed_template_list, row, 0);
	layout->addWidget(open_template_list, row++, 1);
	layout->addWidget(clear_button, row++, 0);
	layout->addWidget(button_box, row++, 0, 1, 2);
	setLayout(layout);
	
	connect(clear_button, &QAbstractButton::clicked, this, &ReopenTemplateDialog::clearClicked);
	connect(button_box, &QDialogButtonBox::clicked, this, &ReopenTemplateDialog::doAccept);
}

void ReopenTemplateDialog::updateClosedTemplateList()
{
	closed_template_list->clear();
	for (int i = map->getNumClosedTemplates() - 1; i >= 0; --i)
	{
		Template* temp = map->getClosedTemplate(i);
		QListWidgetItem* item = new QListWidgetItem(temp->getTemplateFilename());
		item->setToolTip(temp->getTemplatePath());
		item->setData(Qt::UserRole, i);
		closed_template_list->addItem(item);
	}
}

void ReopenTemplateDialog::clearClicked()
{
	closed_template_list->clear();
	map->clearClosedTemplates();
	clear_button->setEnabled(false);
}

void ReopenTemplateDialog::doAccept(QAbstractButton* button)
{
	Q_UNUSED(button);
	accept();
}

ReopenTemplateDialog::OpenTemplateList::OpenTemplateList(ReopenTemplateDialog* dialog)
 : QListWidget(), dialog(dialog)
{
}

void ReopenTemplateDialog::OpenTemplateList::dropEvent(QDropEvent* event)
{
    QListWidget::dropEvent(event);
	
	// Pretty backward way of finding the drop source and destination:
	// Loop over the resulting list and find the item with integer UserData.
	// NOTE: if you know a better way of doing this, I would like to hear it!
	int src_pos = -1;
	int target_pos = dialog->map->getNumTemplates();
	int item_index = -1;
	int map_row = count();
	for (int i = 0; src_pos < 0; ++i)
	{
		auto item_data = item(i)->data(Qt::UserRole);
		switch (item_data.type())
		{
		case QVariant::Int:
			item_index = i;
			src_pos = item_data.toInt();
			break;
		case QVariant::Bool:
			map_row = i;
			break;
		default:
			--target_pos;
			Q_ASSERT(target_pos >= 0);
		}
	}
	
	auto const insert_pos = (item_index == map_row + 1) ? -1 : target_pos;
	if (!dialog->map->reloadClosedTemplate(src_pos, insert_pos, dialog, dialog->map_directory))
	{
		// Revert action
		delete item(item_index);
	}
	else
	{
		// Template filename may change if file has moved
		item(item_index)->setText(dialog->map->getTemplate(target_pos)->getTemplateFilename());
		// Prepare for next drop
		item(item_index)->setData(Qt::UserRole, QVariant());
		
		dialog->clear_button->setEnabled(dialog->map->getNumClosedTemplates() > 0);
	}
	
	// Always re-fill this list to update the list indices in the item data
	dialog->updateClosedTemplateList();
}


}  // namespace OpenOrienteering
