/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2014-2017 Kai Pastor
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


#include "gps_track.h"

#include <QApplication>
#include <QFile>
#include <QFileInfo>  // IWYU pragma: keep
#include <QHash>
#include <QMessageBox>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
// IWYU pragma: no_include <qxmlstream.h>

#include "core/georeferencing.h"
#include "core/storage_location.h"  // IWYU pragma: keep
#include "templates/template_track.h"
#include "util/dxfparser.h"


namespace OpenOrienteering {

// There is some (mis?)use of TrackPoint's gps_coord LatLon
// as sort-of MapCoordF.
// This function serves both for explicit conversion and highlighting.
MapCoordF fakeMapCoordF(const LatLon &latlon)
{
	return MapCoordF(latlon.longitude(), latlon.latitude());
}

TrackPoint::TrackPoint(LatLon coord, const QDateTime& datetime, float elevation, int num_satellites, float hDOP)
{
	gps_coord = coord;
	is_curve_start = false;
	this->datetime = datetime;
	this->elevation = elevation;
	this->num_satellites = num_satellites;
	this->hDOP = hDOP;
}
void TrackPoint::save(QXmlStreamWriter* stream) const
{
	stream->writeAttribute(QStringLiteral("lat"), QString::number(gps_coord.latitude(), 'f', 12));
	stream->writeAttribute(QStringLiteral("lon"), QString::number(gps_coord.longitude(), 'f', 12));
	
	if (datetime.isValid())
		stream->writeTextElement(QStringLiteral("time"), datetime.toString(Qt::ISODate));
	if (elevation > -9999)
		stream->writeTextElement(QStringLiteral("ele"), QString::number(elevation, 'f', 3));
	if (num_satellites >= 0)
		stream->writeTextElement(QStringLiteral("sat"), QString::number(num_satellites));
	if (hDOP >= 0)
		stream->writeTextElement(QStringLiteral("hdop"), QString::number(hDOP, 'f', 3));
}



// ### Track ###

Track::Track() : track_crs(nullptr)
{
	current_segment_finished = true;
}

Track::Track(const Georeferencing& map_georef) : track_crs(nullptr), map_georef(map_georef)
{
	current_segment_finished = true;
}

Track::Track(const Track& other)
{
	waypoints = other.waypoints;
	waypoint_names = other.waypoint_names;
	
	segment_points = other.segment_points;
	segment_starts = other.segment_starts;
	segment_names  = other.segment_names;
	
	current_segment_finished = other.current_segment_finished;
	
	element_tags   = other.element_tags;
	
	map_georef = other.map_georef;
	
	if (other.track_crs)
	{
		track_crs = new Georeferencing(*other.track_crs);
	}
}

Track::~Track()
{
	delete track_crs;
}

Track& Track::operator=(const Track& rhs)
{
	if (this == &rhs)
		return *this;
	
	clear();
	
	waypoints = rhs.waypoints;
	waypoint_names = rhs.waypoint_names;
	
	segment_points = rhs.segment_points;
	segment_starts = rhs.segment_starts;
	segment_names  = rhs.segment_names;
	
	current_segment_finished = rhs.current_segment_finished;
	
	element_tags   = rhs.element_tags;
	
	map_georef = rhs.map_georef;
	
	if (rhs.track_crs)
	{
		track_crs = new Georeferencing(*rhs.track_crs);
	}
	
	return *this;
}

void Track::clear()
{
	waypoints.clear();
	waypoint_names.clear();
	segment_points.clear();
	segment_starts.clear();
	segment_names.clear();
	current_segment_finished = true;
	element_tags.clear();
	delete track_crs;
	track_crs = nullptr;
}

bool Track::loadFrom(const QString& path, bool project_points, QWidget* dialog_parent)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;
	
	clear();

	if (path.endsWith(QLatin1String(".gpx"), Qt::CaseInsensitive))
	{
		if (!loadFromGPX(&file, project_points, dialog_parent))
			return false;
	}
	else if (path.endsWith(QLatin1String(".dxf"), Qt::CaseInsensitive))
	{
		if (!loadFromDXF(&file, project_points, dialog_parent))
			return false;
	}
	else if (path.endsWith(QLatin1String(".osm"), Qt::CaseInsensitive))
	{
		if (!loadFromOSM(&file, project_points, dialog_parent))
			return false;
	}
	else
		return false;

