/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2019, 2020 Kai Pastor
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


#include "stretch_map_dialog.h"

#include <Qt>
#include <QAbstractButton>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QRadioButton>
#include <QSpacerItem>
#include <QVBoxLayout>

#include "core/georeferencing.h"
#include "core/map.h"
#include "core/map_coord.h"
#include "gui/util_gui.h"


namespace OpenOrienteering {

StretchMapDialog::StretchMapDialog(const Map& map, double stretch_factor, QWidget* parent, Qt::WindowFlags f)
: QDialog(parent, f)
, stretch_factor(stretch_factor)
{
	setWindowTitle(tr("Change scale factor"));
	
	auto* layout = new QFormLayout();
	
	layout->addRow(Util::Headline::create(tr("Scaling parameters")));
	
	layout->addRow(new QLabel(tr("Scaling center:")));
	
	//: Scaling center point
	center_origin_radio = new QRadioButton(tr("Map coordinate system origin"));
	if (map.getGeoreferencing().getState() == Georeferencing::Local)
		center_origin_radio->setChecked(true);
	layout->addRow(center_origin_radio);
	
	//: Scaling center point
	center_georef_radio = new QRadioButton(tr("Georeferencing reference point"));
	if (map.getGeoreferencing().getState() != Georeferencing::Local)
		center_georef_radio->setChecked(true);
	else
		center_georef_radio->setEnabled(false);
	layout->addRow(center_georef_radio);
	
	//: Scaling center point
	center_other_radio = new QRadioButton(tr("Other point,"));
	layout->addRow(center_other_radio);
	
	other_x_edit = Util::SpinBox::create<MapCoordF>();
	//: x coordinate
	layout->addRow(tr("X:"), other_x_edit);
	
	other_y_edit = Util::SpinBox::create<MapCoordF>();
	//: y coordinate
	layout->addRow(tr("Y:"), other_y_edit);
	
	layout->addItem(Util::SpacerItem::create(this));
	layout->addRow(Util::Headline::create(tr("Options")));
	
	adjust_georeferencing_check = new QCheckBox(tr("Adjust georeferencing reference point"));
	if (map.getGeoreferencing().getState() == Georeferencing::Geospatial)
		adjust_georeferencing_check->setChecked(true);
	else
		adjust_georeferencing_check->setEnabled(false);
	layout->addRow(adjust_georeferencing_check);
	
	adjust_templates_check = new QCheckBox(tr("Scale non-georeferenced templates"));
	adjust_templates_check->setChecked(true);
	layout->addRow(adjust_templates_check);
	
	
	QDialogButtonBox* button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal);
	ok_button = button_box->button(QDialogButtonBox::Ok);
	
	auto* box_layout = new QVBoxLayout();
	box_layout->addLayout(layout);
	box_layout->addItem(Util::SpacerItem::create(this));
	box_layout->addStretch();
	box_layout->addWidget(button_box);
	
	setLayout(box_layout);
	
	connect(center_origin_radio, &QAbstractButton::clicked, this, &StretchMapDialog::updateWidgets);
	connect(center_georef_radio, &QAbstractButton::clicked, this, &StretchMapDialog::updateWidgets);
	connect(center_other_radio, &QAbstractButton::clicked, this, &StretchMapDialog::updateWidgets);
	connect(button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
	
	updateWidgets();
}

void StretchMapDialog::updateWidgets()
{
	other_x_edit->setEnabled(center_other_radio->isChecked());
	other_y_edit->setEnabled(center_other_radio->isChecked());
	adjust_georeferencing_check->setEnabled(!center_georef_radio->isChecked());
}

void StretchMapDialog::stretch(Map& map) const
{
	makeStretch()(map);
}

StretchMapDialog::StretchOp StretchMapDialog::makeStretch() const
{
	auto center = MapCoord(0, 0);
	if (center_other_radio->isChecked())
		center = MapCoord(other_x_edit->value(), -other_y_edit->value());
	
	auto adjust_georeferencing = adjust_georeferencing_check->isChecked();
	auto adjust_templates = adjust_templates_check->isChecked();
	auto center_georef = center_georef_radio->isChecked();
	auto factor = stretch_factor;
	return [factor, center, center_georef, adjust_georeferencing, adjust_templates](Map& map) {
		auto actual_center = center_georef ? map.getGeoreferencing().getMapRefPoint() : center;
		map.changeScale(map.getScaleDenominator(), factor, actual_center, false, true, adjust_georeferencing, adjust_templates);
	};
}


}  // namespace OpenOrienteering
