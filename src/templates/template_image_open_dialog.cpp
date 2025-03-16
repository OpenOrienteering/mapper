/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2020, 2025 Kai Pastor
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


#include "template_image_open_dialog.h"

#include <Qt>
#include <QtGlobal>
#include <QAbstractButton>
#include <QByteArray>
#include <QDialog>
#include <QHBoxLayout>
#include <QIcon>
#include <QImage>
#include <QIntValidator>
#include <QLabel>
#include <QLatin1String>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

#include "core/map.h"
#include "gui/util_gui.h"
#include "templates/template_image.h"


namespace OpenOrienteering {

TemplateImageOpenDialog::TemplateImageOpenDialog(TemplateImage* templ, QWidget* parent)
: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
, templ(templ)
{
	setWindowTitle(tr("Opening %1").arg(templ->getTemplateFilename()));
	
	auto* size_label = new QLabel(QLatin1String("<b>") + tr("Image size:") + QLatin1String("</b> ")
	                                + QString::number(templ->getImage().width()) + QLatin1String(" x ")
	                                + QString::number(templ->getImage().height()));
	auto* desc_label = new QLabel(tr("Specify how to position or scale the image:"));
	
	const auto& defaults = templ->getMap()->getImageTemplateDefaults();
	
	auto georef_source = templ->availableGeoreferencing().effective.transform.source;
	auto const georef_radio_enabled = !georef_source.isEmpty();
	
	// Georeferencing source translations which already existed in this context
	// and need to be preserved here, until moved to a different file.
	// Now the source strings come from TemplateImage and from GDAL (driver names).
	Q_UNUSED(QT_TR_NOOP("World file"))
	Q_UNUSED(QT_TR_NOOP("GeoTIFF"))
	Q_UNUSED(QT_TR_NOOP("no georeferencing information"))
	// GDAL's GeoTIFF driver reports itself as "GTiff".
	if (georef_source == "GTiff")
		georef_source = "GeoTIFF";
	
	georef_radio = new QRadioButton(tr("Georeferenced (%1)").arg(tr(georef_source)));
	georef_radio->setEnabled(georef_radio_enabled);
	
	mpp_radio = new QRadioButton(tr("Meters per pixel:"));
	mpp_edit = new QLineEdit((defaults.meters_per_pixel > 0) ? QString::number(defaults.meters_per_pixel) : QString{});
	mpp_edit->setValidator(new DoubleValidator(0, 999999, mpp_edit));
	
	dpi_radio = new QRadioButton(tr("Scanned with"));
	dpi_edit = new QLineEdit((defaults.dpi > 0) ? QString::number(defaults.dpi) : QString{});
	dpi_edit->setValidator(new DoubleValidator(1, 999999, dpi_edit));
	auto* dpi_label = new QLabel(tr("dpi"));
	
	auto* scale_label = new QLabel(tr("Template scale:  1 :"));
	scale_edit = new QLineEdit((defaults.scale > 0) ? QString::number(defaults.scale) : QString{});
	scale_edit->setValidator(new QIntValidator(1, 999999, scale_edit));
	
	if (georef_radio->isEnabled())
		georef_radio->setChecked(true);
	else if (defaults.use_meters_per_pixel)
		mpp_radio->setChecked(true);
	else
		dpi_radio->setChecked(true);
	
	auto* mpp_layout = new QHBoxLayout();
	mpp_layout->addWidget(mpp_radio);
	mpp_layout->addWidget(mpp_edit);
	mpp_layout->addStretch(1);
	auto* dpi_layout = new QHBoxLayout();
	dpi_layout->addWidget(dpi_radio);
	dpi_layout->addWidget(dpi_edit);
	dpi_layout->addWidget(dpi_label);
	dpi_layout->addStretch(1);
	auto* scale_layout = new QHBoxLayout();
	scale_layout->addSpacing(16);
	scale_layout->addWidget(scale_label);
	scale_layout->addWidget(scale_edit);
	scale_layout->addStretch(1);
	
	auto* cancel_button = new QPushButton(tr("Cancel"));
	open_button = new QPushButton(QIcon(QString::fromLatin1(":/images/arrow-right.png")), tr("Open"));
	open_button->setDefault(true);
	
	auto* buttons_layout = new QHBoxLayout();
	buttons_layout->addWidget(cancel_button);
	buttons_layout->addStretch(1);
	buttons_layout->addWidget(open_button);
	
	auto* layout = new QVBoxLayout();
	layout->addWidget(size_label);
	layout->addSpacing(16);
	layout->addWidget(desc_label);
	layout->addWidget(georef_radio);
	layout->addLayout(mpp_layout);
	layout->addLayout(dpi_layout);
	layout->addLayout(scale_layout);
	layout->addSpacing(16);
	layout->addLayout(buttons_layout);
	setLayout(layout);
	
	connect(mpp_edit, &QLineEdit::textEdited, this, &TemplateImageOpenDialog::setOpenEnabled);
	connect(dpi_edit, &QLineEdit::textEdited, this, &TemplateImageOpenDialog::setOpenEnabled);
	connect(scale_edit, &QLineEdit::textEdited, this, &TemplateImageOpenDialog::setOpenEnabled);
	connect(cancel_button, &QAbstractButton::clicked, this, &QDialog::reject);
	connect(open_button, &QAbstractButton::clicked, this, &TemplateImageOpenDialog::doAccept);
	connect(georef_radio, &QAbstractButton::clicked, this, &TemplateImageOpenDialog::radioClicked);
	connect(mpp_radio, &QAbstractButton::clicked, this, &TemplateImageOpenDialog::radioClicked);
	connect(dpi_radio, &QAbstractButton::clicked, this, &TemplateImageOpenDialog::radioClicked);
	
	radioClicked();
}

double TemplateImageOpenDialog::getMpp() const
{
	if (mpp_radio->isChecked())
		return mpp_edit->text().toDouble();
	
	double dpi = dpi_edit->text().toDouble();			// dots/pixels per inch(on map)
	double ipd = 1.0 / dpi;								// inch(on map) per pixel
	double mpd = ipd * 0.0254;							// meters(on map) per pixel	
	double mpp = mpd * scale_edit->text().toDouble();	// meters(in reality) per pixel
	return mpp;
}

bool TemplateImageOpenDialog::isGeorefRadioChecked() const
{
	return georef_radio->isChecked();
}

void TemplateImageOpenDialog::radioClicked()
{
	bool mpp_checked = mpp_radio->isChecked();
	bool dpi_checked = dpi_radio->isChecked();
	dpi_edit->setEnabled(dpi_checked);
	scale_edit->setEnabled(dpi_checked);
	mpp_edit->setEnabled(mpp_checked);
	setOpenEnabled();
}

void TemplateImageOpenDialog::setOpenEnabled()
{
	if (mpp_radio->isChecked())
		open_button->setEnabled(!mpp_edit->text().isEmpty());
	else if (dpi_radio->isChecked())
		open_button->setEnabled(!scale_edit->text().isEmpty() && !dpi_edit->text().isEmpty());
	else //if (georef_radio->isChecked())
		open_button->setEnabled(true);
}

void TemplateImageOpenDialog::doAccept()
{
	Map::ImageTemplateDefaults image_template_defaults = { mpp_radio->isChecked(),
	                                                       mpp_edit->text().toDouble(),
	                                                       dpi_edit->text().toDouble(),
	                                                       scale_edit->text().toDouble() };
	templ->getMap()->setImageTemplateDefaults(image_template_defaults);
	accept();
}


}  // namespace OpenOrienteering
