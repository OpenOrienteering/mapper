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


#ifndef _OPENORIENTEERING_MAP_WIDGET_H_
#define _OPENORIENTEERING_MAP_WIDGET_H_

#include <QWidget>

#include "core/map_view.h"
#include "map.h"
#include "util_pie_menu.h"

class QLabel;

class MapEditorActivity;
class MapEditorTool;
class MapView;
class TouchCursor;

/**
 * QWidget for displaying a map. Needs a pointer to a MapView which defines
 * the view properties.
 * 
 * For faster display, the widget keeps some cached image internally which
 * are of the same size as the widget area. If then for example the map changes,
 * the other caches do not need to be redrawn.
 * <ul>
 * <li>The <b>map cache</b> contains the currently visible part of the map</li>
 * <li>The <b>below template cache</b> contains the currently
 *     visible part of all templates below the map</li>
 * <li>The <b>above template cache</b> contains the currently
 *     visible part of all templates above the map</li>
 * </ul>
 */
class MapWidget : public QWidget
{
Q_OBJECT
friend class MapView;
public:
	/** Describes different display formats for coordinates. */
	enum CoordsType
	{
		/** Map coordinates: millimeters on map paper */
		MAP_COORDS,
		/** Projected coordinates, e.g. UTM */
		PROJECTED_COORDS,
		/** Geographic WGS84 coordinates */
		GEOGRAPHIC_COORDS,
		/** Geographic WGS84 coordinates in degrees, minutes, seconds */
		GEOGRAPHIC_COORDS_DMS
	};
	
	/**
	 * Constructs a new MapWidget.
	 * 
	 * @param show_help If set to true, the map widget shows help texts for
	 *     empty maps.
	 * @param force_antialiasing If set to true, the map widget uses antialiasing
	 *     for display, even if it is disabled in the program settings.
	 *     Useful for the symbol editor.
	 * @param parent Optional QWidget parent.
	 */
	MapWidget(bool show_help, bool force_antialiasing, QWidget* parent = NULL);
	
	/** Destructs the MapWidget. */
	~MapWidget();
	
	/** Sets the map view to use for display. Does not take ownership of the view. */
	void setMapView(MapView* view);
	
	/** Returns the map view used for display. */
	inline MapView* getMapView() const {return view;}
	
	
	/** Sets the tool to use in this widget. Does not take ownership of the tool. */
	void setTool(MapEditorTool* tool);
	
	/** Sets the activity to use in this widget. Does not take ownership of the activity. */
	void setActivity(MapEditorActivity* activity);
	
	/**
	 * Applies the complete transform to the painter which enables to draw
	 * map objects with map coordinates and have them correctly displayed in
	 * the widget with the settings of the used MapView.
	 */
	void applyMapTransform(QPainter* painter);
	
	
	// Coordinate transformations
	
	/** Maps viewport (GUI) coordinates to view coordinates (see MapView). */
	QRectF viewportToView(const QRect& input);
	/** Maps viewport (GUI) coordinates to view coordinates (see MapView). */
	QPointF viewportToView(QPoint input);
	/** Maps viewport (GUI) coordinates to view coordinates (see MapView). */
	QPointF viewportToView(QPointF input);
	/** Maps view coordinates (see MapView) to viewport (GUI) coordinates. */
	QRectF viewToViewport(const QRectF& input);
	/** Maps view coordinates (see MapView) to viewport (GUI) coordinates. */
	QRectF viewToViewport(const QRect& input);
	/** Maps view coordinates (see MapView) to viewport (GUI) coordinates. */
	QPointF viewToViewport(QPoint input);
	/** Maps view coordinates (see MapView) to viewport (GUI) coordinates. */
	QPointF viewToViewport(QPointF input);
	
	/** Maps viewport (GUI) coordinates to map coordinates. */
	MapCoord viewportToMap(QPoint input);
	/** Maps viewport (GUI) coordinates to map coordinates. */
	MapCoordF viewportToMapF(QPoint input);
	/** Maps viewport (GUI) coordinates to map coordinates. */
	MapCoordF viewportToMapF(QPointF input);
	/** Maps map coordinates to viewport (GUI) coordinates. */
	QPointF mapToViewport(MapCoord input);
	/** Maps map coordinates to viewport (GUI) coordinates. */
	QPointF mapToViewport(MapCoordF input);
	/** Maps map coordinates to viewport (GUI) coordinates. */
	QPointF mapToViewport(QPointF input);
	/** Maps map coordinates to viewport (GUI) coordinates. */
	QRectF mapToViewport(const QRectF& input);
	
