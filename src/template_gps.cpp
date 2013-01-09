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


#include "template_gps.h"

#include <qmath.h>
#include <QtGui>

#include "map_widget.h"
#include "map_undo.h"
#include "object.h"
#include "symbol_line.h"
#include "symbol_point.h"
#include "georeferencing_dialog.h"
#include "util.h"
#include "util_task_dialog.h"
#include "util_gui.h"

TemplateTrack::TemplateTrack(const QString& path, Map* map)
 : Template(path, map)
{
	// set default value
	track_crs_spec = "+proj=latlong +datum=WGS84";
	
	const Georeferencing& georef = map->getGeoreferencing();
	connect(&georef, SIGNAL(projectionChanged()), this, SLOT(updateGeoreferencing()));
	connect(&georef, SIGNAL(transformationChanged()), this, SLOT(updateGeoreferencing()));
}

TemplateTrack::~TemplateTrack()
{
	if (template_state == Loaded)
		unloadTemplateFile();
}

void TemplateTrack::saveTypeSpecificTemplateConfiguration(QIODevice* stream)
{
	saveString(stream, track_crs_spec);
}
bool TemplateTrack::loadTypeSpecificTemplateConfiguration(QIODevice* stream, int version)
{
	if (version >= 30)
		loadString(stream, track_crs_spec);
	else
		track_crs_spec = "+proj=latlong +datum=WGS84";
	return true;
}

void TemplateTrack::saveTypeSpecificTemplateConfiguration(QXmlStreamWriter& xml)
{
	// Follow map georeferencing XML structure
	xml.writeStartElement("crs_spec");
	// TODO: xml.writeAttribute("language", "PROJ.4");
	xml.writeCharacters(track_crs_spec);
	xml.writeEndElement(/*crs_spec*/);
}
bool TemplateTrack::loadTypeSpecificTemplateConfiguration(QXmlStreamReader& xml)
{
	if (xml.name() == "crs_spec")
	{
		// TODO: check specification language
		track_crs_spec = xml.readElementText();
	}
	else
		xml.skipCurrentElement(); // unsupported
	
	return true;
}

bool TemplateTrack::saveTemplateFile()
{
    return track.saveTo(template_path);
}

bool TemplateTrack::loadTemplateFileImpl(bool configuring)
{
	if (!track.loadFrom(template_path, false))
		return false;
	
	if (!configuring)
	{
		Georeferencing* track_crs = new Georeferencing();
		if (!track_crs_spec.isEmpty())
			track_crs->setProjectedCRS("", track_crs_spec);
		track_crs->setTransformationDirectly(QTransform());
		track.setTrackCRS(track_crs);
		
		bool crs_is_geographic = track_crs_spec.contains("+proj=latlong");
		if (!is_georeferenced && crs_is_geographic)
			calculateLocalGeoreferencing();
		else
			track.changeMapGeoreferencing(map->getGeoreferencing());
	}
	
	return true;
}

