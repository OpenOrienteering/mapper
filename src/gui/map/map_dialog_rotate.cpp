/*
 *    Copyright 2012, 2013 Thomas Schöps
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

#include <Qt>
#include <QtMath>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFlags>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QSpacerItem>

#include "core/georeferencing.h"
#include "core/map.h"
#include "core/map_coord.h"
#include "templates/template.h"
#include "gui/util_gui.h"


namespace OpenOrienteering {

RotateMapDialog::RotateMapDialog(QWidget* parent, Map* map) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint), map(map)
{
	setWindowTitle(tr("Rotate map"));
	
	QFormLayout* layout = new QFormLayout();
	
	layout->addRow(Util::Headline::create(tr("Rotation parameters")));
	
	rotation_edit = Util::SpinBox::create(Georeferencing::declinationPrecision(), -180.0, +180.0, trUtf8("°"));
	rotation_edit->setWrapping(true);
	layout->addRow(tr("Angle (counter-clockwise):"), rotation_edit);
	
	layout->addRow(new QLabel(tr("Rotate around:")));
	
	center_origin_radio = new QRadioButton(tr("Map coordinate system origin", "Rotation center point"));
	center_origin_radio->setChecked(true);
	layout->addRow(center_origin_radio);
	
	center_georef_radio = new QRadioButton(tr("Georeferencing reference point", "Rotation center point"));
	if (!map->getGeoreferencing().isValid())
		center_georef_radio->setEnabled(false);
	layout->addRow(center_georef_radio);
	
	center_other_radio = new QRadioButton(tr("Other point,", "Rotation center point"));
	other_x_edit = Util::SpinBox::create<MapCoordF>(tr("mm"));
	other_y_edit = Util::SpinBox::create<MapCoordF>(tr("mm"));
	auto other_center_layout = new QHBoxLayout();
	other_center_layout->addWidget(center_other_radio);
	other_center_layout->addWidget(new QLabel(tr("X:", "x coordinate")), 0);
	other_center_layout->addWidget(other_x_edit, 1);
	other_center_layout->addWidget(new QLabel(tr("Y:", "y coordinate")), 0);
	other_center_layout->addWidget(other_y_edit, 1);
	layout->addRow(other_center_layout);
	
	
	layout->addItem(Util::SpacerItem::create(this));
	layout->addRow(Util::Headline::create(tr("Options")));
	
	adjust_georeferencing_check = new QCheckBox(tr("Adjust georeferencing reference point"));
	if (map->getGeoreferencing().isValid())
		adjust_georeferencing_check->setChecked(true);
	else
		adjust_georeferencing_check->setEnabled(false);
	layout->addRow(adjust_georeferencing_check);
	
	adjust_declination_check = new QCheckBox(tr("Adjust georeferencing declination"));
	if (map->getGeoreferencing().isValid())
		adjust_declination_check->setChecked(true);
	else
		adjust_declination_check->setEnabled(false);
	layout->addRow(adjust_declination_check);
	
	adjust_templates_check = new QCheckBox(tr("Rotate non-georeferenced templates"));
	bool have_non_georeferenced_template = false;
	for (int i = 0; i < map->getNumTemplates() && !have_non_georeferenced_template; ++i)
		have_non_georeferenced_template = !map->getTemplate(i)->isTemplateGeoreferenced();
	for (int i = 0; i < map->getNumClosedTemplates() && !have_non_georeferenced_template; ++i)
		have_non_georeferenced_template = !map->getClosedTemplate(i)->isTemplateGeoreferenced();
	if (have_non_georeferenced_template)
		adjust_templates_check->setChecked(true);
	else
		adjust_templates_check->setEnabled(false);
	layout->addRow(adjust_templates_check);
	
	
	layout->addItem(Util::SpacerItem::create(this));
	QDialogButtonBox* button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal);
	layout->addRow(button_box);
	
	setLayout(layout);
	
	connect(center_origin_radio, &QAbstractButton::clicked, this, &RotateMapDialog::updateWidgets);
	connect(center_georef_radio, &QAbstractButton::clicked, this, &RotateMapDialog::updateWidgets);
	connect(center_other_radio, &QAbstractButton::clicked, this, &RotateMapDialog::updateWidgets);
	connect(button_box, &QDialogButtonBox::accepted, this, &RotateMapDialog::okClicked);
	connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
	
	updateWidgets();
}

void RotateMapDialog::setRotationDegrees(float rotation)
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

void RotateMapDialog::okClicked()
{
	double rotation = M_PI * rotation_edit->value() / 180;
	MapCoord center = MapCoord(0, 0);
	if (center_georef_radio->isChecked())
		center = map->getGeoreferencing().getMapRefPoint();
	else if (center_other_radio->isChecked())
		center = MapCoord(other_x_edit->value(), -1 * other_y_edit->value());
	
	map->rotateMap(rotation, center, adjust_georeferencing_check->isChecked(), adjust_declination_check->isChecked(), adjust_templates_check->isChecked());
	accept();
}


}  // namespace OpenOrienteering
