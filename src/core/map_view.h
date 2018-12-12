/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2014-2018  Kai Pastor
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

#include <vector>

#include <QtGlobal>
#include <QFlags>
#include <QObject>
#include <QPoint>
#include <QPointF>
#include <QRectF>
#include <QTransform>

#include "map_coord.h"

class QLatin1String;
class QRectF;
class QXmlStreamReader;
class QXmlStreamWriter;

namespace OpenOrienteering {

class Map;
class Template;


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
	
	/** Returns true when the template is visible but not opaque. */
	bool hasAlpha() const;
};

bool operator==(TemplateVisibility lhs, TemplateVisibility rhs);

bool operator!=(TemplateVisibility lhs, TemplateVisibility rhs);


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
	Q_OBJECT
	Q_FLAGS(ChangeFlags  VisibilityFeature)
	
public:
	enum ChangeFlag
	{
		NoChange       = 0,
		CenterChange   = 1,
		ZoomChange     = 2,
		RotationChange = 4,
	};
	
	Q_DECLARE_FLAGS(ChangeFlags, ChangeFlag)
	
	enum VisibilityFeature
	{
		MultipleFeatures    =  0,
		GridVisible         =  1,
		OverprintingEnabled =  2,
		AllTemplatesHidden  =  4,
		TemplateVisible     =  8,
		MapVisible          = 16,
	};
	
	
	/**
	 * Creates a default view looking at the origin.
	 * 
	 * The parameter map must not be null.
	 */
	MapView(QObject* parent, Map* map);
	
	/** Creates a default view looking at the origin.
	 *
	 * The map takes ownership of the map view. It must not be null.
	 */
	MapView(Map* map);
	
	/** Destroys the map view. */
	~MapView() override;
	
	
	/**
	 * Saves the map view state to an XML stream.
	 * @param xml The XML output stream.
	 * @param element_name The name of the element which will be written.
	 * @param template_details Save template visibilities (default: true)
	 */
	void save(QXmlStreamWriter& xml, const QLatin1String& element_name, bool template_details = true) const;
	
	/** Loads the map view state from the current element of an xml stream. */
	void load(QXmlStreamReader& xml);
	
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
	
	
	/** Converts the point (with origin at the center of the view) to map coordinates */
	MapCoord viewToMap(const QPointF& point) const;
	
	/** Converts the point (with origin at the center of the view) to map coordinates */
	MapCoordF viewToMapF(const QPointF& point) const;
	
	
	/// Converts map coordinates to view coordinates (with origin at the center of the view)
	QPointF mapToView(const MapCoord& coords) const;
	
	/// Converts map coordinates to view coordinates (with origin at the center of the view)
	QPointF mapToView(const QPointF& coords) const;
	
	
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
	void setPanOffset(const QPoint& offset);
	
	/**
	 * Finishes panning the map.
	 * 
	 * @param offset The final offset, relative to the start of the operation.
	 */
	void finishPanning(const QPoint& offset);
	
	
	/** Returns the map this view is defined on. */
	const Map* getMap() const;
	
	/** Returns the map this view is defined on. */
	Map* getMap();
	
	
	/**
	 * Zooms the maps (in steps), preserving the given cursor position.
	 * 
	 * @param num_steps Number of zoom steps to zoom in. Negative numbers zoom out.
	 * @param cursor_pos_view The cursor position in view coordinates, must be
	 *     set if preserve_cursor_pos is used.
	 */
	void zoomSteps(double num_steps, const QPointF& cursor_pos_view);
	
	/**
	 * Zooms the maps (in steps), preserving the center of the view.
	 * 
	 * @param num_steps Number of zoom steps to zoom in. Negative numbers zoom out.
	 */
	void zoomSteps(double num_steps);
	
	/**
	 * Returns the final zoom factor for use in transformations.
	 * Depends on the pixel per millimeter of the display.
	 */
	double calculateFinalZoomFactor() const;
	
	/** Returns the raw zoom facor, see also calculateFinalZoomFactor(). */
	double getZoom() const;
	
	/** Sets the zoom factor relative to the given point.*/
	void setZoom(double value, const QPointF& center);
	
	/** Sets the zoom factor. */
	void setZoom(double value);
	
	
	/** Returns the view rotation (in radians). */
	double getRotation() const;
	
	/** Sets the view roation (in radians). */
	void setRotation(double value);
	
	
	/** Returns the position of the view center. */
	MapCoord center() const;
	
	/** Sets the position of the view center. */
	void setCenter(const MapCoord& pos);
	
	
	// Map and template visibilities
	
	/** Returns the effectiv visibility settings of the map drawing.
	 *
	 * Other than getMapVisibility, this will always return an (100 %) opaque,
	 * visible configuration when areAllTemplatesHidden() is true,
	 * and it return an invisible configuration when the map's opacity is
	 * below 0.005.
	 */
	TemplateVisibility effectiveMapVisibility() const;
	
	/** Returns the visibility settings of the map drawing. */
	TemplateVisibility getMapVisibility() const;
	
	void setMapVisibility(TemplateVisibility vis);
	
	/**
	 * Checks if the template is visible without creating
	 * a template visibility object if none exists
	 */
	bool isTemplateVisible(const Template* temp) const;
	
	/**
	 * Returns the template visibility.
	 * 
	 * If the template is unknown, returns default settings.
	 */
	TemplateVisibility getTemplateVisibility(const Template* temp) const;
	
	/**
	 * Sets the template visibility, and emits a change signal.
	 */
	void setTemplateVisibility(Template* temp, TemplateVisibility vis);
	
	
	/** Enables or disables hiding all templates in this view */
	void setAllTemplatesHidden(bool value);
	
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
	
	
	/**
	 * Returns true if any of the visible elements is not opaque.
	 */
	bool hasAlpha() const;
	
	
	/** Temporarily blocks automatic template loading on visibility changes. */
	void setTemplateLoadingBlocked(bool blocked);
	
	/** Returns true when template loading on visibility changes is disabled. */
	bool templateLoadingBlocked() const { return template_loading_blocked; }
	
	