bool TemplateTrack::postLoadConfiguration(QWidget* dialog_parent, bool& out_center_in_view)
{
	is_georeferenced = true;
	
	// If no track CRS is given by the template file, ask the user
	if (!track.hasTrackCRS())
	{
		if (map->getGeoreferencing().getState() == Georeferencing::ScaleOnly ||
			map->getGeoreferencing().isLocal())
			track_crs_spec = "";
		else
		{
			SelectCRSDialog dialog(map, dialog_parent, true, true, true, tr("Select the coordinate reference system of the track coordinates"));
			if (dialog.exec() == QDialog::Rejected)
				return false;
			track_crs_spec = dialog.getCRSSpec();
		}
		
		Georeferencing* track_crs = new Georeferencing();
		if (!track_crs_spec.isEmpty())
			track_crs->setProjectedCRS("", track_crs_spec);
		track_crs->setTransformationDirectly(QTransform());
		track.setTrackCRS(track_crs);
	}
	
	// If the CRS is geographic, ask if track should be loaded using map georeferencing or ad-hoc georeferencing
	track_crs_spec = track.getTrackCRS()->getProjectedCRSSpec();
	bool crs_is_geographic = track_crs_spec.contains("+proj=latlong"); // TODO: should that be case insensitive?
	if (crs_is_geographic)
	{
		TaskDialog georef_dialog(dialog_parent, tr("Opening track ..."),
			tr("Load the track in georeferenced or non-georeferenced mode?"),
			QDialogButtonBox::Abort);
		QString georef_text = tr("Positions the track according to the map's georeferencing settings.");
		if (!map->getGeoreferencing().isValid())
			georef_text += " " + tr("These are not configured yet, so they will be shown as the next step.");
		QAbstractButton* georef_button = georef_dialog.addCommandButton(tr("Georeferenced"), georef_text);
		QAbstractButton* non_georef_button = georef_dialog.addCommandButton(tr("Non-georeferenced"), tr("Projects the track using an orthographic projection with center at the track's coordinate average. Allows adjustment of the transformation and setting the map georeferencing using the adjusted track position."));
		
		georef_dialog.exec();
		if (georef_dialog.clickedButton() == georef_button)
			is_georeferenced = true;
		else if (georef_dialog.clickedButton() == non_georef_button)
			is_georeferenced = false;
		else // abort
			return false;
	}
	
	// If the CRS is local, show positioning dialog
	if (track_crs_spec.isEmpty())
	{
		is_georeferenced = false;
		
		LocalCRSPositioningDialog dialog(this, dialog_parent);
		if (dialog.exec() == QDialog::Rejected)
			return false;
		
		transform.template_scale_x = dialog.getUnitScale() / (map->getScaleDenominator() / 1000.0);
		transform.template_scale_y = transform.template_scale_x;
		updateTransformationMatrices();
		out_center_in_view = dialog.centerOnView();
	}
	
	// If the track is loaded as georeferenced and the transformation parameters
	// were not set yet, it must be done now
	if (is_georeferenced &&
		(!map->getGeoreferencing().isValid() || map->getGeoreferencing().isLocal()))
	{
		// Set default for real world reference point as some average of the track coordinates
		Georeferencing georef(map->getGeoreferencing());
		georef.setGeographicRefPoint(track.calcAveragePosition());
		
		// Show the parameter dialog
		GeoreferencingDialog dialog(dialog_parent, map, &georef);
		if (dialog.exec() == QDialog::Rejected || map->getGeoreferencing().isLocal())
			return false;
	}
	
	// If the track is loaded as not georeferenced,
	// the map coords for the track coordinates have to be calculated
	if (!is_georeferenced && crs_is_geographic)
		calculateLocalGeoreferencing();
	else
		track.changeMapGeoreferencing(map->getGeoreferencing());
	
	return true;
}

void TemplateTrack::unloadTemplateFileImpl()
{
	track.clear();
}

void TemplateTrack::drawTemplate(QPainter* painter, QRectF& clip_rect, double scale, float opacity)
{
	drawTracks(painter);

	// Get map-to-paintdevice transformation from painter
	QTransform map_to_device = painter->worldTransform();
	
	drawWaypoints(painter, map_to_device);
}

void TemplateTrack::drawTracks(QPainter* painter)
{
	painter->save();
	if (!is_georeferenced)
		applyTemplateTransform(painter);
	
	// Tracks
	// TODO: could speed that up by storing the template coords of the GPS points in a separate vector or caching the painter paths
	painter->setPen(qRgb(212, 0, 244));
	painter->setBrush(Qt::NoBrush);
	for (int i = 0; i < track.getNumSegments(); ++i)
	{
		QPainterPath path;
		int size = track.getSegmentPointCount(i);
		for (int k = 0; k < size; ++k)
		{
			const TrackPoint& point = track.getSegmentPoint(i, k);
			
			if (k > 0)
			{
				if (track.getSegmentPoint(i, k - 1).is_curve_start && k < track.getSegmentPointCount(i) - 2)
				{
					path.cubicTo(point.map_coord.toQPointF(),
						track.getSegmentPoint(i, k + 1).map_coord.toQPointF(),
						track.getSegmentPoint(i, k + 2).map_coord.toQPointF());
					k += 2;
				}
				else
					path.lineTo(point.map_coord.getX(), point.map_coord.getY());
			}
			else
				path.moveTo(point.map_coord.getX(), point.map_coord.getY());
		}
		painter->drawPath(path);
	}
	
	painter->restore();
}

