/*
 *    Copyright 2016 Mitchell Krome
 *    Copyright 2017-2019, 2025 Kai Pastor
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

#include <initializer_list>
#include <utility>

#include <Qt>
#include <QtGlobal>
#include <QAbstractButton>
#include <QAbstractItemView>
#include <QBrush>
#include <QComboBox>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QLatin1String>
#include <QPalette>
#include <QShowEvent>
#include <QStringList>
#include <QToolButton>
#include <QTableWidgetItem>
#include <QVariant>
#include <QWidget>

#include "core/objects/object_query.h"
#include "gui/util_gui.h"
#include "gui/widgets/segmented_button_layout.h"


namespace OpenOrienteering {

namespace {

/// Local wrapper for Util::ToolButton::create() adding the What's This bit.
QToolButton* createToolButton(const QIcon& icon, const QString& text)
{
	return Util::ToolButton::create(icon, text, "find_objects.html#query-editor");
}

}  // anonymous namespace


// ### TagSelectWidget ###

TagSelectWidget::TagSelectWidget(QWidget* parent)
: QTableWidget{1, 4, parent}
{
	setWhatsThis(Util::makeWhatThis("find_objects.html#query-editor"));
	
	setEditTriggers(QAbstractItemView::AllEditTriggers);
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setSelectionMode(QAbstractItemView::SingleSelection);
	setHorizontalHeaderLabels(QStringList() << tr("Relation") << tr("Key") << tr("Comparison") << tr("Value"));
	verticalHeader()->setVisible(false);
	
	auto* header_view = horizontalHeader();
	header_view->setSectionsClickable(false);
	header_view->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	header_view->setSectionResizeMode(1, QHeaderView::Stretch);
	header_view->setSectionResizeMode(2, QHeaderView::ResizeToContents);
	header_view->setSectionResizeMode(3, QHeaderView::Stretch);
	header_view->resizeSections(QHeaderView::ResizeToContents);
	setMinimumWidth(150
	                + header_view->sectionSize(0)
	                + header_view->sectionSize(1)
	                + header_view->sectionSize(2)
	                + header_view->sectionSize(3));
	
	addRowItems(0);

	connect(this, &QTableWidget::cellChanged, this, &TagSelectWidget::onCellChanged);
	connect(this, &QTableWidget::currentCellChanged, this, &TagSelectWidget::updateRowButtons, Qt::QueuedConnection);
}


TagSelectWidget::~TagSelectWidget() = default;



QWidget* TagSelectWidget::makeButtons(QWidget* parent)
{
	auto* add_button = createToolButton(QIcon(QString::fromLatin1(":/images/plus.png")), tr("Add Row"));
	delete_button = createToolButton(QIcon(QString::fromLatin1(":/images/minus.png")), tr("Remove Row"));
	delete_button->setEnabled(false);
	
	auto* add_remove_layout = new SegmentedButtonLayout();
	add_remove_layout->addWidget(add_button);
	add_remove_layout->addWidget(delete_button);

	move_up_button = createToolButton(QIcon(QString::fromLatin1(":/images/arrow-up.png")), tr("Move Up"));
	move_up_button->setAutoRepeat(true);
	move_up_button->setEnabled(false);
	move_down_button = createToolButton(QIcon(QString::fromLatin1(":/images/arrow-down.png")), tr("Move Down"));
	move_down_button->setAutoRepeat(true);
	move_down_button->setEnabled(false);

	auto* up_down_layout = new SegmentedButtonLayout();
	up_down_layout->addWidget(move_up_button);
	up_down_layout->addWidget(move_down_button);
	
	auto* list_buttons_layout = new QHBoxLayout();
	list_buttons_layout->setContentsMargins(0,0,0,0);
	list_buttons_layout->addLayout(add_remove_layout);
	list_buttons_layout->addLayout(up_down_layout);
	
	auto* list_buttons_group = new QWidget(parent);
	list_buttons_group->setLayout(list_buttons_layout);
	
	// Connections
	connect(add_button, &QAbstractButton::clicked, this, &TagSelectWidget::addRow);
	connect(delete_button, &QAbstractButton::clicked, this, &TagSelectWidget::deleteRow);
	connect(move_up_button, &QAbstractButton::clicked, this, &TagSelectWidget::moveRowUp);
	connect(move_down_button, &QAbstractButton::clicked, this, &TagSelectWidget::moveRowDown);
	
	return list_buttons_group;
}


void TagSelectWidget::showEvent(QShowEvent* event)
{
	if (!event->spontaneous())
	{
		auto last_row = rowCount() - 1;
		setCurrentCell(last_row, 1);
		if (!currentItem()->text().isEmpty())
			setCurrentCell(last_row, 3);
		editItem(currentItem());
	}
}



void TagSelectWidget::addRowItems(int row)
{
	auto* item = new QTableWidgetItem();
	setItem(row, 1, item);
	item = new QTableWidgetItem();
	setItem(row, 3, item);

	auto* compare_op = new QComboBox();
	for (auto op : { ObjectQuery::OperatorIs, ObjectQuery::OperatorIsNot, ObjectQuery::OperatorContains })
		compare_op->addItem(ObjectQuery::labelFor(op), QVariant::fromValue(op));
	setCellWidget(row, 2, compare_op);

	if (row == 0)
	{
		// The first row doesn't use a logical operator
		item = new QTableWidgetItem();
		item->setFlags(Qt::NoItemFlags);
		item->setBackground(QBrush(QGuiApplication::palette().window()));
		setItem(row, 0, item);
	}
	else
	{
		auto* logical_op = new QComboBox();
		auto and_label = QString { QLatin1String("  ") + ObjectQuery::labelFor(ObjectQuery::OperatorAnd) };
		logical_op->addItem(and_label, QVariant::fromValue(ObjectQuery::OperatorAnd));
		logical_op->addItem(ObjectQuery::labelFor(ObjectQuery::OperatorOr), QVariant::fromValue(ObjectQuery::OperatorOr));
		setCellWidget(row, 0, logical_op);
	}
}



void TagSelectWidget::onCellChanged(int row, int column)
{
	switch(column)
	{
	case 1:
	case 3:
		if (auto* item = this->item(row, column))
			item->setText(item->text().trimmed());
		break;
	default:
		; // nothing
	}
}


void TagSelectWidget::updateRowButtons()
{
	auto current_row = currentRow();
	move_up_button->setEnabled(current_row > 0);
	move_down_button->setEnabled(current_row+1 < rowCount());
}



void TagSelectWidget::addRow()
{
	int row = currentRow() + 1;

	// When nothing is selected, add to the end
	if (row == 0)
		row = rowCount();

	insertRow(row);
	addRowItems(row);
	delete_button->setEnabled(rowCount() > 1);

	// Move the selection to the new row
	int col = currentColumn();
	setCurrentCell(row, col);
	resizeColumnToContents(0);
	resizeColumnToContents(2);
}


void TagSelectWidget::deleteRow()
{
	// Always need one row
	if (rowCount() == 1)
		return;

	int row = currentRow();

	// When nothing is selected, delete from the end
	if (row == -1)
		row = rowCount() - 1;

	removeRow(row);
	delete_button->setEnabled(rowCount() > 1);

	// If we delete first row, need to fix logical operator
	if (row == 0)
	{
		removeCellWidget(row, 0);
		auto* item = new QTableWidgetItem();
		item->setFlags(Qt::NoItemFlags);
		item->setBackground(QBrush(QGuiApplication::palette().window()));
		setItem(row, 0, item);
	}
}


void TagSelectWidget::moveRow(bool up)
{
	int row = currentRow();
	int max_row = rowCount();

	// When nothing is selected, move the last row
	if (row == -1)
		row = max_row - 1;

	// Can't move first row up or last row down
	if ((up && row < 1) || (!up && row == max_row - 1))
		return;

	// Moving the current row down is just moving the row below up.. 
	if (!up)
		row += 1;

	// Commit current cell by changing the index.
	auto const col = currentColumn();
	setCurrentCell(row > 0 ? 0 : 1, col);
	
	// Col 1, 3 are items
	auto* top_item = takeItem(row - 1, 1);
	auto* bottom_item = takeItem(row, 1);
	setItem(row, 1, top_item);
	setItem(row - 1, 1, bottom_item);
	top_item = takeItem(row - 1, 3);
	bottom_item = takeItem(row, 3);
	setItem(row, 3, top_item);
	setItem(row - 1, 3, bottom_item);

	// Col 2 is comparison combobox
	auto* top_combo = qobject_cast<QComboBox*>(cellWidget(row - 1, 2));
	auto* bottom_combo = qobject_cast<QComboBox*>(cellWidget(row, 2));
	auto top_selection = top_combo->currentIndex();
	auto bottom_selection = bottom_combo->currentIndex();
	top_combo->setCurrentIndex(bottom_selection);
	bottom_combo->setCurrentIndex(top_selection);

	// Ignore the swap on the first row because there is nothing to swap
	if (row - 1 != 0 )
	{
		// Col 0 is relation combobox
		top_combo = qobject_cast<QComboBox*>(cellWidget(row - 1, 0));
		bottom_combo = qobject_cast<QComboBox*>(cellWidget(row, 0));
		top_selection = top_combo->currentIndex();
		bottom_selection = bottom_combo->currentIndex();
		top_combo->setCurrentIndex(bottom_selection);
		bottom_combo->setCurrentIndex(top_selection);
	}

	// For the down case we already adjusted the row
	if (up)
		row -= 1;
	// Keep the moved row selected
	setCurrentCell(row, col);
}


void TagSelectWidget::moveRowDown()
{
	moveRow(false);
}


void TagSelectWidget::moveRowUp()
{
	moveRow(true);
}



ObjectQuery TagSelectWidget::makeQuery() const
{
	// If there is at least one OR, query will become an OR expression.
	ObjectQuery query;
	// AND has precedence over OR, so its terms are collected separately.
	ObjectQuery and_expression;

	for (int row = 0; row < rowCount(); ++row)
	{
		auto key = item(row, 1)->text();
		auto compare_op = qobject_cast<QComboBox*>(cellWidget(row, 2))->currentData().value<ObjectQuery::Operator>();
		auto value = item(row, 3)->text();

		auto comparison = ObjectQuery(key, compare_op, value);
		if (row == 0)
		{
			// The first term at all
			and_expression = std::move(comparison);
		}
		else
		{
			auto logical_op = qobject_cast<QComboBox*>(cellWidget(row, 0))->currentData().value<ObjectQuery::Operator>();
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


}  // namespace OpenOrienteering
