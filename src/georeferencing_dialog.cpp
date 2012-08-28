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


#include "georeferencing_dialog.h"

#include <limits>

#include <QtGui>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QXmlStreamReader>

#include "georeferencing.h"
#include "main_window.h"
#include "map.h"
#include "map_editor.h"
#include "util_gui.h"

GeoreferencingDialog::GeoreferencingDialog(MapEditorController* controller, const Georeferencing* initial)
: QDialog(controller->getWindow(), Qt::WindowSystemMenuHint | Qt::WindowTitleHint),
  controller(controller), 
  map(controller->getMap())
{
	init(initial);
}

GeoreferencingDialog::GeoreferencingDialog(QWidget* parent, Map* map, const Georeferencing* initial)
: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint),
  controller(NULL), 
  map(map)
{
	init(initial);
}

void GeoreferencingDialog::init(const Georeferencing* initial)
{
	setWindowModality(Qt::WindowModal);
	
	tool_active = false;
	changed_coords = NONE;
	
	// A working copy of the current or given initial Georeferencing
	georef.reset( new Georeferencing(initial == NULL ? map->getGeoreferencing() : *initial) );
	
	setWindowTitle(tr("Map Georeferencing"));
	
	scale_edit  = new QLabel();
	declination_edit = Util::SpinBox::create(1, -180.0, +180.0, trUtf8("°"));
	declination_button = new QPushButton(tr("Lookup..."));
	declination_button->setEnabled(!georef->isLocal());
	QHBoxLayout* declination_layout = new QHBoxLayout();
	declination_layout->addWidget(declination_edit, 1);
	declination_layout->addWidget(declination_button, 0);
	grivation_edit = Util::SpinBox::create(1, -180.0, +180.0, trUtf8("°"));
	ref_point_edit = new QLabel();
	QPushButton* ref_point_button = new QPushButton(tr("&Select..."));
	ref_point_button->setEnabled(controller != NULL);
	QHBoxLayout* ref_point_layout = new QHBoxLayout();
	ref_point_layout->addWidget(ref_point_edit, 1);
	ref_point_layout->addWidget(ref_point_button, 0);
	
	crs_edit = new QComboBox();
	crs_edit->setEditable(true);
	// TODO: move crs registry out of georeferencing GUI
	crs_edit->addItem(tr("Local coordinates"), "");
	crs_edit->addItem("UTM", "+proj=utm +zone=!ZONE!");
	crs_edit->addItem("Gauss-Krueger zone 3, datum: Potsdam", "+proj=tmerc +lat_0=0 +lon_0=9 +k=1.000000 +x_0=3500000 +y_0=0 +ellps=bessel +datum=potsdam +units=m +no_defs");
	crs_edit->addItem(tr("Edit projection parameters..."), "!EDIT!");
	
	zone_edit = new QLineEdit();
	
	status_label = new QLabel();
	
	easting_edit = Util::SpinBox::create(0, std::numeric_limits<double>::min(), std::numeric_limits<double>::max(), tr("m"));
	northing_edit = Util::SpinBox::create(0, std::numeric_limits<double>::min(), std::numeric_limits<double>::max(), tr("m"));
	
	convergence_edit = new QLabel();
	
	QLabel* datum_edit = new QLabel("WGS84");
	lat_edit = Util::SpinBox::create(8, -90.0, +90.0, trUtf8("°"), 1);
	lon_edit = Util::SpinBox::create(8, -180.0, +180.0, trUtf8("°"), 1);
	link_label = new QLabel();
	link_label->setOpenExternalLinks(true);
	
	QDialogButtonBox* buttons_box = new QDialogButtonBox(
	  QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Reset | QDialogButtonBox::Help,
	  Qt::Horizontal);
	reset_button = buttons_box->button(QDialogButtonBox::Reset);
	reset_button->setEnabled(initial != NULL);
	QPushButton* help_button = buttons_box->button(QDialogButtonBox::Help);
	
	QFormLayout* edit_layout = new QFormLayout;
	
	edit_layout->addRow(Util::Headline::create(tr("General")));
	edit_layout->addRow(tr("Map scale:"), scale_edit);
	edit_layout->addRow(tr("Declination:"), declination_layout); 
	edit_layout->addRow(tr("Reference point:"), ref_point_layout);
	edit_layout->addItem(Util::SpacerItem::create(this));
	
	edit_layout->addRow(Util::Headline::create(tr("Projected coordinates")));
	edit_layout->addRow(tr("&Coordinate reference system:"), crs_edit);
	edit_layout->addRow(tr("&Zone:"), zone_edit);
	edit_layout->addRow(tr("Status:"), status_label);
	edit_layout->addRow(tr("Reference point &easting:"), easting_edit);
	edit_layout->addRow(tr("Reference point &northing:"), northing_edit);
	edit_layout->addRow(tr("Convergence:"), convergence_edit);
	edit_layout->addRow(tr("&Grivation:"), grivation_edit);
	edit_layout->addItem(Util::SpacerItem::create(this));
	
	edit_layout->addRow(Util::Headline::create(tr("Geographic coordinates")));
	edit_layout->addRow(tr("Datum"), datum_edit);
	edit_layout->addRow(tr("Reference point &latitude:"), lat_edit);
	edit_layout->addRow(tr("Reference point longitude:"), lon_edit);
	edit_layout->addRow(tr("Show reference point in:"), link_label);
	
	QVBoxLayout* layout = new QVBoxLayout();
	layout->addLayout(edit_layout);
	layout->addStretch();
	layout->addSpacing(16);
	layout->addWidget(buttons_box);
	
	setLayout(layout);
	
	connect(declination_button, SIGNAL(clicked(bool)), this, SLOT(requestDeclination()));
	connect(declination_edit, SIGNAL(valueChanged(double)), this, SLOT(declinationChanged(double)));
	connect(grivation_edit, SIGNAL(valueChanged(double)), this, SLOT(grivationChanged(double)));
	connect(ref_point_button, SIGNAL(clicked(bool)), this,SLOT(selectRefPoint()));
	connect(crs_edit, SIGNAL(editTextChanged(QString)), this, SLOT(crsChanged()));
	connect(zone_edit, SIGNAL(textChanged(QString)), this, SLOT(crsChanged()));
	connect(easting_edit, SIGNAL(valueChanged(double)), this, SLOT(eastingNorthingChanged()));
	connect(northing_edit, SIGNAL(valueChanged(double)), this, SLOT(eastingNorthingChanged()));
	connect(lat_edit, SIGNAL(valueChanged(double)), this, SLOT(latLonChanged()));
	connect(lon_edit, SIGNAL(valueChanged(double)), this, SLOT(latLonChanged()));
	connect(buttons_box, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttons_box, SIGNAL(rejected()), this, SLOT(reject()));
	connect(reset_button, SIGNAL(clicked(bool)), this, SLOT(reset()));
	connect(help_button, SIGNAL(clicked(bool)), this, SLOT(showHelp()));
	
	changed_coords = NONE;
	updateGeneral();
	updateNorth();
	updateCRS();
	updateStatus();
	updateEastingNorthing();
	updateNorth();
	updateLatLon();
	updateZone();
	reset_button->setEnabled(false);
}

