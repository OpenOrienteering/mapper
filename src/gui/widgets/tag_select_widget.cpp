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
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QTableWidget>
#include <QToolButton>

#include "segmented_button_layout.h"
#include "../main_window.h"
#include "../../map.h"
#include "../../map_editor.h"
#include "../../map_part.h"
#include "../../object.h"
#include "../../tool.h"
#include "../../util.h"

// ### TagSelectWidget ###

TagSelectWidget::TagSelectWidget(Map* map, MapView* main_view, MapEditorController* controller, QWidget* parent)
: QWidget(parent)
, map(map)
, main_view(main_view)
, controller(controller)
{
	Q_ASSERT(main_view);
	Q_ASSERT(controller);
	
	setWhatsThis(Util::makeWhatThis("tag_selector.html"));
	
	QStyleOption style_option(QStyleOption::Version, QStyleOption::SO_DockWidget);
	
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

	// Call moveRow using lambda with up true or false. 
	connect(move_up_button, &QAbstractButton::clicked, this, [this]{ moveRow(true); });
	connect(move_down_button, &QAbstractButton::clicked, this, [this]{ moveRow(false); });

	connect(select_button, &QAbstractButton::clicked, this, &TagSelectWidget::makeSelection);

	connect(help_button, &QAbstractButton::clicked, this, &TagSelectWidget::showHelp);
}

TagSelectWidget::~TagSelectWidget()
{
	; // Nothing
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
	compare_op->addItem(tr("is"), QVariant::fromValue(IS_OP));
	compare_op->addItem(tr("contains"), QVariant::fromValue(CONTAINS_OP));
	compare_op->addItem(tr("is not"), QVariant::fromValue(NOT_OP));
	query_table->setCellWidget(row, 2, compare_op);

	// Special case to handle the first logical operator which is grayed out
	if (row != 0)
	{
		QComboBox* logical_op = new QComboBox();
		logical_op->addItem(tr("or"), QVariant::fromValue(OR_OP));
		logical_op->addItem(tr("and"), QVariant::fromValue(AND_OP));
		query_table->setCellWidget(row, 0, logical_op);
	}
	else
	{
		item = new QTableWidgetItem();
		item->setFlags(Qt::NoItemFlags);
		item->setBackground(QBrush(Qt::darkGray));
		query_table->setItem(row, 0, item);
	}
}

void TagSelectWidget::addRow()
{
	int row = query_table->currentRow();

	// When nothing is selected, currentRow is -1
	if (row == -1)
		row = 0;

	query_table->insertRow(row + 1);
	addRowItems(row + 1);

	// Move the selection to the new row
	int col = query_table->currentColumn();
	query_table->setCurrentCell(row + 1, col);
}

void TagSelectWidget::deleteRow()
{
	// Always need one row
	if (query_table->rowCount() == 1)
		return;

	int row = query_table->currentRow();

	// When nothing is selected, currentRow is -1
	if (row == -1)
		row = 0;

	query_table->removeRow(row);

	// If we delete first row, need to fix logical operator
	if (row == 0)
	{
		query_table->removeCellWidget(row, 0);
		QTableWidgetItem* item = new QTableWidgetItem();
		item->setFlags(Qt::NoItemFlags);
		item->setBackground(QBrush(Qt::darkGray));
		query_table->setItem(row, 0, item);
	}
}

