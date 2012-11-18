/*
 *    Copyright 2012 Thomas Schöps, Kai Pastor
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


#include "color_dock_widget.h"

#if QT_VERSION < 0x050000
#include <QtGui>
#else
#include <QtWidgets>
#endif

#include "map.h"
#include "map_color.h"

ColorWidget::ColorWidget(Map* map, MainWindow* window, QWidget* parent)
: EditorDockWidgetChild(parent), 
  map(map), 
  window(window)
{
	react_to_changes = true;
	
	// Color table
	color_table = new QTableWidget(map->getNumColors(), 10);
	color_table->setWhatsThis("<a href=\"symbols.html#colors\">See more</a>");
	color_table->setEditTriggers(QAbstractItemView::AllEditTriggers);
	color_table->setHorizontalHeaderLabels(QStringList() << "" << tr("Name") << tr("C") << tr("M") << tr("Y") << tr("K") << tr("Opacity") << tr("R") << tr("G") << tr("B"));
	color_table->verticalHeader()->setVisible(false);
	
	QHeaderView* header_view = color_table->horizontalHeader();
	header_view->setResizeMode(0, QHeaderView::Fixed);
	header_view->resizeSection(0, 40);
	header_view->setResizeMode(1, QHeaderView::Stretch);
	for (int i = 2; i < 10; ++i)
		header_view->setResizeMode(i, QHeaderView::ResizeToContents);
	header_view->setClickable(false);
	
	for (int i = 0; i < map->getNumColors(); ++i)
		addRow(i);
	
	// Buttons
	QWidget* buttons_group = new QWidget();
	
	QPushButton* new_button = new QPushButton(QIcon(":/images/plus.png"), tr("New"));
	new_button->setWhatsThis("<a href=\"symbols.html#colors\">See more</a>");
	delete_button = new QPushButton(QIcon(":/images/minus.png"), tr("Delete"));
	delete_button->setWhatsThis("<a href=\"symbols.html#colors\">See more</a>");
	duplicate_button = new QPushButton(QIcon(":/images/copy.png"), tr("Duplicate"));
	duplicate_button->setWhatsThis("<a href=\"symbols.html#colors\">See more</a>");
	move_up_button = new QPushButton(QIcon(":/images/arrow-up.png"), tr("Move Up"));
	move_up_button->setWhatsThis("<a href=\"symbols.html#colors\">See more</a>");
	move_down_button = new QPushButton(QIcon(":/images/arrow-down.png"), tr("Move Down"));
	move_down_button->setWhatsThis("<a href=\"symbols.html#colors\">See more</a>");
	QPushButton* help_button = new QPushButton(QIcon(":/images/help.png"), tr("Help"));

	QGridLayout* buttons_group_layout = new QGridLayout();
	buttons_group_layout->setMargin(0);
	buttons_group_layout->addWidget(new_button, 0, 0);
	buttons_group_layout->addWidget(delete_button, 0, 1);
	buttons_group_layout->addWidget(duplicate_button, 1, 0, 1, 2);
	buttons_group_layout->setRowStretch(2, 1);
	buttons_group_layout->addWidget(move_up_button, 3, 1);
	buttons_group_layout->addWidget(move_down_button, 3, 0);
	buttons_group_layout->setRowStretch(4, 1);
	buttons_group_layout->addWidget(help_button, 5, 0, 1, 2);
	buttons_group->setLayout(buttons_group_layout);
	
	currentCellChange(color_table->currentRow(), 0, 0, 0);	// enable / disable move color buttons
	
	// Create main layout
	QBoxLayout* layout = new QVBoxLayout();
	layout->setMargin(0);
	layout->addWidget(color_table, 1);
	layout->addWidget(buttons_group);
	setLayout(layout);
	
	// Connections
	connect(color_table, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(showEditingInfo(int,int)));
	connect(color_table, SIGNAL(cellChanged(int,int)), this, SLOT(cellChange(int,int)));
	connect(color_table, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(currentCellChange(int,int,int,int)));
	connect(color_table, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(cellDoubleClick(int,int)));
	
	connect(new_button, SIGNAL(clicked(bool)), this, SLOT(newColor()));
	connect(delete_button, SIGNAL(clicked(bool)), this, SLOT(deleteColor()));
	connect(duplicate_button, SIGNAL(clicked(bool)), this, SLOT(duplicateColor()));
	connect(move_up_button, SIGNAL(clicked(bool)), this, SLOT(moveColorUp()));
	connect(move_down_button, SIGNAL(clicked(bool)), this, SLOT(moveColorDown()));
	connect(help_button, SIGNAL(clicked(bool)), this, SLOT(showHelp()));
	
	connect(map, SIGNAL(colorAdded(int,MapColor*)), this, SLOT(colorAdded(int,MapColor*)));
	connect(map, SIGNAL(colorDeleted(int,MapColor*)), this, SLOT(colorDeleted(int,MapColor*)));
}

ColorWidget::~ColorWidget()
{
}

QSize ColorWidget::sizeHint() const
{
    return QSize(450, 300);
}

void ColorWidget::newColor()
{
	int row = color_table->currentRow();
	if (row < 0)
		row = color_table->rowCount();
	map->addColor(row);
	
	map->updateAllObjects();
}

void ColorWidget::deleteColor()
{
	int row = color_table->currentRow();
	Q_ASSERT(row >= 0);
	
	// Show a warning if the color is used
	if (map->isColorUsedByASymbol(map->getColor(row)))
	{
		if (QMessageBox::warning(this, tr("Confirmation"), tr("The map contains symbols with this color. Deleting it will remove the color from these objects! Do you really want to do that?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
			return;
	}
	
	map->deleteColor(row);
	
	map->setColorsDirty();
	map->updateAllObjects();
}

void ColorWidget::duplicateColor()
{
	int row = color_table->currentRow();
	Q_ASSERT(row >= 0);
	
	MapColor* new_color = new MapColor(*map->getColor(row));
	new_color->name = new_color->name + tr(" (Duplicate)");
	map->addColor(new_color, row);
	
	map->updateAllObjects();
}

void ColorWidget::moveColorUp()
{
	int row = color_table->currentRow();
	Q_ASSERT(row >= 1);
	
	MapColor* above_color = map->getColor(row - 1);
	MapColor* cur_color = map->getColor(row);
	map->setColor(cur_color, row - 1);
	map->setColor(above_color, row);
	updateRow(row - 1);
	updateRow(row);
	
	color_table->setCurrentCell(row - 1, color_table->currentColumn());
	
	map->setColorsDirty();
	map->updateAllObjects();
}

void ColorWidget::moveColorDown()
{
	int row = color_table->currentRow();
	Q_ASSERT(row < color_table->rowCount() - 1);
	
	MapColor* below_color = map->getColor(row + 1);
	MapColor* cur_color = map->getColor(row);
	map->setColor(cur_color, row + 1);
	map->setColor(below_color, row);
	updateRow(row + 1);
	updateRow(row);
	
	color_table->setCurrentCell(row + 1, color_table->currentColumn());
	
	map->setColorsDirty();
	map->updateAllObjects();
}

void ColorWidget::showHelp()
{
	window->showHelp("symbols.html", "colors");
}

void ColorWidget::showEditingInfo(int row, int column)
{
	Q_UNUSED(row);
	
	if (column >= 2 && column <= 9)
		window->statusBar()->showMessage(tr("Enter a number from 0 to 255, or a percentage from 0% to 100%."), 3000);
	else
		window->statusBar()->clearMessage();
}

void ColorWidget::cellChange(int row, int column)
{
	if (!react_to_changes)
		return;
	react_to_changes = false;
	
	window->statusBar()->clearMessage();
	
	MapColor* color = map->getColor(row);
	QString text = color_table->item(row, column)->text().trimmed();
	
	if (column == 1)
		color->name = text;
	else if (column >= 2 && column <= 9)
	{
		bool ok = true;
		float fvalue;
		
		if (text.endsWith('%'))
		{
			text.chop(1);
			fvalue = text.toFloat(&ok) / 100.0f;
		}
		else
			fvalue = text.toFloat(&ok) / 255.0f;
		if (fvalue < 0 || fvalue > 1)
			ok = false;
		
		if (!ok)
		{
			QMessageBox::warning(window, tr("Error"), tr("Please enter a valid number from 0 to 255, or specify a percentage from 0% to 100%!"));
		
			if (column == 2)		color_table->item(row, column)->setText(QString::number(color->c * 100) + "%");
			else if (column == 3)	color_table->item(row, column)->setText(QString::number(color->m * 100) + "%");
			else if (column == 4)	color_table->item(row, column)->setText(QString::number(color->y * 100) + "%");
			else if (column == 5)	color_table->item(row, column)->setText(QString::number(color->k * 100) + "%");
			else if (column == 6)	color_table->item(row, column)->setText(QString::number(color->opacity * 100) + "%");
			else if (column == 7)	color_table->item(row, column)->setText(QString::number(color->r * 100) + "%");
			else if (column == 8)	color_table->item(row, column)->setText(QString::number(color->g * 100) + "%");
			else if (column == 9)	color_table->item(row, column)->setText(QString::number(color->b * 100) + "%");
		}
		else
		{
			if (column == 2)		color->c = fvalue;
			else if (column == 3)	color->m = fvalue;
			else if (column == 4)	color->y = fvalue;
			else if (column == 5)	color->k = fvalue;
			else if (column == 6)	color->opacity = fvalue;
			else if (column == 7)	color->r = fvalue;
			else if (column == 8)	color->g = fvalue;
			else if (column == 9)	color->b = fvalue;
			
			if (column <= 5)
				color->updateFromCMYK();
			else if (column >= 7)
				color->updateFromRGB();
			updateRow(row);
		}
	}
	
	react_to_changes = true;
	
	map->setColor(color, row); // trigger colorChanged signal
	map->setColorsDirty();
	map->updateAllObjects();
}

void ColorWidget::currentCellChange(int current_row, int current_column, int previous_row, int previous_column)
{
	if (!react_to_changes)
		return;
	
	delete_button->setEnabled(current_row >= 0);
	duplicate_button->setEnabled(current_row >= 0);
	move_up_button->setEnabled(current_row >= 1);
	move_down_button->setEnabled(current_row < color_table->rowCount() - 1 && current_row != -1);
}

void ColorWidget::cellDoubleClick(int row, int column)
{
	if (column == 0)
	{
		MapColor* color = map->getColor(row);
		
		QColor newColor = QColorDialog::getColor(color->color, window);
		if (newColor.isValid())
		{
			color->color = newColor;
			qreal r, g, b;
			color->color.getRgbF(&r, &g, &b);
			color->r = r;
			color->g = g;
			color->b = b;
			color->updateFromRGB();
			
			updateRow(row);
			
			map->setColor(color, row); // trigger colorChanged signal
			map->setColorsDirty();
			map->updateAllObjects();
		}
	}
}

void ColorWidget::colorAdded(int index, MapColor* color)
{
	color_table->insertRow(index);
	addRow(index);
	color_table->setCurrentCell(index, 1);
}

void ColorWidget::colorDeleted(int index, MapColor* color)
{
	color_table->removeRow(index);
	currentCellChange(color_table->currentRow(), color_table->currentColumn(), -1, -1);
}

void ColorWidget::addRow(int row)
{
	react_to_changes = false;
	
	QTableWidgetItem* item = new QTableWidgetItem();
	item->setFlags(Qt::ItemIsEnabled);
	item->setToolTip(tr("Double click to pick a color"));
	color_table->setItem(row, 0, item);
	
	for (int i = 1; i <= 9; ++i)
		color_table->setItem(row, i, new QTableWidgetItem());
	
	react_to_changes = true;
	
	updateRow(row);
}

void ColorWidget::updateRow(int row)
{
	react_to_changes = false;
	
	MapColor* color = map->getColor(row);
	
	color_table->item(row, 0)->setBackgroundColor(color->color);
	
	color_table->item(row, 1)->setText(color->name);
	
	color_table->item(row, 2)->setText(QString::number(color->c * 100) + "%");
	color_table->item(row, 3)->setText(QString::number(color->m * 100) + "%");
	color_table->item(row, 4)->setText(QString::number(color->y * 100) + "%");
	color_table->item(row, 5)->setText(QString::number(color->k * 100) + "%");
	
	color_table->item(row, 6)->setText(QString::number(color->opacity * 100) + "%");
	
	color_table->item(row, 7)->setText(QString::number(color->r * 255));
	color_table->item(row, 8)->setText(QString::number(color->g * 255));
	color_table->item(row, 9)->setText(QString::number(color->b * 255));
	react_to_changes = true;
}
