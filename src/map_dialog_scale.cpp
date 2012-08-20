/*
 *    Copyright 2012 Thomas Schöps
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


#include "map_dialog_scale.h"

#include "assert.h"

#include <QtGui>

#include "map.h"
#include "georeferencing.h"

ScaleMapDialog::ScaleMapDialog(QWidget* parent, Map* map) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint), map(map)
{
	setWindowTitle(tr("Change map scale"));
	
	QLabel* scale_label = new QLabel(tr("New scale:  1 :"));
	scale_edit = new QLineEdit(QString::number(map->getScaleDenominator()));
	scale_edit->setValidator(new QIntValidator(1, 9999999, scale_edit));
	
	adjust_symbols_check = new QCheckBox(tr("Scale symbol sizes"));
	adjust_symbols_check->setChecked(true);
	adjust_objects_check = new QCheckBox(tr("Scale map object positions"));
	adjust_objects_check->setChecked(true);
	adjust_georeferencing_check = new QCheckBox(tr("Adjust georeferencing reference point"));
	adjust_georeferencing_check->setChecked(true);
	
	QPushButton* cancel_button = new QPushButton(tr("Cancel"));
	ok_button = new QPushButton(QIcon(":/images/arrow-right.png"), tr("Adjust"));
	ok_button->setDefault(true);
	
	QHBoxLayout* scale_layout = new QHBoxLayout();
	scale_layout->addWidget(scale_label);
	scale_layout->addWidget(scale_edit);
	scale_layout->addStretch(1);
	
	QHBoxLayout* buttons_layout = new QHBoxLayout();
	buttons_layout->addWidget(cancel_button);
	buttons_layout->addStretch(1);
	buttons_layout->addWidget(ok_button);
	
	QVBoxLayout* layout = new QVBoxLayout();
	layout->addLayout(scale_layout);
	layout->addWidget(adjust_symbols_check);
	layout->addWidget(adjust_objects_check);
	layout->addWidget(adjust_georeferencing_check);
	layout->addSpacing(16);
	layout->addLayout(buttons_layout);
	setLayout(layout);
	
	connect(cancel_button, SIGNAL(clicked(bool)), this, SLOT(reject()));
	connect(ok_button, SIGNAL(clicked(bool)), this, SLOT(okClicked()));
}

void ScaleMapDialog::okClicked()
{
	int scale = scale_edit->text().toInt();
	map->changeScale(scale, adjust_symbols_check->isChecked(), adjust_objects_check->isChecked(), adjust_georeferencing_check->isChecked());
	accept();
}
