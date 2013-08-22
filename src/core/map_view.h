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


#ifndef _OPENORIENTEERING_MAP_VIEW_H_
#define _OPENORIENTEERING_MAP_VIEW_H_

#include <QHash>

#include "../map_coord.h"
#include "../matrix.h"

class Map;
class MapWidget;
class Template;
struct TemplateVisibility;

class QPainter;
class QXmlStreamReader;
class QXmlStreamWriter;


/**
 * Stores view position, zoom, rotation and grid / template visibilities
 * to define a view onto a map.
 * 
 * These parameters define the view coordinates with origin at the view's center,
 * measured in pixels. The class provides methods to convert between view
 * coordinates and map coordinates (defined as millimeter on map paper).
 */
class MapView
{
public:
	/** Creates a default view looking at the origin */
	MapView(Map* map);
	
	/** Destroys the map view. */
	~MapView();
	
	/** Loads the map view in the old "native" format from the file. */
	void load(QIODevice* file, int version);
	
	/** Saves the map view state to an XML stream.
	 *  @param xml The XML output stream.
	 *  @param element_name The name of the element which will be written. 
	 *  @param skip_templates If true, visibility details of individual templates are not saved.
	 */
	void save(QXmlStreamWriter& xml, const QString& element_name, bool skip_templates = false);
	
	/** Loads the map view state from the current element of an xml stream. */
	void load(QXmlStreamReader& xml);
	
	/**
	 * Must be called to notify the map view of new widgets displaying it.
	 * Useful to notify the widgets which need to be redrawn
	 */
	void addMapWidget(MapWidget* widget);
	
	/** Removes a widget, see addMapWidget(). */
	void removeMapWidget(MapWidget* widget);
	
	/**
	 * Redraws all map widgets completely - that can be slow!
	 * Try to do partial updates instead, if possible.
	 */
	void updateAllMapWidgets();
	
	/** Converts x, y (with origin at the center of the view) to map coordinates */
	MapCoord viewToMap(double x, double y);
	
	/** Converts the point (with origin at the center of the view) to map coordinates */
	inline MapCoord viewToMap(QPointF point);
	
	/** Converts x, y (with origin at the center of the view) to map coordinates */
	inline MapCoordF viewToMapF(double x, double y);
	
	/** Converts the point (with origin at the center of the view) to map coordinates */
	inline MapCoordF viewToMapF(QPointF point);
	
	
	/// Converts map coordinates to view coordinates (with origin at the center of the view)
	inline void mapToView(MapCoord coords, double& out_x, double& out_y);
	
	/// Converts map coordinates to view coordinates (with origin at the center of the view)
	inline void mapToView(MapCoord coords, float& out_x, float& out_y);
	
	/// Converts map coordinates to view coordinates (with origin at the center of the view)
	inline QPointF mapToView(MapCoord coords);
	
	/// Converts map coordinates to view coordinates (with origin at the center of the view)
	inline QPointF mapToView(MapCoordF coords);
	
	
	/**
	 * Converts a length defined in native map coordinates (i.e. 1 unit equals
	 * 1 / 1000th millimeter on the map paper) to the corresponding length in pixels
	 * with the current view settings.
	 */
	double lengthToPixel(qint64 length);
	
	/**
	 * Converts a length defined in pixels to the corresponding length in native
	 * map coordinates (i.e. 1 unit equals 1 / 1000th millimeter on the map paper)
	 * with the current view settings.
	 */
	qint64 pixelToLength(double pixel);
	
	/**
	 * Calculates the bounding box of the map coordinates which can be viewed
	 * using the given view coordinates rect
	 */
	QRectF calculateViewedRect(QRectF view_rect);
	
	/**
	 * Calculates the bounding box in view coordinates
	 * of the given map coordinates rect
	 */
	QRectF calculateViewBoundingBox(QRectF map_rect);
	
	/**
	 * Applies the view transform to the given painter, so objects defined in
	 * map coordinates will be drawn at their view coordinates. Append a
	 * viewport transformation to this to get a complete map-to-viewport transformation
	 * which makes the view center appear at the viewport center.
	 * 
	 * Note: the transform is combined with the painter's existing transform.
	 */
	void applyTransform(QPainter* painter);
	
	// Dragging
	
	/** Sets the current drag offset while the map is being dragged. */
	void setDragOffset(QPoint offset);
	
	/**
	 * Finishes dragging the map.
	 * @param offset Drag offset of the whole pan operation, from start to end.
	 */
	void completeDragging(QPoint offset);
	
