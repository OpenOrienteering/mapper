/*
 *    Copyright 2012, 2013 Thomas Schöps
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

QT_BEGIN_NAMESPACE
class QFile;
class QXmlStreamWriter;
QT_END_NAMESPACE

class MapEditorController;

/// A point in a track or a waypoint, which stores position on ellipsoid and map and more attributes (e.g. number of satellites)
struct TrackPoint
{
	LatLon gps_coord;
	MapCoordF map_coord;
	bool is_curve_start;
	
	QDateTime datetime;		// QDateTime() if invalid
	float elevation;		// -9999 if invalid
	int num_satellites;		// -1 if invalid
	float hDOP;				// -1 if invalid
	
	TrackPoint(LatLon coord = LatLon(), QDateTime datetime = QDateTime(), float elevation = -9999, int num_satellites = -1, float hDOP = -1);
	void save(QXmlStreamWriter* stream) const;
};

/// Stores a set of tracks and / or waypoints, e.g. taken from a GPS device.
/// Can optionally store a track coordinate reference system in track_georef;
/// if no track CRS is given, assumes that coordinates are geographic WGS84 coordinates
class Track
{
public:
	/// Constructs an empty track
	Track();
	Track(const Georeferencing& map_georef);
	/// Duplicates a track
	Track(const Track& other);
	
	~Track();
	
	/// Deletes all data of the track, except the projection parameters
	void clear();
	
	/// Attempts to load the track from the given file.
	/// If you choose not to project_point, you have to call changeProjectionParams() afterwards.
	bool loadFrom(const QString& path, bool project_points, QWidget* dialog_parent = NULL);
	/// Attempts to save the track to the given file
	bool saveTo(const QString& path) const;
	
	// Modifiers
	void appendTrackPoint(TrackPoint& point);	// also converts the point's gps coords to map coords
	void finishCurrentSegment();
	
	void appendWaypoint(TrackPoint& point, const QString& name);	// also converts the point's gps coords to map coords
	
	void changeMapGeoreferencing(const Georeferencing& new_georef);
	
	/// Sets the track coordinate reference system.
	/// The Track object takes ownership of the Georeferencing object.
	void setTrackCRS(Georeferencing* track_crs);
	
	// Getters
	int getNumSegments() const;
	int getSegmentPointCount(int segment_number) const;
	const TrackPoint& getSegmentPoint(int segment_number, int point_number) const;
	const QString& getSegmentName(int segment_number) const;
	
	int getNumWaypoints() const;
	const TrackPoint& getWaypoint(int number) const;
	const QString& getWaypointName(int number) const;
	
	bool hasTrackCRS() const {return track_crs != NULL;}
	Georeferencing* getTrackCRS() const {return track_crs;}
	
	/// Averages all track coordinates
	LatLon calcAveragePosition() const;
	
private:
	bool loadFromGPX(QFile* file, bool project_points, QWidget* dialog_parent);
	bool loadFromDXF(QFile* file, bool project_points, QWidget* dialog_parent);
	bool loadFromOSM(QFile* file, bool project_points, QWidget* dialog_parent);
	
	void projectPoints();
	
	
	std::vector<TrackPoint> waypoints;
	std::vector<QString> waypoint_names;
	
	std::vector<TrackPoint> segment_points;
	// The indices of the first points of every track segment in this track
	std::vector<int> segment_starts;
	std::vector<QString> segment_names;
	
	bool current_segment_finished;
	
	Georeferencing* track_crs;
	Georeferencing map_georef;
};

#endif
