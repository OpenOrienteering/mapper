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

#include <algorithm>
#include <iterator>
#include <numeric>

#include <Qt>
#include <QtGlobal>
#include <QtNumeric>
#include <QApplication>
#include <QFile>
#include <QFileInfo>  // IWYU pragma: keep
#include <QIODevice>
#include <QLatin1String>
#include <QSaveFile>
#include <QStringRef>
#include <QXmlStreamAttributes>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
// IWYU pragma: no_include <qxmlstream.h>

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
	if (!name.isEmpty())
		stream->writeTextElement(QStringLiteral("name"), name);
}

bool operator==(const TrackPoint& lhs, const TrackPoint& rhs)
{
	auto fuzzyCompare =[](auto a, auto b) {
		return (qIsNaN(a) && qIsNaN(b))
		       || qFuzzyCompare(a, b);
	};
	return lhs.latlon == rhs.latlon
	       && lhs.datetime == rhs.datetime
	       && fuzzyCompare(lhs.elevation, rhs.elevation)
	       && fuzzyCompare(lhs.hDOP, rhs.hDOP)
	       && lhs.name == rhs.name;
}



// ### Track ###

Track::Track(const Track& other)
{
	*this = other;
}

Track::~Track() = default;

Track& Track::operator=(const Track& rhs)
{
	if (this != &rhs)
	{
		waypoints = rhs.waypoints;
		segments = rhs.segments;
		current_segment_finished = rhs.current_segment_finished;
	}
	return *this;
}

bool Track::empty() const
{
	return waypoints.empty() 
	       && std::all_of(begin(segments), end(segments), [](const auto& s) { return s.empty(); });
}

void Track::clear()
{
	waypoints.clear();
	segments.clear();
	segments.squeeze();  // QVarLengthArray: reset to local storage
	current_segment_finished = true;
}

void Track::squeeze()
{
	if (segments.size() > 1)
	{
		auto current = segments.end() - 1;
		if (current->empty())
		{
			auto previous = current - 1;
			*current = *previous;
			previous->swap(*current);
			current->clear();
		}
	}
}


bool Track::loadFrom(const QString& path)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;
	
	clear();
	if (path.endsWith(QLatin1String(".gpx"), Qt::CaseInsensitive))
	{
		if (!loadGpxFrom(file))
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
	
	for (const auto& waypoint : waypoints)
	{
		stream.writeCharacters(newline);
		stream.writeStartElement(QStringLiteral("wpt"));
		waypoint.save(&stream);
		stream.writeEndElement();
	}
	
	stream.writeCharacters(newline);
	stream.writeStartElement(QStringLiteral("trk"));
	for (const auto& segment : segments)
	{
		stream.writeCharacters(newline);
		stream.writeStartElement(QStringLiteral("trkseg"));
		for (const auto& trackPoint : segment)
		{
			stream.writeCharacters(newline);
			stream.writeStartElement(QStringLiteral("trkpt"));
			trackPoint.save(&stream);
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
	if (Q_UNLIKELY(current_segment_finished))
	{
		segments.push_back({});
		squeeze();
		current_segment_finished = false;
	}
	
	Q_ASSERT(!segments.empty());
	segments.back().push_back(point);
}

void Track::finishCurrentSegment()
{
	current_segment_finished = true;
}

void Track::appendWaypoint(const TrackPoint& point)
{
	waypoints.push_back(point);
}

int Track::getNumSegments() const
{
	return static_cast<int>(segments.size());
}

const TrackSegment& Track::getSegment(int segment_number) const
{
	return segments[segment_number];
}

int Track::getSegmentPointCount(int segment_number) const
{
	return static_cast<int>(segments[segment_number].size());
}

const TrackPoint& Track::getSegmentPoint(int segment_number, int point_number) const
{
	return segments[segment_number][static_cast<TrackSegment::size_type>(point_number)];
}

const TrackSegment&Track::getWaypoints() const
{
	return waypoints;
}

int Track::getNumWaypoints() const
{
	return static_cast<int>(waypoints.size());
}

const TrackPoint& Track::getWaypoint(int number) const
{
	return waypoints[static_cast<TrackSegment::size_type>(number)];
}

LatLon Track::calcAveragePosition() const
{
	auto num_samples = waypoints.size();
	auto acc = std::accumulate(begin(waypoints), end(waypoints), LatLon{}, [](LatLon a, const TrackPoint& t) {
		return LatLon{a.latitude() + t.latlon.latitude(), a.longitude() + t.latlon.longitude()};
	});
	for (const auto& segment : segments)
	{
		num_samples += segment.size();
		acc = std::accumulate(begin(segment), end(segment), acc, [](const LatLon& a, const TrackPoint& t) {
			return LatLon{a.latitude() + t.latlon.latitude(), a.longitude() + t.latlon.longitude()};
		});
	}
	if (num_samples == 0)
		num_samples = 1;
	
	return { acc.latitude() / num_samples, acc.longitude() / num_samples };
}

bool Track::loadGpxFrom(QIODevice& device)
{
	TrackPoint point;
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
			}
			else if (stream.name().compare(QLatin1String("trkseg"), Qt::CaseInsensitive) == 0
			         || stream.name().compare(QLatin1String("rte"), Qt::CaseInsensitive) == 0)
			{
				segments.push_back({});
				squeeze();
			}
			else if (stream.name().compare(QLatin1String("ele"), Qt::CaseInsensitive) == 0)
				point.elevation = stream.readElementText().toFloat();
			else if (stream.name().compare(QLatin1String("time"), Qt::CaseInsensitive) == 0)
				point.datetime = QDateTime::fromString(stream.readElementText(), Qt::ISODate);
			else if (stream.name().compare(QLatin1String("hdop"), Qt::CaseInsensitive) == 0)
				point.hDOP = stream.readElementText().toFloat();
			else if (stream.name().compare(QLatin1String("name"), Qt::CaseInsensitive) == 0)
				point.name = stream.readElementText();
		}
		else if (stream.tokenType() == QXmlStreamReader::EndElement)
		{
			if (stream.name().compare(QLatin1String("wpt"), Qt::CaseInsensitive) == 0)
			{
				waypoints.push_back(point);
			}
			else if (stream.name().compare(QLatin1String("trkpt"), Qt::CaseInsensitive) == 0
			         || stream.name().compare(QLatin1String("rtept"), Qt::CaseInsensitive) == 0)
			{
				/// \todo Don't load route points as track points.
				if (Q_UNLIKELY(segments.empty()))
					segments.push_back({});
				segments.back().push_back(point);
			}
		}
	}
	
	current_segment_finished = true;
	
	return !stream.hasError();
}


bool operator==(const Track& lhs, const Track& rhs)
{
	return lhs.waypoints == rhs.waypoints
	       && lhs.segments == rhs.segments;
}


}  // namespace OpenOrienteering
