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
#include <QVarLengthArray>

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
	QString name        = {};
	
	// Default special member functions are fine.
	
	void save(QXmlStreamWriter* stream) const;
};

bool operator==(const TrackPoint& lhs, const TrackPoint& rhs);

inline bool operator!=(const TrackPoint& lhs, const TrackPoint& rhs) { return !(lhs==rhs); }



/** 
 * A TrackSegment is a continuous span of track data.
 * 
 * \see https://www.topografix.com/GPX/1/1/#type_trksegType
 */
using TrackSegment = std::vector<TrackPoint>;



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
	
	/// Returns true when the track contains no points.
	bool empty() const;
	
	/// Deletes all data of the track
	void clear();
	
	/// If the current segment is empty, squeezes the previous segment, but
	/// moves its allocation to the current segment.
	void squeeze();
   
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
	void appendWaypoint(const TrackPoint& point);
	
	
	// Getters
	
	/// Returns the number of track segments.
	int getNumSegments() const;
	
	/// Returns the given track segment.
	const TrackSegment& getSegment(int segment_number) const;
	
	int getSegmentPointCount(int segment_number) const; /// @deprecated
	const TrackPoint& getSegmentPoint(int segment_number, int point_number) const; /// @deprecated
	
	
	/// Returns the waypoints.
	const TrackSegment& getWaypoints() const;
	
	int getNumWaypoints() const; /// @deprecated
	const TrackPoint& getWaypoint(int number) const; /// @deprecated
	
	
	/// Averages all track coordinates
	LatLon calcAveragePosition() const;
	
	/** Assigns a copy of another Track's data to this object. */
	Track& operator=(const Track& rhs);
	
private:
	TrackSegment waypoints;
	QVarLengthArray<TrackSegment, 8> segments;
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