GeoreferencingDialog::~GeoreferencingDialog()
{
	if (tool_active)
		controller->setTool(NULL);
}

int GeoreferencingDialog::exec()
{
	if (tool_active)
		controller->setTool(NULL);
	
	return QDialog::exec();
}

void GeoreferencingDialog::setRefPoint(MapCoord coords)
{
	georef->setMapRefPoint(coords);
	updateGeneral();
	reset_button->setEnabled(true);
}

void GeoreferencingDialog::updateGeneral()
{
	scale_edit->setText( QString("1:") % QString::number(georef->getScaleDenominator()) );
	const MapCoord& coords(georef->getMapRefPoint());
	ref_point_edit->setText(tr("%1 %2 (mm)").arg(locale().toString(coords.xd())).arg(locale().toString(-coords.yd())));
}

void GeoreferencingDialog::updateCRS()
{
	crs_spec_template = georef->getProjectedCRSSpec();
	
	int index = crs_edit->findText(georef->getProjectedCRS(), Qt::MatchExactly);
	
	if (index == -1)
		index = crs_edit->findData(crs_spec_template);
	
	crs_edit->blockSignals(true);
	crs_edit->setCurrentIndex(index);
	if (index >= 0)
	{
		crs_spec_template = crs_edit->itemData(index).toString();
	}
	else
	{
		crs_edit->setEditText(crs_spec_template);
	}
	crs_edit->blockSignals(false);
}

