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


#include "gps_track.h"

#include <QApplication>
#include <QFile>
#include <QHash>
#include <QInputDialog> // TODO: get rid of this
#include <QMessageBox>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <mapper_config.h> // TODO: Replace APP_NAME by runtime function to remove this dependency

#include "dxfparser.h"

TrackPoint::TrackPoint(LatLon coord, QDateTime datetime, float elevation, int num_satellites, float hDOP)
{
	gps_coord = coord;
	this->datetime = datetime;
	this->elevation = elevation;
	this->num_satellites = num_satellites;
	this->hDOP = hDOP;
}
void TrackPoint::save(QXmlStreamWriter* stream) const
{
	stream->writeAttribute("lat", QString::number(gps_coord.getLatitudeInDegrees(), 'f', 12));
	stream->writeAttribute("lon", QString::number(gps_coord.getLongitudeInDegrees(), 'f', 12));
	
	if (datetime.isValid())
		stream->writeTextElement("time", datetime.toString(Qt::ISODate) + 'Z');
	if (elevation > -9999)
		stream->writeTextElement("ele", QString::number(elevation, 'f', 3));
	if (num_satellites >= 0)
		stream->writeTextElement("sat", QString::number(num_satellites));
	if (hDOP >= 0)
		stream->writeTextElement("hdop", QString::number(hDOP, 'f', 3));
}



// ### Track ###

Track::Track() : track_crs(NULL)
{
	current_segment_finished = true;
}

Track::Track(const Georeferencing& map_georef) : track_crs(NULL), map_georef(map_georef)
{
	current_segment_finished = true;
}

Track::Track(const Track& other)
{
	waypoints = other.waypoints;
	waypoint_names = other.waypoint_names;
	
	segment_points = other.segment_points;
	segment_starts = other.segment_starts;
	
	current_segment_finished = other.current_segment_finished;
	
	track_crs = new Georeferencing(*other.track_crs);
	map_georef = other.map_georef;
}

Track::~Track()
{
	delete track_crs;
}

void Track::clear()
{
	waypoints.clear();
	waypoint_names.clear();
	segment_points.clear();
	segment_starts.clear();
	current_segment_finished = true;
	delete track_crs;
	track_crs = NULL;
}

bool Track::loadFrom(const QString& path, bool project_points, QWidget* dialog_parent)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;
	
	clear();

	if (path.endsWith(".gpx", Qt::CaseInsensitive))
	{
		if (!loadFromGPX(&file, project_points, dialog_parent))
			return false;
	}
	else if (path.endsWith(".dxf", Qt::CaseInsensitive))
	{
		if (!loadFromDXF(&file, project_points, dialog_parent))
			return false;
	}
	else if (path.endsWith(".osm", Qt::CaseInsensitive))
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
	stream.writeStartElement("gpx");
	stream.writeAttribute("version", "1.1");
	stream.writeAttribute("creator", APP_NAME);
	
	int size = getNumWaypoints();
	for (int i = 0; i < size; ++i)
	{
		stream.writeStartElement("wpt");
		const TrackPoint& point = getWaypoint(i);
		point.save(&stream);
		stream.writeTextElement("name", waypoint_names[i]);
		stream.writeEndElement();
	}
	
	stream.writeStartElement("trk");
	for (int i = 0; i < getNumSegments(); ++i)
	{
		stream.writeStartElement("trkseg");
		size = getSegmentPointCount(i);
		for (int k = 0; k < size; ++k)
		{
			stream.writeStartElement("trkpt");
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
	return true;
}

void Track::appendTrackPoint(TrackPoint& point)
{
	point.map_coord = map_georef.toMapCoordF(track_crs, MapCoordF(point.gps_coord.longitude, point.gps_coord.latitude), NULL); // FIXME: check for errors
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
	point.map_coord = map_georef.toMapCoordF(track_crs, MapCoordF(point.gps_coord.longitude, point.gps_coord.latitude), NULL); // FIXME: check for errors
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
	assert(segment_number >= 0 && segment_number < (int)segment_starts.size());
	if (segment_number == (int)segment_starts.size() - 1)
		return segment_points.size() - segment_starts[segment_number];
	else
		return segment_starts[segment_number + 1] - segment_starts[segment_number];
}

const TrackPoint& Track::getSegmentPoint(int segment_number, int point_number) const
{
	assert(segment_number >= 0 && segment_number < (int)segment_starts.size());
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
		avg_latitude += point.gps_coord.latitude;
		avg_longitude += point.gps_coord.longitude;
		++num_samples;
	}
	for (int i = 0; i < getNumSegments(); ++i)
	{
		size = getSegmentPointCount(i);
		for (int k = 0; k < size; ++k)
		{
			const TrackPoint& point = getSegmentPoint(i, k);
			avg_latitude += point.gps_coord.latitude;
			avg_longitude += point.gps_coord.longitude;
			++num_samples;
		}
	}
	
	return LatLon((num_samples > 0) ? (avg_latitude / num_samples) : 0,
				  (num_samples > 0) ? (avg_longitude / num_samples) : 0);
}

