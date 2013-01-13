/*
 *    Copyright 2012, 2013 Thomas Schöps, Kai Pastor
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

#if QT_VERSION < 0x050000
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QXmlStreamReader>

#include "georeferencing.h"
#include "gui/main_window.h"
#include "map.h"
#include "map_editor.h"
#include "map_dialog_rotate.h"
#include "util_gui.h"

GeoreferencingDialog::GeoreferencingDialog(MapEditorController* controller, const Georeferencing* initial, bool allow_no_georeferencing)
: QDialog(controller->getWindow(), Qt::WindowSystemMenuHint | Qt::WindowTitleHint),
  controller(controller), 
  map(controller->getMap()),
  allow_no_georeferencing(allow_no_georeferencing),
  zone_update_in_progress(false)
{
	init(initial);
}

GeoreferencingDialog::GeoreferencingDialog(QWidget* parent, Map* map, const Georeferencing* initial, bool allow_no_georeferencing)
: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint),
  controller(NULL), 
  map(map),
  allow_no_georeferencing(allow_no_georeferencing),
  zone_update_in_progress(false)
{
	init(initial);
}

void GeoreferencingDialog::init(const Georeferencing* initial)
{
	setWindowModality(Qt::WindowModal);
	
	tool_active = false;
	declination_query_in_progress = false;
	
	// A working copy of the current or given initial Georeferencing
	initial_georef = (initial == NULL) ? &map->getGeoreferencing() : initial;
	georef.reset( new Georeferencing(*initial_georef) );
	
	setWindowTitle(tr("Map Georeferencing"));
	
	// Create widgets
	QLabel* map_crs_label = Util::Headline::create(tr("Map coordinate reference system"));
	
	crs_edit = new ProjectedCRSSelector();
	crs_edit->addCustomItem(tr("- none -"), 0);
	crs_edit->addCustomItem(tr("- from Proj.4 specification -"), 1);
	crs_edit->addCustomItem(tr("- local -"), 2);
	
	crs_spec_label = new QLabel(tr("CRS specification:"));
	crs_spec_edit = new QLineEdit();
	status_label = new QLabel(tr("Status:"));
	status_display_label = new QLabel();
	
	QLabel* reference_point_label = Util::Headline::create(tr("Reference point"));
	
	ref_point_button = new QPushButton(tr("&Pick on map"));
	int ref_point_button_width = ref_point_button->sizeHint().width();
	QLabel* geographic_datum_label = new QLabel(tr("(Datum: WGS84)"));
	int geographic_datum_label_width = geographic_datum_label->sizeHint().width();
	
	map_x_edit = Util::SpinBox::create(2, -1 * std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), tr("mm"));
	map_y_edit = Util::SpinBox::create(2, -1 * std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), tr("mm"));
	ref_point_button->setEnabled(controller != NULL);
	QHBoxLayout* map_ref_layout = new QHBoxLayout();
	map_ref_layout->addWidget(map_x_edit, 1);
	map_ref_layout->addWidget(new QLabel(tr("X", "x coordinate")), 0);
	map_ref_layout->addWidget(map_y_edit, 1);
	map_ref_layout->addWidget(new QLabel(tr("Y", "y coordinate")), 0);
	if (ref_point_button_width < geographic_datum_label_width)
		map_ref_layout->addSpacing(geographic_datum_label_width - ref_point_button_width);
	map_ref_layout->addWidget(ref_point_button, 0);
	
	easting_edit = Util::SpinBox::create(2, -1 * std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), tr("m"));
	northing_edit = Util::SpinBox::create(2, -1 * std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), tr("m"));
	QHBoxLayout* projected_ref_layout = new QHBoxLayout();
	projected_ref_layout->addWidget(easting_edit, 1);
	projected_ref_layout->addWidget(new QLabel(tr("E", "west / east")), 0);
	projected_ref_layout->addWidget(northing_edit, 1);
	projected_ref_layout->addWidget(new QLabel(tr("N", "north / south")), 0);
	projected_ref_layout->addSpacing(qMax(ref_point_button_width, geographic_datum_label_width));
	
	projected_ref_label = new QLabel();
	lat_edit = Util::SpinBox::create(8, -90.0, +90.0, trUtf8("°"));
	lon_edit = Util::SpinBox::create(8, -180.0, +180.0, trUtf8("°"));
	QHBoxLayout* geographic_ref_layout = new QHBoxLayout();
	geographic_ref_layout->addWidget(lat_edit, 1);
	geographic_ref_layout->addWidget(new QLabel(tr("N", "north")), 0);
	geographic_ref_layout->addWidget(lon_edit, 1);
	geographic_ref_layout->addWidget(new QLabel(tr("E", "east")), 0);
	if (geographic_datum_label_width < ref_point_button_width)
		geographic_ref_layout->addSpacing(ref_point_button_width - geographic_datum_label_width);
	geographic_ref_layout->addWidget(geographic_datum_label, 0);
	
	show_refpoint_label = new QLabel(tr("Show reference point in:"));
	link_label = new QLabel();
	link_label->setOpenExternalLinks(true);
	
	keep_projected_radio = new QRadioButton(tr("Projected coordinates"));
	keep_geographic_radio = new QRadioButton(tr("Geographic coordinates"));
	keep_geographic_radio->setChecked(true);
	
	QLabel* map_north_label = Util::Headline::create(tr("Map north"));
	
	declination_edit = Util::SpinBox::create(1, -180.0, +180.0, trUtf8("°"));
	declination_button = new QPushButton("");
	QHBoxLayout* declination_layout = new QHBoxLayout();
	declination_layout->addWidget(declination_edit, 1);
	declination_layout->addWidget(declination_button, 0);
	
	grivation_label = new QLabel();
	
	buttons_box = new QDialogButtonBox(
	  QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Reset | QDialogButtonBox::Help,
	  Qt::Horizontal);
	reset_button = buttons_box->button(QDialogButtonBox::Reset);
	reset_button->setEnabled(initial != NULL);
	QPushButton* help_button = buttons_box->button(QDialogButtonBox::Help);
	
	QFormLayout* edit_layout = new QFormLayout();
	
	edit_layout->addRow(map_crs_label);
	edit_layout->addRow(crs_edit);
	edit_layout->addRow(crs_spec_label, crs_spec_edit);
	edit_layout->addRow(status_label, status_display_label);
	edit_layout->addItem(Util::SpacerItem::create(this));
	
	edit_layout->addRow(reference_point_label);
	edit_layout->addRow(tr("Map coordinates:"), map_ref_layout);
	edit_layout->addRow(projected_ref_label, projected_ref_layout);
	edit_layout->addRow(tr("Geographic coordinates:"), geographic_ref_layout);
	edit_layout->addRow(show_refpoint_label, link_label);
	edit_layout->addRow(tr("On CRS changes, keep:"), keep_projected_radio);
	edit_layout->addRow("", keep_geographic_radio);
	edit_layout->addItem(Util::SpacerItem::create(this));
	
	edit_layout->addRow(map_north_label);
	edit_layout->addRow(tr("Declination:"), declination_layout);
	edit_layout->addRow(tr("Grivation:"), grivation_label);
	
	QVBoxLayout* layout = new QVBoxLayout();
	layout->addLayout(edit_layout);
	layout->addStretch();
	layout->addSpacing(16);
	layout->addWidget(buttons_box);
	
	setLayout(layout);
	
	connect(crs_edit, SIGNAL(crsEdited()), this, SLOT(crsEdited()));
	connect(crs_spec_edit, SIGNAL(textEdited(QString)), this, SLOT(crsEdited()));
	
	connect(map_x_edit, SIGNAL(valueChanged(double)), this, SLOT(mapRefChanged(double)));
	connect(map_y_edit, SIGNAL(valueChanged(double)), this, SLOT(mapRefChanged(double)));
	connect(ref_point_button, SIGNAL(clicked(bool)), this,SLOT(selectMapRefPoint()));
	
	connect(easting_edit, SIGNAL(valueChanged(double)), this, SLOT(eastingNorthingChanged(double)));
	connect(northing_edit, SIGNAL(valueChanged(double)), this, SLOT(eastingNorthingChanged(double)));
	
	connect(lat_edit, SIGNAL(valueChanged(double)), this, SLOT(latLonChanged(double)));
	connect(lon_edit, SIGNAL(valueChanged(double)), this, SLOT(latLonChanged(double)));
	
	connect(declination_edit, SIGNAL(valueChanged(double)), this, SLOT(declinationChanged(double)));
	connect(declination_button, SIGNAL(clicked(bool)), this, SLOT(requestDeclination()));
	
	connect(buttons_box, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttons_box, SIGNAL(rejected()), this, SLOT(reject()));
	connect(reset_button, SIGNAL(clicked(bool)), this, SLOT(reset()));
	connect(help_button, SIGNAL(clicked(bool)), this, SLOT(showHelp()));
	
	reset();
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

void GeoreferencingDialog::setValuesFrom(Georeferencing* values)
{
	if (values->getState() == Georeferencing::ScaleOnly)
	{
		crs_edit->selectCustomItem(0);
		crs_spec_edit->setText("");
	}
	else if (values->getState() == Georeferencing::Local)
	{
		crs_edit->selectCustomItem(2);
		crs_spec_edit->setText("");
	}
	else
	{
		QString crs_id = values->getProjectedCRSId();
		if (crs_id.isEmpty())
			crs_edit->selectCustomItem(1);
		else
		{
			CRSTemplate* temp = CRSTemplate::getCRSTemplate(crs_id);
			if (!temp || temp->getNumParams() != (int)values->getProjectedCRSParameters().size())
			{
				// The CRS id is not there anymore or the number of parameters has changed.
				// Enter as custom spec.
				crs_edit->selectCustomItem(1);
			}
			else
			{
				crs_edit->selectItem(temp);
				std::vector< QString > params = values->getProjectedCRSParameters();
				for (int i = 0; i < crs_edit->getNumParams(); ++i)
					crs_edit->setParam(i, params[i]);
			}
		}
		
		crs_spec_edit->setText(values->getProjectedCRSSpec());
	}
	
	setMapRefValuesFrom(values);
	setEastingNorthingValuesFrom(values);
	setLatLonValuesFrom(values);
	
	declination_edit->blockSignals(true);
	declination_edit->setValue(values->getDeclination());
	declination_edit->blockSignals(false);
	
	updateWidgets();
}

void GeoreferencingDialog::requestDeclination(bool no_confirm)
{
	if (georef->isLocal() || georef->getState() == Georeferencing::ScaleOnly)
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
	
	declination_query_in_progress = true;
	updateDeclinationButton();
	
	QNetworkAccessManager *network = new QNetworkAccessManager(this);
	connect(network, SIGNAL(finished(QNetworkReply*)), this, SLOT(declinationReplyFinished(QNetworkReply*)));
	
#if QT_VERSION < 0x050000
	QUrl& query = service_url;
#else
	QUrlQuery query;
#endif
	query.addQueryItem("lat1", QString::number(latlon.getLatitudeInDegrees()));
	query.addQueryItem("lon1", QString::number(latlon.getLongitudeInDegrees()));
	query.addQueryItem("resultFormat", "xml");
#if QT_VERSION >= 0x050000
	service_url.setQuery(query);
#endif
	network->get(QNetworkRequest(service_url));
}

void GeoreferencingDialog::setMapRefPoint(MapCoord coords)
{
	georef->setMapRefPoint(coords);
	setMapRefValuesFrom(georef.data());
	reset_button->setEnabled(true);
}

void GeoreferencingDialog::setKeepProjectedRefCoords()
{
	keep_projected_radio->setChecked(true);
}

void GeoreferencingDialog::setKeepGeographicRefCoords()
{
	keep_geographic_radio->setChecked(true);
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
	*georef.data() = *initial_georef;
	setValuesFrom(georef.data());
	reset_button->setEnabled(false);
}

void GeoreferencingDialog::accept()
{
	float declination_change_degrees = georef->getDeclination() - initial_georef->getDeclination();
	bool declination_changed = initial_georef->isValid() && declination_change_degrees != 0;
	if (declination_changed &&
		(map->getNumObjects() > 0 ||
		map->getNumTemplates() > 0))
	{
		int result = QMessageBox::question(this, tr("Declination change"), tr("The declination has been changed. Do you want to rotate the map content accordingly, too?"), QMessageBox::Yes | QMessageBox::No);
		if (result == QMessageBox::Yes)
		{
			RotateMapDialog dialog(this, map);
			dialog.setWindowModality(Qt::WindowModal);
			dialog.setRotationDegrees(declination_change_degrees);
			dialog.setRotateAroundGeorefRefPoint();
			dialog.setAdjustDeclination(false);
			dialog.showAdjustDeclination(false);
			dialog.exec();
		}
	}
	
	map->setGeoreferencing(*georef);
	map->setOtherDirty(true);
	QDialog::accept();
}

void GeoreferencingDialog::updateWidgets()
{
	// Dialog is enabled if anything different from "none" is selected
	bool dialog_enabled = crs_edit->getSelectedCustomItemId() != 0;
	bool proj_spec_visible = crs_edit->getSelectedCustomItemId() == 1;
	crs_spec_label->setVisible(proj_spec_visible);
	crs_spec_edit->setVisible(proj_spec_visible);
	status_label->setVisible(proj_spec_visible);
	status_display_label->setVisible(proj_spec_visible);
	if (status_display_label->isVisible())
	{
		QString error = georef->getErrorText();
		if (error.length() == 0)
			status_display_label->setText(tr("valid"));
		else
			status_display_label->setText(QString("<b style=\"color:red\">") % error % "</b>");
	}
	
	map_x_edit->setEnabled(dialog_enabled);
	map_y_edit->setEnabled(dialog_enabled);
	ref_point_button->setEnabled(dialog_enabled && controller != NULL);
	
	QString projected_ref_label_string;
	if (proj_spec_visible || !dialog_enabled)
		projected_ref_label_string = tr("Projected coordinates:");
	else if (crs_edit->getSelectedCustomItemId() == 2)
		projected_ref_label_string = tr("Local coordinates:");
	else if (dialog_enabled)
		projected_ref_label_string = crs_edit->getSelectedCRSTemplate()->getCoordinatesName() + ":";
	if (!projected_ref_label_string.isEmpty())
		projected_ref_label->setText(projected_ref_label_string);
	easting_edit->setEnabled(dialog_enabled);
	northing_edit->setEnabled(dialog_enabled);
	
	bool geographic_coords_enabled =
		dialog_enabled &&
		(proj_spec_visible ||
		 crs_edit->getSelectedCustomItemId() == -1);
	lat_edit->setEnabled(geographic_coords_enabled);
	lon_edit->setEnabled(geographic_coords_enabled);
	
	link_label->setEnabled(geographic_coords_enabled);
	double latitude = georef->getGeographicRefPoint().getLatitudeInDegrees();
	double longitude = georef->getGeographicRefPoint().getLongitudeInDegrees();
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
	
	declination_edit->setEnabled(dialog_enabled);
	updateDeclinationButton();
	
	updateNorth();
	
	bool ok_enabled = (georef->getState() == Georeferencing::ScaleOnly && allow_no_georeferencing) || georef->isValid();
	buttons_box->button(QDialogButtonBox::Ok)->setEnabled(ok_enabled);
}

void GeoreferencingDialog::updateDeclinationButton()
{
	bool dialog_enabled = crs_edit->getSelectedCustomItemId() != 0;
	bool proj_spec_visible = crs_edit->getSelectedCustomItemId() == 1;
	bool geographic_coords_enabled =
		dialog_enabled &&
		(proj_spec_visible ||
		 crs_edit->getSelectedCustomItemId() == -1);
	
	declination_button->setEnabled(geographic_coords_enabled && !declination_query_in_progress);
	declination_button->setText(declination_query_in_progress ? tr("Loading...") : tr("Lookup..."));
}

void GeoreferencingDialog::crsEdited()
{
	QString spec = crs_edit->getSelectedCRSSpec();
	if (!spec.isEmpty())
		crs_spec_edit->setText(spec);
	
	int selected_item_id = crs_edit->getSelectedCustomItemId();
	if (selected_item_id == -1)
	{
		// CRS from list
		CRSTemplate* temp = crs_edit->getSelectedCRSTemplate();
		Q_ASSERT(temp);
		
		georef->setProjectedCRS(temp->getId(), crs_edit->getSelectedCRSSpec());
		std::vector<QString> crs_params;
		for (int i = 0; i < temp->getNumParams(); ++i)
			crs_params.push_back(crs_edit->getParam(i));
		georef->setProjectedCRSParameters(crs_params);
	}
	else if (selected_item_id == 0)
	{
		// None
		georef->setState(Georeferencing::ScaleOnly);
	}
	else if (selected_item_id == 1)
	{
		// CRS from spec edit, no id
		georef->setProjectedCRS("", crs_spec_edit->text());
		georef->setProjectedCRSParameters(std::vector<QString>());
	}
	else if (selected_item_id == 2)
	{
		// Local
		georef->setProjectedCRS("", "");
		georef->setProjectedCRSParameters(std::vector<QString>());
		georef->setState(Georeferencing::Local);
	}
	
	if (selected_item_id == -1 || selected_item_id == 1)
	{
		if (keep_geographic_radio->isChecked())
		{
			// TODO: This makes it impossible (well, not impossible, but hard)
			//       to use another UTM zone than the "correct" one, because
			//       this here is called when changing the zone parameter,
			//       which will then be reset by the updateZone() inside latLonChanged().
			//       Solution: determine if this call comes from a zone edit,
			//       don't call updateZone() in this case - this also applies to the call below!
			latLonChanged();
		}
		else
		{
			eastingNorthingChanged();
			updateZone();
		}
	}
	
	updateNorth();
	updateWidgets();
	reset_button->setEnabled(true);
}

void GeoreferencingDialog::selectMapRefPoint()
{
	if (controller != NULL)
	{
		controller->setTool(new GeoreferencingTool(this, controller));
		tool_active = true;
		hide();
	}
}

void GeoreferencingDialog::mapRefChanged(double value)
{
	MapCoord coord(map_x_edit->value(), -1 * map_y_edit->value());
	setMapRefPoint(coord);
}

void GeoreferencingDialog::eastingNorthingChanged(double value)
{
	QPointF easting_northing(easting_edit->value(), northing_edit->value());
	georef->setProjectedRefPoint(easting_northing);
	setLatLonValuesFrom(georef.data());
	
	updateWidgets();
	reset_button->setEnabled(true);
}

void GeoreferencingDialog::latLonChanged(double value)
{
	double latitude = lat_edit->value() * M_PI / 180;
	double longitude = lon_edit->value() * M_PI / 180;
	georef->setGeographicRefPoint(LatLon(latitude, longitude));
	setEastingNorthingValuesFrom(georef.data());
	
	updateZone();
	updateWidgets();
	reset_button->setEnabled(true);
}

void GeoreferencingDialog::declinationChanged(double value)
{
	georef->setDeclination(value);
	updateNorth();
	reset_button->setEnabled(true);
}

void GeoreferencingDialog::declinationReplyFinished(QNetworkReply* reply)
{
	declination_query_in_progress = false;
	updateDeclinationButton();
	
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

void GeoreferencingDialog::updateZone()
{
	// HACK to prevent infinite recursion via crsEdited().
	if (zone_update_in_progress)
		return;
	
	CRSTemplate* temp = crs_edit->getSelectedCRSTemplate();
	if (!temp || temp->getId() != "UTM")
		return;
	
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
		if (zone != crs_edit->getParam(0))
		{
			crs_edit->setParam(0, zone);
			zone_update_in_progress = true;
			crsEdited();
			zone_update_in_progress = false;
		}
	}
}

void GeoreferencingDialog::updateNorth()
{
	grivation_label->setText(trUtf8("%1 °", "degree value").arg(QLocale().toString(georef->getGrivation(), 'f', 1)));
}

void GeoreferencingDialog::setMapRefValuesFrom(Georeferencing* values)
{
	map_x_edit->blockSignals(true);
	map_y_edit->blockSignals(true);
	map_x_edit->setValue(values->getMapRefPoint().xd());
	map_y_edit->setValue(-1 * values->getMapRefPoint().yd());
	map_x_edit->blockSignals(false);
	map_y_edit->blockSignals(false);
}

void GeoreferencingDialog::setEastingNorthingValuesFrom(Georeferencing* values)
{
	easting_edit->blockSignals(true);
	northing_edit->blockSignals(true);
	easting_edit->setValue(values->getProjectedRefPoint().x());
	northing_edit->setValue(values->getProjectedRefPoint().y());
	easting_edit->blockSignals(false);
	northing_edit->blockSignals(false);
}

void GeoreferencingDialog::setLatLonValuesFrom(Georeferencing* values)
{
	lat_edit->blockSignals(true);
	lon_edit->blockSignals(true);
	lat_edit->setValue(values->getGeographicRefPoint().getLatitudeInDegrees());
	lon_edit->setValue(values->getGeographicRefPoint().getLongitudeInDegrees());
	lat_edit->blockSignals(false);
	lon_edit->blockSignals(false);
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
		dialog->setMapRefPoint(map_coord.toMapCoord());
	}
	QTimer::singleShot(0, dialog, SIGNAL(exec()));
	return true;
}

QCursor* GeoreferencingTool::cursor = NULL;



// ### ProjectedCRSSelector ###

ProjectedCRSSelector::ProjectedCRSSelector(QWidget* parent) : QWidget(parent)
{
	num_custom_items = 0;
	crs_dropdown = new QComboBox();
	for (int i = 0; i < CRSTemplate::getNumCRSTemplates(); ++i)
	{
		CRSTemplate& temp = CRSTemplate::getCRSTemplate(i);
		crs_dropdown->addItem(temp.getId(), qVariantFromValue<void*>(&temp));
	}
	
	connect(crs_dropdown, SIGNAL(currentIndexChanged(int)), this, SLOT(crsDropdownChanged(int)));
	
	layout = NULL;
	crsDropdownChanged(crs_dropdown->currentIndex());
}

void ProjectedCRSSelector::addCustomItem(const QString& text, int id)
{
	crs_dropdown->insertItem(num_custom_items, text, QVariant(id));
	if (num_custom_items == 0)
		crs_dropdown->insertSeparator(1);
	++num_custom_items;
}

CRSTemplate* ProjectedCRSSelector::getSelectedCRSTemplate()
{
	QVariant item_data = crs_dropdown->itemData(crs_dropdown->currentIndex());
	if (!item_data.canConvert<void*>())
		return NULL;
	return static_cast<CRSTemplate*>(item_data.value<void*>());
}

QString ProjectedCRSSelector::getSelectedCRSSpec()
{
	QVariant item_data = crs_dropdown->itemData(crs_dropdown->currentIndex());
	if (!item_data.canConvert<void*>())
		return QString();
	
	CRSTemplate* temp = static_cast<CRSTemplate*>(item_data.value<void*>());
	QString spec = temp->getSpecTemplate();
	
	for (int param = 0; param < temp->getNumParams(); ++param)
	{
		QWidget* edit_widget = layout->itemAt(1 + param, QFormLayout::FieldRole)->widget();
		spec = spec.arg(temp->getParam(param).getSpecValue(edit_widget));
	}
	
	return spec;
}

int ProjectedCRSSelector::getSelectedCustomItemId()
{
	QVariant item_data = crs_dropdown->itemData(crs_dropdown->currentIndex());
	if (!item_data.canConvert<int>())
		return -1;
	return item_data.toInt();
}

void ProjectedCRSSelector::selectItem(CRSTemplate* temp)
{
	int index = crs_dropdown->findData(qVariantFromValue<void*>(temp));
	blockSignals(true);
	crs_dropdown->setCurrentIndex(index);
	blockSignals(false);
}

void ProjectedCRSSelector::selectCustomItem(int id)
{
	int index = crs_dropdown->findData(QVariant(id));
	blockSignals(true);
	crs_dropdown->setCurrentIndex(index);
	blockSignals(false);
}

int ProjectedCRSSelector::getNumParams()
{
	CRSTemplate* temp = getSelectedCRSTemplate();
	if (temp == NULL)
		return 0;
	else
		return temp->getNumParams();
}

QString ProjectedCRSSelector::getParam(int i)
{
	CRSTemplate* temp = getSelectedCRSTemplate();
	Q_ASSERT(temp != NULL);
	Q_ASSERT(i < temp->getNumParams());
	
	int widget_index = 3 + 2 * i;
	return temp->getParam(i).getValue(layout->itemAt(widget_index)->widget());
}

void ProjectedCRSSelector::setParam(int i, const QString& value)
{
	CRSTemplate* temp = getSelectedCRSTemplate();
	Q_ASSERT(temp != NULL);
	Q_ASSERT(i < temp->getNumParams());
	
	int widget_index = 3 + 2 * i;
	QWidget* edit_widget = layout->itemAt(widget_index)->widget();
	edit_widget->blockSignals(true);
	temp->getParam(i).setValue(edit_widget, value);
	edit_widget->blockSignals(false);
}

void ProjectedCRSSelector::crsDropdownChanged(int index)
{
	if (layout)
	{
		for (int i = 2 * layout->rowCount() - 1; i >= 2; --i)
			delete layout->takeAt(i)->widget();
		if (layout->count() > 0)
		{
			layout->takeAt(1);
			delete layout->takeAt(0)->widget();
		}
		delete layout;
		layout = NULL;
	}
	
	layout = new QFormLayout();
	layout->addRow(tr("&Coordinate reference system:"), crs_dropdown);
	
	QVariant item_data = crs_dropdown->itemData(crs_dropdown->currentIndex());
	if (item_data.canConvert<void*>())
	{
		CRSTemplate* temp = static_cast<CRSTemplate*>(item_data.value<void*>());
		for (int param = 0; param < temp->getNumParams(); ++param)
		{
			layout->addRow(temp->getParam(param).desc + ":", temp->getParam(param).createEditWidget(this));
		}
	}
	
	setLayout(layout);
	
	emit crsEdited();
}

void ProjectedCRSSelector::crsParamEdited(QString dont_use)
{
	emit crsEdited();
}



// ### SelectCRSDialog ###

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
	crs_edit = new ProjectedCRSSelector();
	
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
	connect(crs_edit, SIGNAL(crsEdited()), this, SLOT(updateWidgets()));
	connect(crs_spec_edit, SIGNAL(textEdited(QString)), this, SLOT(crsSpecEdited(QString)));
	connect(button_box, SIGNAL(accepted()), this, SLOT(accept()));
	connect(button_box, SIGNAL(rejected()), this, SLOT(reject()));
	
	updateWidgets();
}

QString SelectCRSDialog::getCRSSpec() const
{
	return crs_spec_edit->text();
}

void SelectCRSDialog::crsSpecEdited(QString text)
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
