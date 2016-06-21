/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2014-2016  Kai Pastor
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


#ifndef OPENORIENTEERING_MAP_VIEW_H
#define OPENORIENTEERING_MAP_VIEW_H

#include <QHash>
#include <QObject>
#include <QRectF>
#include <QTransform>

#include "../core/map_coord.h"

class Map;
class MapWidget;
class Template;
class TemplateVisibility;

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
class MapView : public QObject
{
public:
	enum ChangeFlag
	{
		NoChange       = 0,
		CenterChange   = 1,
		ZoomChange     = 2,
		RotationChange = 4,
	};
	
	Q_DECLARE_FLAGS(ChangeFlags, ChangeFlag)
	
	/** Creates a default view looking at the origin. */
	MapView(QObject* parent, Map* map);
	
	/** Creates a default view looking at the origin.
	 *
	 * The map takes ownership of the map view.
	 */
	MapView(Map* map);
	
	/** Destroys the map view. */
	~MapView() override;
	
	/** Loads the map view in the old "native" format from the file. */
	void load(QIODevice* file, int version);
	
	/** Saves the map view state to an XML stream.
	 *  @param xml The XML output stream.
	 *  @param element_name The name of the element which will be written. 
	 */
	void save(QXmlStreamWriter& xml, const QLatin1String& element_name) const;
	
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
	 * Redraws all map widgets completely.
	 * 
	 * Note that this calls QWidget::update() which does not cause an immediate
	 * repaint; instead it schedules a paint event.
	 * 
	 * Completely repainting widgets can be slow.
	 * Try to do partial updates instead, if possible.
	 */
	void updateAllMapWidgets();
	
	
	/** Converts x, y (with origin at the center of the view) to map coordinates */
	MapCoord viewToMap(double x, double y) const;
	
	/** Converts the point (with origin at the center of the view) to map coordinates */
	MapCoord viewToMap(QPointF point) const;
	
	/** Converts x, y (with origin at the center of the view) to map coordinates */
	MapCoordF viewToMapF(double x, double y) const;
	
	/** Converts the point (with origin at the center of the view) to map coordinates */
	MapCoordF viewToMapF(QPointF point) const;
	
	
	/// Converts map coordinates to view coordinates (with origin at the center of the view)
	QPointF mapToView(MapCoord coords) const;
	
	/// Converts map coordinates to view coordinates (with origin at the center of the view)
	QPointF mapToView(MapCoordF coords) const;
	
	
	/**
	 * Converts a length from native map coordinates to the current length in view pixels.
	 */
	qreal lengthToPixel(qreal length) const;
	
	/**
	 * Converts a length from current view pixels to native map coordinates.
	 */
	qreal pixelToLength(qreal pixel) const;
	
	
	/**
	 * Calculates the bounding box of the map coordinates which can be viewed
	 * using the given view coordinates rect
	 */
	QRectF calculateViewedRect(QRectF view_rect) const;
	
	/**
	 * Calculates the bounding box in view coordinates
	 * of the given map coordinates rect
	 */
	QRectF calculateViewBoundingBox(QRectF map_rect) const;
	
	/**
	 * Returns a QTransform suitable for QPainter, so objects defined in
	 * map coordinates will be drawn at their view coordinates. Append a
	 * viewport transformation to this to get a complete map-to-viewport transformation
	 * which makes the view center appear at the viewport center.
	 * 
	 * Note: The transform is to be combined with the painter's existing transform.
	 */
	const QTransform& worldTransform() const;
	
	
	// Panning
	
	/** Returns the current pan offset (when dragging the map). */
	QPoint panOffset() const;
	
	/** Sets the current pan offset while the map is being dragged. */
	void setPanOffset(QPoint offset);
	
	/**
	 * Finishes panning the map.
	 * 
	 * @param offset The final offset, relative to the start of the operation.
	 */
	void finishPanning(QPoint offset);
	
	
	/** Returns the map this view is defined on. */
	Map* getMap() const;
	
	
	/**
	 * Zooms the maps (in steps).
	 * 
	 * @param num_steps Number of zoom steps to zoom in. Can be negative to zoom out.
	 * @param preserve_cursor_pos If set, shifts the center of the view so the
	 *     cursor position will stay at the same map coordinate.
	 * @param cursor_pos_view The cursor position in view coordinates, must be
	 *     set if preserve_cursor_pos is used.
	 */
	void zoomSteps(float num_steps, bool preserve_cursor_pos, QPointF cursor_pos_view = QPointF());
	
