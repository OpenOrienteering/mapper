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

#include "georeferencing.h"
#include "map.h"
#include "map_editor.h"

#define HEADLINE(text) QLabel(QString("<b>") % text % "</b>")

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

	// A working copy of the current or given initial Georeferencing
	georef.reset( new Georeferencing(initial == NULL ? map->getGeoreferencing() : *initial) );
	
	setWindowTitle(tr("Map Georeferencing"));
	
	scale_edit  = new QLabel();
	grivation_edit = new QDoubleSpinBox();
	grivation_edit->setSuffix(QString::fromUtf8(" °"));
	grivation_edit->setDecimals(1);
	grivation_edit->setSingleStep(0.1);
	grivation_edit->setRange(-180.0, +180.0);
	grivation_edit->setWhatsThis("<a href=\"gps_menu.html\">See more</a>");
	ref_point_edit = new QLabel();
	QPushButton* ref_point_button = new QPushButton("&Select...");
	ref_point_button->setEnabled(controller != NULL);
	QHBoxLayout* ref_point_layout = new QHBoxLayout();
	ref_point_layout->addWidget(ref_point_edit, 1);
	ref_point_layout->addWidget(ref_point_button, 0);
	updateGeneral();
	
	crs_edit = new QComboBox();
	crs_edit->setEditable(true);
	// TODO: move crs registry out of georeferencing GUI
	crs_edit->addItem(tr("Local coordinates"), "");
	crs_edit->addItem("UTM", "+proj=utm +zone=!ZONE!");
	crs_edit->addItem("Gauss-Krueger zone 3, datum: Potsdam", "+proj=tmerc +lat_0=0 +lon_0=9 +k=1.000000 +x_0=3500000 +y_0=0 +ellps=bessel +datum=potsdam +units=m +no_defs");
	crs_edit->addItem(tr("Edit projection parameters..."), "!EDIT!");
	updateCRS();
	
	zone_edit = new QLineEdit();
	updateZone();
	
	easting_edit = new QDoubleSpinBox();
	easting_edit->setSuffix(" m");
	easting_edit->setDecimals(0);
	easting_edit->setRange(std::numeric_limits<double>::min(), std::numeric_limits<double>::max());
	easting_edit->setWhatsThis("<a href=\"gps_menu.html\">See more</a>");
	northing_edit = new QDoubleSpinBox();
	northing_edit->setSuffix(" m");
	northing_edit->setDecimals(0);
	northing_edit->setRange(std::numeric_limits<double>::min(), std::numeric_limits<double>::max());
	northing_edit->setWhatsThis("<a href=\"gps_menu.html\">See more</a>");
	updateEastingNorthing();
	
	QLabel* datum_edit = new QLabel("WGS84");
	lat_edit = new QDoubleSpinBox();
	lat_edit->setSuffix(QString::fromUtf8(" °"));
	lat_edit->setDecimals(8);
	lat_edit->setRange(-90.0, +90.0);
	lat_edit->setWhatsThis("<a href=\"gps_menu.html\">See more</a>");
	lon_edit = new QDoubleSpinBox();
	lon_edit->setSuffix(QString::fromUtf8(" °"));
	lon_edit->setDecimals(8);
	lon_edit->setRange(-90.0, +90.0);
	lon_edit->setWhatsThis("<a href=\"gps_menu.html\">See more</a>");
	link_label = new QLabel();
	link_label->setOpenExternalLinks(true);
	updateLatLon();
	
	QDialogButtonBox* buttons_box = new QDialogButtonBox(
	  QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Reset,
	  Qt::Horizontal);
	reset_button = buttons_box->button(QDialogButtonBox::Reset);
	reset_button->setEnabled(initial != NULL);
	
	QFormLayout* edit_layout = new QFormLayout;
	
	edit_layout->addRow(new HEADLINE(tr("General")));
	edit_layout->addRow(tr("Map scale:"), scale_edit);
	edit_layout->addRow(tr("&Grivation:"), grivation_edit);
	edit_layout->addRow(tr("Reference point:"), ref_point_layout);
	edit_layout->addRow(new QWidget());
	
	edit_layout->addRow(new HEADLINE(tr("Projected coordinates")));
	edit_layout->addRow(tr("&Coordinate reference system:"), crs_edit);
	edit_layout->addRow(tr("&Zone:"), zone_edit);
	edit_layout->addRow(tr("Reference point &easting:"), easting_edit);
	edit_layout->addRow(tr("Reference point &northing:"), northing_edit);
	edit_layout->addRow(new QWidget());
	
	edit_layout->addRow(new HEADLINE(tr("Geographic coordinates")));
	edit_layout->addRow(tr("Datum"), datum_edit);
	edit_layout->addRow(tr("Reference point &latitude:"), lat_edit);
	edit_layout->addRow(tr("Reference point longitude:"), lon_edit);
	edit_layout->addRow(link_label);
	edit_layout->addRow(new QWidget());
	
	QVBoxLayout* layout = new QVBoxLayout();
	layout->addLayout(edit_layout);
	layout->addStretch();
	layout->addWidget(buttons_box);
	
	setLayout(layout);
	
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
	grivation_edit->setValue(georef->getGrivation());
	const MapCoord& coords(georef->getMapRefPoint());
	ref_point_edit->setText(tr("%1 %2 (mm)").arg(locale().toString(coords.xd())).arg(locale().toString(coords.yd())));
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
		zone_edit->blockSignals(true);
		if (crs_spec_template.startsWith("+proj=utm") && zone_edit->text().isEmpty() && !zone_edit->isModified())
		{
			// FIXME: adjust for non-standard UTM zones (e.g. Norway)
			const LatLon ref_point(georef->getGeographicRefPoint());
			QString zone = QString::number(int(floor(ref_point.longitude * 180 / M_PI) + 180) / 6 % 60 + 1, 'f', 0);
			zone.append((ref_point.latitude >= 0.0) ? " N" : " S");
			zone_edit->setText(zone);
		}
		zone_edit->blockSignals(false);
		zone_edit->setEnabled(true);
	}
	else
	{
		zone_edit->setText("");
		zone_edit->setEnabled(false);
	}
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
	  tr("Show reference point in: <a href=\"%1\">OpenStreetMap</a> | <a href=\"%2\">World of O Maps</a>").
	  arg(osm_link).
	  arg(worldofo_link)
	);
	
	if (!georef->isLocal())
	{
		lat_edit->setEnabled(true);
		lon_edit->setEnabled(true);
		link_label->setEnabled(true);
	}
	else
	{
		lat_edit->setEnabled(false);
		lon_edit->setEnabled(false);
		link_label->clear();
	}
}

