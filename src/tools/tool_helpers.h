/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2016 Kai Pastor
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


#ifndef OPENORIENTEERING_TOOL_HELPERS_H
#define OPENORIENTEERING_TOOL_HELPERS_H

#include <cstddef>
#include <memory>
#include <set>

#include <QObject>

#include "core/map_coord.h"
#include "core/path_coord.h"
#include "gui/point_handles.h"

class QPainter;
class QPoint;
class QPointF;
class QRectF;

class Map;
class MapEditorTool;
class MapWidget;
class Object;
class PathObject;


/**
 * Helper class to constrain cursor positions to specific lines / directions
 * which originate from a given point
 */
class ConstrainAngleToolHelper : public QObject
{
Q_OBJECT
public:
	/**
	 * Constructs a helper without allowed angles.
	 * Use addAngle() and / or addAngles() to add allowed lines.
	 */
	ConstrainAngleToolHelper();
	ConstrainAngleToolHelper(const MapCoordF& center);
	~ConstrainAngleToolHelper();
	
	/** Sets the center of the lines */
	void setCenter(const MapCoordF& center);
	
	/**
	 * Adds a single allowed angle. Zero is to the right,
	 * the direction counter-clockwise in Qt's coordinate system.
	 */
	void addAngle(double angle);
	
	/**
	 * Adds a circular set of allowed angles, starting from 'base' with
	 * interval 'stepping'. Zero is to the right, the direction counter-clockwise
	 * in Qt's coordinate system.
	 */
	void addAngles(double base, double stepping);
	
	/**
	 * Like addAngles, but in degrees. Helps to avoid floating-point
	 * inaccuracies if using angle steppings like 15 degrees which could lead
	 * to two near-zero allowed angles otherwise.
	 */
	void addAnglesDeg(double base, double stepping);
	
	/**
	 * Adds the default angles given by the MapEditor_FixedAngleStepping setting.
	 * Usage of this method has the advantage that the stepping is updated
	 * automatically when the setting is changed.
	 */
	void addDefaultAnglesDeg(double base);
	
	/** Removes all allowed angles */
	void clearAngles();
	
	/**
	 * Get the cursor position, projected onto the closest of the allowed angles.
	 * Returns the chosen angle and marks it as active.
	 */
	double getConstrainedCursorPos(const QPoint& in_pos, QPointF& out_pos, MapWidget* widget);
	
	/**
	 * Get the cursor position, projected onto the closest of the allowed angles.
	 * Returns the chosen angle and marks it as active.
	 */
	double getConstrainedCursorPosMap(const MapCoordF& in_pos, MapCoordF& out_pos);
	
	/** Combination of the above methods for convenience */
	double getConstrainedCursorPositions(const MapCoordF& in_pos_map,
		MapCoordF& out_pos_map, QPointF& out_pos, MapWidget* widget);
	
	/**
	 * Activates or deactivates this tool.
	 * 
	 * If deactivated, it does nothing. This is just for convenience to avoid
	 * if-clauses in places where this tool is used sometimes.
	 * If activated, the center is replaced with the new one.
	 * 
	 * TODO: This is ugly and should be two functions. Do not use in new code.
	 *       Instead of setting the center when activating, the center should
	 *       be always kept up-to-date independently of the activation which
	 *       is more intuitive.
	 */
	void setActive(bool active, const MapCoordF& center);
	
	/** Version of setActive() which does not override the center */
	void setActive(bool active);
	
	inline bool isActive() const {return active;}
	
	/**
	 * Draws the set of allowed angles as lines radiating out from the
	 * center point. The active angle, if any, is highlighted.
	 */
	void draw(QPainter* painter, MapWidget* widget);
	
	/** Includes this helper's drawing region in the given rect. */
	void includeDirtyRect(QRectF& rect);
	
	/** Returns the radius of the visualization in pixels */
	inline int getDisplayRadius() const {return active ? 40 : 0;}
	
public slots:
	void settingsChanged();
	
signals:
	/** Emitted when the angle the cursor position is constrained to changes */
	void activeAngleChanged() const;
	
	/**
	 * Emitted whenever the display of this tool helper changes.
	 * This is when the active angle changes or the tool is activated / deactivated.
	 */
	void displayChanged() const;
	
private:
	inline void emitActiveAngleChanged() const {emit activeAngleChanged(); emit displayChanged();}
	
	/** The active angle or a negative number if no angle is active */
	double active_angle;
	/** The set of allowed angles. Values are in the range [0, 2*M_PI) */
	std::set<double> angles;
	/** The center point of all lines */
	MapCoordF center;
	/** Is this helper active? */
	bool active;
	
	bool have_default_angles_only;
	double default_angles_base;
};


class SnappingToolHelperSnapInfo;