signals:
	/**
	 * Indicates a change of the viewed area of the map.
	 * 
	 * @param change The aspects that have chaneged.
	 */
	void viewChanged(ChangeFlags change);
	
	/**
	 * Indicates a change of the pan offset.
	 */
	void panOffsetChanged(const QPoint& offset);
	
	/**
	 * Indicates a particular change of visibility.
	 * 
	 * @param feature The map view feature that has changed.
	 * @param active  The features current state of activation.
	 * @param temp    If a the feature is a template, a pointer to this template.
	 */
	void visibilityChanged(VisibilityFeature feature, bool active, const Template* temp = nullptr);
	
	
public:
	// Static
	
	/** The global zoom in limit for the zoom factor. */
	static const double zoom_in_limit;
	/** The global zoom out limit for the zoom factor. */
	static const double zoom_out_limit;
	
protected:
	/**
	 * Sets the template visibility without emitting signals.
	 */
	bool setTemplateVisibilityHelper(const Template *temp, TemplateVisibility vis);
	
	/**
	 * Creates the visibility data when a template is added to the map.
	 */
	void onTemplateAdded(int pos, Template* temp);
	
	/**
	 * Removes the visibility data when a template is deleted.
	 */
	void onTemplateDeleted(int pos, const Template* temp);
	
private:
	Q_DISABLE_COPY(MapView)
	
	struct TemplateVisibilityEntry : public TemplateVisibility
	{
		const Template* temp;
		TemplateVisibilityEntry() = default;
		TemplateVisibilityEntry(const TemplateVisibilityEntry&) = default;
		TemplateVisibilityEntry(TemplateVisibilityEntry&&) = default;
		TemplateVisibilityEntry& operator=(const TemplateVisibilityEntry&) = default;
		TemplateVisibilityEntry& operator=(TemplateVisibilityEntry&&) = default;
		TemplateVisibilityEntry(const Template* temp, TemplateVisibility vis)
		: TemplateVisibility(vis)
		, temp(temp)
		{}
	};
	typedef std::vector<TemplateVisibilityEntry> TemplateVisibilityVector;
	
	
	void updateTransform();		// recalculates the x_to_y matrices
	
	TemplateVisibilityVector::const_iterator findVisibility(const Template* temp) const;
	
	TemplateVisibilityVector::iterator findVisibility(const Template* temp);
	
	
	Map* map;
	
	double zoom;		// factor
	double rotation;	// counterclockwise 0 to 2*PI. This is the viewer rotation, so the map is rotated clockwise
	MapCoord center_pos;// position of the viewer, positive values move the map to the left; the position is in 1/1000 mm
	QPoint pan_offset;	// the distance the content of the view was dragged with the mouse, in pixels
	
	QTransform view_to_map;
	QTransform map_to_view;
	
	TemplateVisibility map_visibility;
	TemplateVisibilityVector template_visibilities;
	
	bool all_templates_hidden;
	bool grid_visible;
	bool overprinting_simulation_enabled;
	
	bool template_loading_blocked;
};



// ### TemplateVisibility inline code ###

inline
bool operator==(TemplateVisibility lhs, TemplateVisibility rhs)
{
	return lhs.visible == rhs.visible
	       && qFuzzyCompare(1.0f+rhs.opacity, 1.0f+lhs.opacity);
}

inline
bool operator!=(TemplateVisibility lhs, TemplateVisibility rhs)
{
	return !(lhs == rhs);
}



// ### MapView inline code ###

inline
MapCoord MapView::viewToMap(const QPointF& point) const
{
	return MapCoord(view_to_map.map(point));
}

inline
MapCoordF MapView::viewToMapF(const QPointF& point) const
{
	return MapCoordF(view_to_map.map(point));
}

inline
const QTransform& MapView::worldTransform() const
{
	return map_to_view;
}

inline
Map* MapView::getMap()
{
	return map;
}

inline
const Map* MapView::getMap() const
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
double MapView::getRotation() const
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
TemplateVisibility MapView::getMapVisibility() const
{
	return map_visibility;
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


}  // namespace OpenOrienteering


Q_DECLARE_OPERATORS_FOR_FLAGS(OpenOrienteering::MapView::ChangeFlags)


#endif
