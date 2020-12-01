/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2013-2020 Kai Pastor
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

#include <utility>

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
#include <QPainterPath>
#include <QPen>
#include <QRect>
#include <QRgb>
#include <QSize>
#include <QStringRef>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "core/georeferencing.h"
#include "core/latlon.h"
#include "core/map.h"
#include "core/map_color.h"
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

class Symbol;

namespace {

const MapColor& makeTrackColor(Map& map)
{
	auto* track_color = new MapColor(QLatin1String{"Purple"}, 0); 
	track_color->setSpotColorName(QLatin1String{"PURPLE"});
	track_color->setCmyk({0.35f, 0.85f, 0.0, 0.0});
	track_color->setRgbFromCmyk();
	map.addColor(track_color, 0);
	return *track_color;
}

const LineSymbol& makeTrackSymbol(Map& map, const MapColor& color)
{
	auto* track_symbol = new LineSymbol();
	track_symbol->setName(TemplateTrack::tr("Track"));
	track_symbol->setNumberComponent(0, 1);
	track_symbol->setColor(&color);
	track_symbol->setLineWidth(0.1); // mm, almost cosmetic
	track_symbol->setCapStyle(LineSymbol::FlatCap);
	track_symbol->setJoinStyle(LineSymbol::MiterJoin);
	map.addSymbol(track_symbol, map.getNumSymbols());
	return *track_symbol;
}

const LineSymbol& makeRouteSymbol(Map& map, const MapColor& color)
{
	auto* route_symbol = new LineSymbol();
	route_symbol->setName(TemplateTrack::tr("Route"));
	route_symbol->setNumberComponent(0, 2);
	route_symbol->setColor(&color);
	route_symbol->setLineWidth(0.5); // mm
	route_symbol->setCapStyle(LineSymbol::FlatCap);
	route_symbol->setJoinStyle(LineSymbol::MiterJoin);
	map.addSymbol(route_symbol, map.getNumSymbols());
	return *route_symbol;
}

const PointSymbol& makeWaypointSymbol(Map& map, const MapColor& color)
{
	auto* waypoint_symbol = new PointSymbol();
	waypoint_symbol->setName(TemplateTrack::tr("Waypoint"));
	waypoint_symbol->setNumberComponent(0, 3);
	waypoint_symbol->setInnerColor(&color);
	waypoint_symbol->setInnerRadius(500); // (um)
	map.addSymbol(waypoint_symbol, map.getNumSymbols());
	return *waypoint_symbol;
}

PathObject* importPath(Map& map, const Symbol& symbol, MapCoordVector coords)
{
	if (coords.empty())
		return nullptr;
	
	if (coords.size() == 1)
		coords.push_back(coords.front());
	
	auto* path = new PathObject(&symbol, std::move(coords));
	map.addObject(path);
	map.addObjectToSelection(path, false);
	return path;
}

PointObject* importPoint(Map& map, const Symbol& symbol, const MapCoordF& position, const QString& name)
{
	auto* point = new PointObject(&symbol);
	point->setPosition(position);
	if (!name.isEmpty())
		point->setTag(QStringLiteral("name"), name);
	map.addObject(point);
	map.addObjectToSelection(point, false);
	return point;
}

}  // namespace


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

