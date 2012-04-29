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

#include "map.h"

CartesianGeoreferencing::CartesianGeoreferencing(const Map& map)
: map(map), easting(0.0), northing(0.0), declination(0.0)
{
	geoToPaper = NULL;
	paperToGeo = NULL;
}

CartesianGeoreferencing::CartesianGeoreferencing(const Map& map, double easting, double northing, double declination)
: map(map), easting(easting), northing(northing), declination(declination)
{
	geoToPaper = NULL;
	paperToGeo = NULL;
	updateTransforms();
}

CartesianGeoreferencing::CartesianGeoreferencing(const CartesianGeoreferencing& georeferencing)
: map(georeferencing.map), easting(georeferencing.easting), northing(georeferencing.northing), declination(georeferencing.declination)
{
	geoToPaper = NULL;
	paperToGeo = NULL;
	updateTransforms();
}

CartesianGeoreferencing::~CartesianGeoreferencing()
{
	delete geoToPaper;
	delete paperToGeo;
}

void CartesianGeoreferencing::set(double easting, double northing)
{
	this->easting = easting;
	this->northing = northing;
	updateTransforms();
}

void CartesianGeoreferencing::set(double easting, double northing, double declination)
{
	this->easting = easting;
	this->northing = northing;
	this->declination = declination;
	updateTransforms();
}

void CartesianGeoreferencing::setDeclination(double declination)
{
	this->declination = declination;
	updateTransforms();
}

void CartesianGeoreferencing::updateTransforms()
{
	delete paperToGeo;
	paperToGeo = new QTransform();
	double scale = double(map.getScaleDenominator()) / 1000.0;
	paperToGeo->translate(easting, northing);
	paperToGeo->rotate(declination);
	paperToGeo->scale(scale, -scale);
	
	delete geoToPaper;
	geoToPaper = new QTransform(paperToGeo->inverted());
}

QPointF CartesianGeoreferencing::toGeoCoords(const MapCoord& paper_coords) const
{
	return paperToGeo->map(paper_coords.toQPointF());
}

QPointF CartesianGeoreferencing::toGeoCoords(const MapCoordF& paper_coords) const
{
	return paperToGeo->map(paper_coords.toQPointF());
}

MapCoord CartesianGeoreferencing::toPaperCoords(const QPointF& geo_coords) const
{
	return MapCoordF(geoToPaper->map(geo_coords)).toMapCoord(); // FIXME: not tested
}



