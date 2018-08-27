/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2017 Kai Pastor
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

#include <vector>

#include <Qt>
#include <QtGlobal>
#include <QCursor>
#include <QDate>
#include <QDebug>
#include <QDesktopServices>  // IWYU pragma: keep
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFlags>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLatin1Char>
#include <QLatin1String>
#include <QLocale>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPixmap>
#include <QPointF>
#include <QPushButton>
#include <QRadioButton>
#include <QSignalBlocker>
#include <QSize>
#include <QSpacerItem>
#include <QString>
#include <QStringRef>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QVBoxLayout>
#include <QXmlStreamReader>
// IWYU pragma: no_include <qxmlstream.h>

#if defined(QT_NETWORK_LIB)
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#endif

#include "core/crs_template.h"
#include "core/georeferencing.h"
#include "core/latlon.h"
#include "core/map.h"
#include "gui/main_window.h"
#include "gui/map/map_dialog_rotate.h"
#include "gui/map/map_editor.h"
#include "gui/widgets/crs_selector.h"
#include "gui/util_gui.h"
#include "util/scoped_signals_blocker.h"


namespace OpenOrienteering {

// ### GeoreferencingDialog ###

GeoreferencingDialog::GeoreferencingDialog(MapEditorController* controller, const Georeferencing* initial, bool allow_no_georeferencing)
 : GeoreferencingDialog(controller->getWindow(), controller, controller->getMap(), initial, allow_no_georeferencing)
{
	// nothing else
}

GeoreferencingDialog::GeoreferencingDialog(QWidget* parent, Map* map, const Georeferencing* initial, bool allow_no_georeferencing)
 : GeoreferencingDialog(parent, nullptr, map, initial, allow_no_georeferencing)
{
	// nothing else
}

GeoreferencingDialog::GeoreferencingDialog(
        QWidget* parent,
        MapEditorController* controller,
        Map* map,
        const Georeferencing* initial,
        bool allow_no_georeferencing )
 : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
 , controller(controller)
 , map(map)
 , initial_georef(initial ? initial : &map->getGeoreferencing())
 , georef(new Georeferencing(*initial_georef))
 , allow_no_georeferencing(allow_no_georeferencing)
 , tool_active(false)
 , declination_query_in_progress(false)
 , grivation_locked(!initial_georef->isValid() || initial_georef->getState() != Georeferencing::Normal)
 , original_declination(0.0)
{
	if (!grivation_locked)
		original_declination = initial_georef->getDeclination();
	
	setWindowTitle(tr("Map Georeferencing"));
	setWindowModality(Qt::WindowModal);
	
	// Create widgets
	auto map_crs_label = Util::Headline::create(tr("Map coordinate reference system"));
	
	crs_selector = new CRSSelector(*georef, nullptr);
	crs_selector->addCustomItem(tr("- local -"), Georeferencing::Local);
	
	status_label = new QLabel(tr("Status:"));
	status_field = new QLabel();
	
	/*: The grid scale factor is the ratio between a length in the grid plane
	    and the corresponding length on the curved earth model. It is applied
	    as a factor to ground distances to get grid plane distances. */
	auto scale_factor_label = new QLabel(tr("Grid scale factor:"));
	scale_factor_edit = Util::SpinBox::create(Georeferencing::scaleFactorPrecision(), 0.001, 1000.0);
	
	auto reference_point_label = Util::Headline::create(tr("Reference point"));
	
	ref_point_button = new QPushButton(tr("&Pick on map"));
	int ref_point_button_width = ref_point_button->sizeHint().width();
	auto geographic_datum_label = new QLabel(tr("(Datum: WGS84)"));
	int geographic_datum_label_width = geographic_datum_label->sizeHint().width();
	
	map_x_edit = Util::SpinBox::create<MapCoordF>(tr("mm"));
	map_y_edit = Util::SpinBox::create<MapCoordF>(tr("mm"));
	ref_point_button->setEnabled(controller);
	auto map_ref_layout = new QHBoxLayout();
	map_ref_layout->addWidget(map_x_edit, 1);
	map_ref_layout->addWidget(new QLabel(tr("X", "x coordinate")), 0);
	map_ref_layout->addWidget(map_y_edit, 1);
	map_ref_layout->addWidget(new QLabel(tr("Y", "y coordinate")), 0);
	if (ref_point_button_width < geographic_datum_label_width)
		map_ref_layout->addSpacing(geographic_datum_label_width - ref_point_button_width);
	map_ref_layout->addWidget(ref_point_button, 0);
	
	easting_edit = Util::SpinBox::create<Util::RealMeters>(tr("m"));
	northing_edit = Util::SpinBox::create<Util::RealMeters>(tr("m"));
	auto projected_ref_layout = new QHBoxLayout();
	projected_ref_layout->addWidget(easting_edit, 1);
	projected_ref_layout->addWidget(new QLabel(tr("E", "west / east")), 0);
	projected_ref_layout->addWidget(northing_edit, 1);
	projected_ref_layout->addWidget(new QLabel(tr("N", "north / south")), 0);
	projected_ref_layout->addSpacing(qMax(ref_point_button_width, geographic_datum_label_width));
	
	projected_ref_label = new QLabel();
	lat_edit = Util::SpinBox::create(8, -90.0, +90.0, trUtf8("°"));
	lon_edit = Util::SpinBox::create(8, -180.0, +180.0, trUtf8("°"));
	auto geographic_ref_layout = new QHBoxLayout();
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
	if (georef->getState() == Georeferencing::Normal && georef->isValid())
	{
		keep_geographic_radio->setChecked(true);
	}
	else
	{
		keep_geographic_radio->setEnabled(false);
		keep_projected_radio->setCheckable(true);
	}
	
	auto map_north_label = Util::Headline::create(tr("Map north"));
	
	declination_edit = Util::SpinBox::create(Georeferencing::declinationPrecision(), -180.0, +180.0, trUtf8("°"));
	declination_button = new QPushButton(tr("Lookup..."));
	auto declination_layout = new QHBoxLayout();
	declination_layout->addWidget(declination_edit, 1);
	declination_layout->addWidget(declination_button, 0);
	
	grivation_label = new QLabel();
	
	buttons_box = new QDialogButtonBox(
	  QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Reset | QDialogButtonBox::Help,
	  Qt::Horizontal);
	reset_button = buttons_box->button(QDialogButtonBox::Reset);
	reset_button->setEnabled(initial);
	auto help_button = buttons_box->button(QDialogButtonBox::Help);
	
	auto edit_layout = new QFormLayout();
	
	edit_layout->addRow(map_crs_label);
	edit_layout->addRow(tr("&Coordinate reference system:"), crs_selector);
	crs_selector->setDialogLayout(edit_layout);
	edit_layout->addRow(status_label, status_field);
	edit_layout->addRow(scale_factor_label, scale_factor_edit);
	edit_layout->addItem(Util::SpacerItem::create(this));
	
	edit_layout->addRow(reference_point_label);
	edit_layout->addRow(tr("Map coordinates:"), map_ref_layout);
	edit_layout->addRow(projected_ref_label, projected_ref_layout);
	edit_layout->addRow(tr("Geographic coordinates:"), geographic_ref_layout);
	edit_layout->addRow(show_refpoint_label, link_label);
	edit_layout->addRow(show_refpoint_label, link_label);
	edit_layout->addRow(tr("On CRS changes, keep:"), keep_projected_radio);
	edit_layout->addRow({}, keep_geographic_radio);
	edit_layout->addItem(Util::SpacerItem::create(this));
	
	edit_layout->addRow(map_north_label);
	edit_layout->addRow(tr("Declination:"), declination_layout);
	edit_layout->addRow(tr("Grivation:"), grivation_label);
	
	auto layout = new QVBoxLayout();
	layout->addLayout(edit_layout);
	layout->addStretch();
	layout->addSpacing(16);
	layout->addWidget(buttons_box);
	
	setLayout(layout);
	
	connect(crs_selector, &CRSSelector::crsChanged, this, &GeoreferencingDialog::crsEdited);
	
	using TakingDoubleArgument = void (QDoubleSpinBox::*)(double);
	connect(scale_factor_edit, (TakingDoubleArgument)&QDoubleSpinBox::valueChanged, this, &GeoreferencingDialog::scaleFactorEdited);
	
	connect(map_x_edit, (TakingDoubleArgument)&QDoubleSpinBox::valueChanged, this, &GeoreferencingDialog::mapRefChanged);
	connect(map_y_edit, (TakingDoubleArgument)&QDoubleSpinBox::valueChanged, this, &GeoreferencingDialog::mapRefChanged);
	connect(ref_point_button, &QPushButton::clicked, this, &GeoreferencingDialog::selectMapRefPoint);
	
	connect(easting_edit, (TakingDoubleArgument)&QDoubleSpinBox::valueChanged, this, &GeoreferencingDialog::eastingNorthingEdited);
	connect(northing_edit, (TakingDoubleArgument)&QDoubleSpinBox::valueChanged, this, &GeoreferencingDialog::eastingNorthingEdited);
	
	connect(lat_edit, (TakingDoubleArgument)&QDoubleSpinBox::valueChanged, this, &GeoreferencingDialog::latLonEdited);
	connect(lon_edit, (TakingDoubleArgument)&QDoubleSpinBox::valueChanged, this, &GeoreferencingDialog::latLonEdited);
	connect(keep_geographic_radio, &QRadioButton::toggled, this, &GeoreferencingDialog::keepCoordsChanged);
	
	connect(declination_edit, (TakingDoubleArgument)&QDoubleSpinBox::valueChanged, this, &GeoreferencingDialog::declinationEdited);
	connect(declination_button, &QPushButton::clicked, this, &GeoreferencingDialog::requestDeclination);
	
	connect(buttons_box, &QDialogButtonBox::accepted, this, &GeoreferencingDialog::accept);
	connect(buttons_box, &QDialogButtonBox::rejected, this, &GeoreferencingDialog::reject);
	connect(reset_button, &QPushButton::clicked, this, &GeoreferencingDialog::reset);
	connect(help_button, &QPushButton::clicked, this, &GeoreferencingDialog::showHelp);
	
	connect(georef.data(), &Georeferencing::stateChanged, this, &GeoreferencingDialog::georefStateChanged);
	connect(georef.data(), &Georeferencing::transformationChanged, this, &GeoreferencingDialog::transformationChanged);
	connect(georef.data(), &Georeferencing::projectionChanged, this, &GeoreferencingDialog::projectionChanged);
	connect(georef.data(), &Georeferencing::declinationChanged, this, &GeoreferencingDialog::declinationChanged);
	
	transformationChanged();
	georefStateChanged();
	declinationChanged();
}

GeoreferencingDialog::~GeoreferencingDialog()
{
	if (tool_active)
		controller->setOverrideTool(nullptr);
}

// slot
void GeoreferencingDialog::georefStateChanged()
{
	const QSignalBlocker block(crs_selector);
	
	switch (georef->getState())
	{
	case Georeferencing::Local:
		crs_selector->setCurrentItem(Georeferencing::Local);
		keep_geographic_radio->setEnabled(false);
		keep_projected_radio->setChecked(true);
		break;
	default:
		qDebug() << "Unhandled georeferencing state:" << georef->getState();
		// fall through
	case Georeferencing::Normal:
		projectionChanged();
		keep_geographic_radio->setEnabled(true);
	}
	
	updateWidgets();
}

// slot
void GeoreferencingDialog::transformationChanged()
{
	ScopedMultiSignalsBlocker block(
	            map_x_edit, map_y_edit,
	            easting_edit, northing_edit,
	            scale_factor_edit
	);
	
	map_x_edit->setValue(georef->getMapRefPoint().x());
	map_y_edit->setValue(-1 * georef->getMapRefPoint().y());
	
	easting_edit->setValue(georef->getProjectedRefPoint().x());
	northing_edit->setValue(georef->getProjectedRefPoint().y());
	
	scale_factor_edit->setValue(georef->getGridScaleFactor());
	
	updateGrivation();
}

// slot
void GeoreferencingDialog::projectionChanged()
{
	ScopedMultiSignalsBlocker block(
	            crs_selector,
	            lat_edit, lon_edit
	);
	
	if (georef->getState() == Georeferencing::Normal)
	{
		const std::vector< QString >& parameters = georef->getProjectedCRSParameters();
		auto temp = CRSTemplateRegistry().find(georef->getProjectedCRSId());
		if (!temp || temp->parameters().size() != parameters.size())
		{
			// The CRS id is not there anymore or the number of parameters has changed.
			// Enter as custom spec.
			crs_selector->setCurrentCRS(CRSTemplateRegistry().find(QString::fromLatin1("PROJ.4")), { georef->getProjectedCRSSpec() });
		}
		else
		{
			crs_selector->setCurrentCRS(temp, parameters);
		}
	}
	
	LatLon latlon = georef->getGeographicRefPoint();
	double latitude  = latlon.latitude();
	double longitude = latlon.longitude();
	lat_edit->setValue(latitude);
	lon_edit->setValue(longitude);
	QString osm_link =
	  QString::fromLatin1("http://www.openstreetmap.org/?lat=%1&lon=%2&zoom=18&layers=M").
	  arg(latitude).arg(longitude);
	QString worldofo_link =
	  QString::fromLatin1("http://maps.worldofo.com/?zoom=15&lat=%1&lng=%2").
	  arg(latitude).arg(longitude);
	link_label->setText(
	  tr("<a href=\"%1\">OpenStreetMap</a> | <a href=\"%2\">World of O Maps</a>").
	  arg(osm_link, worldofo_link)
	);
	
	QString error = georef->getErrorText();
	if (error.length() == 0)
		status_field->setText(tr("valid"));
	else
		status_field->setText(QLatin1String("<b style=\"color:red\">") + error + QLatin1String("</b>"));
}

// slot
void GeoreferencingDialog::declinationChanged()
{
	const QSignalBlocker block(declination_edit);
	declination_edit->setValue(georef->getDeclination());
}

void GeoreferencingDialog::requestDeclination(bool no_confirm)
{
	if (georef->isLocal())
		return;
	
	/// \todo Move URL (template) to settings.
	QString user_url(QString::fromLatin1("https://www.ngdc.noaa.gov/geomag-web/"));
	QUrl service_url(user_url + QLatin1String("calculators/calculateDeclination"));
	LatLon latlon(georef->getGeographicRefPoint());
	
	if (!no_confirm)
	{
		int result = QMessageBox::question(this, tr("Online declination lookup"),
		  trUtf8("The magnetic declination for the reference point %1° %2° will now be retrieved from <a href=\"%3\">%3</a>. Do you want to continue?").
		    arg(latlon.latitude()).arg(latlon.longitude()).arg(user_url),
		  QMessageBox::Yes | QMessageBox::No,
		  QMessageBox::Yes );
		if (result != QMessageBox::Yes)
			return;
	}
	
	QUrlQuery query;
	QDate today = QDate::currentDate();
	query.addQueryItem(QString::fromLatin1("lat1"), QString::number(latlon.latitude()));
	query.addQueryItem(QString::fromLatin1("lon1"), QString::number(latlon.longitude()));
	query.addQueryItem(QString::fromLatin1("startYear"), QString::number(today.year()));
	query.addQueryItem(QString::fromLatin1("startMonth"), QString::number(today.month()));
	query.addQueryItem(QString::fromLatin1("startDay"), QString::number(today.day()));
	
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS) || defined(Q_OS_ANDROID) || !defined(QT_NETWORK_LIB)
	// No QtNetwork or no OpenSSL: open result in system browser.
	query.addQueryItem(QString::fromLatin1("resultFormat"), QString::fromLatin1("html"));
	service_url.setQuery(query);
	QDesktopServices::openUrl(service_url);
#else
	// Use result directly
	query.addQueryItem(QString::fromLatin1("resultFormat"), QString::fromLatin1("xml"));
	service_url.setQuery(query);
	
