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

#include "map.h"
#include "map_color.h"
#include "util_gui.h"
#include "gui/color_dialog.h"

ColorWidget::ColorWidget(Map* map, MainWindow* window, QWidget* parent)
: QWidget(parent), 
  map(map), 
  window(window)
{
	react_to_changes = true;
	
	// Color table
	color_table = new QTableWidget(map->getNumColors(), 6);
	color_table->setWhatsThis("<a href=\"symbols.html#colors\">See more</a>");
	color_table->setEditTriggers(QAbstractItemView::AllEditTriggers);
	color_table->verticalHeader()->setVisible(false);
	color_table->setHorizontalHeaderLabels(QStringList() <<
	  "" << tr("Name") << tr("Spot color") << tr("CMYK") << tr("RGB") << tr("K.o.") << tr("Opacity") );
	
	// Buttons
	QAbstractButton* new_button = newToolButton(QIcon(":/images/plus.png"), tr("New"));
	delete_button = newToolButton(QIcon(":/images/minus.png"), tr("Delete"));
	duplicate_button = newToolButton(QIcon(":/images/copy.png"), tr("Duplicate"));
	move_up_button = newToolButton(QIcon(":/images/arrow-up.png"), tr("Move Up"));
	move_down_button = newToolButton(QIcon(":/images/arrow-down.png"), tr("Move Down"));
 	QAbstractButton* help_button = newToolButton(QIcon(":/images/help.png"), tr("Help"));
	
	QBoxLayout* buttons_group_layout = new QHBoxLayout();
	buttons_group_layout->setMargin(0);
	buttons_group_layout->addWidget(new_button);
	buttons_group_layout->addWidget(move_up_button);
	buttons_group_layout->addWidget(move_down_button);
	buttons_group_layout->addWidget(duplicate_button);
	buttons_group_layout->addWidget(delete_button);
	buttons_group_layout->addWidget(help_button);
	
	// Create main layout
	QBoxLayout* layout = new QVBoxLayout();
	layout->setMargin(0);
	layout->addWidget(color_table, 1);
	layout->addLayout(buttons_group_layout);
	layout->addWidget(new QLabel(tr("Double-click a color value to open a dialog.")));
	layout->addSpacing(2);
	setLayout(layout);
	
	for (int i = 0; i < map->getNumColors(); ++i)
		addRow(i);
	
	QHeaderView* header_view = color_table->horizontalHeader();
#if QT_VERSION < 0x050000
	header_view->setResizeMode(QHeaderView::Interactive);
	header_view->resizeSections(QHeaderView::ResizeToContents);
	header_view->setResizeMode(0, QHeaderView::Fixed); // Color
	header_view->resizeSection(0, 32);
	header_view->setResizeMode(1, QHeaderView::Stretch); // Name
	header_view->setResizeMode(5, QHeaderView::Fixed); // Knockout
	header_view->resizeSection(5, 32);
#else
	header_view->setSectionResizeMode(QHeaderView::Interactive);
	header_view->resizeSections(QHeaderView::ResizeToContents);
	header_view->setSectionResizeMode(0, QHeaderView::Fixed); // Color
	header_view->resizeSection(0, 32);
	header_view->setSectionResizeMode(1, QHeaderView::Stretch); // Name
	header_view->setSectionResizeMode(5, QHeaderView::Fixed); // Knockout
	header_view->resizeSection(5, 32);
#endif
	
	currentCellChange(color_table->currentRow(), 0, 0, 0);	// enable / disable move color buttons
	
	// Connections
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

QAbstractButton* ColorWidget::newToolButton(const QIcon& icon, const QString& text, QAbstractButton* prototype)
{
	if (prototype == NULL)
	{
		QToolButton* button = new QToolButton();
		button->setAutoRaise(true);
		button->setToolButtonStyle(Qt::ToolButtonFollowStyle);
		prototype = button;
	}
	prototype->setIcon(icon);
	prototype->setText(text);
	prototype->setWhatsThis("<a href=\"symbols.html#colors\">See more</a>");
	return prototype;
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
	new_color->setName(new_color->getName() + tr(" (Duplicate)"));
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

void ColorWidget::cellChange(int row, int column)
{
	if (!react_to_changes)
		return;
	
	react_to_changes = false;
	
	MapColor* color = map->getColor(row);
	QString text = color_table->item(row, column)->text().trimmed();
	
	if (column == 1)
	{
		color->setName(text);
		react_to_changes = true;
	}
	else if (column == 6) // Opacity
	{
		bool ok = true;
		float fvalue;
		
		if (text.endsWith('%'))
			text.chop(1);
		
		fvalue = text.toFloat(&ok) / 100.0f;
		
		if (fvalue < 0.0f || fvalue > 1.0f)
			ok = false;
		
		if (ok)
		{
			color->setOpacity(fvalue);
			updateRow(row);
		}
		else
		{
			QMessageBox::warning(window, tr("Error"), tr("Please enter a percentage from 0% to 100%!"));
			color_table->item(row, column)->setText(QString::number(color->getOpacity() * 100) + "%");
		}
		react_to_changes = true;
	}
	else 
	{
		react_to_changes = true;
		return;
	}
	
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
	if (column != 1)
	{
		MapColor* color = map->getColor(row);
		ColorDialog dialog(*map, *color, this);
		dialog.setWindowModality(Qt::WindowModal);
		connect(&dialog, SIGNAL(showHelp(QString,QString)), window, SLOT(showHelp(QString,QString)));
		int result = dialog.exec();
		if (result == QDialog::Accepted)
		{
			*color = dialog.getColor();
			map->setColor(color, row); // trigger colorChanged signal
			map->setColorsDirty();
			map->updateAllObjects();
			updateRow(row);
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
	
	for (int i = 0; i < 6; ++i)
		color_table->setItem(row, i, new QTableWidgetItem());
	
	react_to_changes = true;
	
	updateRow(row);
}

void ColorWidget::updateRow(int row)
{
	react_to_changes = false;
	
	MapColor* color = map->getColor(row);
	
	// Color preview
	QTableWidgetItem* item = color_table->item(row, 0);
	item->setFlags(Qt::ItemIsEnabled);
	item->setBackgroundColor(*color);
	
	// Name
	item = color_table->item(row, 1);
	item->setText(color->getName());
	item->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled);
	
	// Spot color
	item = color_table->item(row, 2);
	item->setText(color->getSpotColorName());
	item->setFlags(Qt::ItemIsEnabled);
	item->setToolTip(tr("Double click to define the color"));
	
	// CMYK
	item = color_table->item(row, 3);
	item->setFlags(Qt::ItemIsEnabled);
	item->setToolTip(tr("Double click to define the color"));
	const MapColorCmyk& cmyk = color->getCmyk();
	item->setText(QString("%1/%2/%3/%4").arg(100*cmyk.c,0,'g',3).arg(100*cmyk.m,0,'g',3).arg(100*cmyk.y,0,'g',3).arg(100*cmyk.k,0,'g',3));
	switch (color->getCmykColorMethod())
	{
		case MapColor::SpotColor:
		case MapColor::RgbColor:
			item->setForeground(Qt::gray);
			break;
		default:
			item->setForeground(Qt::black);
	}
	
	// RGB
	item = color_table->item(row, 4);
	item->setFlags(Qt::ItemIsEnabled);
	item->setText(QColor(color->getRgb()).name());
	item->setToolTip(item->text());
	switch (color->getRgbColorMethod())
	{
		case MapColor::SpotColor:
		case MapColor::CmykColor:
			item->setForeground(Qt::gray);
			break;
		default:
			item->setForeground(Qt::black);
	}
	
	// Knockout
	item = color_table->item(row, 5);
	item->setFlags(0);
	item->setCheckState(color->getKnockout() ? Qt::Checked : Qt::Unchecked);
	item->setText("");
	
// 	color_table->item(row, 6)->setText(QString::number(color->getOpacity() * 100) + "%");
	
	react_to_changes = true;
}