void GeoreferencingDialog::updateZone()
{
	if (crs_spec_template.contains("!ZONE!"))
	{
		if (!zone_edit->isModified())
		{
			zone_edit->blockSignals(true);
			if (crs_spec_template.startsWith("+proj=utm"))
			{
				const LatLon ref_point(georef->getGeographicRefPoint());
				double lat = Georeferencing::radToDeg(ref_point.latitude);
				if (abs(lat) < 84.0)
				{
					double lon = Georeferencing::radToDeg(ref_point.longitude);
					int zone_no = int(floor(lon) + 180) / 6 % 60 + 1;
					if (zone_no == 31 && lon >= 3.0 && lat >= 56.0 && lat < 64.0)
						zone_no = 32; // South Norway
					else if (lat >= 72.0 && lon >= 3.0 && lon <= 39.0)
						zone_no = 2 * (int(floor(lon) + 3.0) / 12) + 31; // Svalbard
					QString zone = QString::number(zone_no);
					zone.append((ref_point.latitude >= 0.0) ? " N" : " S");
					if (zone != zone_edit->text())
					{
						zone_edit->setText(zone);
						crsChanged();
					}
				}
			}
			zone_edit->blockSignals(false);
		}
		zone_edit->setEnabled(true);
	}
	else
	{
		zone_edit->setText("");
		zone_edit->setEnabled(false);
	}
}

void GeoreferencingDialog::updateStatus()
{
	QString error = georef->getErrorText();
	if (error.length() == 0)
		status_label->setText(tr("valid"));
	else
		status_label->setText(QString("<b>") % error % "</b>");
}

void GeoreferencingDialog::updateEastingNorthing()
{
	easting_edit->blockSignals(true);
	northing_edit->blockSignals(true);
	easting_edit->setValue(georef->getProjectedRefPoint().x());
	northing_edit->setValue(georef->getProjectedRefPoint().y());
	easting_edit->blockSignals(false);
	northing_edit->blockSignals(false);
}

void GeoreferencingDialog::updateNorth()
{
	declination_edit->blockSignals(true);
	grivation_edit->blockSignals(true);
	declination_edit->setValue(georef->getDeclination());
	grivation_edit->setValue(georef->getGrivation());
	convergence_edit->setText( locale().toString(georef->getConvergence(), 'f', 1) % QString::fromUtf8(" °") );
	declination_edit->blockSignals(false);
	grivation_edit->blockSignals(false);
}

void GeoreferencingDialog::updateLatLon()
{
	// NOTE: There might be lat/lon already saved in the file
	//       even when the (new) Georeferencing is "local".
	const LatLon ref_point(georef->getGeographicRefPoint());
	double latitude = ref_point.latitude * 180.0 / M_PI;
	double longitude = ref_point.longitude * 180.0 / M_PI;
	
	lat_edit->blockSignals(true);
	lon_edit->blockSignals(true);
	lat_edit->setValue(latitude);
	lon_edit->setValue(longitude);
	lat_edit->blockSignals(false);
	lon_edit->blockSignals(false);
	
	QString osm_link = 
	  QString("http://www.openstreetmap.org/?lat=%1&lon=%2&zoom=18&layers=M").
	  arg(latitude).arg(longitude);
	QString worldofo_link =
	  QString("http://maps.worldofo.com/?zoom=15&lat=%1&lng=%2").
	  arg(latitude).arg(longitude);
	link_label->setText(
	  tr("<a href=\"%1\">OpenStreetMap</a> | <a href=\"%2\">World of O Maps</a>").
	  arg(osm_link).
	  arg(worldofo_link)
	);
	
	if (!georef->isLocal())
	{
		lat_edit->setEnabled(true);
		lon_edit->setEnabled(true);
		link_label->setEnabled(true);
		declination_button->setEnabled(true);
	}
	else
	{
		lat_edit->setEnabled(false);
		lon_edit->setEnabled(false);
		link_label->clear();
		declination_button->setEnabled(false);
	}
}

