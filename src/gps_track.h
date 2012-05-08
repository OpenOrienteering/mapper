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


#ifndef _OPENORIENTEERING_GPS_TRACK_H_
#define _OPENORIENTEERING_GPS_TRACK_H_

#include <QString>
#include <QDate>

#include "georeferencing.h"
#include "gps_coordinates.h"

class QXmlStreamWriter;

class MapEditorController;

/// A point in a GPS track or a GPS waypoint, which stores position on ellipsoid and map and more attributes (e.g. number of satellites)
struct GPSPoint
{
	LatLon gps_coord;
	MapCoordF map_coord;
	
	QDateTime datetime;		// QDateTime() if invalid
	float elevation;		// -9999 if invalid
	int num_satellites;		// -1 if invalid
	float hDOP;				// -1 if invalid
	
	GPSPoint(LatLon coord = LatLon(), QDateTime datetime = QDateTime(), float elevation = -9999, int num_satellites = -1, float hDOP = -1);
	void save(QXmlStreamWriter* stream) const;
};

class GPSTrack
{
public:
	/// Constructs an empty GPS track
	GPSTrack();
	GPSTrack(const Georeferencing& Georeferencing);
	/// Duplicates a track
	GPSTrack(const GPSTrack& other);
	
	/// Deletes all data of the track, except the projection parameters
	void clear();
	
	/// Attempts to load the track from the given file. If you choose not to project_point, you have to call changeProjectionParams() afterwards.
	bool loadFrom(const QString& path, bool project_points, QWidget* dialog_parent = NULL);
	/// Attempts to save the track to the given file
	bool saveTo(const QString& path) const;
	
	// Modifiers
	void appendTrackPoint(GPSPoint& point);	// also converts the point's gps coords to map coords
	void finishCurrentSegment();
	
	void appendWaypoint(GPSPoint& point, const QString& name);	// also converts the point's gps coords to map coords
	
	void changeGeoreferencing(const Georeferencing& new_georef);
	
	// Getters
	int getNumSegments() const;
	int getSegmentPointCount(int segment_number) const;
	const GPSPoint& getSegmentPoint(int segment_number, int point_number) const;
	
	int getNumWaypoints() const;
	const GPSPoint& getWaypoint(int number) const;
	const QString& getWaypointName(int number) const;
	
private:
	std::vector<GPSPoint> waypoints;
	std::vector<QString> waypoint_names;
	
	std::vector<GPSPoint> segment_points;
	std::vector<int> segment_starts;	// the indices of the first points of every track segment in this track
	
	bool current_segment_finished;
	
	Georeferencing georef;
};

#endif
