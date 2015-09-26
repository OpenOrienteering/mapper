/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2015 Kai Pastor
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


#include "select_crs_dialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>

#include "../core/georeferencing.h"
#include "../map.h"
#include "../map_editor.h"
#include "../util_gui.h"
#include "../util.h"
#include "../util/scoped_signals_blocker.h"
#include "widgets/crs_selector.h"


SelectCRSDialog::SelectCRSDialog(Map* map, QWidget* parent, bool show_take_from_map,
                                 bool show_local, bool show_geographic, const QString& desc_text)
 : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint), map(map)
{
	Q_ASSERT(map->getGeoreferencing().getState() != Georeferencing::ScaleOnly);
	
	setWindowModality(Qt::WindowModal);
	setWindowTitle(tr("Select coordinate reference system"));
	
	QLabel* desc_label = NULL;
	if (!desc_text.isEmpty())
		desc_label = new QLabel(desc_text);
	
	map_radio = show_take_from_map ? (new QRadioButton(tr("Same as map's"))) : NULL;
	if (map_radio)
		map_radio->setChecked(true);
	
	local_radio = show_local ? (new QRadioButton(tr("Local"))) : NULL;
	
	geographic_radio = show_geographic ? (new QRadioButton(tr("Geographic coordinates (WGS84)"))) : NULL;
	
	projected_radio = new QRadioButton(tr("From list"));
	if (!map_radio)
		projected_radio->setChecked(true);
	crs_edit = new CRSSelector(map->getGeoreferencing());
	
	spec_radio = new QRadioButton(tr("From specification"));
	
	crs_spec_edit = new QLineEdit();
	status_label = new QLabel();
	
	button_box = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
	
	if (map->getGeoreferencing().isLocal())
	{
		if (map_radio)
			map_radio->setText(map_radio->text() + " " + tr("(local)"));
		if (geographic_radio)
			geographic_radio->setEnabled(false);
		projected_radio->setEnabled(false);
		spec_radio->setEnabled(false);
		crs_spec_edit->setEnabled(false);
	}
	
	QHBoxLayout* crs_layout = new QHBoxLayout();
	crs_layout->addSpacing(16);
	crs_layout->addWidget(crs_edit);
	
	crs_spec_layout = new QFormLayout();
	crs_spec_layout->addRow(tr("CRS Specification:"), crs_spec_edit);
	crs_spec_layout->addRow(tr("Status:"), status_label);
	
	QVBoxLayout* layout = new QVBoxLayout();
	if (desc_label)
	{
		layout->addWidget(desc_label);
		layout->addSpacing(16);
	}
	if (map_radio)
		layout->addWidget(map_radio);
	if (local_radio)
		layout->addWidget(local_radio);
	if (geographic_radio)
		layout->addWidget(geographic_radio);
	layout->addWidget(projected_radio);
	layout->addLayout(crs_layout);
	layout->addWidget(spec_radio);
	layout->addSpacing(16);
	layout->addLayout(crs_spec_layout);
	layout->addSpacing(16);
	layout->addWidget(button_box);
	setLayout(layout);
	
	if (map_radio)
		connect(map_radio, SIGNAL(clicked()), this, SLOT(updateWidgets()));
	if (local_radio)
		connect(local_radio, SIGNAL(clicked()), this, SLOT(updateWidgets()));
	if (geographic_radio)
		connect(geographic_radio, SIGNAL(clicked()), this, SLOT(updateWidgets()));
	connect(projected_radio, SIGNAL(clicked()), this, SLOT(updateWidgets()));
	connect(spec_radio, SIGNAL(clicked()), this, SLOT(updateWidgets()));
	connect(crs_edit, SIGNAL(crsEdited(bool)), this, SLOT(updateWidgets()));
	connect(crs_spec_edit, SIGNAL(textEdited(QString)), this, SLOT(crsSpecEdited(QString)));
	connect(button_box, SIGNAL(accepted()), this, SLOT(accept()));
	connect(button_box, SIGNAL(rejected()), this, SLOT(reject()));
	
	updateWidgets();
}

QString SelectCRSDialog::getCRSSpec() const
{
	return crs_spec_edit->text();
}

void SelectCRSDialog::crsSpecEdited(QString)
{
	spec_radio->setChecked(true);
	
	updateWidgets();
}

void SelectCRSDialog::updateWidgets()
{
	if (map_radio && map_radio->isChecked())
		crs_spec_edit->setText(map->getGeoreferencing().isLocal() ? "" : map->getGeoreferencing().getProjectedCRSSpec());
	else if (local_radio && local_radio->isChecked())
		crs_spec_edit->setText("");
	else if (geographic_radio && geographic_radio->isChecked())
		crs_spec_edit->setText("+proj=latlong +datum=WGS84");
	else if (projected_radio->isChecked())
		crs_spec_edit->setText(crs_edit->getSelectedCRSSpec());
	
	crs_edit->setEnabled(projected_radio->isChecked());
	crs_spec_layout->itemAt(0, QFormLayout::LabelRole)->widget()->setVisible(spec_radio->isChecked());
	crs_spec_edit->setVisible(spec_radio->isChecked());
	
	// Update status field and enable/disable ok button
	bool valid;
	QString error_text;
	if (crs_spec_edit->text().isEmpty())
		valid = true;
	else
	{
		Georeferencing georef;
		valid = georef.setProjectedCRS("", crs_spec_edit->text());
		if (!valid)
			error_text = georef.getErrorText();
	}
	
	button_box->button(QDialogButtonBox::Ok)->setEnabled(valid);
	if (error_text.length() == 0)
		status_label->setText(tr("valid"));
	else
		status_label->setText(QString("<b>") % error_text % "</b>");
}
