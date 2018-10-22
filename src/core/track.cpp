/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2014-2018 Kai Pastor
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


#include "track.h"

#include <QApplication>
#include <QFile>
#include <QFileInfo>  // IWYU pragma: keep
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
// IWYU pragma: no_include <qxmlstream.h>

#include "core/georeferencing.h"
#include "core/storage_location.h"  // IWYU pragma: keep


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