void TemplateTrack::drawWaypoints(QPainter* painter, QTransform map_to_device)
{
	painter->save();
	painter->resetMatrix();
	painter->setRenderHint(QPainter::Antialiasing);
	
	painter->setPen(Qt::NoPen);
	painter->setBrush(QBrush(qRgb(255, 0, 0)));
	
	QFont font = painter->font();
	font.setPointSizeF(8);
	painter->setFont(font);
	
	int size = track.getNumWaypoints();
	for (int i = 0; i < size; ++i)
	{
		const TrackPoint& point = track.getWaypoint(i);
		const QString& point_name = track.getWaypointName(i);
		
		MapCoordF viewport_coord = point.map_coord;
		if (!is_georeferenced)
			viewport_coord = templateToMap(viewport_coord);
		QPointF viewport_point = map_to_device.map(viewport_coord.toQPointF());
		
		float radius = 0.25f / 25.4f * painter->device()->physicalDpiX();
		painter->drawEllipse(viewport_point, radius, radius);
		if (!point_name.isEmpty())
		{
			painter->setPen(qRgb(255, 0, 0));
			int width = painter->fontMetrics().width(point_name);
			painter->drawText(QRect(viewport_point.x() - 0.5f*width,
									viewport_point.y() - 2 - painter->fontMetrics().height(),
									width, painter->fontMetrics().height()),
							   Qt::AlignCenter, point_name);
			painter->setPen(Qt::NoPen);
		}
	}
	
	painter->restore();
}

QRectF TemplateTrack::getTemplateExtent()
{
	// Infinite because the extent of the waypoint texts is unknown
	return infinteRectF();
}

QRectF TemplateTrack::calculateTemplateBoundingBox()
{
	QRectF bbox;
	
	int size = track.getNumWaypoints();
	for (int i = 0; i < size; ++i)
	{
		const TrackPoint& track_point = track.getWaypoint(i);
		MapCoordF point = track_point.map_coord;
		rectIncludeSafe(bbox, is_georeferenced ? point.toQPointF() : templateToMap(point).toQPointF());
	}
	for (int i = 0; i < track.getNumSegments(); ++i)
	{
		size = track.getSegmentPointCount(i);
		for (int k = 0; k < size; ++k)
		{
			const TrackPoint& track_point = track.getSegmentPoint(i, k);
			MapCoordF point = track_point.map_coord;
			rectIncludeSafe(bbox, is_georeferenced ? point.toQPointF() : templateToMap(point).toQPointF());
		}
	}
	
	return bbox;
}

int TemplateTrack::getTemplateBoundingBoxPixelBorder()
{
	// As we don't estimate the extent of the widest waypoint text,
	// return a "very big" number to cover everything
	return 10e8;
}

Template* TemplateTrack::duplicateImpl()
{
	TemplateTrack* copy = new TemplateTrack(template_path, map);
	copy->track = Track(track);
	return copy;
}

PathObject* TemplateTrack::importPathStart()
{
	PathObject* path = new PathObject();
	path->setSymbol(map->getUndefinedLine(), true);
	return path;
}

void TemplateTrack::importPathEnd(PathObject* path)
{
	map->addObject(path);
	map->addObjectToSelection(path, false);
}

PointObject* TemplateTrack::importWaypoint(const MapCoordF& position)
{
	PointObject* point = new PointObject(map->getUndefinedPoint());
	point->setPosition(position);
	map->addObject(point);
	map->addObjectToSelection(point, false);
	return point;
}