	declination_query_in_progress = true;
	updateDeclinationButton();
	
	auto network = new QNetworkAccessManager(this);
	connect(network, &QNetworkAccessManager::finished, this, &GeoreferencingDialog::declinationReplyFinished);
	network->get(QNetworkRequest(service_url));
#endif
}

void GeoreferencingDialog::setMapRefPoint(const MapCoord& coords)
{
	georef->setMapRefPoint(coords);
	reset_button->setEnabled(true);
}

void GeoreferencingDialog::setKeepProjectedRefCoords()
{
	keep_projected_radio->setChecked(true);
	reset_button->setEnabled(true);
}

void GeoreferencingDialog::setKeepGeographicRefCoords()
{
	keep_geographic_radio->setChecked(true);
	reset_button->setEnabled(true);
}

void GeoreferencingDialog::toolDeleted()
{
	tool_active = false;
}

void GeoreferencingDialog::showHelp()
{
	Util::showHelp(parentWidget(), "georeferencing.html");
}

void GeoreferencingDialog::reset()
{
	grivation_locked = ( !initial_georef->isValid() || initial_georef->getState() != Georeferencing::Normal );
	if (!grivation_locked)
		original_declination = initial_georef->getDeclination();
	*georef.data() = *initial_georef;
	reset_button->setEnabled(false);
}