void GeoreferencingDialog::requestDeclination(bool no_confirm)
{
	if (georef->isLocal())
		return;
	
	// TODO: Move to resources or preferences. Assess security risks of url distinction.
	QString user_url("http://www.ngdc.noaa.gov/geomag-web/");
	QUrl service_url("http://www.ngdc.noaa.gov/geomag-web/calculators/calculateDeclination");
	LatLon latlon(georef->getGeographicRefPoint());
	
	if (!no_confirm)
	{
		int result = QMessageBox::question(this, tr("Online declination lookup"),
		  trUtf8("The magnetic declination for the reference point %1° %2° will now be retrieved from <a href=\"%3\">%3</a>. Do you want to continue?").
		    arg(latlon.getLatitudeInDegrees()).arg(latlon.getLongitudeInDegrees()).arg(user_url),
		  QMessageBox::Yes | QMessageBox::No,
		  QMessageBox::Yes );
		if (result != QMessageBox::Yes)
			return;
	}
	
	declination_button->setEnabled(false);
	declination_button->setText(tr("Loading..."));
	
	QNetworkAccessManager *network = new QNetworkAccessManager(this);
	connect(network, SIGNAL(finished(QNetworkReply*)), this, SLOT(declinationReplyFinished(QNetworkReply*)));
	
	service_url.addQueryItem("lat1", QString::number(latlon.getLatitudeInDegrees()));
	service_url.addQueryItem("lon1", QString::number(latlon.getLongitudeInDegrees()));
	service_url.addQueryItem("resultFormat", "xml");
	network->get(QNetworkRequest(service_url));
}

void GeoreferencingDialog::declinationReplyFinished(QNetworkReply* reply)
{
	declination_button->setEnabled(!georef->isLocal());
	declination_button->setText(tr("Lookup..."));
	
	QString error_string;
	if (reply->error() != QNetworkReply::NoError)
	{
		error_string = reply->errorString();
	}
	else
	{
		QXmlStreamReader xml(reply);	
		if (xml.readNextStartElement() && xml.name() == "maggridresult")
		{
			if (xml.readNextStartElement() && xml.name() == "result")
			{
				while (xml.readNextStartElement())
				{
					if (xml.name() == "declination")
					{
						QString text = xml.readElementText(QXmlStreamReader::IncludeChildElements);
						bool ok;
						double declination = text.toDouble(&ok);
						if (ok)
						{
							// success
							declination_edit->setValue(declination);
							return;
						}
						else 
						{
							error_string = tr("Could not parse data.") % ' ';
						}
					}
					xml.skipCurrentElement();
				}
			}
		}
		else if (xml.readNextStartElement() && xml.name() == "errors")
		{
			error_string.append(xml.readElementText(QXmlStreamReader::IncludeChildElements) % ' ');
		}
		
		if (xml.error() != QXmlStreamReader::NoError)
			error_string.append(xml.errorString());
		else if (error_string.isEmpty())
			error_string = tr("Declination value not found.");
	}
		
	int result = QMessageBox::critical(this, tr("Online declination lookup"),
		tr("The online declination lookup failed:\n%1").arg(error_string),
		QMessageBox::Retry | QMessageBox::Close,
		QMessageBox::Close );
	if (result == QMessageBox::Retry)
		requestDeclination(true);
}

void GeoreferencingDialog::declinationChanged(double value)
{
	georef->setDeclination(value);
	updateNorth();
	reset_button->setEnabled(true);
}

void GeoreferencingDialog::grivationChanged(double value)
{
	georef->setGrivation(value);
	updateNorth();
	reset_button->setEnabled(true);
}

