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

#include <vector>

#include <QDateTime>
#include <QHash>
#include <QString>

#include "core/georeferencing.h"
#include "core/latlon.h"
#include "core/map_coord.h"

class QFile;
class QWidget;
class QXmlStreamWriter;

namespace OpenOrienteering {


/**
 * A point in a track or a waypoint, which stores position on ellipsoid and
 * map and more attributes (e.g. number of satellites)
 */
struct TrackPoint
{
	LatLon gps_coord;
	MapCoordF map_coord;
	bool is_curve_start;
	
	QDateTime datetime;		// QDateTime() if invalid
	float elevation;		// -9999 if invalid
	int num_satellites;		// -1 if invalid
	float hDOP;				// -1 if invalid
	
	TrackPoint(LatLon coord = LatLon(), const QDateTime& datetime = QDateTime(),
			   float elevation = -9999, int num_satellites = -1, float hDOP = -1);
	void save(QXmlStreamWriter* stream) const;
};

/**
 * Stores a set of tracks and / or waypoints, e.g. taken from a GPS device.
 * Can optionally store a track coordinate reference system in track_georef;
 * if no track CRS is given, assumes that coordinates are geographic WGS84 coordinates
 */
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
	bool loadFrom(const QString& path, bool project_points, QWidget* dialog_parent = nullptr);
	/// Attempts to save the track to the given file
	bool saveTo(const QString& path) const;
	
	// Modifiers
	
	/**
	 * @brief Appends the point and updates the point's map coordinates.
	 * 
	 * The point's map coordinates are determined from its geographic coodinates
	 * according to the map's georeferencing.
	 */
	void appendTrackPoint(TrackPoint& point);
	
	/**
	 * Marks the current track segment as finished, so the next added point
	 * will define the start of a new track segment.
	 */
	void finishCurrentSegment();
	
	/**
	 * @brief Appends the waypoint and updates the point's map coordinates.
	 * 
	 * The point's map coordinates are determined from its geographic coodinates
	 * according to the map's georeferencing.
	 */
	void appendWaypoint(TrackPoint& point, const QString& name);
	
	/** Updates the map positions of all points based on the new georeferencing. */
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
	
	bool hasTrackCRS() const {return track_crs;}
	Georeferencing* getTrackCRS() const {return track_crs;}
	
	/// Averages all track coordinates
	LatLon calcAveragePosition() const;
	
	/** A collection of key:value tags. Cf. Object::Tags. */
	typedef QHash<QString, QString> Tags;
	
	/** A mapping of an element name to a tags collection. */
	typedef QHash<QString, Tags> ElementTags;
	
	/** Returns the mapping of element names to tag collections. */
	const ElementTags& tags() const;

	/** Assigns a copy of another Track's data to this object. */
	Track& operator=(const Track& rhs);
	
private:
	bool loadFromGPX(QFile* file, bool project_points, QWidget* dialog_parent);
	bool loadFromDXF(QFile* file, bool project_points, QWidget* dialog_parent);
	bool loadFromOSM(QFile* file, bool project_points, QWidget* dialog_parent);
	
	void projectPoints();
	
	
	/** A mapping of element id to tags. */
	ElementTags element_tags; 
	
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


// ### Track inline code ###

inline
const Track::ElementTags& Track::tags() const
{
	return element_tags;
}


}  // namespace OpenOrienteering

#endif  // OPENORIENTEERING_TRACK_H