bool Track::loadFromGPX(QFile* file, bool project_points, QWidget* dialog_parent)
{
	track_crs = new Georeferencing();
	track_crs->setProjectedCRS("", "+proj=latlong +datum=WGS84");
	track_crs->setTransformationDirectly(QTransform());
	
	TrackPoint point;
	QString point_name;

	QXmlStreamReader stream(file);
	while (!stream.atEnd())
	{
		stream.readNext();
		if (stream.tokenType() == QXmlStreamReader::StartElement)
		{
			if (stream.name().compare("wpt", Qt::CaseInsensitive) == 0 || stream.name().compare("trkpt", Qt::CaseInsensitive) == 0)
			{
				point = TrackPoint(LatLon(stream.attributes().value("lat").toString().toDouble(),
												stream.attributes().value("lon").toString().toDouble(), true));
				if (project_points)
					point.map_coord = map_georef.toMapCoordF(track_crs, MapCoordF(point.gps_coord.longitude, point.gps_coord.latitude), NULL); // FIXME: check for errors
				point_name = "";
			}
			else if (stream.name().compare("trkseg", Qt::CaseInsensitive) == 0)
				segment_starts.push_back(segment_points.size());
			else if (stream.name().compare("ele", Qt::CaseInsensitive) == 0)
				point.elevation = stream.readElementText().toFloat();
			else if (stream.name().compare("time", Qt::CaseInsensitive) == 0)
				point.datetime = QDateTime::fromString(stream.readElementText(), Qt::ISODate);
			else if (stream.name().compare("sat", Qt::CaseInsensitive) == 0)
				point.num_satellites = stream.readElementText().toInt();
			else if (stream.name().compare("hdop", Qt::CaseInsensitive) == 0)
				point.hDOP = stream.readElementText().toFloat();
			else if (stream.name().compare("name", Qt::CaseInsensitive) == 0)
				point_name = stream.readElementText();
		}
		else if (stream.tokenType() == QXmlStreamReader::EndElement)
		{
			if (stream.name().compare("wpt", Qt::CaseInsensitive) == 0)
			{
				waypoints.push_back(point);
				waypoint_names.push_back(point_name);
			}
			else if (stream.name().compare("trkpt", Qt::CaseInsensitive) == 0)
			{
				segment_points.push_back(point);
			}
		}
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
		QMessageBox::critical(dialog_parent, QObject::tr("Error reading"), QObject::tr("There was an error reading the DXF file %1:\n\n%1").arg(file->fileName(), result));
		delete parser;
		return false;
	}
	QList<path_t> paths = parser->getData();
	delete parser;
	
	// TODO: Re-implement the possibility to load degree values somewhere else.
	//       It does not fit here as this method is called again every time a map
	//       containing a track is re-loaded, and in this case the question should
	//       not be asked again.
	//int res = QMessageBox::question(dialog_parent, QObject::tr("Question"), QObject::tr("Are the coordinates in the DXF file in degrees?"), QMessageBox::Yes|QMessageBox::No);
	bool degrees = false; //(res == QMessageBox::Yes);
	foreach (path_t path, paths)
	{
		if (path.type == POINT)
		{
			if(path.coords.size() < 1)
				continue;
			TrackPoint point = TrackPoint(LatLon(path.coords.at(0).y, path.coords.at(0).x, degrees));
			if (project_points)
				point.map_coord = map_georef.toMapCoordF(track_crs, MapCoordF(point.gps_coord.longitude, point.gps_coord.latitude), NULL); // FIXME: check for errors
			waypoints.push_back(point);
			waypoint_names.push_back(path.layer);
		}
		if (path.type == LINE)
		{
			if (path.coords.size() < 1)
				continue;
			segment_starts.push_back(segment_points.size());
			foreach(coordinate_t coord, path.coords)
			{
				TrackPoint point = TrackPoint(LatLon(coord.y, coord.x, degrees), QDateTime());
				if (project_points)
					point.map_coord = map_georef.toMapCoordF(track_crs, MapCoordF(point.gps_coord.longitude, point.gps_coord.latitude), NULL); // FIXME: check for errors
				segment_points.push_back(point);
			}
		}
	}
	
	return true;
}

