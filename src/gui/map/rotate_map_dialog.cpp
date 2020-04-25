/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2020 Kai Pastor
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


#include "rotate_map_dialog.h"

#include <Qt>
#include <QtMath>
#include <QAbstractButton>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QSpacerItem>

#include "core/georeferencing.h"
#include "core/map.h"
#include "core/map_coord.h"
#include "gui/util_gui.h"


namespace OpenOrienteering {

RotateMapDialog::RotateMapDialog(const Map& map, QWidget* parent, Qt::WindowFlags f)
: QDialog(parent, f)
{
	setWindowTitle(tr("Rotate map"));
	
	auto* layout = new QFormLayout();
	
	layout->addRow(Util::Headline::create(tr("Rotation parameters")));
	
	rotation_edit = Util::SpinBox::create<Util::RotationalDegrees>();
	layout->addRow(tr("Angle (counter-clockwise):"), rotation_edit);
	
	layout->addRow(new QLabel(tr("Rotate around:")));
	
	//: Rotation center point
	center_origin_radio = new QRadioButton(tr("Map coordinate system origin"));
	center_origin_radio->setChecked(true);
	layout->addRow(center_origin_radio);
	
	//: Rotation center point
	center_georef_radio = new QRadioButton(tr("Georeferencing reference point"));
	if (!map.getGeoreferencing().isValid())
		center_georef_radio->setEnabled(false);
	layout->addRow(center_georef_radio);
	
	//: Rotation center point
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
	if (map.getGeoreferencing().isValid())
		adjust_georeferencing_check->setChecked(true);
	else
		adjust_georeferencing_check->setEnabled(false);
	layout->addRow(adjust_georeferencing_check);
	
	adjust_declination_check = new QCheckBox(tr("Adjust georeferencing declination"));
	if (map.getGeoreferencing().isValid())
		adjust_declination_check->setChecked(true);
	else
		adjust_declination_check->setEnabled(false);
	layout->addRow(adjust_declination_check);
	
	adjust_templates_check = new QCheckBox(tr("Rotate non-georeferenced templates"));
	adjust_templates_check->setChecked(true);
	layout->addRow(adjust_templates_check);
	
	
	QDialogButtonBox* button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal);
	
	auto* box_layout = new QVBoxLayout();
	box_layout->addLayout(layout);
	box_layout->addItem(Util::SpacerItem::create(this));
	box_layout->addStretch();
	box_layout->addWidget(button_box);
	
	setLayout(box_layout);
	
	connect(center_origin_radio, &QAbstractButton::clicked, this, &RotateMapDialog::updateWidgets);
	connect(center_georef_radio, &QAbstractButton::clicked, this, &RotateMapDialog::updateWidgets);
	connect(center_other_radio, &QAbstractButton::clicked, this, &RotateMapDialog::updateWidgets);
	connect(button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
	
	updateWidgets();
}

void RotateMapDialog::setRotationDegrees(double rotation)
{
	rotation_edit->setValue(rotation);
}

void RotateMapDialog::setRotateAroundGeorefRefPoint()
{
	if (center_georef_radio->isEnabled())
	{
		center_georef_radio->setChecked(true);
		updateWidgets();
	}
}

void RotateMapDialog::setAdjustDeclination(bool adjust)
{
	adjust_declination_check->setChecked(adjust);
}

void RotateMapDialog::showAdjustDeclination(bool show)
{
	adjust_declination_check->setVisible(show);
}

void RotateMapDialog::updateWidgets()
{
	other_x_edit->setEnabled(center_other_radio->isChecked());
	other_y_edit->setEnabled(center_other_radio->isChecked());
	adjust_georeferencing_check->setEnabled(!center_georef_radio->isChecked());
}

void RotateMapDialog::rotate(Map& map) const
{
	makeRotation()(map);
}

RotateMapDialog::RotationOp RotateMapDialog::makeRotation() const
{
	auto const rotation = qDegreesToRadians(rotation_edit->value());
	auto center = MapCoord(0, 0);
	if (center_other_radio->isChecked())
		center = MapCoord(other_x_edit->value(), -other_y_edit->value());
	
	auto adjust_georeferencing = adjust_georeferencing_check->isChecked();
	auto adjust_declination = adjust_declination_check->isChecked();
	auto adjust_templates = adjust_templates_check->isChecked();
	auto center_georef = center_georef_radio->isChecked();
	return [rotation, center, center_georef, adjust_georeferencing, adjust_declination, adjust_templates](Map& map) {
		auto actual_center = center_georef ? map.getGeoreferencing().getMapRefPoint() : center;
		map.rotateMap(rotation, actual_center, adjust_georeferencing, adjust_declination, adjust_templates);
	};
}


}  // namespace OpenOrienteering