	/**
	 * Zooms the maps (in steps).
	 * 
	 * @param num_steps Number of zoom steps to zoom in. Can be negative to zoom out.
	 * @param preserve_cursor_pos If set, shifts the center of the view so the
	 *     cursor position will stay at the same map coordinate.
	 * @param cursor_pos_view The cursor position in view coordinates, must be
	 *     set if preserve_cursor_pos is used.
	 */
	bool zoomSteps(float num_steps, bool preserve_cursor_pos, QPointF cursor_pos_view = QPointF());
	
	/** Returns the map this view is defined on. */
	Map* getMap() const;
	
	/**
	 * Returns the final zoom factor for use in transformations.
	 * Depends on the pixel per millimeter of the display.
	 */
	float calculateFinalZoomFactor();
	
	/** Returns the raw zoom facor, see also calculateFinalZoomFactor(). */
	float getZoom() const;
	
	/** Sets the zoom factor and repaints all map widgets. */
	void setZoom(float value);
	
	/** Returns the view rotation (in radians). */
	float getRotation() const;
	
	/** Sets the view roation (in radians) and repaints all map widgets. */
	void setRotation(float value);
	
	/**
	 * Returns the x position of the view center in native map coordinates.
	 * (1/1000th of a millimeter on the map paper)
	 */
	qint64 getPositionX() const;
	
	/**
	 * Sets the x position of the view center in native map coordinates.
	 * (1/1000th of a millimeter on the map paper)
	 */
	void setPositionX(qint64 value);
	
	/**
	 * Returns the y position of the view center in native map coordinates.
	 * (1/1000th of a millimeter on the map paper)
	 */
	qint64 getPositionY() const;
	
	/**
	 * Sets the y position of the view center in native map coordinates.
	 * (1/1000th of a millimeter on the map paper)
	 */
	void setPositionY(qint64 value);
	
	/** Returns the view x offset from the rotation center. */
	int getViewX() const;
	
	/** Sets the view x offset from the rotation center. */
	void setViewX(int value);
	
	/** Returns the view y offset from the rotation center. */
	int getViewY() const;
	
	/** Sets the view y offset from the rotation center. */
	void setViewY(int value);
	
	/** Returns the current drag offset (when dragging the map). */
	QPoint getDragOffset() const;
	
	
	// Map and template visibilities
	
	/** Returns the visibility settings of the map. */
	TemplateVisibility* getMapVisibility();
	
	/**
	 * Checks if the template is visible without creating
	 * a template visibility object if none exists
	 */
	bool isTemplateVisible(const Template* temp) const;
	
	/**
	 * Returns the template visibility object, creates one if not there yet
	 * with the default settings (invisible)
	 */
	TemplateVisibility* getTemplateVisibility(Template* temp);
	
	/**
	 * Call this when a template is deleted to destroy the template visibility object
	 */
	void deleteTemplateVisibility(Template* temp);
	
	/** Enables or disables hiding all templates in this view */
	void setHideAllTemplates(bool value);
	
	/**
	 * Returns if the "hide all templates" toggle is active.
	 * See also setHideAllTemplates().
	 */
	bool areAllTemplatesHidden() const;
	
	/** Returns if the map grid is visible. */
	bool isGridVisible() const;
	
	/** Sets the map grid visibility. */
	void setGridVisible(bool visible);
	
	/** Returns if overprinting simulation is enabled. */
	bool isOverprintingSimulationEnabled() const;
	
	/** Enables or disables overprinting simulation. */
	void setOverprintingSimulationEnabled(bool enabled);
	
	// Static
	
	/** The global zoom in limit for the zoom factor. */
	static const double zoom_in_limit;
	/** The global zoom out limit for the zoom factor. */
	static const double zoom_out_limit;
	
private:
	typedef std::vector<MapWidget*> WidgetVector;
	
	void update();		// recalculates the x_to_y matrices
	
	Map* map;
	
	double zoom;		// factor
	double rotation;	// counterclockwise 0 to 2*PI. This is the viewer rotation, so the map is rotated clockwise
	qint64 position_x;	// position of the viewer, positive values move the map to the left; the position is in 1/1000 mm
	qint64 position_y;
	int view_x;			// view offset (so the view can be rotated around a point which is not in its center) ...
	int view_y;			// ... this can be used to always look ahead of the GPS position if a digital compass is present
	QPoint drag_offset;	// the distance the content of the view was dragged with the mouse, in pixels
	
	Matrix view_to_map;
	Matrix map_to_view;
	
