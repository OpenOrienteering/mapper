/*
 *    Copyright 2012 Thomas Schöps
 *    Copyright 2013-2020 Kai Pastor
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


#include "template_positioning_dialog.h"

#include <Qt>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QRadioButton>
#include <QSpacerItem>

#include "gui/util_gui.h"
#include "templates/template_image_open_dialog.h"


namespace OpenOrienteering {

// not inline
TemplatePositioningDialog::~TemplatePositioningDialog() = default;

TemplatePositioningDialog::TemplatePositioningDialog(const QString& display_name, QWidget* parent)
: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{
	setWindowModality(Qt::WindowModal);
	setWindowTitle(TemplateImageOpenDialog::tr("Opening %1").arg(display_name));
	
	auto* layout = new QFormLayout();
	
	coord_system_box = new QComboBox();
	layout->addRow(tr("Coordinate system"), coord_system_box);
	coord_system_box->addItem(tr("Real"));
	coord_system_box->addItem(tr("Map"));
	coord_system_box->setCurrentIndex(0);
	
	unit_scale_edit = Util::SpinBox::create(6, 0, 99999.999999, tr("m", "meters"));
	unit_scale_edit->setValue(1);
	unit_scale_edit->setEnabled(false);
	layout->addRow(tr("One coordinate unit equals:"), unit_scale_edit);
	
	original_pos_radio = new QRadioButton(tr("Position track at given coordinates"));
	original_pos_radio->setChecked(true);
	layout->addRow(original_pos_radio);
	
	view_center_radio = new QRadioButton(tr("Position track at view center"));
	layout->addRow(view_center_radio);
	
	layout->addItem(Util::SpacerItem::create(this));
	
	auto button_box = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
	layout->addWidget(button_box);
	
	setLayout(layout);
	
	connect(button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

bool TemplatePositioningDialog::useRealCoords() const
{
	return coord_system_box->currentIndex() == 0;
}

double TemplatePositioningDialog::getUnitScale() const
{
	return unit_scale_edit->value();
}

bool TemplatePositioningDialog::centerOnView() const
{
	return view_center_radio->isChecked();
}


}  // namespace OpenOrienteering