TemplateTrack::TemplateTrack(const TemplateTrack& proto)
: Template(proto)
, track(proto.track)
, track_crs_spec(proto.track_crs_spec)
, projected_crs_spec(proto.projected_crs_spec)
{
	if (proto.preserved_georef)
		preserved_georef = std::make_unique<Georeferencing>(*proto.preserved_georef);
	
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


TemplateTrack* TemplateTrack::duplicate() const
{
	return new TemplateTrack(*this);
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
		preserved_georef = std::make_unique<Georeferencing>();
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
	if (!track.saveTo(template_path))
		return false;
	
	const_cast<TemplateTrack*>(this)->setHasUnsavedChanges(false);
	return true;
}

bool TemplateTrack::loadTemplateFileImpl()
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
	
	if (!track.loadFrom(template_path, false))
		return false;
	
	if (getTemplateState() != Configuring)
	{
		if (!is_georeferenced)
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

bool TemplateTrack::postLoadSetup(QWidget* dialog_parent, bool& /*out_center_in_view*/)
{
	is_georeferenced = true;
	
	// If the CRS is geographic, ask if track should be loaded using map georeferencing or ad-hoc georeferencing
	/** \todo Remove historical scope */
	{
		TaskDialog georef_dialog(dialog_parent, tr("Opening track ..."),
			tr("Load the track in georeferenced or non-georeferenced mode?"),
			QDialogButtonBox::Abort);
		QString georef_text = tr("Positions the track according to the map's georeferencing settings.");
		if (map->getGeoreferencing().getState() != Georeferencing::Geospatial)
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
	if (is_georeferenced && map->getGeoreferencing().getState() != Georeferencing::Geospatial)
	{
		// Set default for real world reference point as some average of the track coordinates
		Georeferencing georef(map->getGeoreferencing());
		georef.setGeographicRefPoint(track.calcAveragePosition());
		
		// Show the parameter dialog
		GeoreferencingDialog dialog(dialog_parent, map, &georef);
		dialog.setKeepGeographicRefCoords();
		if (dialog.exec() == QDialog::Rejected || map->getGeoreferencing().getState() != Georeferencing::Geospatial)
			return false;
	}
	
	// If the track is loaded as not georeferenced,
	// the map coords for the track coordinates have to be calculated
	if (!is_georeferenced)
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

void TemplateTrack::drawTemplate(QPainter* painter, const QRectF& /*clip_rect*/, double /*scale*/, bool on_screen, qreal opacity) const
{
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
				path.lineTo(point.map_coord.x(), point.map_coord.y());
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
	auto const font_metrics = painter->fontMetrics();
	
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
			auto const size = font_metrics.boundingRect(point_name).size();
			painter->drawText(QRect(qRound(point.map_coord.x() - 0.5*size.width()),
			                        qRound(point.map_coord.y() - size.height()),
			                        size.width(),
			                        size.height()),
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

int TemplateTrack::getTemplateBoundingBoxPixelBorder() const
{
	// As we don't estimate the extent of the widest waypoint text,
	// return a "very big" number to cover everything
	return 10e8;
}


bool TemplateTrack::hasAlpha() const
{
	return false;
}


bool TemplateTrack::import(QWidget* dialog_parent)
{
	if (!map)
	{
		return false;
	}
	
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
	
	auto& track_color = makeTrackColor(*map);
	auto& track_symbol = makeTrackSymbol(*map, track_color);
	
	auto const num_waypoints = track.getNumWaypoints();
	if (num_waypoints > 0)
	{
		auto res = QMessageBox::No;
		if (num_waypoints > 1)
		{
			res = QMessageBox::question(
			          dialog_parent,
			          tr("Question"),
			          tr("Should the waypoints be imported as a line going through all points?"),
			          QMessageBox::Yes | QMessageBox::No,
			          QMessageBox::No);
		}
		if (res == QMessageBox::No)
		{
			auto& waypoint_symbol = makeWaypointSymbol(*map, track_color);
			for (int i = 0; i < num_waypoints; i++)
			{
				auto pos = templateToMap(track.getWaypoint(i).map_coord);
				auto& name = track.getWaypointName(i);
				if (auto* waypoint = importPoint(*map, waypoint_symbol, pos, name))
					result.push_back(waypoint);
			}
		}
		else
		{
			MapCoordVector coords;
			coords.reserve(MapCoordVector::size_type(track.getNumWaypoints()));
			for (int i = 0; i < track.getNumWaypoints(); i++)
				coords.push_back(MapCoord(templateToMap(track.getWaypoint(i).map_coord)));
			
			auto& route_symbol = makeRouteSymbol(*map, track_color);
			if (auto* path = importPath(*map, route_symbol, std::move(coords)))
			    result.push_back(path);
		}
	}
	
	int skipped_paths = 0;
	for (int i = 0; i < track.getNumSegments(); i++)
	{
		auto const segment_size = track.getSegmentPointCount(i);
		if (segment_size == 0)
		{
			++skipped_paths;
			continue; // Don't create path without objects.
		}
		
		MapCoordVector coords;
		coords.reserve(MapCoordVector::size_type(segment_size));
		for (int j = 0; j < segment_size; j++)
			coords.push_back(MapCoord(templateToMap(track.getSegmentPoint(i, j).map_coord)));
		
		if (auto* path = importPath(*map, track_symbol, std::move(coords)))
		{
			if (track.getSegmentPoint(i, 0).latlon == track.getSegmentPoint(i, segment_size-1).latlon)
				path->closeAllParts();
			result.push_back(path);
		}
	}
	
	for (const auto* object : result) // keep as separate loop to get the correct (final) indices
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
	georef.setCombinedScaleFactor(1.0);
	georef.setGrivation(0.0);
	track.changeMapGeoreferencing(georef);
}


}  // namespace OpenOrienteering