void GeoreferencingDialog::accept()
{
	float declination_change_degrees = georef->getDeclination() - initial_georef->getDeclination();
	if ( !grivation_locked &&
	     declination_change_degrees != 0 &&
	     (map->getNumObjects() > 0 || map->getNumTemplates() > 0) )
	{
		int result = QMessageBox::question(this, tr("Declination change"), tr("The declination has been changed. Do you want to rotate the map content accordingly, too?"), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
		if (result == QMessageBox::Cancel)
		{
			return;
		}
		else if (result == QMessageBox::Yes)
		{
			RotateMapDialog dialog(this, map);
			dialog.setWindowModality(Qt::WindowModal);
			dialog.setRotationDegrees(declination_change_degrees);
			dialog.setRotateAroundGeorefRefPoint();
			dialog.setAdjustDeclination(false);
			dialog.showAdjustDeclination(false);
			int result = dialog.exec();
			if (result == QDialog::Rejected)
				return;
		}
	}
	
	map->setGeoreferencing(*georef);
	QDialog::accept();
}

void GeoreferencingDialog::updateWidgets()
{
	ref_point_button->setEnabled(controller);
	
	if (crs_selector->currentCRSTemplate())
		projected_ref_label->setText(crs_selector->currentCRSTemplate()->coordinatesName(crs_selector->parameters()) + QLatin1Char(':'));
	else
		projected_ref_label->setText(tr("Local coordinates:"));
	
	bool geographic_coords_enabled = crs_selector->currentCustomItem() != Georeferencing::Local;
	status_label->setVisible(geographic_coords_enabled);
	status_field->setVisible(geographic_coords_enabled);
	lat_edit->setEnabled(geographic_coords_enabled);
	lon_edit->setEnabled(geographic_coords_enabled);
	link_label->setEnabled(geographic_coords_enabled);
	//keep_geographic_radio->setEnabled(geographic_coords_enabled);
	
	updateDeclinationButton();
	
	buttons_box->button(QDialogButtonBox::Ok)->setEnabled(georef->isValid());
}

void GeoreferencingDialog::updateDeclinationButton()
{
	/*
	bool dialog_enabled = crs_edit->getSelectedCustomItemId() != 0;
	bool proj_spec_visible = crs_edit->getSelectedCustomItemId() == 1;
	bool geographic_coords_enabled =
		dialog_enabled &&
		(proj_spec_visible ||
		 crs_edit->getSelectedCustomItemId() == -1);
	*/
	bool enabled = lat_edit->isEnabled() && !declination_query_in_progress;
	declination_button->setEnabled(enabled);
	declination_button->setText(declination_query_in_progress ? tr("Loading...") : tr("Lookup..."));
}

void GeoreferencingDialog::updateGrivation()
{
	QString text = trUtf8("%1 °", "degree value").arg(QLocale().toString(georef->getGrivation(), 'f', Georeferencing::declinationPrecision()));
	if (grivation_locked)
		text.append(QString::fromLatin1(" (%1)").arg(tr("locked")));
	grivation_label->setText(text);
}

void GeoreferencingDialog::crsEdited()
{
	Georeferencing georef_copy = *georef;
	
	auto crs_template = crs_selector->currentCRSTemplate();
	auto spec = crs_selector->currentCRSSpec();
	
	auto selected_item_id = crs_selector->currentCustomItem();
	switch (selected_item_id)
	{
	default:
		qWarning("Unsupported CRS item id");
		// fall through
	case Georeferencing::Local:
		// Local
		georef_copy.setState(Georeferencing::Local);
		break;
	case -1:
		// CRS from list
		Q_ASSERT(crs_template);
		georef_copy.setProjectedCRS(crs_template->id(), spec, crs_selector->parameters());
		georef_copy.setState(Georeferencing::Normal); // Allow invalid spec
		if (keep_geographic_radio->isChecked())
			georef_copy.setGeographicRefPoint(georef->getGeographicRefPoint(), !grivation_locked);
		else
			georef_copy.setProjectedRefPoint(georef->getProjectedRefPoint(), !grivation_locked);
		break;
	}
	
	// Apply all changes at once
	*georef = georef_copy;
	reset_button->setEnabled(true);
}

void GeoreferencingDialog::scaleFactorEdited()
{
	const QSignalBlocker block{scale_factor_edit};
	georef->setGridScaleFactor(scale_factor_edit->value());
	reset_button->setEnabled(true);
}

void GeoreferencingDialog::selectMapRefPoint()
{
	if (controller)
	{
		controller->setOverrideTool(new GeoreferencingTool(this, controller));
		tool_active = true;
		hide();
	}
}

void GeoreferencingDialog::mapRefChanged()
{
	MapCoord coord(map_x_edit->value(), -1 * map_y_edit->value());
	setMapRefPoint(coord);
}

void GeoreferencingDialog::eastingNorthingEdited()
{
	const QSignalBlocker block1(keep_geographic_radio), block2(keep_projected_radio);
	double easting   = easting_edit->value();
	double northing  = northing_edit->value();
	georef->setProjectedRefPoint(QPointF(easting, northing), !grivation_locked);
	keep_projected_radio->setChecked(true);
	reset_button->setEnabled(true);
}

void GeoreferencingDialog::latLonEdited()
{
	const QSignalBlocker block1(keep_geographic_radio), block2(keep_projected_radio);
	double latitude  = lat_edit->value();
	double longitude = lon_edit->value();
	georef->setGeographicRefPoint(LatLon(latitude, longitude), !grivation_locked);
	keep_geographic_radio->setChecked(true);
	reset_button->setEnabled(true);
}

void GeoreferencingDialog::keepCoordsChanged()
{
	if (grivation_locked && keep_geographic_radio->isChecked())
	{
		grivation_locked = false;
		original_declination = georef->getDeclination();
		updateGrivation();
	}
	reset_button->setEnabled(true);
}

void GeoreferencingDialog::declinationEdited(double value)
{
	if (grivation_locked)
	{
		grivation_locked = false;
		original_declination = georef->getDeclination();
		updateGrivation();
	}
	georef->setDeclination(value);
	reset_button->setEnabled(true);
}

void GeoreferencingDialog::declinationReplyFinished(QNetworkReply* reply)
{
#if defined(QT_NETWORK_LIB)
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
		while (xml.readNextStartElement())
		{
			if (xml.name() == QLatin1String("maggridresult"))
			{
				while(xml.readNextStartElement())
				{
					if (xml.name() == QLatin1String("result"))
					{
						while (xml.readNextStartElement())
						{
							if (xml.name() == QLatin1String("declination"))
							{
								QString text = xml.readElementText(QXmlStreamReader::IncludeChildElements);
								bool ok;
								double declination = text.toDouble(&ok);
								if (ok)
								{
									declination_edit->setValue(Georeferencing::roundDeclination(declination));
									return;
								}
								else 
								{
									error_string = tr("Could not parse data.") + QLatin1Char(' ');
								}
							}
							
							xml.skipCurrentElement(); // child of result
						}
					}
					
					xml.skipCurrentElement(); // child of mapgridresult
				}
			}
			else if (xml.name() == QLatin1String("errors"))
			{
				error_string.append(xml.readElementText(QXmlStreamReader::IncludeChildElements) + QLatin1Char(' '));
			}
			
			xml.skipCurrentElement(); // child of root
		}
		
		if (xml.error() != QXmlStreamReader::NoError)
		{
			error_string.append(xml.errorString());
		}
		else if (error_string.isEmpty())
		{
			error_string = tr("Declination value not found.");
		}
	}
	
	int result = QMessageBox::critical(this, tr("Online declination lookup"),
		tr("The online declination lookup failed:\n%1").arg(error_string),
		QMessageBox::Retry | QMessageBox::Close,
		QMessageBox::Close );
	if (result == QMessageBox::Retry)
		requestDeclination(true);
#else
	Q_UNUSED(reply)
#endif
}



