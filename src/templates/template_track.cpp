/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2013-2017 Kai Pastor
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


#include "template_track.h"

#include <QCommandLinkButton>
#include <QMessageBox>
#include <QPainter>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "core/georeferencing.h"
#include "core/map.h"
#include "core/objects/object.h"
#include "core/symbols/line_symbol.h"
#include "core/symbols/point_symbol.h"
#include "gui/georeferencing_dialog.h"
#include "gui/select_crs_dialog.h"
#include "gui/task_dialog.h"
#include "templates/template_positioning_dialog.h"
#include "undo/object_undo.h"
#include "util/util.h"


namespace OpenOrienteering {

const std::vector<QByteArray>& TemplateTrack::supportedExtensions()
{
	static std::vector<QByteArray> extensions = { "dxf", "gpx", "osm" };
	return extensions;
}

TemplateTrack::TemplateTrack(const QString& path, Map* map)
 : Template(path, map)
{
	// set default value
	track_crs_spec = Georeferencing::geographic_crs_spec;
	
	const Georeferencing& georef = map->getGeoreferencing();
	connect(&georef, &Georeferencing::projectionChanged, this, &TemplateTrack::updateGeoreferencing);
	connect(&georef, &Georeferencing::transformationChanged, this, &TemplateTrack::updateGeoreferencing);
	connect(&georef, &Georeferencing::stateChanged, this, &TemplateTrack::updateGeoreferencing);
	connect(&georef, &Georeferencing::declinationChanged, this, &TemplateTrack::updateGeoreferencing);
}

TemplateTrack::~TemplateTrack()
{
	if (template_state == Loaded)
		unloadTemplateFile();
}



void TemplateTrack::saveTypeSpecificTemplateConfiguration(QXmlStreamWriter& xml) const
{
	if (preserved_georef)
	{
		// Preserve explicit georeferencing from OgrTemplate.
		preserved_georef->save(xml);
		return;
	}
	
	// Follow map georeferencing XML structure
	xml.writeStartElement(QString::fromLatin1("crs_spec"));
	xml.writeCharacters(track_crs_spec);
	xml.writeEndElement(/*crs_spec*/);
	if (!projected_crs_spec.isEmpty())
	{
		Q_ASSERT(!is_georeferenced);
		xml.writeStartElement(QString::fromLatin1("projected_crs_spec"));
		xml.writeCharacters(projected_crs_spec);
		xml.writeEndElement(/*crs_spec*/);
	}
}


bool TemplateTrack::loadTypeSpecificTemplateConfiguration(QXmlStreamReader& xml)
{
	if (xml.name() == QLatin1String("crs_spec"))
	{
		track_crs_spec = xml.readElementText();
	}
	else if (xml.name() == QLatin1String("projected_crs_spec"))
	{
		Q_ASSERT(!is_georeferenced);
		projected_crs_spec = xml.readElementText();
	}
	else if (xml.name() == QLatin1String("georeferencing"))
	{
		// Preserve explicit georeferencing from OgrTemplate.
		preserved_georef.reset(new Georeferencing());
		preserved_georef->load(xml, false);
	}
	else
	{
		xml.skipCurrentElement(); // unsupported
	}
	
	return true;
}


bool TemplateTrack::saveTemplateFile() const
{
    return track.saveTo(template_path);
}

bool TemplateTrack::loadTemplateFileImpl(bool configuring)
{
	if (preserved_georef)
	{
		setErrorString(tr("This template must be loaded with GDAL/OGR."));
		return false;
	}
	
	if (!track.loadFrom(template_path, false))
		return false;
	
	if (!configuring)
	{
		Georeferencing* track_crs = new Georeferencing();
		if (!track_crs_spec.isEmpty())
			track_crs->setProjectedCRS(QString{}, track_crs_spec);
		track_crs->setTransformationDirectly(QTransform());
		track.setTrackCRS(track_crs);
		
		bool crs_is_geographic = track_crs_spec.contains(QLatin1String("+proj=latlong"));
		if (!is_georeferenced && crs_is_geographic)
		{
			if (projected_crs_spec.isEmpty())
				projected_crs_spec = calculateLocalGeoreferencing();
			applyProjectedCrsSpec();
		}
		else
		{
			projected_crs_spec.clear();
			track.changeMapGeoreferencing(map->getGeoreferencing());
		}
	}
	
	return true;
}

bool TemplateTrack::postLoadConfiguration(QWidget* dialog_parent, bool& out_center_in_view)
{
	is_georeferenced = true;
	
	// If no track CRS is given by the template file, ask the user
	if (!track.hasTrackCRS())
	{
		if (map->getGeoreferencing().isLocal())
		{
			track_crs_spec.clear();
		}
		else
		{
			SelectCRSDialog dialog(
			            map->getGeoreferencing(),
			            dialog_parent,
			            SelectCRSDialog::TakeFromMap | SelectCRSDialog::Local | SelectCRSDialog::Geographic,
			            tr("Select the coordinate reference system of the track coordinates") );
			if (dialog.exec() == QDialog::Rejected)
				return false;
			track_crs_spec = dialog.currentCRSSpec();
		}
		
		Georeferencing* track_crs = new Georeferencing();
		if (!track_crs_spec.isEmpty())
			track_crs->setProjectedCRS(QString{}, track_crs_spec);
		track_crs->setTransformationDirectly(QTransform());
		track.setTrackCRS(track_crs);
	}
	
	// If the CRS is geographic, ask if track should be loaded using map georeferencing or ad-hoc georeferencing
	track_crs_spec = track.getTrackCRS()->getProjectedCRSSpec();
	bool crs_is_geographic = track_crs_spec.contains(QLatin1String("+proj=latlong")); // TODO: should that be case insensitive?
	if (crs_is_geographic)
	{
		TaskDialog georef_dialog(dialog_parent, tr("Opening track ..."),
			tr("Load the track in georeferenced or non-georeferenced mode?"),
			QDialogButtonBox::Abort);
		QString georef_text = tr("Positions the track according to the map's georeferencing settings.");
		if (!map->getGeoreferencing().isValid())
			georef_text += QLatin1Char(' ') + tr("These are not configured yet, so they will be shown as the next step.");
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
		
		TemplatePositioningDialog dialog(dialog_parent);
		if (dialog.exec() == QDialog::Rejected)
			return false;
		
		transform.template_scale_x = dialog.getUnitScale();
		if (!dialog.useRealCoords())
		{
			transform.template_scale_x /= (map->getScaleDenominator() / 1000.0);
		}
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
		dialog.setKeepGeographicRefCoords();
		if (dialog.exec() == QDialog::Rejected || map->getGeoreferencing().isLocal())
			return false;
	}
	
	// If the track is loaded as not georeferenced,
	// the map coords for the track coordinates have to be calculated
	if (!is_georeferenced && crs_is_geographic)
	{
		if (projected_crs_spec.isEmpty())
			projected_crs_spec = calculateLocalGeoreferencing();
		applyProjectedCrsSpec();
	}
	else
	{
		projected_crs_spec.clear();
		track.changeMapGeoreferencing(map->getGeoreferencing());
	}
	
	return true;
}

void TemplateTrack::unloadTemplateFileImpl()
{
	track.clear();
}

void TemplateTrack::drawTemplate(QPainter* painter, const QRectF& clip_rect, double scale, bool on_screen, float opacity) const
{
	Q_UNUSED(clip_rect);
	Q_UNUSED(scale);
	
	painter->save();
	painter->setOpacity(opacity);
	drawTracks(painter, on_screen);
	drawWaypoints(painter);
	painter->restore();
}

void TemplateTrack::drawTracks(QPainter* painter, bool on_screen) const
{
	painter->save();
	if (!is_georeferenced)
		applyTemplateTransform(painter);
	
	// Tracks
	QPen pen(qRgb(212, 0, 244));
	if (on_screen)
		pen.setCosmetic(true);
	else
		pen.setWidthF(0.1); // = 0.1 mm at 100%
	painter->setPen(pen);
	painter->setBrush(Qt::NoBrush);
	
	// TODO: could speed that up by storing the template coords of the GPS points in a separate vector or caching the painter paths
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
					path.cubicTo(point.map_coord,
					             track.getSegmentPoint(i, k + 1).map_coord,
					             track.getSegmentPoint(i, k + 2).map_coord);
					k += 2;
				}
				else
					path.lineTo(point.map_coord.x(), point.map_coord.y());
			}
			else
				path.moveTo(point.map_coord.x(), point.map_coord.y());
		}
		painter->drawPath(path);
	}
	
	painter->restore();
}