	file.close();
	return true;
}
bool Track::saveTo(const QString& path) const
{
	QFile file(path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		return false;
	
	QXmlStreamWriter stream(&file);
	stream.writeStartDocument();
	stream.writeStartElement(QString::fromLatin1("gpx"));
	stream.writeAttribute(QString::fromLatin1("version"), QString::fromLatin1("1.1"));
	stream.writeAttribute(QString::fromLatin1("creator"), qApp->applicationDisplayName());
	
	int size = getNumWaypoints();
	for (int i = 0; i < size; ++i)
	{
		stream.writeStartElement(QStringLiteral("wpt"));
		const TrackPoint& point = getWaypoint(i);
		point.save(&stream);
		stream.writeTextElement(QStringLiteral("name"), waypoint_names[i]);
		stream.writeEndElement();
	}
	
	stream.writeStartElement(QStringLiteral("trk"));
	for (int i = 0; i < getNumSegments(); ++i)
	{
		stream.writeStartElement(QStringLiteral("trkseg"));
		size = getSegmentPointCount(i);
		for (int k = 0; k < size; ++k)
		{
			stream.writeStartElement(QStringLiteral("trkpt"));
			const TrackPoint& point = getSegmentPoint(i, k);
			point.save(&stream);
			stream.writeEndElement();
		}
		stream.writeEndElement();
	}
	stream.writeEndElement();
	
	stream.writeEndElement();
	stream.writeEndDocument();
	
	file.close();
#ifdef Q_OS_ANDROID
	// Make the MediaScanner aware of the *updated* file.
	Android::mediaScannerScanFile(QFileInfo(path).absolutePath());
#endif
	return true;
}

void Track::appendTrackPoint(TrackPoint& point)
{
	point.map_coord = map_georef.toMapCoordF(point.gps_coord, nullptr); // TODO: check for errors
	segment_points.push_back(point);
	
	if (current_segment_finished)
	{
		segment_starts.push_back(segment_points.size() - 1);
		current_segment_finished = false;
	}
}
void Track::finishCurrentSegment()
{
	current_segment_finished = true;
}

void Track::appendWaypoint(TrackPoint& point, const QString& name)
{
	point.map_coord = map_georef.toMapCoordF(point.gps_coord, nullptr); // TODO: check for errors
	waypoints.push_back(point);
	waypoint_names.push_back(name);
}

void Track::changeMapGeoreferencing(const Georeferencing& new_map_georef)
{
	map_georef = new_map_georef;
	
	projectPoints();
}

void Track::setTrackCRS(Georeferencing* track_crs)
{
	delete this->track_crs;
	this->track_crs = track_crs;
	
	projectPoints();
}

int Track::getNumSegments() const
{
	return (int)segment_starts.size();
}

int Track::getSegmentPointCount(int segment_number) const
{
	Q_ASSERT(segment_number >= 0 && segment_number < (int)segment_starts.size());
	if (segment_number == (int)segment_starts.size() - 1)
		return segment_points.size() - segment_starts[segment_number];
	else
		return segment_starts[segment_number + 1] - segment_starts[segment_number];
}

const TrackPoint& Track::getSegmentPoint(int segment_number, int point_number) const
{
	Q_ASSERT(segment_number >= 0 && segment_number < (int)segment_starts.size());
	return segment_points[segment_starts[segment_number] + point_number];
}

const QString& Track::getSegmentName(int segment_number) const
{
	// NOTE: Segment names not [yet] supported by most track importers.
	if (segment_names.size() == 0)
	{
		static const QString empty_string;
		return empty_string;
	}
	
	return segment_names[segment_number];
}

int Track::getNumWaypoints() const
{
	return waypoints.size();
}

const TrackPoint& Track::getWaypoint(int number) const
{
	return waypoints[number];
}

const QString& Track::getWaypointName(int number) const
{
	return waypoint_names[number];
}

LatLon Track::calcAveragePosition() const
{
	double avg_latitude = 0;
	double avg_longitude = 0;
	int num_samples = 0;
	
	int size = getNumWaypoints();
	for (int i = 0; i < size; ++i)
	{
		const TrackPoint& point = getWaypoint(i);
		avg_latitude += point.gps_coord.latitude();
		avg_longitude += point.gps_coord.longitude();
		++num_samples;
	}
	for (int i = 0; i < getNumSegments(); ++i)
	{
		size = getSegmentPointCount(i);
		for (int k = 0; k < size; ++k)
		{
			const TrackPoint& point = getSegmentPoint(i, k);
			avg_latitude += point.gps_coord.latitude();
			avg_longitude += point.gps_coord.longitude();
			++num_samples;
		}
	}
	
	return LatLon((num_samples > 0) ? (avg_latitude / num_samples) : 0,
				  (num_samples > 0) ? (avg_longitude / num_samples) : 0);
}

bool Track::loadFromGPX(QFile* file, bool project_points, QWidget* dialog_parent)
{
	Q_UNUSED(dialog_parent);
	
	track_crs = new Georeferencing();
	track_crs->setProjectedCRS({}, Georeferencing::geographic_crs_spec);
	track_crs->setTransformationDirectly(QTransform());
	
	TrackPoint point;
	QString point_name;

	QXmlStreamReader stream(file);
	while (!stream.atEnd())
	{
		stream.readNext();
		if (stream.tokenType() == QXmlStreamReader::StartElement)
		{
			if (stream.name().compare(QLatin1String("wpt"), Qt::CaseInsensitive) == 0
			    || stream.name().compare(QLatin1String("trkpt"), Qt::CaseInsensitive) == 0
				|| stream.name().compare(QLatin1String("rtept"), Qt::CaseInsensitive) == 0)
			{
				point = TrackPoint(LatLon(stream.attributes().value(QLatin1String("lat")).toDouble(),
				                          stream.attributes().value(QLatin1String("lon")).toDouble()));
				if (project_points)
					point.map_coord = map_georef.toMapCoordF(point.gps_coord); // TODO: check for errors
				point_name.clear();
			}
			else if (stream.name().compare(QLatin1String("trkseg"), Qt::CaseInsensitive) == 0
			         || stream.name().compare(QLatin1String("rte"), Qt::CaseInsensitive) == 0)
			{
				if (segment_starts.size() == 0 ||
					segment_starts.back() < (int)segment_points.size())
				{
					segment_starts.push_back(segment_points.size());
				}
			}
			else if (stream.name().compare(QLatin1String("ele"), Qt::CaseInsensitive) == 0)
				point.elevation = stream.readElementText().toFloat();
			else if (stream.name().compare(QLatin1String("time"), Qt::CaseInsensitive) == 0)
				point.datetime = QDateTime::fromString(stream.readElementText(), Qt::ISODate);
			else if (stream.name().compare(QLatin1String("sat"), Qt::CaseInsensitive) == 0)
				point.num_satellites = stream.readElementText().toInt();
			else if (stream.name().compare(QLatin1String("hdop"), Qt::CaseInsensitive) == 0)
				point.hDOP = stream.readElementText().toFloat();
			else if (stream.name().compare(QLatin1String("name"), Qt::CaseInsensitive) == 0)
				point_name = stream.readElementText();
		}
		else if (stream.tokenType() == QXmlStreamReader::EndElement)
		{
			if (stream.name().compare(QLatin1String("wpt"), Qt::CaseInsensitive) == 0)
			{
				waypoints.push_back(point);
				waypoint_names.push_back(point_name);
			}
			else if (stream.name().compare(QLatin1String("trkpt"), Qt::CaseInsensitive) == 0
			         || stream.name().compare(QLatin1String("rtept"), Qt::CaseInsensitive) == 0)
			{
				segment_points.push_back(point);
			}
		}
	}
	
	if (segment_starts.size() > 0 &&
		segment_starts.back() == (int)segment_points.size())
	{
		segment_starts.pop_back();
	}
	
	return true;
}

bool Track::loadFromDXF(QFile* file, bool project_points, QWidget* dialog_parent)
{
	DXFParser* parser = new DXFParser();
	parser->setData(file);
	QString result = parser->parse();
	if (!result.isEmpty())
	{
		QMessageBox::critical(dialog_parent, OpenOrienteering::TemplateTrack::tr("Error reading"), OpenOrienteering::TemplateTrack::tr("There was an error reading the DXF file %1:\n\n%2").arg(file->fileName(), result));
		delete parser;
		return false;
	}
	QList<DXFPath> paths = parser->getData();
	delete parser;
	
	// TODO: Re-implement the possibility to load degree values somewhere else.
	//       It does not fit here as this method is called again every time a map
	//       containing a track is re-loaded, and in this case the question should
	//       not be asked again.
	//int res = QMessageBox::question(dialog_parent, OpenOrienteering::TemplateTrack::tr("Question"), OpenOrienteering::TemplateTrack::tr("Are the coordinates in the DXF file in degrees?"), QMessageBox::Yes|QMessageBox::No);
	for (auto&& path : paths)
	{
		if (path.type == POINT)
		{
			if(path.coords.size() < 1)
				continue;
			TrackPoint point = TrackPoint(LatLon(path.coords.at(0).y, path.coords.at(0).x));
			if (project_points)
				point.map_coord = map_georef.toMapCoordF(track_crs, fakeMapCoordF(point.gps_coord)); // TODO: check for errors
			waypoints.push_back(point);
			waypoint_names.push_back(path.layer);
		}
		if (path.type == LINE ||
			path.type == SPLINE	)
		{
			if (path.coords.size() < 1)
				continue;
			segment_starts.push_back(segment_points.size());
			segment_names.push_back(path.layer);
			int i = 0;
			for (auto&& coord : path.coords)
			{
				TrackPoint point = TrackPoint(LatLon(coord.y, coord.x), QDateTime());
				if (project_points)
					point.map_coord = map_georef.toMapCoordF(track_crs, fakeMapCoordF(point.gps_coord)); // TODO: check for errors
				if (path.type == SPLINE &&
					i % 3 == 0 &&
					i < path.coords.size() - 3)
					point.is_curve_start = true;
					
				segment_points.push_back(point);
				++i;
			}
			if (path.closed && !segment_points.empty())
			{
				const TrackPoint& start = segment_points[segment_starts.back()];
				if (start.gps_coord != segment_points.back().gps_coord)
				{
					segment_points.push_back(start);
					segment_points.back().is_curve_start = false;
				}
			}
		}
	}
	
	return true;
}

bool Track::loadFromOSM(QFile* file, bool project_points, QWidget* dialog_parent)
{
	track_crs = new Georeferencing();
	track_crs->setProjectedCRS({}, Georeferencing::geographic_crs_spec);
	track_crs->setTransformationDirectly(QTransform());
	
	// Basic OSM file support
	// Reference: https://wiki.openstreetmap.org/wiki/OSM_XML
	const double min_supported_version = 0.5;
	const double max_supported_version = 0.6;
	QHash<QString, TrackPoint> nodes;
	int node_problems = 0;
	
	QXmlStreamReader xml(file);
	if (xml.readNextStartElement())
	{
		if (xml.name() != QLatin1String("osm"))
		{
			QMessageBox::critical(dialog_parent, OpenOrienteering::TemplateTrack::tr("Error"), OpenOrienteering::TemplateTrack::tr("%1:\nNot an OSM file."));
			return false;
		}
		else
		{
			QXmlStreamAttributes attributes(xml.attributes());
			const double osm_version = attributes.value(QLatin1String("version")).toDouble();
			if (osm_version < min_supported_version)
			{
				QMessageBox::critical(dialog_parent, OpenOrienteering::TemplateTrack::tr("Error"),
				                      OpenOrienteering::TemplateTrack::tr("The OSM file has version %1.\nThe minimum supported version is %2.").arg(
				                          attributes.value(QLatin1String("version")).toString(), QString::number(min_supported_version, 'g', 1)));
				return false;
			}
			if (osm_version > max_supported_version)
			{
				QMessageBox::critical(dialog_parent, OpenOrienteering::TemplateTrack::tr("Error"),
				                      OpenOrienteering::TemplateTrack::tr("The OSM file has version %1.\nThe maximum supported version is %2.").arg(
				                          attributes.value(QLatin1String("version")).toString(), QString::number(max_supported_version, 'g', 1)));
				return false;
			}
		}
	}
	
	qint64 internal_node_id = 0;
	while (xml.readNextStartElement())
	{
		const QStringRef name(xml.name());
		QXmlStreamAttributes attributes(xml.attributes());
		if (attributes.value(QLatin1String("visible")) == QLatin1String("false"))
		{
			xml.skipCurrentElement();
			continue;
		}
		
		QString id(attributes.value(QLatin1String("id")).toString());
		if (id.isEmpty())
		{
			id = QLatin1Char('!') + QString::number(++internal_node_id);
		}
		
		if (name == QLatin1String("node"))
		{
			bool ok = true;
			double lat = 0.0, lon = 0.0;
			if (ok) lat = attributes.value(QLatin1String("lat")).toDouble(&ok);
			if (ok) lon = attributes.value(QLatin1String("lon")).toDouble(&ok);
			if (!ok)
			{
				node_problems++;
				xml.skipCurrentElement();
				continue;
			}
			
			TrackPoint point(LatLon(lat, lon));
			if (project_points)
			{
				point.map_coord = map_georef.toMapCoordF(point.gps_coord); // TODO: check for errors
			}
			nodes.insert(id, point);
			
			while (xml.readNextStartElement())
			{
				if (xml.name() == QLatin1String("tag"))
				{
					const QString k(xml.attributes().value(QLatin1String("k")).toString());
					const QString v(xml.attributes().value(QLatin1String("v")).toString());
					element_tags[id][k] = v;
					
					if (k == QLatin1String("ele"))
					{
						bool ok;
						double elevation = v.toDouble(&ok);
						if (ok) nodes[id].elevation = elevation;
					}
					else if (k == QLatin1String("name"))
					{
						if (!v.isEmpty() && !nodes.contains(v)) 
						{
							waypoints.push_back(point);
							waypoint_names.push_back(v);
						}
					}
				}
				xml.skipCurrentElement();
			}
		}
		else if (name == QLatin1String("way"))
		{
			segment_starts.push_back(segment_points.size());
			segment_names.push_back(id);
			while (xml.readNextStartElement())
			{
				if (xml.name() == QLatin1String("nd"))
				{
					QString ref = xml.attributes().value(QLatin1String("ref")).toString();
					if (ref.isEmpty() || !nodes.contains(ref))
						node_problems++;
					else
						segment_points.push_back(nodes[ref]);
				}
				else if (xml.name() == QLatin1String("tag"))
				{
					const QString k(xml.attributes().value(QLatin1String("k")).toString());
					const QString v(xml.attributes().value(QLatin1String("v")).toString());
					element_tags[id][k] = v;
				}
				xml.skipCurrentElement();
			}
		}
		else
		{
			xml.skipCurrentElement();
		}
	}
	
	if (node_problems > 0)
		QMessageBox::warning(dialog_parent, OpenOrienteering::TemplateTrack::tr("Problems"), OpenOrienteering::TemplateTrack::tr("%1 nodes could not be processed correctly.").arg(node_problems));
	
	return true;
}

void Track::projectPoints()
{
	if (track_crs->getProjectedCRSSpec() == Georeferencing::geographic_crs_spec)
	{
		int size = waypoints.size();
		for (int i = 0; i < size; ++i)
			waypoints[i].map_coord = map_georef.toMapCoordF(waypoints[i].gps_coord, nullptr); // FIXME: check for errors
			
		size = segment_points.size();
		for (int i = 0; i < size; ++i)
			segment_points[i].map_coord = map_georef.toMapCoordF(segment_points[i].gps_coord, nullptr); // FIXME: check for errors
	}
	else
	{
		int size = waypoints.size();
		for (int i = 0; i < size; ++i)
			waypoints[i].map_coord = map_georef.toMapCoordF(track_crs, fakeMapCoordF(waypoints[i].gps_coord), nullptr); // FIXME: check for errors
			
		size = segment_points.size();
		for (int i = 0; i < size; ++i)
			segment_points[i].map_coord = map_georef.toMapCoordF(track_crs, fakeMapCoordF(segment_points[i].gps_coord), nullptr); // FIXME: check for errors
	}
}


}  // namespace OpenOrienteering
