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

#include "core/latlon.h"

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
	QDateTime datetime  = {};       // QDateTime() when invalid
	float elevation     = NAN;      // NaN when invalid
	float hDOP          = NAN;      // NaN when invalid
	
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
	
	/// Duplicates a track
	Track(const Track& other);
	
	~Track();
	
	/// Deletes all data of the track
	void clear();
	
	/// Attempts to load the track from the given file.
	bool loadFrom(const QString& path);
	/// Attempts to load GPX data from the open device.
	bool loadGpxFrom(QIODevice& device);
	/// Attempts to save the track to the given file
	bool saveTo(const QString& path) const;
	/// Saves the track as GPX data to the open device.
	bool saveGpxTo(QIODevice& device) const;
	
	// Modifiers
	
	/// Appends a track point to the current segment.
	void appendTrackPoint(const TrackPoint& point);
	
	/**
	 * Ends the current track segment, so that a new segment will be started
	 * when the next track point is added.
	 */
	void finishCurrentSegment();
	
	/// Appends a waypoint.
	void appendWaypoint(const TrackPoint& point, const QString& name);
	
	
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
	std::vector<TrackPoint> waypoints;
	std::vector<QString> waypoint_names;
	
	std::vector<TrackPoint> segment_points;
	// The indices of the first points of every track segment in this track
	std::vector<int> segment_starts;
	
	bool current_segment_finished = true;
	
	friend bool operator==(const Track& lhs, const Track& rhs);
};

/**
 * Compares waypoints, segments, and track points for equality.
 */
bool operator==(const Track& lhs, const Track& rhs);

inline bool operator!=(const Track& lhs, const Track& rhs) { return !(lhs==rhs); }


}  // namespace OpenOrienteering

#endif  // OPENORIENTEERING_TRACK_H