void TemplateTrack::drawWaypoints(QPainter* painter) const
{
	painter->save();
	painter->setRenderHint(QPainter::Antialiasing);
	
	painter->setPen(Qt::NoPen);
	painter->setBrush(QBrush(qRgb(255, 0, 0)));
	
	QFont font = painter->font();
	font.setPixelSize(2); // 2 mm at 100%
	painter->setFont(font);
	int height = painter->fontMetrics().height();
	
	int size = track.getNumWaypoints();
	for (int i = 0; i < size; ++i)
	{
		const TrackPoint& point = track.getWaypoint(i);
		const QString& point_name = track.getWaypointName(i);
		
		double const radius = 0.25;
		painter->drawEllipse(point.map_coord, radius, radius);
		if (!point_name.isEmpty())
		{
			painter->setPen(qRgb(255, 0, 0));
			int width = painter->fontMetrics().width(point_name);
			painter->drawText(QRect(point.map_coord.x() - 0.5*width,
			                        point.map_coord.y() - height,
			                        width,
			                        height),
			                  Qt::AlignCenter,
			                  point_name);
			painter->setPen(Qt::NoPen);
		}
	}
	
	painter->restore();
}

QRectF TemplateTrack::getTemplateExtent() const
{
	// Infinite because the extent of the waypoint texts is unknown
	return infiniteRectF();
}