void GeoreferencingDialog::crsChanged()
{
	QString crs, crs_spec;
	const int index = crs_edit->currentIndex();
	const QVariant itemData(crs_edit->itemData(index));
	if (index < 0 || itemData == QVariant::Invalid)
	{
		crs_spec = crs_edit->currentText();
		crs = crs_spec.length() > 0 ? tr("Custom coordinates") : tr("Local coordinates");
	}
	else
	{
		crs_spec_template = itemData.toString();
		if (crs_spec_template == "!EDIT!")
		{
			// set text and proj_params to current params
			crs = tr("Custom coordinates");
			crs_spec_template = georef->getProjectedCRSSpec();
			crs_edit->blockSignals(true);
			crs_edit->setCurrentIndex(-1);
			crs_edit->setEditText(crs_spec_template);
			crs_edit->blockSignals(false);
		}
		else
		{
			crs = crs_edit->itemText(index);
		}
		
		crs_spec = crs_spec_template;
	}
	
	updateZone();
	if (crs_spec.contains("!ZONE!"))
	{
		QString zone = zone_edit->text();
		zone.replace(" N", "");
		zone.replace(" S", " +south");
		crs_spec.replace("!ZONE!", zone);
	}
	
	bool crs_was_local = georef->isLocal();
	bool crs_ok = georef->setProjectedCRS(crs, crs_spec);
	if (crs_ok)
	{
		if (crs_was_local && changed_coords == PROJECTED)
			eastingNorthingChanged();
		else
			latLonChanged();
	}
	else
		updateNorth();
	updateLatLon();
	updateStatus();
	reset_button->setEnabled(true);
}

void GeoreferencingDialog::latLonChanged()
{
	double latitude = lat_edit->value() * M_PI / 180;
	double longitude = lon_edit->value() * M_PI / 180;
	georef->setGeographicRefPoint(LatLon(latitude, longitude));
	changed_coords = GEOGRAPHIC;
	
	updateZone();
	updateEastingNorthing();
	updateNorth();
	updateStatus();
	reset_button->setEnabled(true);
}

void GeoreferencingDialog::eastingNorthingChanged()
{
	QPointF easting_northing(easting_edit->value(), northing_edit->value());
	georef->setProjectedRefPoint(easting_northing);
	changed_coords = PROJECTED;
	
	updateLatLon();
	updateNorth();
	updateStatus();
	reset_button->setEnabled(true);
}

void GeoreferencingDialog::selectRefPoint()
{
	if (controller != NULL)
	{
		controller->setTool(new GeoreferencingTool(this, controller));
		tool_active = true;
		hide();
	}
}

void GeoreferencingDialog::toolDeleted()
{
	tool_active = false;
}

void GeoreferencingDialog::showHelp()
{
	controller->getWindow()->showHelp("georeferencing.html");
}

void GeoreferencingDialog::reset()
{
	*georef = map->getGeoreferencing();
	changed_coords = NONE;
	updateGeneral();
	updateNorth();
	updateCRS();
	updateStatus();
	updateEastingNorthing();
	updateLatLon();
	updateZone();
	reset_button->setEnabled(false);
}

void GeoreferencingDialog::accept()
{
	map->setGeoreferencing(*georef);
	QDialog::accept();
}



// ### GeoreferencingTool ###

GeoreferencingTool::GeoreferencingTool(GeoreferencingDialog* dialog, MapEditorController* controller, QAction* action)
: MapEditorTool(controller, Other, action), dialog(dialog)
{
	if (!cursor)
		cursor = new QCursor(QPixmap(":/images/cursor-crosshair.png"), 11, 11); // TODO: custom icon
}

GeoreferencingTool::~GeoreferencingTool()
{
	dialog->toolDeleted();
}

void GeoreferencingTool::init()
{
	setStatusBarText(tr("<b>Left click</b> to set the reference point, another button to cancel"));
}

bool GeoreferencingTool::mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (event->button() == Qt::LeftButton)
	{
		dialog->setRefPoint(map_coord.toMapCoord());
	}
	QTimer::singleShot(0, dialog, SIGNAL(exec()));
	return true;
}

QCursor* GeoreferencingTool::cursor = NULL;
