/*
 *    Copyright 2016 Mitchell Krome
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


#include "tag_select_widget.h"

#include <QComboBox>
#include <QGuiApplication>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QShowEvent>
#include <QTableWidget>
#include <QToolButton>

#include "segmented_button_layout.h"
#include "../main_window.h"
#include "core/map.h"
#include "gui/map/map_editor.h"
#include "core/map_part.h"
#include "core/objects/object.h"
#include "core/objects/object_query.h"
#include "tools/tool.h"
#include "util/util.h"


// ### TagSelectWidget ###

TagSelectWidget::TagSelectWidget(Map* map, MapView* main_view, MapEditorController* controller, QWidget* parent)
: QWidget    { parent }
, map        { map }
, main_view  { main_view }
, controller { controller }
{
	Q_ASSERT(main_view);
	Q_ASSERT(controller);
	
	setWhatsThis(Util::makeWhatThis("tag_selector.html"));
	
	query_table = new QTableWidget(1, 4);
	query_table->setEditTriggers(QAbstractItemView::AllEditTriggers);
	query_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	query_table->setSelectionMode(QAbstractItemView::SingleSelection);
	query_table->setHorizontalHeaderLabels(QStringList() << tr("Relation") << tr("Key") << tr("Comparison") << tr("Value"));
	query_table->verticalHeader()->setVisible(false);
	
	QHeaderView* header_view = query_table->horizontalHeader();
	header_view->setSectionResizeMode(0, QHeaderView::Fixed);
	header_view->setSectionResizeMode(1, QHeaderView::Stretch);
	header_view->setSectionResizeMode(2, QHeaderView::Fixed);
	header_view->setSectionResizeMode(3, QHeaderView::Stretch);
	header_view->setSectionsClickable(false);
	
	all_query_layout = new QVBoxLayout();
	all_query_layout->setMargin(0);
	all_query_layout->addWidget(query_table, 1);

	addRowItems(0);

	add_button = newToolButton(QIcon(QString::fromLatin1(":/images/plus.png")), tr("Add Row"));
	delete_button = newToolButton(QIcon(QString::fromLatin1(":/images/minus.png")), tr("Remove Row"));
	
	SegmentedButtonLayout* add_remove_layout = new SegmentedButtonLayout();
	add_remove_layout->addWidget(add_button);
	add_remove_layout->addWidget(delete_button);

	move_up_button = newToolButton(QIcon(QString::fromLatin1(":/images/arrow-up.png")), tr("Move Up"));
	move_up_button->setAutoRepeat(true);
	move_down_button = newToolButton(QIcon(QString::fromLatin1(":/images/arrow-down.png")), tr("Move Down"));
	move_down_button->setAutoRepeat(true);

	SegmentedButtonLayout* up_down_layout = new SegmentedButtonLayout();
	up_down_layout->addWidget(move_up_button);
	up_down_layout->addWidget(move_down_button);
	
	select_button = newToolButton(QIcon(QString::fromLatin1(":/images/tool-edit.png")), tr("Select"));
	select_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

	selection_info = new QLabel();

	QToolButton* help_button = newToolButton(QIcon(QString::fromLatin1(":/images/help.png")), tr("Help"));
	help_button->setAutoRaise(true);

	QBoxLayout* list_buttons_layout = new QHBoxLayout();
	list_buttons_layout->setContentsMargins(0,0,0,0);
	list_buttons_layout->addLayout(add_remove_layout);
	list_buttons_layout->addLayout(up_down_layout);
	list_buttons_layout->addWidget(select_button);
	list_buttons_layout->addWidget(selection_info);
	
	list_buttons_group = new QWidget();
	list_buttons_group->setLayout(list_buttons_layout);
	
	QBoxLayout* all_buttons_layout = new QHBoxLayout();
	QStyleOption style_option(QStyleOption::Version, QStyleOption::SO_DockWidget);
	all_buttons_layout->setContentsMargins(
		style()->pixelMetric(QStyle::PM_LayoutLeftMargin, &style_option) / 2,
		0, // Covered by the main layout's spacing.
		style()->pixelMetric(QStyle::PM_LayoutRightMargin, &style_option) / 2,
		style()->pixelMetric(QStyle::PM_LayoutBottomMargin, &style_option) / 2
	);
	all_buttons_layout->addWidget(list_buttons_group);
	all_buttons_layout->addWidget(new QLabel(QString::fromLatin1("   ")), 1);
	all_buttons_layout->addWidget(help_button);

	all_query_layout->addLayout(all_buttons_layout);
	
	setLayout(all_query_layout);

	// Connections
	connect(add_button, &QAbstractButton::clicked, this, &TagSelectWidget::addRow);
	connect(delete_button, &QAbstractButton::clicked, this, &TagSelectWidget::deleteRow);
	connect(move_up_button, &QAbstractButton::clicked, this, [this]{ moveRow(true); });
	connect(move_down_button, &QAbstractButton::clicked, this, [this]{ moveRow(false); });
	connect(select_button, &QAbstractButton::clicked, this, &TagSelectWidget::makeSelection);
	connect(help_button, &QAbstractButton::clicked, this, &TagSelectWidget::showHelp);
	
	connect(query_table, &QTableWidget::cellChanged, this, &TagSelectWidget::cellChanged);
	connect(map, &Map::objectSelectionChanged, this, &TagSelectWidget::resetSelectionInfo);
}


TagSelectWidget::~TagSelectWidget()
{
	; // Nothing
}



void TagSelectWidget::resetSelectionInfo()
{
	selection_info->setText({});
}



QToolButton* TagSelectWidget::newToolButton(const QIcon& icon, const QString& text)
{
	QToolButton* button = new QToolButton();
	button->setToolButtonStyle(Qt::ToolButtonIconOnly);
	button->setToolTip(text);
	button->setIcon(icon);
	button->setText(text);
	button->setWhatsThis(Util::makeWhatThis("tag_selector.html"));
	return button;
}



void TagSelectWidget::showEvent(QShowEvent* event)
{
	if (!event->spontaneous())
	{
		auto last_row = query_table->rowCount() - 1;
		query_table->setCurrentCell(last_row, 1);
		if (!query_table->currentItem()->text().isEmpty())
			query_table->setCurrentCell(last_row, 3);
		query_table->editItem(query_table->currentItem());
	}
}



void TagSelectWidget::showHelp()
{
	Util::showHelp(controller->getWindow(), "tag_selector.html");
}



void TagSelectWidget::addRowItems(int row)
{
	QTableWidgetItem* item = new QTableWidgetItem();
	query_table->setItem(row, 1, item);
	item = new QTableWidgetItem();
	query_table->setItem(row, 3, item);

	QComboBox* compare_op = new QComboBox();
	for (auto op : { ObjectQuery::OperatorIs, ObjectQuery::OperatorIsNot, ObjectQuery::OperatorContains })
		compare_op->addItem(ObjectQuery::labelFor(op), QVariant::fromValue(op));
	query_table->setCellWidget(row, 2, compare_op);
	connect(compare_op, &QComboBox::currentTextChanged, this, &TagSelectWidget::resetSelectionInfo);

	if (row == 0)
	{
		// The first row doesn't use a logical operator
		item = new QTableWidgetItem();
		item->setFlags(Qt::NoItemFlags);
		item->setBackground(QBrush(QGuiApplication::palette().window()));
		query_table->setItem(row, 0, item);
	}
	else
	{
		QComboBox* logical_op = new QComboBox();
		auto and_label = QString { QLatin1String("  ") + ObjectQuery::labelFor(ObjectQuery::OperatorAnd) };
		logical_op->addItem(and_label, QVariant::fromValue(ObjectQuery::OperatorAnd));
		logical_op->addItem(ObjectQuery::labelFor(ObjectQuery::OperatorOr), QVariant::fromValue(ObjectQuery::OperatorOr));
		query_table->setCellWidget(row, 0, logical_op);
		connect(logical_op, &QComboBox::currentTextChanged, this, &TagSelectWidget::resetSelectionInfo);
	}
}



void TagSelectWidget::cellChanged(int row, int column)
{
	resetSelectionInfo();
	switch(column)
	{
	case 1:
	case 3:
		{
			auto item = query_table->item(row, column);
			item->setText(item->text().trimmed());
			break;
		}
	default:
		; // nothing
	}
}



void TagSelectWidget::addRow()
{
	int row = query_table->currentRow() + 1;

	// When nothing is selected, add to the end
	if (row == 0)
		row = query_table->rowCount();

	query_table->insertRow(row);
	addRowItems(row);

	// Move the selection to the new row
	int col = query_table->currentColumn();
	query_table->setCurrentCell(row, col);
}


void TagSelectWidget::deleteRow()
{
	// Always need one row
	if (query_table->rowCount() == 1)
		return;

	int row = query_table->currentRow();

	// When nothing is selected, delete from the end
	if (row == -1)
		row = query_table->rowCount() - 1;

	query_table->removeRow(row);

	// If we delete first row, need to fix logical operator
	if (row == 0)
	{
		query_table->removeCellWidget(row, 0);
		QTableWidgetItem* item = new QTableWidgetItem();
		item->setFlags(Qt::NoItemFlags);
		item->setBackground(QBrush(QGuiApplication::palette().window()));
		query_table->setItem(row, 0, item);
	}
}


void TagSelectWidget::moveRow(bool up)
{
	int row = query_table->currentRow();
	int max_row = query_table->rowCount();

	// When nothing is selected, move the last row
	if (row == -1)
		row = max_row - 1;

	// Cant move first row up or last row down
	if ((up && row < 1) || (!up && row == max_row - 1))
		return;

	// Moving the current row down is just moving the row below up.. 
	if (!up)
		row += 1;

	// Col 1, 3 are items
	auto top_item = query_table->takeItem(row - 1, 1);
	auto bottom_item = query_table->takeItem(row, 1);
	query_table->setItem(row, 1, top_item);
	query_table->setItem(row - 1, 1, bottom_item);
	top_item = query_table->takeItem(row - 1, 3);
	bottom_item = query_table->takeItem(row, 3);
	query_table->setItem(row, 3, top_item);
	query_table->setItem(row - 1, 3, bottom_item);

	// Col 2 is comparison combobox
	auto top_combo = qobject_cast<QComboBox*>(query_table->cellWidget(row - 1, 2));
	auto bottom_combo = qobject_cast<QComboBox*>(query_table->cellWidget(row, 2));
	auto top_selection = top_combo->currentIndex();
	auto bottom_selection = bottom_combo->currentIndex();
	top_combo->setCurrentIndex(bottom_selection);
	bottom_combo->setCurrentIndex(top_selection);

	// Ignore the swap on the first row because there is nothing to swap
	if (row - 1 != 0 )
	{
		// Col 0 is relation combobox
		top_combo = qobject_cast<QComboBox*>(query_table->cellWidget(row - 1, 0));
		bottom_combo = qobject_cast<QComboBox*>(query_table->cellWidget(row, 0));
		top_selection = top_combo->currentIndex();
		bottom_selection = bottom_combo->currentIndex();
		top_combo->setCurrentIndex(bottom_selection);
		bottom_combo->setCurrentIndex(top_selection);
	}

	// Keep the moved row selected
	int col = query_table->currentColumn();
	// For the down case we already adjusted the row
	if (up)
		row -= 1;
	query_table->setCurrentCell(row, col);
}



void TagSelectWidget::makeSelection()
{
	query_table->setCurrentItem(nullptr);
	if (auto query = makeQuery())
	{
		query.selectMatchingObjects(map, controller);
		selection_info->setText(tr("%n object(s) selected", nullptr, map->getNumSelectedObjects()));
	}
	else
	{
		selection_info->setText(tr("Invalid query"));
	}
}



ObjectQuery TagSelectWidget::makeQuery() const
{
	// If there is at least one OR, query will become an OR expression.
	ObjectQuery query;
	// AND has precedence over OR, so its terms are collected separately.
	ObjectQuery and_expression;

	int rowCount = query_table->rowCount();
	for (int row = 0; row < rowCount; ++row)
	{
		auto key = query_table->item(row, 1)->text();
		auto compare_op = qobject_cast<QComboBox*>(query_table->cellWidget(row, 2))->currentData().value<ObjectQuery::Operator>();
		auto value = query_table->item(row, 3)->text();

		auto comparison = ObjectQuery(key, compare_op, value);
		if (row == 0)
		{
			// The first term at all
			and_expression = std::move(comparison);
		}
		else
		{
			auto logical_op = qobject_cast<QComboBox*>(query_table->cellWidget(row, 0))->currentData().value<ObjectQuery::Operator>();
			if (logical_op == ObjectQuery::OperatorAnd)
			{
				// Add another term to the AND expression
				and_expression = ObjectQuery(std::move(comparison), ObjectQuery::OperatorAnd, std::move(and_expression));
			}
			else if (logical_op == ObjectQuery::OperatorOr)
			{
				if (query)
					// Add another term to the OR expression
					query = ObjectQuery(std::move(and_expression), ObjectQuery::OperatorOr, std::move(query));
				else
					// The first term of an OR expression
					query = std::move(and_expression);
				
				// The first term of the next AND expression
				and_expression = std::move(comparison);
			}
			else
			{
				Q_UNREACHABLE();
			}
		}
		
		// The validness of and_expression is determined by the current line.
		if (!and_expression)
			break;
	}
	
	if (query && and_expression)
	{
		// Add the last AND expression to the OR expression
		query = ObjectQuery(std::move(and_expression), ObjectQuery::OperatorOr, std::move(query));
	}
	else
	{
		// There was no OR, or the last visited row was not a valid expression.
		query = std::move(and_expression);
	}

	return query;
}
