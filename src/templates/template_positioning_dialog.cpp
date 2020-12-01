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
#include <QtGlobal>
#include <QChar>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLatin1String>
#include <QPushButton>
#include <QRadioButton>
#include <QSpacerItem>
#include <QVBoxLayout>

#include "core/georeferencing.h"
#include "gui/util_gui.h"
#include "gui/widgets/crs_selector.h"
#include "templates/template_image_open_dialog.h"
#include "util/backports.h"  // IWYU pragma: keep


namespace OpenOrienteering {

class MapCoordF;


// not inline
TemplatePositioningDialog::~TemplatePositioningDialog() = default;

TemplatePositioningDialog::TemplatePositioningDialog(const QString& display_name, const Georeferencing& data_georef, QWidget* parent)
: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{
	setWindowModality(Qt::WindowModal);
	setWindowTitle(TemplateImageOpenDialog::tr("Opening %1").arg(display_name));
	
	auto* layout = new QFormLayout();
	
	coord_system_box = new CRSSelector(data_georef);
	layout->addRow(tr("Coordinate system"), coord_system_box);
	
	unit_scale_edit = Util::SpinBox::create(4, 0, 1000.0);
	unit_scale_edit->setValue(1);
	unit_scale_edit->setEnabled(false);
	layout->addRow(tr("One coordinate unit equals:"), unit_scale_edit);
	
	status_label = new QLabel();
	layout->addRow(tr("Status:"), status_label);
	
	layout->addItem(Util::SpacerItem::create(this));
	coord_system_box->setDialogLayout(layout);
	
	if (data_georef.getState() == Georeferencing::Geospatial)
		coord_system_box->addCustomItem(tr("From the data"), CoordinateSystem::DomainGeospatial);
	coord_system_box->addCustomItem(tr("Ground"), CoordinateSystem::DomainGround);
	coord_system_box->addCustomItem(tr("Paper"), CoordinateSystem::DomainMap);
	coord_system_box->setCurrentIndex(0);
	
	layout->addItem(Util::SpacerItem::create(this));
	
	original_pos_radio = new QRadioButton(tr("Position data at given coordinates"));
	original_pos_radio->setChecked(true);
	layout->addRow(original_pos_radio);
	
	view_center_radio = new QRadioButton(tr("Position data at view center"));
	layout->addRow(view_center_radio);
	
	layout->addItem(Util::SpacerItem::create(this));
	
	auto* vbox_layout = new QVBoxLayout();
	vbox_layout->addLayout(layout);
	vbox_layout->addStretch(1);
	
	button_box = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
	vbox_layout->addWidget(button_box);
	
	setLayout(vbox_layout);
	
	connect(coord_system_box, &CRSSelector::crsChanged, this, &TemplatePositioningDialog::updateWidgets);
	updateWidgets();
	
	connect(button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

CoordinateSystem::Domain TemplatePositioningDialog::csDomain() const
{
	auto const domain = coord_system_box->currentCustomItem();
	switch (domain)
	{
	case CoordinateSystem::DomainMap:
	case CoordinateSystem::DomainGround:
		return static_cast<CoordinateSystem::Domain>(domain);
	default:
		return CoordinateSystem::DomainGeospatial;
	}
}

double TemplatePositioningDialog::unitScaleFactor() const
{
	return unit_scale_edit->value();
}

bool TemplatePositioningDialog::centerOnView() const
{
	return view_center_radio->isChecked();
}


QString TemplatePositioningDialog::currentCRSSpec() const
{
	switch (coord_system_box->currentCustomItem())
	{
	case CoordinateSystem::DomainMap:
	case CoordinateSystem::DomainGround:
	case CoordinateSystem::DomainGeospatial:
		return {};
	default:
		return coord_system_box->currentCRSSpec();
	}
}


// slot
void TemplatePositioningDialog::updateWidgets()
{
	auto georeferenced = false;
	switch (csDomain())
	{
	case CoordinateSystem::DomainGeospatial:
		georeferenced = true;
		unit_scale_edit->setSuffix({});
		original_pos_radio->setChecked(true);
		break;
		
	case CoordinateSystem::DomainGround:  // Real
		unit_scale_edit->setMinimum(0.0001);
		unit_scale_edit->setDecimals(3);
		unit_scale_edit->setSuffix(QChar::Space + Util::InputProperties<Util::RealMeters>::unit());
		break;
		
	case CoordinateSystem::DomainMap: // Map
		unit_scale_edit->setMinimum(0.01);
		unit_scale_edit->setDecimals(2);
		unit_scale_edit->setSuffix(QChar::Space + Util::InputProperties<MapCoordF>::unit());
		break;
		
	default:
		Q_UNREACHABLE();
	}
	
	unit_scale_edit->setValue(1);
	unit_scale_edit->setEnabled(!georeferenced);
	view_center_radio->setEnabled(!georeferenced);
	view_center_radio->setChecked(!georeferenced);
	
	auto valid = true;
	auto error_text = QString{};
	if (georeferenced)
	{
		Georeferencing georef;
		auto spec =  coord_system_box->currentCRSSpec();
		valid = spec.isEmpty() || georef.setProjectedCRS({}, spec);
		error_text = georef.getErrorText();
	}
	if (valid)
		status_label->setText(tr("valid"));
	else
		status_label->setText(QLatin1String("<b style=\"color:red\">") + error_text + QLatin1String("</b>"));
	button_box->button(QDialogButtonBox::Ok)->setEnabled(valid);
}


}  // namespace OpenOrienteering