	TemplateVisibility* map_visibility;
	QHash<const Template*, TemplateVisibility*> template_visibilities;
	bool all_templates_hidden;
	
	bool grid_visible;
	
	bool overprinting_simulation_enabled;
	
	WidgetVector widgets;
};



/**
 * Contains all visibility information for a template.
 * This is stored in the MapViews.
 */
struct TemplateVisibility
{
	/** Opacity from 0.0f (invisible) to 1.0f (opaque) */
	float opacity;
	
	/** Visibility flag */
	bool visible;
	
	/** Creates an opaque, but invisible TemplateVisibility. */
	inline TemplateVisibility() : opacity(1.0f), visible(false) { }
};



// ### MapView inline code ###

inline
MapCoord MapView::viewToMap(double x, double y)
{
	return MapCoord(view_to_map.get(0, 0) * x + view_to_map.get(0, 1) * y + view_to_map.get(0, 2), view_to_map.get(1, 0) * x + view_to_map.get(1, 1) * y + view_to_map.get(1, 2));
}

inline
MapCoord MapView::viewToMap(QPointF point)
{
	return viewToMap(point.x(), point.y());
}

inline
MapCoordF MapView::viewToMapF(double x, double y)
{
	return MapCoordF(view_to_map.get(0, 0) * x + view_to_map.get(0, 1) * y + view_to_map.get(0, 2), view_to_map.get(1, 0) * x + view_to_map.get(1, 1) * y + view_to_map.get(1, 2));
}

inline
MapCoordF MapView::viewToMapF(QPointF point)
{
	return viewToMapF(point.x(), point.y());
}

inline
void MapView::mapToView(MapCoord coords, double& out_x, double& out_y)
{
	out_x = map_to_view.get(0, 0) * coords.xd() + map_to_view.get(0, 1) * coords.yd() + map_to_view.get(0, 2);
	out_y = map_to_view.get(1, 0) * coords.xd() + map_to_view.get(1, 1) * coords.yd() + map_to_view.get(1, 2);
}

inline
void MapView::mapToView(MapCoord coords, float& out_x, float& out_y)
{
	out_x = map_to_view.get(0, 0) * coords.xd() + map_to_view.get(0, 1) * coords.yd() + map_to_view.get(0, 2);
	out_y = map_to_view.get(1, 0) * coords.xd() + map_to_view.get(1, 1) * coords.yd() + map_to_view.get(1, 2);
}

inline
QPointF MapView::mapToView(MapCoord coords)
{
	return QPointF(map_to_view.get(0, 0) * coords.xd() + map_to_view.get(0, 1) * coords.yd() + map_to_view.get(0, 2),
				   map_to_view.get(1, 0) * coords.xd() + map_to_view.get(1, 1) * coords.yd() + map_to_view.get(1, 2));
}

inline
QPointF MapView::mapToView(MapCoordF coords)
{
	return QPointF(map_to_view.get(0, 0) * coords.getX() + map_to_view.get(0, 1) * coords.getY() + map_to_view.get(0, 2),
				   map_to_view.get(1, 0) * coords.getX() + map_to_view.get(1, 1) * coords.getY() + map_to_view.get(1, 2));
}

inline
Map* MapView::getMap() const
{
	return map;
}

inline
float MapView::calculateFinalZoomFactor()
{
	return lengthToPixel(1000);
}

inline
float MapView::getZoom() const
{
	return zoom;
}

inline
float MapView::getRotation() const
{
	return zoom;
}

inline
void MapView::setRotation(float value)
{
	rotation = value;
	update();
}

inline
qint64 MapView::getPositionX() const
{
	return position_x;
}

inline
qint64 MapView::getPositionY() const
{
	return position_y;
}

inline
int MapView::getViewX() const
{
	return view_x;
}

inline
void MapView::setViewX(int value)
{
	view_x = value;
	update();
}

inline
int MapView::getViewY() const
{
	return view_y;
}

inline
void MapView::setViewY(int value)
{
	view_y = value;
	update();
}

inline
QPoint MapView::getDragOffset() const
{
	return drag_offset;
}

inline
bool MapView::areAllTemplatesHidden() const
{
	return all_templates_hidden;
}

inline
bool MapView::isGridVisible() const
{
	return grid_visible;
}

inline
void MapView::setGridVisible(bool visible)
{
	grid_visible = visible;
}

inline
bool MapView::isOverprintingSimulationEnabled() const
{
	return overprinting_simulation_enabled;
}

inline
void MapView::setOverprintingSimulationEnabled(bool enabled)
{
	overprinting_simulation_enabled = enabled;
}


#endif