bool Track::loadFromOSM(QFile* file, bool project_points, QWidget* dialog_parent)
{
	track_crs = new Georeferencing();
	track_crs->setProjectedCRS("", "+proj=latlong +datum=WGS84");
	track_crs->setTransformationDirectly(QTransform());
	
	// Basic OSM file support
	// Reference: http://wiki.openstreetmap.org/wiki/OSM_XML
	const double min_supported_version = 0.5;
	const double max_supported_version = 0.6;
	QHash<QString, TrackPoint> nodes;
	int node_problems = 0;
	
	QXmlStreamReader stream(file);
	while (!stream.atEnd())
	{
		stream.readNext();
		QXmlStreamAttributes attributes(stream.attributes());
		if (stream.tokenType() == QXmlStreamReader::StartElement)
		{
			if (stream.name() == "node")
			{
				if (attributes.value("visible") == "false")
				{
					stream.skipCurrentElement();
					continue;
				}
				
				bool ok = !attributes.value("id").isEmpty();
				double lat, lon;
				if (ok) lat = attributes.value("lat").toString().toDouble(&ok);
				if (ok) lon = attributes.value("lon").toString().toDouble(&ok);
				if (!ok)
				{
					node_problems++;
					stream.skipCurrentElement();
					continue;
				}
				
				QString  point_name(attributes.value("id").toString());
				TrackPoint point(LatLon(lat, lon, true));
				if (project_points)
					point.map_coord = map_georef.toMapCoordF(track_crs, MapCoordF(point.gps_coord.longitude, point.gps_coord.latitude), NULL); // FIXME: check for errors
				nodes.insert(point_name, point);
				
				while (!stream.atEnd())
				{
					stream.readNext();
					if (stream.tokenType() == QXmlStreamReader::EndElement && stream.name() == "node")
						break;
					if (stream.tokenType() == QXmlStreamReader::StartElement && stream.name() != "tag")
						continue;
					
					if (stream.attributes().value("k") == "ele")
					{
						bool ok;
						double elevation = stream.attributes().value("v").toString().toDouble(&ok);
						if (ok) nodes[point_name].elevation = elevation;
					}
					else if (stream.attributes().value("k") == "name")
					{
						QString name = stream.attributes().value("v").toString();
						if (!name.isEmpty() && !nodes.contains(name)) 
						{
							waypoints.push_back(point);
							waypoint_names.push_back(name);
						}
					}
				}
			}
			else if (stream.name() == "way")
			{
				if (attributes.value("visible") == "false")
				{
					stream.skipCurrentElement();
					continue;
				}
				
				segment_starts.push_back(segment_points.size());
			}
			else if (stream.name() == "nd")
			{
				QString ref = attributes.value("ref").toString();
				if (ref.isEmpty() || !nodes.contains(ref))
					node_problems++;
				else
					segment_points.push_back(nodes[ref]);
			}
			else if (stream.name() == "osm")
			{
				double osm_version = attributes.value("version").toString().toDouble();
				if (osm_version < min_supported_version)
				{
					QMessageBox::critical(dialog_parent, QObject::tr("Error"), QObject::tr("The OSM file has version %1.\nThe minimum supported version is %2.").arg(attributes.value("version").toString(), QString::number(min_supported_version, 'g', 1)));
					return false;
				}
				if (osm_version > max_supported_version)
				{
					QMessageBox::critical(dialog_parent, QObject::tr("Error"), QObject::tr("The OSM file has version %1.\nThe maximum supported version is %2.").arg(attributes.value("version").toString(), QString::number(min_supported_version, 'g', 1)));
					return false;
				}
			}
			else
			{
				stream.skipCurrentElement();
			}
		}
	}
	
	if (node_problems > 0)
		QMessageBox::warning(dialog_parent, QObject::tr("Problems"), QObject::tr("%1 nodes could not be processed correctly.").arg(node_problems));
	
	return true;
}

void Track::projectPoints()
{
	int size = waypoints.size();
	for (int i = 0; i < size; ++i)
		waypoints[i].map_coord = map_georef.toMapCoordF(track_crs, MapCoordF(waypoints[i].gps_coord.longitude, waypoints[i].gps_coord.latitude), NULL); // FIXME: check for errors
		
	size = segment_points.size();
	for (int i = 0; i < size; ++i)
		segment_points[i].map_coord = map_georef.toMapCoordF(track_crs, MapCoordF(segment_points[i].gps_coord.longitude, segment_points[i].gps_coord.latitude), NULL); // FIXME: check for errors
}
