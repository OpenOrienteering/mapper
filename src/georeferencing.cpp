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


#include "georeferencing.h"

#include <QtGui>

GeoreferencingDialog::GeoreferencingDialog(QWidget* parent, const GPSProjectionParameters* initial_values) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{
	setWindowTitle(tr("Georeferencing settings"));
	
	if (initial_values)
		params = *initial_values;
	
	QLabel* projection_label = new QLabel(tr("Orthographic projection for WGS84 (GPS) coordinates:"));
	projection_label->setAlignment(Qt::AlignCenter);
	QLabel* lat_label = new QLabel(tr("Origin latitude:"));
	lat_edit = new QLineEdit(QString::number(params.center_latitude * 180 / M_PI, 'f', 12));
	QLabel* lon_label = new QLabel(tr("Origin longitude:"));
	lon_edit = new QLineEdit(QString::number(params.center_longitude * 180 / M_PI, 'f', 12));
	
	QGridLayout* edit_layout = new QGridLayout();
	edit_layout->addWidget(projection_label, 0, 0, 1, 2);
	edit_layout->addWidget(lat_label, 1, 0);
	edit_layout->addWidget(lat_edit, 1, 1);
	edit_layout->addWidget(lon_label, 2, 0);
	edit_layout->addWidget(lon_edit, 2, 1);
	
	edit_layout->setRowStretch(7, 1);
	
	QPushButton* cancel_button = new QPushButton(tr("Cancel"));
	ok_button = new QPushButton(QIcon(":/images/arrow-right.png"), tr("OK"));
	ok_button->setDefault(true);
	
	QHBoxLayout* buttons_layout = new QHBoxLayout();
	buttons_layout->addWidget(cancel_button);
	buttons_layout->addStretch(1);
	buttons_layout->addWidget(ok_button);
	
	QVBoxLayout* layout = new QVBoxLayout();
	layout->addLayout(edit_layout);
	layout->addSpacing(16);
	layout->addLayout(buttons_layout);
	setLayout(layout);
	
	connect(cancel_button, SIGNAL(clicked(bool)), this, SLOT(reject()));
	connect(ok_button, SIGNAL(clicked(bool)), this, SLOT(accept()));
	connect(lat_edit, SIGNAL(textChanged(QString)), this, SLOT(editChanged()));
	connect(lon_edit, SIGNAL(textChanged(QString)), this, SLOT(editChanged()));
}
void GeoreferencingDialog::editChanged()
{
	bool ok = false;
	
	params.center_latitude = lat_edit->text().toDouble(&ok) * M_PI / 180;
	if (!ok)
	{
		ok_button->setEnabled(false);
		return;
	}
	
	params.center_longitude = lon_edit->text().toDouble(&ok) * M_PI / 180;
	if (!ok)
	{
		ok_button->setEnabled(false);
		return;
	}
	
	ok_button->setEnabled(true);
}

#include "georeferencing.moc"