void GeoreferencingDialog::grivationChanged(double value)
{
	georef->setGrivation(value);
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
		
		updateZone();
		crs_spec = crs_spec_template;
		if (crs_spec.contains("!ZONE!"))
		{
			QString zone = zone_edit->text();
			zone.replace(" N", "");
			zone.replace(" S", " +south");
			crs_spec.replace("!ZONE!", zone);
		}
	}
	
	bool crs_was_local = georef->isLocal();
	bool crs_ok = georef->setProjectedCRS(crs, crs_spec);
	if (crs_ok && (!crs_was_local || (lat_edit->cleanText().length() > 0 && lon_edit->cleanText().length() > 0)))
		latLonChanged();
	updateLatLon();
	reset_button->setEnabled(true);
}

void GeoreferencingDialog::latLonChanged()
{
	double latitude = lat_edit->value() * M_PI / 180;
	double longitude = lon_edit->value() * M_PI / 180;
	georef->setGeographicRefPoint(LatLon(latitude, longitude));
	
	updateEastingNorthing();
	updateZone();
	
	reset_button->setEnabled(true);
}

void GeoreferencingDialog::eastingNorthingChanged()
{
	QPointF easting_northing(easting_edit->value(), northing_edit->value());
	georef->setProjectedRefPoint(easting_northing);
	
	updateLatLon();
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

void GeoreferencingDialog::reset()
{
	*georef = map->getGeoreferencing();
	updateGeneral();
	updateCRS();
	updateZone();
	updateEastingNorthing();
	updateLatLon();
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



#include "georeferencing_dialog.moc"