/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2015 Kai Pastor
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


#ifndef _OPENORIENTEERING_TOOL_HELPERS_H_
#define _OPENORIENTEERING_TOOL_HELPERS_H_

#include <set>

#include <QCursor>
#include <QObject>
#include <QRectF>

#include "core/path_coord.h"
#include "tool.h"

QT_BEGIN_NAMESPACE
class QMouseEvent;
class QKeyEvent;
class QPainter;
class QTimer;
QT_END_NAMESPACE

class Map;
class MapWidget;
class MapEditorController;
class TextObject;
class TextObjectAlignmentDockWidget;
class Object;
class PathObject;

/**
 * Helper class to enable text editing (using the DrawTextTool and the EditTool).
 * 
 * To use it, after constructing an instance for the TextObject to edit, you must
 * pass through all mouse and key events to it and call draw() & includeDirtyRect().
 * If mousePressEvent() or mouseReleaseEvent() returns false, editing is finished.
 */
class TextObjectEditorHelper : public QObject
{
Q_OBJECT
public:
	TextObjectEditorHelper(TextObject* object, MapEditorController* editor);
	~TextObjectEditorHelper();
	
	bool mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	bool mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	bool mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	
	bool keyPressEvent(QKeyEvent* event);
	bool keyReleaseEvent(QKeyEvent* event);
	
	void draw(QPainter* painter, MapWidget* widget);
	void includeDirtyRect(QRectF& rect);
	
	inline void setSelection(int start, int end) {selection_start = start; selection_end = end; click_position = start;}
	inline TextObject* getObject() const {return object;}
	
public slots:
	void alignmentChanged(int horz, int vert);
	void setFocus();
	
signals:
	/// Emitted when a user action changes the selection (not called by setSelection()), or the text alignment. If the text is also changed, text_change is true.
	void selectionChanged(bool text_change);
	
private:
	void insertText(QString text);
	void updateDragging(MapCoordF map_coord);
	bool getNextLinesSelectionRect(int& line, QRectF& out);
	
	bool dragging;
	int click_position;
	int selection_start;
	int selection_end;
	TextObject* object;
	MapEditorController* editor;
	TextObjectAlignmentDockWidget* dock_widget;
	QCursor original_cursor;
	bool original_cursor_retrieved;
	QTimer* timer;
};


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
	MapCoord snapToObject(MapCoordF position, MapWidget* widget, SnappingToolHelperSnapInfo* info = NULL, Object* exclude_object = NULL, float snap_distance = -1);
	
	/**
	 * Checks for existing objects in map at position and if one is found,
	 * returns true and sets related angles in angle_tool.
	 * Internally remembers the position so the next call to draw() will
	 * draw the snap mark there.
	 */
	bool snapToDirection(MapCoordF position, MapWidget* widget, ConstrainAngleToolHelper* angle_tool, MapCoord* out_snap_position = NULL);
	
	/** Draws the snap mark which was last returned by snapToObject(). */
	void draw(QPainter* painter, MapWidget* widget);
	
	/** Includes this helper's drawing region in the given rect. */
	void includeDirtyRect(QRectF& rect);
	
	/** Returns the radius of the visualization in pixels. */
	inline int getDisplayRadius() const {return (snapped_type != NoSnapping) ? 6 : 0;}
	
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
	/** Object snapped onto, if type is ObjectCorners or ObjectPaths, else NULL */
	Object* object;
	/** Index of the map coordinate which was snapped onto if type is ObjectCorners,
	 *  else -1 (not snapped to a specific coordinate) */
	MapCoordVector::size_type coord_index;
	/** The closest point on the snapped path is returned
	 *  in path_coord if type == ObjectPaths  */
	PathCoord path_coord;
};


/** Helper class to 'follow' (i.e. extract continuous parts from) PathObjects */
class FollowPathToolHelper
{
public:
	FollowPathToolHelper();
	
	/**
	 * Starts following the given object from a coordinate.
	 * FollowPathToolHelpers can be reused for following different paths.
	 */
	void startFollowingFromCoord(PathObject* path, MapCoordVector::size_type coord_index);
	
	/**
	 * Starts following the given object from an arbitrary position indicated by the path coord.
	 * The path coord does not need to be from the object's path coord vector.
	 * FollowPathToolHelpers can be reused for following different paths.
	 */
	void startFollowingFromPathCoord(PathObject* path, PathCoord& coord);
	
	/**
	 * Updates the process and returns the followed part of the path as a
	 * new path object in 'result' (ownership of this object is transferred
	 * to the caller!)
	 * 
	 * Returns false if the following failed, e.g. because the path coord is
	 * on another path part than the beginning or path and end are the same.
	 */
	bool updateFollowing(PathCoord& end_coord, PathObject*& result);
	
	/** Returns the index of the path part which is being followed */
	inline std::size_t getPartIndex() const {return part_index;}
	
private:
	PathObject* path;
	
	PathCoord::length_type start_clen;
	PathCoord::length_type end_clen;
	std::size_t part_index;
	bool drag_forward;
};

#endif
