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


#include "map_dialog_rotate.h"

#include "assert.h"

#include <QtGui>
#include <qmath.h>

#include "map.h"
#include "georeferencing.h"
#include "util_gui.h"

RotateMapDialog::RotateMapDialog(QWidget* parent, Map* map) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint), map(map)
{
	setWindowTitle(tr("Rotate map"));
	
	QLabel* rotate_label = new QLabel(tr("Angle (counter-clockwise):"));
	rotation_edit = Util::SpinBox::create(1, -999999, +999999, trUtf8("°"));
	
	adjust_georeferencing_check = new QCheckBox(tr("Adjust georeferencing reference point"));
	adjust_georeferencing_check->setChecked(true);
	adjust_declination_check = new QCheckBox(tr("Adjust georeferencing declination"));
	adjust_declination_check->setChecked(true);
	
	QDialogButtonBox* button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal);
	
	QHBoxLayout* scale_layout = new QHBoxLayout();
	scale_layout->addWidget(rotate_label);
	scale_layout->addWidget(rotation_edit);
	scale_layout->addStretch(1);
	
	QVBoxLayout* layout = new QVBoxLayout();
	layout->addLayout(scale_layout);
	layout->addWidget(adjust_georeferencing_check);
	layout->addWidget(adjust_declination_check);
	layout->addSpacing(16);
	layout->addWidget(button_box);
	setLayout(layout);
	
	connect(button_box, SIGNAL(accepted()), this, SLOT(okClicked()));
	connect(button_box, SIGNAL(rejected()), this, SLOT(reject()));
}

void RotateMapDialog::okClicked()
{
	double rotation = M_PI * rotation_edit->value() / 180;
	map->rotateMap(rotation, adjust_georeferencing_check->isChecked(), adjust_declination_check->isChecked());
	accept();
}