// ### GeoreferencingTool ###

GeoreferencingTool::GeoreferencingTool(GeoreferencingDialog* dialog, MapEditorController* controller, QAction* action)
 : MapEditorTool(controller, Other, action)
 , dialog(dialog)
{
	// nothing
}

GeoreferencingTool::~GeoreferencingTool()
{
	dialog->toolDeleted();
}

void GeoreferencingTool::init()
{
	setStatusBarText(tr("<b>Click</b>: Set the reference point. <b>Right click</b>: Cancel."));
	MapEditorTool::init();
}

bool GeoreferencingTool::mousePressEvent(QMouseEvent* event, const MapCoordF& /*map_coord*/, MapWidget* /*widget*/)
{
	bool handled = false;
	switch (event->button())
	{
	case Qt::LeftButton:
	case Qt::RightButton:
		handled = true;
		break;
	default:
		; // nothing
	}

	return handled;
}

bool GeoreferencingTool::mouseReleaseEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* /*widget*/)
{
	bool handled = false;
	switch (event->button())
	{
	case Qt::LeftButton:
		dialog->setMapRefPoint(MapCoord(map_coord));
		// fall through
	case Qt::RightButton:
		QTimer::singleShot(0, dialog, SIGNAL(exec()));  // clazy:exclude=old-style-connect
		handled = true;
		break;
	default:
		; // nothing
	}
	
	return handled;
}

const QCursor& GeoreferencingTool::getCursor() const
{
	static auto const cursor = scaledToScreen(QCursor{ QPixmap{ QString::fromLatin1(":/images/cursor-crosshair.png") }, 11, 11 });
	return cursor;
}


}  // namespace OpenOrienteering