QRectF TemplateTrack::calculateTemplateBoundingBox() const
{
	QRectF bbox;
	
	int size = track.getNumWaypoints();
	for (int i = 0; i < size; ++i)
	{
		const TrackPoint& track_point = track.getWaypoint(i);
		MapCoordF point = track_point.map_coord;
		rectIncludeSafe(bbox, is_georeferenced ? point : templateToMap(point));
	}
	for (int i = 0; i < track.getNumSegments(); ++i)
	{
		size = track.getSegmentPointCount(i);
		for (int k = 0; k < size; ++k)
		{
			const TrackPoint& track_point = track.getSegmentPoint(i, k);
			MapCoordF point = track_point.map_coord;
			rectIncludeSafe(bbox, is_georeferenced ? point : templateToMap(point));
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


bool TemplateTrack::hasAlpha() const
{
	return false;
}


Template* TemplateTrack::duplicateImpl() const
{
	TemplateTrack* copy = new TemplateTrack(template_path, map);
	copy->track = track;
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

PointObject* TemplateTrack::importWaypoint(const MapCoordF& position, const QString& name)
{
	PointObject* point = new PointObject(map->getUndefinedPoint());
	point->setPosition(position);
	point->setTag(QStringLiteral("name"), name);
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
	
	const Track::ElementTags& tags = track.tags();
	DeleteObjectsUndoStep* undo_step = new DeleteObjectsUndoStep(map);
	MapPart* part = map->getCurrentPart();
	std::vector< Object* > result;
	// clazy:excludeall=reserve-candidates
	
	map->clearObjectSelection(false);
	
	if (track.getNumWaypoints() > 0)
	{
		int res = QMessageBox::question(dialog_parent, tr("Question"), tr("Should the waypoints be imported as a line going through all points?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
		if (res == QMessageBox::No)
		{
			for (int i = 0; i < track.getNumWaypoints(); i++)
				result.push_back(importWaypoint(templateToMap(track.getWaypoint(i).map_coord), track.getWaypointName(i)));
		}
		else
		{
			PathObject* path = importPathStart();
			for (int i = 0; i < track.getNumWaypoints(); i++)
				path->addCoordinate(MapCoord(templateToMap(track.getWaypoint(i).map_coord)));
			importPathEnd(path);
			path->setTag(QStringLiteral("name"), QString{});
			result.push_back(path);
		}
	}
	
	int skipped_paths = 0;
	for (int i = 0; i < track.getNumSegments(); i++)
	{
		const int segment_size = track.getSegmentPointCount(i);
		if (segment_size == 0)
		{
			++skipped_paths;
			continue; // Don't create path without objects.
		}
		
		PathObject* path = importPathStart();
		QString name = track.getSegmentName(i);
		if (!tags[name].isEmpty())
		{
			path->setTags(tags[name]);
		}
		else
		{
			path->setTag(QStringLiteral("name"), name);
		}
		
		for (int j = 0; j < segment_size; j++)
		{
			const TrackPoint& track_point = track.getSegmentPoint(i, j);
			auto coord = MapCoord { templateToMap(track_point.map_coord) };
			if (track_point.is_curve_start && j < segment_size - 3)
				coord.setCurveStart(true);
			path->addCoordinate(coord);
		}
		if (track.getSegmentPoint(i, 0).gps_coord == track.getSegmentPoint(i, segment_size-1).gps_coord)
		{
			path->closeAllParts();
		}
		importPathEnd(path);
		result.push_back(path);
	}
	
	for (const auto& object : result) // keep as separate loop to get the correct (final) indices
		undo_step->addObject(part->findObjectIndex(object));
	
	map->setObjectsDirty();
	map->push(undo_step);
	
	map->emitSelectionChanged();
	map->emitSelectionEdited();		// TODO: is this necessary here?
	
	if (skipped_paths)
	{
		QMessageBox::information(
		  dialog_parent,
		  tr("Import problems"),
		  tr("%n path object(s) could not be imported (reason: missing coordinates).", "", skipped_paths) );
	}
	
	return true;
}

void TemplateTrack::configureForGPSTrack()
{
	is_georeferenced = true;
	
	track_crs_spec = Georeferencing::geographic_crs_spec;
	Georeferencing* track_crs = new Georeferencing();
	track_crs->setProjectedCRS(QString{}, track_crs_spec);
	track_crs->setTransformationDirectly(QTransform());
	track.setTrackCRS(track_crs);
	
	projected_crs_spec.clear();
	track.changeMapGeoreferencing(map->getGeoreferencing());
	
	template_state = Template::Loaded;
}

void TemplateTrack::updateGeoreferencing()
{
	if (is_georeferenced && template_state == Template::Loaded)
	{
		projected_crs_spec.clear();
		track.changeMapGeoreferencing(map->getGeoreferencing());
		map->updateAllMapWidgets();
	}
}

QString TemplateTrack::calculateLocalGeoreferencing() const
{
	LatLon proj_center = track.calcAveragePosition();
	return QString::fromLatin1("+proj=ortho +datum=WGS84 +lat_0=%1 +lon_0=%2")
	        .arg(proj_center.latitude(), 0, 'f')
	        .arg(proj_center.longitude(), 0, 'f');
	
}

void TemplateTrack::applyProjectedCrsSpec()
{
	Georeferencing georef;
	georef.setScaleDenominator(int(map->getScaleDenominator()));
	georef.setProjectedCRS(QString{}, projected_crs_spec);
	georef.setProjectedRefPoint({});
	track.changeMapGeoreferencing(georef);
}


}  // namespace OpenOrienteering