	// View changes
	
	/**
	 * Notifies the MapWidget of the view having zoomed,
	 * making it repaint its widget area.
	 * 
	 * @param factor The ratio new_zoom / old_zoom
	 */
	void zoom(float factor);
	
	/**
	 * Notifies the MapWidget of the view having moved.
	 * Internally, just redraws the complete widget area.
	 * 
	 * @param x X offset of the view change in native map coordinates
	 * @param y Y offset of the view change in native map coordinates
	 */
	void moveView(qint64 x, qint64 y);
	
	/**
	 * Notifies the MapWidget of the user having finished panning the map.
	 * Internally, decides whether to do a complete or partial redraw.
	 * @param x X offset of the total view change in native map coordinates
	 * @param y Y offset of the total view change in native map coordinates
	 */
	void panView(qint64 x, qint64 y);
	
	/** Sets the current drag offset during a map pan operation. */
	void setDragOffset(QPoint offset);
	
	/** Returns the current drag offset during a map pan operation. */
	QPoint getDragOffset() const;
	
	/**
	 * Completes a map panning operation. Calls panView() internally and
	 * resets the current drag offset.
	 * @param x X offset of the total view change in native map coordinates
	 * @param y Y offset of the total view change in native map coordinates
	 */
	void completeDragging(qint64 dx, qint64 dy);
	
	/**
	 * Adjusts the viewport so the given rect is inside the view.
	 * @param show_completely If true, the method ensures that 100% of the rect
	 *     is visible. If false, a weaker definition of "visible" is used:
	 *     then a certain area of the rect must be visible.
	 * @param zoom_in_steps If true, zoom is done in the usual power-of-two
	 *     steps only. If false, the zoom level is chosen to fit the rect.
	 */
	void ensureVisibilityOfRect(const QRectF& map_rect, bool show_completely, bool zoom_in_steps);
	
	/**
	 * Sets the view so the rect is centered and zooomed to fill the widget.
	 * @param zoom_in_steps If true, zoom is done in the usual power-of-two
	 *     steps only. If false, the zoom level is chosen to fit the rect.
	 */
	void adjustViewToRect(const QRectF& map_rect, bool zoom_in_steps);
	
	/**
	 * Mark a rectangular region of a template cache as "dirty", i.e. redraw needed.
	 * This rect is united with possible previous dirty rects of that cache.
	 * @param view_rect Affected rect in view coordinates.
	 * @param pixel_border Additional affected extent around the view rect in
	 *     pixels. Allows to specify zoom-independent extents.
	 * @param front_cache If set to true, invalidates the cache for templates
	 *     in front of the map, else invalidates the cache for templates behind the map.
	 */
	void markTemplateCacheDirty(QRectF view_rect, int pixel_border, bool front_cache);
	
	/**
	 * Mark a rectangular region given in map coordinates of the map cache
	 * as dirty, i.e. redraw needed.
	 * This rect is united with possible previous dirty rects of that cache.
	 */
	void markObjectAreaDirty(QRectF map_rect);
	
	/**
	 * Set the given rect as bounding box for the current drawing, i.e. the
	 * graphical display of the active tool.
	 * NOTE: Unlike for markTemplateCacheDirty(), multiple calls to
	 * these methodsdo not result in uniting all given rects,
	 * instead only the last rect is used!
	 * Pass QRect() to disable the current drawing.
	 * @param map_rect Affected rect in map coordinates.
	 * @param pixel_border Additional affected extent around the map rect in
	 *     pixels. Allows to specify zoom-independent extents.
	 * @param do_update If set to true, triggers a redraw of the widget.
	 */
	void setDrawingBoundingBox(QRectF map_rect, int pixel_border, bool do_update);
	/**
	 * Removes the area set with setDrawingBoundingBox() and triggers a redraw
	 * of the widget, if needed.
	 */
	void clearDrawingBoundingBox();
	
