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
#include <QInputDialog> // TODO: get rid of this
#include <QMessageBox>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "global.h"
#include "dxfparser.h"

GPSPoint::GPSPoint(LatLon coord, QDateTime datetime, float elevation, int num_satellites, float hDOP)
{
	gps_coord = coord;
	this->datetime = datetime;
	this->elevation = elevation;
	this->num_satellites = num_satellites;
	this->hDOP = hDOP;
}
void GPSPoint::save(QXmlStreamWriter* stream) const
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



// ### GPSTrack ###

GPSTrack::GPSTrack()
{
	current_segment_finished = true;
}

GPSTrack::GPSTrack(const Georeferencing& georef) : georef(georef)
{
	current_segment_finished = true;
}

GPSTrack::GPSTrack(const GPSTrack& other)
{
	waypoints = other.waypoints;
	waypoint_names = other.waypoint_names;
	
	segment_points = other.segment_points;
	segment_starts = other.segment_starts;
	
	current_segment_finished = other.current_segment_finished;
	
	georef = other.georef;
}

void GPSTrack::clear()
{
	waypoints.clear();
	waypoint_names.clear();
	segment_points.clear();
	segment_starts.clear();
	current_segment_finished = true;
}

bool GPSTrack::loadFrom(const QString& path, bool project_points, QWidget* dialog_parent)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;
	
	clear();

	if(path.endsWith(".gpx", Qt::CaseInsensitive)){
		GPSPoint point;
		QString point_name;

		QXmlStreamReader stream(&file);
		while (!stream.atEnd())
		{
			stream.readNext();
			if (stream.tokenType() == QXmlStreamReader::StartElement)
			{
				if (stream.name().compare("wpt", Qt::CaseInsensitive) == 0 || stream.name().compare("trkpt", Qt::CaseInsensitive) == 0)
				{
					point = GPSPoint(LatLon(stream.attributes().value("lat").toString().toDouble(),
												   stream.attributes().value("lon").toString().toDouble(), true));
					if (project_points)
						point.map_coord = georef.toMapCoordF(point.gps_coord, NULL); // FIXME: check for errors
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
	}
	else if(path.endsWith(".dxf", Qt::CaseInsensitive)){
		DXFParser *parser = new DXFParser();
		parser->setData(&file);
		QString result = parser->parse();
		if(!result.isEmpty()){
			QMessageBox::critical(dialog_parent, QObject::tr("Error reading"), QObject::tr("There was an error reading the DXF file %1:\n\n%1").arg(file.fileName(), result));
			delete parser;
			return false;
		}
		QList<path_t> paths = parser->getData();
		//QRectF size = parser->getSize();
		delete parser;
		int res = QMessageBox::question(dialog_parent, QObject::tr("Question"), QObject::tr("Are the coordinates in the DXF file in degrees?"), QMessageBox::Yes|QMessageBox::No);
		bool degrees = (res == QMessageBox::Yes);
		qreal val1 = QInputDialog::getDouble(dialog_parent, QObject::tr("Scale value"), QObject::tr("Choose a value to scale latitude coordinates by. A value of 1 does nothing, over one scales up and under one scales down."), 1, 0.000000001, 100000000, 10);
		qreal val2 = QInputDialog::getDouble(dialog_parent, QObject::tr("Scale value"), QObject::tr("Choose a value to scale longitude coordinates by. A value of 1 does nothing, over one scales up and under one scales down."), val1, 0.000000001, 100000000, 10);
		foreach(path_t path, paths){
			if(path.type == POINT){
				if(path.coords.size() < 1)
					continue;
				GPSPoint point = GPSPoint(LatLon(path.coords.at(0).y*val1, path.coords.at(0).x*val2, degrees));
				if (project_points)
					point.map_coord = georef.toMapCoordF(point.gps_coord, NULL); // FIXME: check for errors
				waypoints.push_back(point);
				waypoint_names.push_back(path.layer);
			}
			if(path.type == LINE){
				if(path.coords.size() < 1)
					continue;
				segment_starts.push_back(segment_points.size());
				foreach(coordinate_t coord, path.coords){
					GPSPoint point = GPSPoint(LatLon(coord.y*val1, coord.x*val2, degrees), QDateTime());
					if (project_points)
						point.map_coord = georef.toMapCoordF(point.gps_coord, NULL); // FIXME: check for errors
					segment_points.push_back(point);
				}
			}
		}
	}
	else if (path.endsWith(".osm", Qt::CaseInsensitive))
	{
		// Basic OSM file support
		// Reference: http://wiki.openstreetmap.org/wiki/OSM_XML
		const double min_supported_version = 0.5;
		const double max_supported_version = 0.6;
		QHash<QString, GPSPoint> nodes;
		int node_problems = 0;
		
		QXmlStreamReader stream(&file);
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
					GPSPoint point(LatLon(lat, lon, true));
					if (project_points)
						point.map_coord = georef.toMapCoordF(point.gps_coord, NULL); // FIXME: check for errors
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
	}


	file.close();
	return true;
}
bool GPSTrack::saveTo(const QString& path) const
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
		const GPSPoint& point = getWaypoint(i);
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
			const GPSPoint& point = getSegmentPoint(i, k);
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

void GPSTrack::appendTrackPoint(GPSPoint& point)
{
	point.map_coord = georef.toMapCoordF(point.gps_coord, NULL); // FIXME: check for errors
	segment_points.push_back(point);
	
	if (current_segment_finished)
	{
		segment_starts.push_back(segment_points.size() - 1);
		current_segment_finished = false;
	}
}
void GPSTrack::finishCurrentSegment()
{
	current_segment_finished = true;
}

void GPSTrack::appendWaypoint(GPSPoint& point, const QString& name)
{
	point.map_coord = georef.toMapCoordF(point.gps_coord, NULL); // FIXME: check for errors
	waypoints.push_back(point);
	waypoint_names.push_back(name);
}

void GPSTrack::changeGeoreferencing(const Georeferencing& new_georef)
{
	georef = new_georef;
	
	int size = waypoints.size();
	for (int i = 0; i < size; ++i)
		waypoints[i].map_coord = georef.toMapCoordF(waypoints[i].gps_coord, NULL); // FIXME: check for errors
	
	size = segment_points.size();
	for (int i = 0; i < size; ++i)
		segment_points[i].map_coord = georef.toMapCoordF(segment_points[i].gps_coord, NULL); // FIXME: check for errors
}

int GPSTrack::getNumSegments() const
{
	return (int)segment_starts.size();
}

int GPSTrack::getSegmentPointCount(int segment_number) const
{
	assert(segment_number >= 0 && segment_number < (int)segment_starts.size());
	if (segment_number == (int)segment_starts.size() - 1)
		return segment_points.size() - segment_starts[segment_number];
	else
		return segment_starts[segment_number + 1] - segment_starts[segment_number];
}

const GPSPoint& GPSTrack::getSegmentPoint(int segment_number, int point_number) const
{
	assert(segment_number >= 0 && segment_number < (int)segment_starts.size());
	return segment_points[segment_starts[segment_number] + point_number];
}

int GPSTrack::getNumWaypoints() const
{
	return waypoints.size();
}

const GPSPoint& GPSTrack::getWaypoint(int number) const
{
	return waypoints[number];
}

const QString& GPSTrack::getWaypointName(int number) const
{
	return waypoint_names[number];
}