GeoreferencingDialog::GeoreferencingDialog(QWidget* parent, const Map& map, const GPSProjectionParameters* initial_values) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{
	setWindowTitle(tr("Georeferencing settings"));
	
	cartesian = new CartesianGeoreferencing(map);
	
	if (initial_values)
		params = *initial_values;
	
	QLabel* projection_label = new QLabel(QString("<b>%1</b>").arg(tr("Latitude/longitude:")));
	
	QLabel* lat_label = new QLabel(tr("Origin latitude:"));
	lat_edit = new QLineEdit(QString::number(params.center_latitude * 180 / M_PI, 'f', 12));
	lat_edit->setWhatsThis("<a href=\"gps_menu.html\">See more</a>");
	QLabel* lon_label = new QLabel(tr("Origin longitude:"));
	lon_edit = new QLineEdit(QString::number(params.center_longitude * 180 / M_PI, 'f', 12));
	lon_edit->setWhatsThis("<a href=\"gps_menu.html\">See more</a>");
	
	QLabel* cartesian_label = new QLabel(QString("<b>%1</b>").arg(tr("Cartesian coordinates:")));
	QLabel* datum_label = new QLabel(tr("Datum:"));
	datum_edit = new QLineEdit("not used yet");
	QLabel* zone_label = new QLabel(tr("Zone:"));
	zone_edit = new QLineEdit("not used yet");
	QLabel* easting_label = new QLabel(tr("Origin easting (m):"));
	easting_edit = new QLineEdit(QString::number(map.getGeoreferencing().getEasting()));
	easting_edit->setValidator(new QIntValidator());
	easting_edit->setWhatsThis("<a href=\"gps_menu.html\">See more</a>");
	QLabel* northing_label = new QLabel(tr("Origin northing (m):"));
	northing_edit = new QLineEdit(QString::number(map.getGeoreferencing().getNorthing()));
	northing_edit->setValidator(new QIntValidator());
	northing_edit->setWhatsThis("<a href=\"gps_menu.html\">See more</a>");
	
	QLabel* declination_label = new QLabel(tr("Declination (<sup>0</sup>):"));
	declination_edit = new QLineEdit(QString::number(map.getGeoreferencing().getDeclination()));
	declination_edit->setValidator(new QDoubleValidator(-90.0, 90.0, 1));
	declination_edit->setWhatsThis("<a href=\"gps_menu.html\">See more</a>");
	
	QGridLayout* edit_layout = new QGridLayout();
	int row=0, col=0;
	edit_layout->addWidget(projection_label, row, col, 1, 2);
	row++, col=0;
	edit_layout->addWidget(lat_label, row, col++);
	edit_layout->addWidget(lat_edit, row, col, 1, -1);
	row++, col=0;
	edit_layout->addWidget(lon_label, row, col++);
	edit_layout->addWidget(lon_edit, row, col, 1, -1);
	
	row++, col=0;
	edit_layout->addWidget(new QWidget(), row, col, 1, -1);
	edit_layout->setRowStretch(row, 1);
	
	row++, col=0;
	edit_layout->addWidget(cartesian_label, row, col, 1, 2);
	row++, col=0;
	edit_layout->addWidget(new QLabel("(Experimental. Input will not be saved to the file!)"), row, col, 1, 2);
	row++, col=0;
	edit_layout->addWidget(datum_label, row, col++);
	edit_layout->addWidget(datum_edit, row, col, 1, -1);
	row++, col=0;
	edit_layout->addWidget(zone_label, row, col++);
	edit_layout->addWidget(zone_edit, row, col, 1, -1);
	row++, col=0;
	edit_layout->addWidget(easting_label, row, col++);
	edit_layout->addWidget(easting_edit, row, col, 1, -1);
	row++, col=0;
	edit_layout->addWidget(northing_label, row, col++);
	edit_layout->addWidget(northing_edit, row, col, 1, -1);
	
	row++, col=0;
	edit_layout->addWidget(new QWidget(), row, col, 1, -1);
	edit_layout->setRowStretch(row, 1);
	
	row++, col=0;
	edit_layout->addWidget(declination_label, row, col++);
	edit_layout->addWidget(declination_edit, row, col, 1, -1);
	
	row++, col=0;
	edit_layout->addWidget(new QWidget(), row, col, 1, -1);
	edit_layout->setRowStretch(row, 1);
	
	QPushButton* cancel_button = new QPushButton(tr("Cancel"));
	ok_button = new QPushButton(QIcon(":/images/arrow-right.png"), tr("OK"));
	ok_button->setDefault(true);
	
	QHBoxLayout* buttons_layout = new QHBoxLayout();
	buttons_layout->addWidget(cancel_button);
	buttons_layout->addStretch(1);
	buttons_layout->addWidget(ok_button);
	
	row++, col=0;
	edit_layout->addLayout(buttons_layout, row, col, 1, -1);
	setLayout(edit_layout);
	
	connect(cancel_button, SIGNAL(clicked(bool)), this, SLOT(reject()));
	connect(ok_button, SIGNAL(clicked(bool)), this, SLOT(accept()));
	connect(lat_edit, SIGNAL(textChanged(QString)), this, SLOT(editChanged()));
	connect(lon_edit, SIGNAL(textChanged(QString)), this, SLOT(editChanged()));
	connect(easting_edit, SIGNAL(textChanged(QString)), this, SLOT(cartesianChanged()));
	connect(northing_edit, SIGNAL(textChanged(QString)), this, SLOT(cartesianChanged()));
	connect(declination_edit, SIGNAL(textChanged(QString)), this, SLOT(cartesianChanged()));
	
	cartesianChanged();
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

void GeoreferencingDialog::cartesianChanged()
{
	bool easting_ok, northing_ok, declination_ok;
	double easting = easting_edit->text().toDouble(&easting_ok);
	double northing = northing_edit->text().toDouble(&northing_ok);
	double declination = declination_edit->text().toDouble(&declination_ok);
	if (easting_ok && northing_ok && declination_ok)
	{
		ok_button->setEnabled(true);
		cartesian->set(easting, northing, declination);
	}
	else
	{
		ok_button->setEnabled(false);
	}
}


#include "georeferencing.moc"