	/**
	 * Returns the final zoom factor for use in transformations.
	 * Depends on the pixel per millimeter of the display.
	 */
	double calculateFinalZoomFactor() const;
	
	/** Returns the raw zoom facor, see also calculateFinalZoomFactor(). */
	double getZoom() const;
	
	/** Sets the zoom factor relative to the given point.*/
	void setZoom(double value, QPointF center);
	
	/** Sets the zoom factor. */
	void setZoom(double value);
	
	
	/** Returns the view rotation (in radians). */
	float getRotation() const;
	
	/** Sets the view roation (in radians). */
	void setRotation(float value);
	
	
	/** Returns the position of the view center. */
	MapCoord center() const;
	
	/** Sets the position of the view center. */
	void setCenter(MapCoord pos);
	
	
	// Map and template visibilities
	
	/** Returns the effectiv visibility settings of the map drawing.
	 *
	 * Other than getMapVisibility, this will always return an (100 %) opaque,
	 * visible configuration when areAllTemplatesHidden() is true,
	 * and it return an invisible configuration when the map's opacity is
	 * below 0.005.
	 */
	const TemplateVisibility* effectiveMapVisibility() const;
	
	/** Returns the visibility settings of the map drawing. */
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
	const TemplateVisibility* getTemplateVisibility(const Template* temp) const;
	
	/**
	 * Returns the template visibility object, creates one if not there yet
	 * with the default settings (invisible)
	 */
	TemplateVisibility* getTemplateVisibility(const Template* temp);
	
	/**
	 * Call this when a template is deleted to destroy the template visibility object
	 */
	void deleteTemplateVisibility(const Template* temp);
	
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
	Q_DISABLE_COPY(MapView)
	
	typedef std::vector<MapWidget*> WidgetVector;
	
	void updateTransform(MapView::ChangeFlags change);		// recalculates the x_to_y matrices
	
	Map* map;
	
	double zoom;		// factor
	double rotation;	// counterclockwise 0 to 2*PI. This is the viewer rotation, so the map is rotated clockwise
	MapCoord center_pos;// position of the viewer, positive values move the map to the left; the position is in 1/1000 mm
	QPoint pan_offset;	// the distance the content of the view was dragged with the mouse, in pixels
	
	QTransform view_to_map;
	QTransform map_to_view;
	QTransform world_transform;
	
	TemplateVisibility* map_visibility;
	QHash<const Template*, TemplateVisibility*> template_visibilities;
	WidgetVector widgets;
	
	bool all_templates_hidden;
	bool grid_visible;
	bool overprinting_simulation_enabled;
};



/**
 * Contains the visibility information for a template (or the map).
 */
class TemplateVisibility
{
public:
	/** Opacity from 0.0f (invisible) to 1.0f (opaque) */
	float opacity;
	
	/** Visibility flag */
	bool visible;
};



// ### MapView inline code ###

Q_DECLARE_OPERATORS_FOR_FLAGS(MapView::ChangeFlags)

inline
MapCoord MapView::viewToMap(QPointF point) const
{
	return viewToMap(point.x(), point.y());
}

inline
MapCoordF MapView::viewToMapF(QPointF point) const
{
	return viewToMapF(point.x(), point.y());
}

inline
const QTransform& MapView::worldTransform() const
{
	return world_transform;
}

inline
Map* MapView::getMap() const
{
	return map;
}

inline
double MapView::calculateFinalZoomFactor() const
{
	return lengthToPixel(1000.0);
}

inline
double MapView::getZoom() const
{
	return zoom;
}

inline
float MapView::getRotation() const
{
	return rotation;
}

inline
MapCoord MapView::center() const
{
	return center_pos;
}

inline
QPoint MapView::panOffset() const
{
	return pan_offset;
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
bool MapView::isOverprintingSimulationEnabled() const
{
	return overprinting_simulation_enabled;
}



#endif
