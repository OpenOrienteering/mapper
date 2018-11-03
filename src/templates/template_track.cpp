/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2013-2018 Kai Pastor
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
#include "gui/task_dialog.h"
#include "undo/object_undo.h"
#include "util/util.h"


namespace OpenOrienteering {

const std::vector<QByteArray>& TemplateTrack::supportedExtensions()
{
	static std::vector<QByteArray> extensions = { "gpx" };
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
	if (projected_georef)
	{
		Q_ASSERT(!is_georeferenced);
		xml.writeStartElement(QString::fromLatin1("projected_crs_spec"));
		xml.writeCharacters(projected_georef->getProjectedCRSSpec());
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
		setCustomProjection(xml.readElementText());
		
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
	
	if (!track_crs_spec.isEmpty() && track_crs_spec != Georeferencing::geographic_crs_spec)
	{
		setErrorString(tr("This template must be loaded with GDAL/OGR."));
		return false;
	}
	
	if (!track.loadFrom(template_path))
		return false;
	
	if (!configuring)
	{
		if (!is_georeferenced)
		{
			if (!projected_georef)
				setCustomProjection(calculateLocalGeoreferencing());
			projectTrack();
		}
		else
		{
			projected_georef.reset();
			projectTrack();
		}
	}
	
	return true;
}

bool TemplateTrack::postLoadConfiguration(QWidget* dialog_parent, bool& out_center_in_view)
{
	is_georeferenced = true;
	
	// If the CRS is geographic, ask if track should be loaded using map georeferencing or ad-hoc georeferencing
	/** \todo Remove historical scope */
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
	if (!is_georeferenced)
	{
		if (!projected_georef)
			setCustomProjection(calculateLocalGeoreferencing());
		projectTrack();
	}
	else
	{
		projected_georef.reset();
		projectTrack();
	}
	
	return true;
}

void TemplateTrack::unloadTemplateFileImpl()
{
	track.clear();
	waypoints.clear();
	track_segments.clear();
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
	
	for (const auto& path : track_segments)
		painter->drawPath(path);
	
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
	
	auto waypoint = begin(waypoints);
	int size = std::min(track.getNumWaypoints(), int(waypoints.size()));
	for (int i = 0; i < size; ++i, ++waypoint)
	{
		const QString& point_name = track.getWaypoint(i).name;
		
		double const radius = 0.25;
		painter->drawEllipse(*waypoint, radius, radius);
		
		if (!point_name.isEmpty())
		{
			painter->setPen(qRgb(255, 0, 0));
			int width = painter->fontMetrics().width(point_name);
			painter->drawText(QRect(waypoint->x() - 0.5*width,
			                        waypoint->y() - height,
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
	const auto& georef = georeferencing();
	
	QRectF bbox;
	
	int size = track.getNumWaypoints();
	for (int i = 0; i < size; ++i)
	{
		const TrackPoint& track_point = track.getWaypoint(i);
		const auto point = georef.toMapCoordF(track_point.latlon);
		rectIncludeSafe(bbox, is_georeferenced ? point : templateToMap(point));
	}
	for (int i = 0; i < track.getNumSegments(); ++i)
	{
		size = track.getSegmentPointCount(i);
		for (int k = 0; k < size; ++k)
		{
			const TrackPoint& track_point = track.getSegmentPoint(i, k);
			const auto point = georef.toMapCoordF(track_point.latlon);
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
	if (projected_georef)
		copy->projected_georef.reset(new Georeferencing(*projected_georef));
	copy->waypoints = waypoints;
	copy->track_segments = track_segments;
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
	
	DeleteObjectsUndoStep* undo_step = new DeleteObjectsUndoStep(map);
	MapPart* part = map->getCurrentPart();
	std::vector< Object* > result;
	// clazy:excludeall=reserve-candidates
	
	map->clearObjectSelection(false);
	
	const auto& georef = georeferencing();
	
	if (track.getNumWaypoints() > 0)
	{
		int res = QMessageBox::question(dialog_parent, tr("Question"), tr("Should the waypoints be imported as a line going through all points?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
		if (res == QMessageBox::No)
		{
			for (int i = 0; i < track.getNumWaypoints(); i++)
			{
				const auto projected = georef.toMapCoordF(track.getWaypoint(i).latlon);
				result.push_back(importWaypoint(templateToMap(projected), track.getWaypoint(i).name));
			}
		}
		else
		{
			PathObject* path = importPathStart();
			for (int i = 0; i < track.getNumWaypoints(); i++)
			{
				const auto projected = georef.toMapCoordF(track.getWaypoint(i).latlon);
				path->addCoordinate(MapCoord(templateToMap(projected)));
			}
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
		for (int j = 0; j < segment_size; j++)
		{
			const TrackPoint& track_point = track.getSegmentPoint(i, j);
			const auto projected = georef.toMapCoordF(track_point.latlon);
			auto coord = MapCoord { templateToMap(projected) };
			path->addCoordinate(coord);
		}
		if (track.getSegmentPoint(i, 0).latlon == track.getSegmentPoint(i, segment_size-1).latlon)
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
	
	projected_georef.reset();
	projectTrack();
	
	template_state = Template::Loaded;
}

void TemplateTrack::updateGeoreferencing()
{
	if (is_georeferenced && template_state == Template::Loaded)
	{
		projected_georef.reset();
		projectTrack();
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

void TemplateTrack::setCustomProjection(const QString& projected_crs_spec)
{
	projected_georef.reset(new Georeferencing);
	projected_georef->setScaleDenominator(int(map->getScaleDenominator()));
	projected_georef->setProjectedCRS(QString{}, projected_crs_spec);
	projected_georef->setProjectedRefPoint({});
}

const Georeferencing& TemplateTrack::georeferencing() const
{
	return projected_georef ? *projected_georef : map->getGeoreferencing();
}

void TemplateTrack::projectTrack()
{
	const auto& georef = georeferencing();
	
	waypoints.clear();
	waypoints.reserve(decltype(waypoints)::size_type(track.getNumWaypoints()));
	for (int i = 0; i < track.getNumWaypoints(); ++i)
		waypoints.push_back(georef.toMapCoordF(track.getWaypoint(i).latlon));
	
	track_segments.resize(track.getNumSegments());
	for (int i = 0; i < track.getNumSegments(); ++i)
	{
		auto& painter_path = track_segments[i];
		painter_path = {};
		auto segment_length = track.getSegmentPointCount(i);
		if (segment_length > 0)
		{
		    painter_path.moveTo(georef.toMapCoordF(track.getSegmentPoint(i, 0).latlon));
			for (int j = 1; j < segment_length; ++j)
				painter_path.lineTo(georef.toMapCoordF(track.getSegmentPoint(i, j).latlon));
		}
	}
}


}  // namespace OpenOrienteering
