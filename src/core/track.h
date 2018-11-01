/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2017, 2018 Kai Pastor
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


#ifndef OPENORIENTEERING_TRACK_H
#define OPENORIENTEERING_TRACK_H

#include <cmath>
#include <vector>

#include <QDateTime>
#include <QString>

#include "core/georeferencing.h"
#include "core/latlon.h"
#include "core/map_coord.h"

class QIODevice;
class QXmlStreamWriter;

namespace OpenOrienteering {


/**
 * A geographic point with optional attributes such as time.
 * 
 * \see GPX `ptType`, https://www.topografix.com/GPX/1/1/#type_ptType
 */
struct TrackPoint
{
	LatLon latlon;
	QDateTime datetime;             // QDateTime() when invalid
	float elevation     = NAN;      // NaN when invalid
	float hDOP          = NAN;      // NaN when invalid
	MapCoordF map_coord;
	
	// Default special member functions are fine.
	
	void save(QXmlStreamWriter* stream) const;
};

bool operator==(const TrackPoint& lhs, const TrackPoint& rhs);

inline bool operator!=(const TrackPoint& lhs, const TrackPoint& rhs) { return !(lhs==rhs); }



/**
 * Stores a set of tracks and / or waypoints, e.g. taken from a GPS device.
 * 
 * All coordinates are assumed to be geographic WGS84 coordinates.
 */
class Track
{
public:
	/// Constructs an empty track
	Track() = default;
	
	Track(const Georeferencing& map_georef);
	/// Duplicates a track
	Track(const Track& other);
	
	~Track();
	
	/// Deletes all data of the track, except the projection parameters
	void clear();
	
	/// Attempts to load the track from the given file.
	/// If you choose not to project_point, you have to call changeProjectionParams() afterwards.
	bool loadFrom(const QString& path, bool project_points);
	/// Attempts to load GPX data from the open device.
	bool loadGpxFrom(QIODevice& device, bool project_points);
	/// Attempts to save the track to the given file
	bool saveTo(const QString& path) const;
	/// Saves the track as GPX data to the open device.
	bool saveGpxTo(QIODevice& device) const;
	
	// Modifiers
	
	/**
	 * @brief Appends the point and updates the point's map coordinates.
	 * 
	 * The point's map coordinates are determined from its geographic coordinates
	 * according to the map's georeferencing.
	 */
	void appendTrackPoint(const TrackPoint& point);
	
	/**
	 * Marks the current track segment as finished, so the next added point
	 * will define the start of a new track segment.
	 */
	void finishCurrentSegment();
	
	/**
	 * @brief Appends the waypoint and updates the point's map coordinates.
	 * 
	 * The point's map coordinates are determined from its geographic coordinates
	 * according to the map's georeferencing.
	 */
	void appendWaypoint(const TrackPoint& point, const QString& name);
	
	/** Updates the map positions of all points based on the new georeferencing. */
	void changeMapGeoreferencing(const Georeferencing& new_map_georef);
	
	// Getters
	int getNumSegments() const;
	int getSegmentPointCount(int segment_number) const;
	const TrackPoint& getSegmentPoint(int segment_number, int point_number) const;
	
	int getNumWaypoints() const;
	const TrackPoint& getWaypoint(int number) const;
	const QString& getWaypointName(int number) const;
	
	/// Averages all track coordinates
	LatLon calcAveragePosition() const;
	
	/** Assigns a copy of another Track's data to this object. */
	Track& operator=(const Track& rhs);
	
private:
	void projectPoints();
	
	
	std::vector<TrackPoint> waypoints;
	std::vector<QString> waypoint_names;
	
	std::vector<TrackPoint> segment_points;
	// The indices of the first points of every track segment in this track
	std::vector<int> segment_starts;
	
	bool current_segment_finished = true;
	
	Georeferencing map_georef;
	
	friend bool operator==(const Track& lhs, const Track& rhs);
};

/**
 * Compares waypoints, segments, and track points for equality.
 * 
 * This operator does not (explicitly) compare the tracks' map georeferencing.
 * When this feature is actually used, it affects the projection to map
 * coordinates, and so its effects are recorded in the waypoints and track
 * points.
 */
bool operator==(const Track& lhs, const Track& rhs);

inline bool operator!=(const Track& lhs, const Track& rhs) { return !(lhs==rhs); }


}  // namespace OpenOrienteering

#endif  // OPENORIENTEERING_TRACK_H