void TagSelectWidget::moveRow(bool up)
{
	int row = query_table->currentRow();
	int max_row = query_table->rowCount();

	// When nothing is selected, currentRow is -1
	if (row == -1)
		row = 0;

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
	std::unique_ptr<QueryOperation> query;
	int rowCount = query_table->rowCount();
	enum Operation logical_op = INVALID_OP;

	for (int row = 0; row < rowCount; ++row)
	{
		const QString key = query_table->item(row, 1)->text().trimmed();
		enum Operation compare_op = qobject_cast<QComboBox*>(query_table->cellWidget(row, 2))->currentData().value<enum Operation>();
		const QString value = query_table->item(row, 3)->text().trimmed();

		std::unique_ptr<QueryOperation> comparison = std::unique_ptr<QueryOperation>(new QueryOperation(key, compare_op, value));

		if (comparison->getOp() == INVALID_OP)
		{
			selection_info->setText(tr("Invalid query"));
			return;
		}

		if (row != 0)
		{
			logical_op = qobject_cast<QComboBox*>(query_table->cellWidget(row, 0))->currentData().value<enum Operation>();
			query = std::unique_ptr<QueryOperation>(new QueryOperation(std::move(query), logical_op, std::move(comparison)));
		}
		else
		{
			query = std::move(comparison);
		}

		if (query->getOp() == INVALID_OP)
		{
			selection_info->setText(tr("Invalid query"));
			return;
		}
	}

	MapPart *part = map->getCurrentPart();
	int num_objects = part->getNumObjects();
	std::vector<Object*> matches;

	for (int i = 0; i < num_objects; ++i)
	{
		Object* obj = part->getObject(i);
		bool ret = query->evaluate(obj);
		if (ret)
			matches.push_back(obj);
	}

	bool selection_changed = false;
	if (map->getNumSelectedObjects() > 0)
		selection_changed = true;
	map->clearObjectSelection(false);

	bool object_selected = false;	
	for (auto obj : matches)
	{
		map->addObjectToSelection(obj, false);
		object_selected = true;
	}

	selection_changed |= object_selected;
	if (selection_changed)
		map->emitSelectionChanged();
	
	if (object_selected)
	{
		auto current_tool = controller->getTool();
		if (current_tool && current_tool->isDrawTool())
			controller->setEditTool();
	}

	QString msg = QString::number(matches.size()) + QLatin1String(" ") + tr("objects selected");
	selection_info->setText(msg);
}

// ### QueryOperation ###

QueryOperation::QueryOperation()
: op(INVALID_OP)
{
	; // Nothing
}

QueryOperation::QueryOperation(const QString& key, enum Operation op, const QString& value)
: op(op)
, key_arg(key)
, value_arg(value)
{
	// Must be a comparison op
	if (op != IS_OP && op != CONTAINS_OP && op != NOT_OP)
		this->op = INVALID_OP;

	// Can't have an empty key (can have empty value but)
	if (key.length() == 0)
		this->op = INVALID_OP;
}

QueryOperation::QueryOperation(std::unique_ptr<QueryOperation> left, enum Operation op, std::unique_ptr<QueryOperation> right)
: op(op)
, left_arg(std::move(left))
, right_arg(std::move(right))
{
	// Must be a logical op
	if (op != OR_OP && op != AND_OP)
		this->op = INVALID_OP;

	// Can't have null queries
	if (left_arg.get() == nullptr || right_arg.get() == nullptr)
		this->op = INVALID_OP;
}

enum Operation QueryOperation::getOp()
{
	return op;
}

bool QueryOperation::evaluate(Object* obj)
{
	const auto tags = obj->tags();
	bool left, right;

	switch(op)
	{
	case IS_OP:
		if (!tags.contains(key_arg))
			return false;
		return tags.value(key_arg) == value_arg;
	case CONTAINS_OP:
		if (!tags.contains(key_arg))
			return false;
		return tags.value(key_arg).contains(value_arg);
	case NOT_OP:
		// If the object does have the tag, not is true
		if (!tags.contains(key_arg))
			return true;
		return tags.value(key_arg) != value_arg;
	case OR_OP:
		left = left_arg->evaluate(obj);
		if (left)
			return true;
		right = right_arg->evaluate(obj);
		return right;
	case AND_OP:
		left = left_arg->evaluate(obj);
		if (!left)
			return false;
		right = right_arg->evaluate(obj);
		return right;
	case INVALID_OP:
		return false;
	}

	return false;
}
