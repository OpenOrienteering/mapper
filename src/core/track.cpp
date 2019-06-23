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

#include <memory>

#include <Qt>
#include <QtGlobal>
#include <QtNumeric>
#include <QApplication>
#include <QFile>
#include <QFileInfo>  // IWYU pragma: keep
#include <QIODevice>
#include <QLatin1String>
#include <QPoint>
#include <QPointF>
#include <QSaveFile>
#include <QStringRef>
#include <QXmlStreamAttributes>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
// IWYU pragma: no_include <qxmlstream.h>

#include "core/georeferencing.h"
#include "core/storage_location.h"  // IWYU pragma: keep


namespace OpenOrienteering {

// ### TrackPoint ###

void TrackPoint::save(QXmlStreamWriter* stream) const
{
	stream->writeAttribute(QStringLiteral("lat"), QString::number(latlon.latitude(), 'f', 12));
	stream->writeAttribute(QStringLiteral("lon"), QString::number(latlon.longitude(), 'f', 12));
	
	if (datetime.isValid())
		stream->writeTextElement(QStringLiteral("time"), datetime.toString(Qt::ISODate));
	if (!qIsNaN(elevation))
		stream->writeTextElement(QStringLiteral("ele"), QString::number(static_cast<qreal>(elevation), 'f', 3));
	if (!qIsNaN(hDOP))
		stream->writeTextElement(QStringLiteral("hdop"), QString::number(static_cast<qreal>(hDOP), 'f', 3));
}

bool operator==(const TrackPoint& lhs, const TrackPoint& rhs)
{
	auto fuzzyCompare =[](auto a, auto b) {
		return (qIsNaN(a) && qIsNaN(b))
		       || qFuzzyCompare(a, b);
	};
	return lhs.latlon == rhs.latlon
	       && lhs.map_coord == rhs.map_coord
	       && lhs.datetime == rhs.datetime
	       && fuzzyCompare(lhs.elevation, rhs.elevation)
	       && fuzzyCompare(lhs.hDOP, rhs.hDOP);
}



// ### Track ###

Track::Track(const Georeferencing& map_georef)
: map_georef(map_georef)
{
	// nothing else
}

Track::Track(const Track& other)
{
	*this = other;
}

Track::~Track() = default;

Track& Track::operator=(const Track& rhs)
{
	if (this == &rhs)
		return *this;
	
	clear();
	
	waypoints = rhs.waypoints;
	waypoint_names = rhs.waypoint_names;
	
	segment_points = rhs.segment_points;
	segment_starts = rhs.segment_starts;
	
	current_segment_finished = rhs.current_segment_finished;
	
	map_georef = rhs.map_georef;
	
	return *this;
}

void Track::clear()
{
	waypoints.clear();
	waypoint_names.clear();
	segment_points.clear();
	segment_starts.clear();
	current_segment_finished = true;
}

bool Track::loadFrom(const QString& path, bool project_points)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;
	
	clear();
	if (path.endsWith(QLatin1String(".gpx"), Qt::CaseInsensitive))
	{
		if (!loadGpxFrom(file, project_points))
			return false;
	}
	else
		return false;

	file.close();
	return true;
}

bool Track::saveTo(const QString& path) const
{
	QSaveFile file(path);
	if (file.open(QIODevice::WriteOnly | QIODevice::Text)
	    && saveGpxTo(file)
	    && file.commit())
	{
#ifdef Q_OS_ANDROID
		// Make the MediaScanner aware of the *updated* file.
		Android::mediaScannerScanFile(QFileInfo(path).absolutePath());
#endif
		return true;  // NOLINT : redundant boolean literal in conditional return statement
	}
	
	return false;
}


bool Track::saveGpxTo(QIODevice& device) const
{
	static const auto newline = QString::fromLatin1("\n");
	
	QXmlStreamWriter stream(&device);
	stream.writeStartDocument();
	stream.writeCharacters(newline);
	stream.writeStartElement(QString::fromLatin1("gpx"));
	stream.writeAttribute(QString::fromLatin1("version"), QString::fromLatin1("1.1"));
	stream.writeAttribute(QString::fromLatin1("creator"), qApp->applicationDisplayName());
	
	int size = getNumWaypoints();
	for (int i = 0; i < size; ++i)
	{
		stream.writeCharacters(newline);
		stream.writeStartElement(QStringLiteral("wpt"));
		const TrackPoint& point = getWaypoint(i);
		point.save(&stream);
		stream.writeTextElement(QStringLiteral("name"), getWaypointName(i));
		stream.writeEndElement();
	}
	
	stream.writeCharacters(newline);
	stream.writeStartElement(QStringLiteral("trk"));
	for (int i = 0; i < getNumSegments(); ++i)
	{
		stream.writeCharacters(newline);
		stream.writeStartElement(QStringLiteral("trkseg"));
		size = getSegmentPointCount(i);
		for (int k = 0; k < size; ++k)
		{
			stream.writeCharacters(newline);
			stream.writeStartElement(QStringLiteral("trkpt"));
			const TrackPoint& point = getSegmentPoint(i, k);
			point.save(&stream);
			stream.writeEndElement();
		}
		stream.writeCharacters(newline);
		stream.writeEndElement();
	}
	stream.writeCharacters(newline);
	stream.writeEndElement();
	
	stream.writeCharacters(newline);
	stream.writeEndElement();
	stream.writeEndDocument();
	return !stream.hasError();
}