bool TemplateTrack::import(QWidget* dialog_parent)
{
	if (track.getNumWaypoints() == 0 && track.getNumSegments() == 0)
	{
		QMessageBox::critical(dialog_parent, tr("Error"), tr("The path is empty, there is nothing to import!"));
		return false;
	}
	
	DeleteObjectsUndoStep* undo_step = new DeleteObjectsUndoStep(map);
	MapPart* part = map->getCurrentPart();
	std::vector< Object* > result;
	
	map->clearObjectSelection(false);
	
	if (track.getNumWaypoints() > 0)
	{
		int res = QMessageBox::question(dialog_parent, tr("Question"), tr("Should the waypoints be imported as a line going through all points?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
		if (res == QMessageBox::No)
		{
			for (int i = 0; i < track.getNumWaypoints(); i++)
				result.push_back(importWaypoint(templateToMap(track.getWaypoint(i).map_coord)));
		}
		else
		{
			PathObject* path = importPathStart();
			for (int i = 0; i < track.getNumWaypoints(); i++)
				path->addCoordinate(templateToMap(track.getWaypoint(i).map_coord).toMapCoord());
			importPathEnd(path);
			result.push_back(path);
		}
	}
	
	for (int i = 0; i < track.getNumSegments(); i++)
	{
		PathObject* path = importPathStart();
		for (int j = 0; j < track.getSegmentPointCount(i); j++)
		{
			const TrackPoint& track_point = track.getSegmentPoint(i, j);
			MapCoord coord = templateToMap(track_point.map_coord).toMapCoord();
			if (track_point.is_curve_start && j < track.getSegmentPointCount(i) - 3)
				coord.setCurveStart(true);
			path->addCoordinate(coord);
		}
		importPathEnd(path);
		result.push_back(path);
	}
	
	for (int i = 0; i < (int)result.size(); ++i) // keep as separate loop to get the correct (final) indices
		undo_step->addObject(part->findObjectIndex(result[i]));
	
	map->objectUndoManager().addNewUndoStep(undo_step);
	
	map->emitSelectionChanged();
	map->emitSelectionEdited();		// TODO: is this necessary here?
	
	return true;
}

void TemplateTrack::updateGeoreferencing()
{
	if (is_georeferenced && template_state == Template::Loaded)
	{
		track.changeMapGeoreferencing(map->getGeoreferencing());
		map->updateAllMapWidgets();
	}
}

void TemplateTrack::calculateLocalGeoreferencing()
{
	LatLon proj_center = track.calcAveragePosition();
	
	Georeferencing georef;
	georef.setScaleDenominator(map->getScaleDenominator());
	georef.setGeographicRefPoint(proj_center);
	georef.setProjectedCRS("", QString("+proj=ortho +datum=WGS84 +lat_0=%1 +lon_0=%2")
		.arg(proj_center.latitude * 180 / M_PI).arg(proj_center.longitude * 180 / M_PI));
	track.changeMapGeoreferencing(georef);
}


LocalCRSPositioningDialog::LocalCRSPositioningDialog(TemplateTrack* temp, QWidget* parent)
 : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{
	setWindowModality(Qt::WindowModal);
	setWindowTitle(tr("Track scaling and positioning"));
	
	QFormLayout* layout = new QFormLayout();
	
	unit_scale_edit = Util::SpinBox::create(6, 0, 1e42, tr("m", "meters"));
	unit_scale_edit->setValue(1);
	layout->addRow(tr("One coordinate unit equals:"), unit_scale_edit);
	
	original_pos_radio = new QRadioButton(tr("Position track at given coordinates"));
	layout->addRow(original_pos_radio);
	
	view_center_radio = new QRadioButton(tr("Position track at view center"));
	view_center_radio->setChecked(true);
	layout->addRow(view_center_radio);
	
	layout->addItem(Util::SpacerItem::create(this));
	
	button_box = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
	layout->addWidget(button_box);
	
	setLayout(layout);
	
	connect(button_box, SIGNAL(accepted()), this, SLOT(accept()));
	connect(button_box, SIGNAL(rejected()), this, SLOT(reject()));
}

double LocalCRSPositioningDialog::getUnitScale() const
{
	return unit_scale_edit->value();
}

bool LocalCRSPositioningDialog::centerOnView() const
{
	return view_center_radio->isChecked();
}