	/** Analogon to setDrawingBoundingBox() for activities. */
	void setActivityBoundingBox(QRectF map_rect, int pixel_border, bool do_update);
	/** Analogon to clearDrawingBoundingBox() for activities. */
	void clearActivityBoundingBox();
	
	/**
	 * Triggers a redraw of the MapWidget at the given area.
	 * @param map_rect Affected rect in map coordinates.
	 * @param pixel_border Additional affected extent around the map rect in
	 *     pixels. Allows to specify zoom-independent extents.
	 */
	void updateDrawing(QRectF map_rect, int pixel_border);
	
	/**
	 * Invalidates all caches and redraws the whole widget. Very slow, try to
	 * avoid this.
	 */
	void updateEverything();
	/**
	 * Sets all "dirty" region markers to the given rect in viewport coordinates
	 * and triggers a redraw of the MapWidget there.
	 */
	void updateEverythingInRect(const QRect& dirty_rect);
	
	/** Specify the label where the MapWidget will display zoom information. */
	void setZoomLabel(QLabel* zoom_label);
	/** Specify the label where the MapWidget will display cursor position information. */
	void setCursorposLabel(QLabel* cursorpos_label);
	/** Specify the label where the MapWidget will display object tag information. */
	void setObjectTagLabel(QLabel* objecttag_label);
	/**
	 * Specify the system and format for displaying coordinates in
	 * the cursorpos label. See CoordsType for the available types.
	 */
	void setCoordsDisplay(CoordsType type);
	/** Returns the coordinate display type set by setCoordsDisplay(). */
	inline CoordsType getCoordsDisplay() const {return coords_type;}
	
	/** Returns a reference to the internally constructed PieMenu. */
	inline PieMenu& getPieMenu() {return pie_menu;}
	
	/** Returns the widget's preferred size. */
	virtual QSize sizeHint() const;
	
public slots:
	/** Delegates the keyPress to the active tool, or handles some shortcuts itself. */
	void keyPressed(QKeyEvent* event);
	/** Delegates the keyRelease to the active tool */
	void keyReleased(QKeyEvent* event);

private slots:
	void updateObjectTagLabel();
	
protected:
	virtual void paintEvent(QPaintEvent* event);
	virtual void resizeEvent(QResizeEvent* event);
	
	// Mouse input
	virtual void mousePressEvent(QMouseEvent* event);
	void _mousePressEvent(QMouseEvent* event);
	virtual void mouseMoveEvent(QMouseEvent* event);
	void _mouseMoveEvent(QMouseEvent* event);
	virtual void mouseReleaseEvent(QMouseEvent* event);
	void _mouseReleaseEvent(QMouseEvent* event);
	virtual void mouseDoubleClickEvent(QMouseEvent* event);
	void _mouseDoubleClickEvent(QMouseEvent* event);
	virtual void wheelEvent(QWheelEvent* event);
	virtual void leaveEvent(QEvent* event);
	
	// Key input (see also slots)
	virtual void focusOutEvent(QFocusEvent* event);
	
private:
	/** Checks if there is a visible template in the range
	 *  from first_template to last_template. */
	bool containsVisibleTemplate(int first_template, int last_template);
	/** Checks if there is any visible template above the map. */
	inline bool isAboveTemplateVisible() {return containsVisibleTemplate(view->getMap()->getFirstFrontTemplate(), view->getMap()->getNumTemplates() - 1);}
	/** Checks if there is any visible template below the map. */
	inline bool isBelowTemplateVisible() {return containsVisibleTemplate(0, view->getMap()->getFirstFrontTemplate() - 1);}
	/**
	 * Redraws the template cache.
	 * @param cache Reference to pointer to the cache.
	 * @param dirty_rect Rectangle of the cache to redraw, in viewport coordinates.
	 * @param first_template Lowest template index to draw.
	 * @param last_template Highest template index to draw.
	 * @param use_background If set to true, fills the cache with white before
	 *     drawing the templates, else makes it transparent.
	 */
	void updateTemplateCache(QImage*& cache, QRect& dirty_rect, int first_template, int last_template, bool use_background);
	/**
	 * Redraws the map cache in the map cache dirty rect.
	 * @param use_background If set to true, fills the cache with white before
	 *     drawing the map, else makes it transparent.
	 */
	void updateMapCache(bool use_background);
	/** Redraws all dirty caches. */
	void updateAllDirtyCaches();
	/** Shifts the content in the cache by the given amount of pixels. */
	void shiftCache(int sx, int sy, QImage*& cache);
	