/** Helper class to snap to existing objects or a grid on the map. */
class SnappingToolHelper : public QObject
{
Q_OBJECT
public:
	enum SnapObjects
	{
		NoSnapping = 0,
		ObjectCorners = 1 << 0,
		ObjectPaths = 1 << 1,
		GridCorners = 1 << 2,
		
		AllTypes = 1 + 2 + 4
	};
	
	/**
	 * Constructs a snapping tool helper. By default it is disabled
	 * (filter set to NoSnapping).
	 */
	SnappingToolHelper(MapEditorTool* tool, SnapObjects filter = NoSnapping);
	
	/** Constrain the objects to snap onto. */
	void setFilter(SnapObjects filter);
	SnapObjects getFilter() const;
	
	/**
	 * Snaps the given position to the closest snapping object, or returns
	 * the original position if no snapping object is close enough.
	 * Internally remembers the position so the next call to draw() will
	 * draw the snap mark there.
	 * 
	 * If the info parameter is set, information about the object
	 * snapped onto is returned there. The snap_distance parameter can be
	 * used to set the maximum snap distance. If it is negative,
	 * the corresponding application setting will be used.
	 * 
	 * TODO: widget parameter is only used for getMapView(). Replace by view parameter?
	 */
	MapCoord snapToObject(MapCoordF position, MapWidget* widget, SnappingToolHelperSnapInfo* info = nullptr, Object* exclude_object = nullptr, float snap_distance = -1);
	
	/**
	 * Checks for existing objects in map at position and if one is found,
	 * returns true and sets related angles in angle_tool.
	 * Internally remembers the position so the next call to draw() will
	 * draw the snap mark there.
	 */
	bool snapToDirection(MapCoordF position, MapWidget* widget, ConstrainAngleToolHelper* angle_tool, MapCoord* out_snap_position = nullptr);
	
	/** Draws the snap mark which was last returned by snapToObject(). */
	void draw(QPainter* painter, MapWidget* widget);
	
	/** Includes this helper's drawing region in the given rect. */
	void includeDirtyRect(QRectF& rect);
	
	/**
	 * Returns the radius of the visualization in pixels.
	 * 
	 * \todo Return a dynamic value, or adjust for screen density.
	 */
	inline int getDisplayRadius() const {return (snapped_type != NoSnapping) ? 12 : 0;}
	
signals:
	/** Emitted whenever the snap mark changes position. */
	void displayChanged() const;
	
private:
	SnapObjects filter;
	
	SnapObjects snapped_type;
	MapCoord snap_mark;
	
	Map* map;
	
	PointHandles point_handles;
};

/** Information returned from a snap process from SnappingToolHelper. */
class SnappingToolHelperSnapInfo
{
public:
	/** Type of object snapped onto */
	SnappingToolHelper::SnapObjects type;
	/** Object snapped onto, if type is ObjectCorners or ObjectPaths, else nullptr */
	Object* object;
	/** Index of the map coordinate which was snapped onto if type is ObjectCorners,
	 *  else -1 (not snapped to a specific coordinate) */
	MapCoordVector::size_type coord_index;
	/** The closest point on the snapped path is returned
	 *  in path_coord if type == ObjectPaths  */
	PathCoord path_coord;
};


/**
 * Helper class to 'follow' (i.e. extract continuous parts from) PathObjects.
 * 
 * A FollowPathToolHelper can be reused for following different paths.
 */
class FollowPathToolHelper
{
public:
	/**
	 * Constructs a new helper.
	 */
	FollowPathToolHelper();
	
	/**
	 * Starts following the given object from a coordinate.
	 */
	void startFollowingFromCoord(const PathObject* path, MapCoordVector::size_type coord_index);
	
	/**
	 * Starts following the given object from an arbitrary position indicated by the path coord.
	 * 
	 * The path coord does not need to be from the object's path coord vector.
	 */
	void startFollowingFromPathCoord(const PathObject* path, const PathCoord& coord);
	
	/**
	 * Updates the process and returns the followed part of the path.
	 * 
	 * Returns a pointer that owns nothing if the following failed, e.g.
	 * because the path coord is on another path part than the beginning or
	 * path and end are the same.
	 */
	std::unique_ptr<PathObject> updateFollowing(const PathCoord& end_coord);
	
	/**
	 * Returns the index of the path part which is being followed.
	 */
	std::size_t getPartIndex() const;
	
private:
	const PathObject* path;
	
	PathCoord::length_type start_clen;
	PathCoord::length_type end_clen;
	std::size_t part_index;
	bool drag_forward;
};



// ### FollowPathToolHelper inline code ###
inline
std::size_t FollowPathToolHelper::getPartIndex() const
{
	return part_index;
}

#endif
