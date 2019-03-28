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

#include <algorithm>
#include <iterator>

#include <Qt>
#include <QBrush>
#include <QByteArray>
#include <QCommandLinkButton>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFont>
#include <QFontMetrics>
#include <QLatin1Char>
#include <QLatin1String>
#include <QMessageBox>
#include <QPainter>
#include <QPen>
#include <QPoint>
#include <QRect>
#include <QRgb>
#include <QStringRef>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "core/georeferencing.h"
#include "core/latlon.h"
#include "core/map.h"
#include "core/map_coord.h"
#include "core/map_part.h"
#include "core/objects/object.h"
#include "core/symbols/line_symbol.h"
#include "core/symbols/point_symbol.h"
#include "gui/georeferencing_dialog.h"
#include "gui/task_dialog.h"
#include "undo/object_undo.h"
#include "util/util.h"

class QAbstractButton;


namespace OpenOrienteering {

namespace {

// Graphic element dimension, in mm at 100%
constexpr qreal waypoint_marker_radius = 0.25;
constexpr int   waypoint_font_size     = 2;
constexpr qreal track_line_width       = 0.1;

}  // namespace



const std::vector<QByteArray>& TemplateTrack::supportedExtensions()
{
	static std::vector<QByteArray> extensions = { "gpx" };
	return extensions;
}

TemplateTrack::TemplateTrack(const QString& path, Map* map)
: Template(path, map)
, track_crs_spec(Georeferencing::geographic_crs_spec)
{
	const Georeferencing& georef = map->getGeoreferencing();
	connect(&georef, &Georeferencing::projectionChanged, this, &TemplateTrack::updateGeoreferencing);
	connect(&georef, &Georeferencing::transformationChanged, this, &TemplateTrack::updateGeoreferencing);
	connect(&georef, &Georeferencing::stateChanged, this, &TemplateTrack::updateGeoreferencing);
	connect(&georef, &Georeferencing::declinationChanged, this, &TemplateTrack::updateGeoreferencing);
	
	connect(&track, &Track::trackChanged, this, &TemplateTrack::trackChanged);
}

TemplateTrack::~TemplateTrack()
{
	if (template_state == Loaded)
		unloadTemplateFile();
}


const char* TemplateTrack::getTemplateType() const
{
	return "TemplateTrack";
}

bool TemplateTrack::isRasterGraphics() const
{
	return false;
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
	xml.writeTextElement(QString::fromLatin1("crs_spec"), track_crs_spec);
	if (projected_georef)
	{
		Q_ASSERT(!is_georeferenced);
		xml.writeTextElement(QString::fromLatin1("projected_crs_spec"), projected_georef->getProjectedCRSSpec());
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
		projectTrack();
	
	return true;
}

bool TemplateTrack::postLoadConfiguration(QWidget* dialog_parent, bool& /*out_center_in_view*/)
{
	is_georeferenced = true;
	
	// If the CRS is geographic, ask if track should be loaded using map georeferencing or ad-hoc georeferencing
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
	
	projectTrack();
	return true;
}

void TemplateTrack::unloadTemplateFileImpl()
{
	track.clear();
	waypoints.clear();
	track_segments.clear();
}

void TemplateTrack::drawTemplate(QPainter* painter, const QRectF& /*clip_rect*/, double /*scale*/, bool on_screen, float opacity) const
{
	drawTracks(painter, on_screen, static_cast<qreal>(opacity));
	drawWaypoints(painter, static_cast<qreal>(opacity));
}

void TemplateTrack::drawTracks(QPainter* painter, bool on_screen, qreal opacity) const
{
	painter->save();
	
	if (!is_georeferenced)
		applyTemplateTransform(painter);
	
	painter->setOpacity(opacity);
	QPen pen(qRgb(212, 0, 244));
	if (on_screen)
		pen.setCosmetic(true);
	else
		pen.setWidthF(track_line_width);
	painter->setPen(pen);
	painter->setBrush(Qt::NoBrush);
	
	for (const auto& path : track_segments)
		painter->drawPath(path);
	
	painter->restore();
}

void TemplateTrack::drawWaypoints(QPainter* painter, qreal opacity) const
{
	painter->save();
	
	painter->setOpacity(opacity);
	painter->setRenderHint(QPainter::Antialiasing);
	painter->setPen(Qt::NoPen);
	painter->setBrush(QBrush(qRgb(255, 0, 0)));
	
	QFont font = painter->font();
	font.setPixelSize(waypoint_font_size);
	painter->setFont(font);
	line_height = static_cast<qreal>(painter->fontMetrics().height());
	half_char_width = 0.5 * painter->fontMetrics().maxWidth();
	
	auto waypoint = begin(waypoints);
	int size = std::min(track.getNumWaypoints(), int(waypoints.size()));
	for (int i = 0; i < size; ++i, ++waypoint)
	{
		const QString& point_name = track.getWaypoint(i).name;
		
		painter->drawEllipse(*waypoint, waypoint_marker_radius, waypoint_marker_radius);
		
		if (!point_name.isEmpty())
		{
			painter->setPen(qRgb(255, 0, 0));
			auto offset = point_name.length() * half_char_width;
			painter->drawText(QRect(static_cast<int>(waypoint->x() - offset),
			                        static_cast<int>(waypoint->y() - line_height),
			                        static_cast<int>(2 * offset),
			                        static_cast<int>(line_height)),
			                  Qt::AlignCenter,
			                  point_name);
			painter->setPen(Qt::NoPen);
		}
	}
	
	painter->restore();
}

QRectF TemplateTrack::calculateTemplateBoundingBox() const
{
	const auto& georef = georeferencing();
	
	QRectF bbox;
	
	int size = track.getNumWaypoints();
	for (int i = 0; i < size; ++i)
	{
		const auto& way_point = track.getWaypoint(i);
		const auto point = georef.toMapCoordF(way_point.latlon);
		const auto map_coord = is_georeferenced ? point : templateToMap(point);
		if (way_point.name.isEmpty())
		{
			rectIncludeSafe(bbox, map_coord);
		}
		else
		{
			auto offset = way_point.name.length() * half_char_width;
			rectIncludeSafe(bbox, map_coord - QPointF{offset, line_height});
			rectInclude(bbox, map_coord + QPointF{offset, 0});
		}
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
	
	// Adjust for line width and for waypoint marker width.
	Q_STATIC_ASSERT(waypoint_marker_radius >= track_line_width);
	bbox.adjust(-waypoint_marker_radius, -waypoint_marker_radius, +waypoint_marker_radius, +waypoint_marker_radius);
	return bbox;
}


bool TemplateTrack::hasAlpha() const
{
	return false;
}


Template* TemplateTrack::duplicateImpl() const
{
	auto* copy = new TemplateTrack(template_path, map);
	copy->track.copyFrom(track);
	copy->track_crs_spec = track_crs_spec;
	if (projected_georef)
		copy->projected_georef.reset(new Georeferencing(*projected_georef));
	if (preserved_georef)
		copy->preserved_georef.reset(new Georeferencing(*preserved_georef));
	copy->waypoints = waypoints;
	copy->track_segments = track_segments;
	return copy;
}

PathObject* TemplateTrack::importPathStart()
{
	auto* path = new PathObject();
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
	auto* point = new PointObject(map->getUndefinedPoint());
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
	
	auto* undo_step = new DeleteObjectsUndoStep(map);
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
				const auto& way_point = track.getWaypoint(i);
				const auto projected = georef.toMapCoordF(way_point.latlon);
				result.push_back(importWaypoint(templateToMap(projected), way_point.name));
			}
		}
		else
		{
			PathObject* path = importPathStart();
			for (int i = 0; i < track.getNumWaypoints(); i++)
			{
				const auto& way_point = track.getWaypoint(i);
				const auto projected = georef.toMapCoordF(way_point.latlon);
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
	
	projectTrack();
	
	template_state = Template::Loaded;
}

void TemplateTrack::trackChanged(Track::TrackChange change, const TrackPoint& point)
{
	if (template_state != Template::Loaded)
		return;
	
	const auto& georef = georeferencing();
	switch (change)
	{
	case Track::NewSegment:
		track_segments.push_back({});
		track_segments.back().moveTo(georef.toMapCoordF(point.latlon));
		// No need for redrawing after QPainterPath::moveTo().
		break;
	case Track::TrackPointAppended:
		{
			auto& segment = track_segments.back();
			const auto p0 = segment.elementAt(segment.elementCount() - 1);
			const auto p1 = georef.toMapCoordF(point.latlon);
			segment.lineTo(p1);
			const auto bounding_box =
			        ( is_georeferenced
			          ? QRectF { p0, p1 }
			          : QRectF { templateToMap(p0), templateToMap(p1) }
			        )
			        .normalized()
			        .adjusted(-track_line_width, -track_line_width, track_line_width, track_line_width);
			map->setTemplateAreaDirty(this, bounding_box, 0);
		}
		break;
	case Track::WaypointAppended:
		{
			const auto p0 = georef.toMapCoordF(point.latlon);
			waypoints.push_back(p0);
			const auto p1 = is_georeferenced ? p0 : templateToMap(p0);
			const auto bounding_box =
			        ( point.name.isEmpty()
			          ? QRectF { p1 - QPointF{waypoint_marker_radius, waypoint_marker_radius},
			                     p1 + QPointF{waypoint_marker_radius, waypoint_marker_radius} }
			          : QRectF { p1 - QPointF{point.name.length() * half_char_width, line_height},
			                     p1 + QPointF{point.name.length() * half_char_width, waypoint_marker_radius} }
			         );
			map->setTemplateAreaDirty(this, bounding_box, 0);
		}
		break;
	}
	
	setHasUnsavedChanges(true);
}

void TemplateTrack::updateGeoreferencing()
{
	if (is_georeferenced && template_state == Template::Loaded)
	{
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
	if (is_georeferenced)
		projected_georef.reset();
	else if (!projected_georef)
		setCustomProjection(calculateLocalGeoreferencing());
	
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