	/**
	 * Calculates the bounding box of the given map coordinates rect and
	 * additional pixel extent in integer viewport coordinates.
	 */
	QRect calculateViewportBoundingBox(QRectF map_rect, int pixel_border);
	/** Internal method for setting a part of a cache as dirty. */
	void setDynamicBoundingBox(QRectF map_rect, int pixel_border, QRect& dirty_rect_old, QRectF& dirty_rect_new, int& dirty_rect_new_border, bool do_update);
	/** Internal method for removing the dirty state of a cache. */
	void clearDynamicBoundingBox(QRect& dirty_rect_old, QRectF& dirty_rect_new, int& dirty_rect_new_border);
	
	/** Changes the dirty rect's coordinates as a zoom operation would do. */
	void zoomDirtyRect(QRect& dirty_rect, qreal zoom_factor);
	/** Changes the dirty rect's coordinates as a zoom operation would do. */
	void zoomDirtyRect(QRectF& dirty_rect, qreal zoom_factor);
	/** Moves the dirty rect by the given amount of pixels. */
	void moveDirtyRect(QRect& dirty_rect, qreal x, qreal y);
	/** Moves the dirty rect by the given amount of pixels. */
	void moveDirtyRect(QRectF& dirty_rect, qreal x, qreal y);
	
	/** Starts panning the map, given the current cursor position on the widget. */
	void startPanning(QPoint cursor_pos);
	/** Ends panning the map, given the current cursor position on the widget. */
	void finishPanning(QPoint cursor_pos);
	/** Moves the map a given number of big "steps" in x and/or y direction. */
	void moveMap(int steps_x, int steps_y);
	
	/** Draws a help message at the center of the MapWidget. */
	void showHelpMessage(QPainter* painter, const QString& text);
	
	/** Updates the content of the zoom label, set by setZoomLabel(). */
	void updateZoomLabel();
	/** Updates the content of the cursorpos label, set by setCursorposLabel(). */
	void updateCursorposLabel(const MapCoordF pos);
	/** Updates the content of the object tag label, set by setObjectTagLabel(). */
	void updateObjectTagLabel(const MapCoordF pos);
	
	MapView* view;
	MapEditorTool* tool;
	MapEditorActivity* activity;
	
	CoordsType coords_type;
	
	QLabel* zoom_label;
	QLabel* cursorpos_label;
	QLabel* objecttag_label;
	MapCoordF last_cursor_pos;
	
	bool show_help;
	bool force_antialiasing;
	
	// Panning
	bool dragging;
	QPoint drag_start_pos;
	QPoint drag_offset;
	/** Cursor used when not dragging */
	QCursor normal_cursor;
	
	// Template caches
	/** Cache for templates below map layer */
	QImage* below_template_cache;
	QRect below_template_cache_dirty_rect;
	/** Cache for templates above map layer */
	QImage* above_template_cache;
	QRect above_template_cache_dirty_rect;
	
	// Map cache
	QImage* map_cache;
	QRect map_cache_dirty_rect;
	
	// Dirty regions for drawings (tools) and activities
	/**
	 * Dirty rect for dynamic display which has been drawn
	 * (and will have to be erased by the next draw operation)
	 */
	QRect drawing_dirty_rect_old;
	/**
	 * Dirty rect for dynamic display which has maybe not been drawn yet,
	 * but must be drawn by the next draw operation
	 */
	QRectF drawing_dirty_rect_new;
	int drawing_dirty_rect_new_border;
	
	QRect activity_dirty_rect_old;
	QRectF activity_dirty_rect_new;
	int activity_dirty_rect_new_border;
	
	/** Right-click menu */
	PieMenu pie_menu;
	
	/** Optional touch cursor for mobile devices */
	TouchCursor* touch_cursor;

#if defined(Q_OS_ANDROID)
	/** Click state to compensate for input quirks */
	int clickState;
#endif
};

#endif