void Track::appendTrackPoint(const TrackPoint& point)
{
	segment_points.push_back(point);
	segment_points.back().map_coord = map_georef.toMapCoordF(point.latlon, nullptr); // TODO: check for errors
	
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

void Track::appendWaypoint(const TrackPoint& point, const QString& name)
{
	waypoints.push_back(point);
	waypoints.back().map_coord = map_georef.toMapCoordF(point.latlon, nullptr); // TODO: check for errors
	waypoint_names.push_back(name);
}

void Track::changeMapGeoreferencing(const Georeferencing& new_map_georef)
{
	map_georef = new_map_georef;
	
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
		avg_latitude += point.latlon.latitude();
		avg_longitude += point.latlon.longitude();
		++num_samples;
	}
	for (int i = 0; i < getNumSegments(); ++i)
	{
		size = getSegmentPointCount(i);
		for (int k = 0; k < size; ++k)
		{
			const TrackPoint& point = getSegmentPoint(i, k);
			avg_latitude += point.latlon.latitude();
			avg_longitude += point.latlon.longitude();
			++num_samples;
		}
	}
	
	return LatLon((num_samples > 0) ? (avg_latitude / num_samples) : 0,
				  (num_samples > 0) ? (avg_longitude / num_samples) : 0);
}

bool Track::loadGpxFrom(QIODevice& device, bool project_points)
{
	TrackPoint point;
	QString point_name;

	QXmlStreamReader stream(&device);
	while (!stream.atEnd())
	{
		stream.readNext();
		if (stream.tokenType() == QXmlStreamReader::StartElement)
		{
			if (stream.name().compare(QLatin1String("wpt"), Qt::CaseInsensitive) == 0
			    || stream.name().compare(QLatin1String("trkpt"), Qt::CaseInsensitive) == 0
			    || stream.name().compare(QLatin1String("rtept"), Qt::CaseInsensitive) == 0)
			{
				point = TrackPoint{LatLon{stream.attributes().value(QLatin1String("lat")).toDouble(),
				                          stream.attributes().value(QLatin1String("lon")).toDouble()}};
				if (project_points)
					point.map_coord = map_georef.toMapCoordF(point.latlon); // TODO: check for errors
				point_name.clear();
			}
			else if (stream.name().compare(QLatin1String("trkseg"), Qt::CaseInsensitive) == 0
			         || stream.name().compare(QLatin1String("rte"), Qt::CaseInsensitive) == 0)
			{
				if (segment_starts.empty()
				    || segment_starts.back() < (int)segment_points.size())
				{
					segment_starts.push_back(segment_points.size());
				}
			}
			else if (stream.name().compare(QLatin1String("ele"), Qt::CaseInsensitive) == 0)
				point.elevation = stream.readElementText().toFloat();
			else if (stream.name().compare(QLatin1String("time"), Qt::CaseInsensitive) == 0)
				point.datetime = QDateTime::fromString(stream.readElementText(), Qt::ISODate);
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
	
	if (!segment_starts.empty()
	    && segment_starts.back() == (int)segment_points.size())
	{
		segment_starts.pop_back();
	}
	
	return !stream.hasError();
}

void Track::projectPoints()
{
	/// \todo Check for errors from Georeferencing::toMapCoordF()
	for (auto& waypoint : waypoints)
		waypoint.map_coord = map_georef.toMapCoordF(waypoint.latlon, nullptr); 
	for (auto& segment_point : segment_points)
		segment_point.map_coord = map_georef.toMapCoordF(segment_point.latlon, nullptr); 
}


bool operator==(const Track& lhs, const Track& rhs)
{
	return lhs.waypoints == rhs.waypoints
	       && lhs.waypoint_names == rhs.waypoint_names
	       && lhs.segment_points == rhs.segment_points
	       && lhs.segment_starts == rhs.segment_starts
	       && lhs.current_segment_finished == rhs.current_segment_finished;
}


}  // namespace OpenOrienteering
