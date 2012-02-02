/*
 *    Copyright 2012 Thomas Schöps
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

#include "map.h"

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE

class MapEditorActivity;
class MapEditorTool;
class MapView;

class MapWidget : public QWidget
{
Q_OBJECT
friend class MapView;
public:
	MapWidget(bool show_help, bool use_antialiasing, QWidget* parent = NULL);
	~MapWidget();
	
	void setMapView(MapView* view);
	inline MapView* getMapView() const {return view;}
	
	void setTool(MapEditorTool* tool);
	void setActivity(MapEditorActivity* activity);
	
	/// Map viewport (GUI) coordinates to view coordinates or the other way round
	QRectF viewportToView(const QRect& input);
	QPointF viewportToView(QPoint input);
	QRectF viewToViewport(const QRectF& input);
	QRectF viewToViewport(const QRect& input);
	QPointF viewToViewport(QPoint input);
	QPointF viewToViewport(QPointF input);
	
	/// Map map coordinates to viewport (GUI) coordinates or the other way round
	MapCoord viewportToMap(QPoint input);
	MapCoordF viewportToMapF(QPoint input);
	QPointF mapToViewport(MapCoord input);
	QPointF mapToViewport(MapCoordF input);
	
	/// View changes
	void zoom(float factor);	// factor is the ratio of new_zoom / old_zoom
	void moveView(qint64 x, qint64 y);
	
	void setDragOffset(QPoint offset);
	void completeDragging(QPoint offset, qint64 dx, qint64 dy);
	
	/// Mark a rectangular region of a template cache as dirty. This rect is united with possible previous dirty rects of that cache.
	void markTemplateCacheDirty(QRectF view_rect, int pixel_border, bool front_cache);
	
	/// Mark a rectangular region of the map cache as dirty. This rect is united with possible previous dirty rects of that cache.
	void markObjectAreaDirty(QRectF map_rect);
	
	/// Set the given rect as bounding box for the current drawing.
	/// NOTE: Unlike with markTemplateCacheDirty(), multiple calls to these methods do not result in uniting all given rects, instead only the last rect is used!
	/// Pass QRect() to disable the current drawing.
	void setDrawingBoundingBox(QRectF map_rect, int pixel_border, bool do_update);
	void clearDrawingBoundingBox();
	
	void setActivityBoundingBox(QRectF map_rect, int pixel_border, bool do_update);
	void clearActivityBoundingBox();
	
	void updateDrawing(QRectF map_rect, int pixel_border);
	void updateEverything();	// invalidates all caches and redraws the whole widget
	
	// Set the labels where the map widget will display the respective piece of information
	void setZoomLabel(QLabel* zoom_label);
	void setCursorposLabel(QLabel* cursorpos_label);
	
    virtual QSize sizeHint() const;
	
public slots:
	void keyPressed(QKeyEvent* event);
	void keyReleased(QKeyEvent* event);
	
protected:
	virtual void paintEvent(QPaintEvent* event);
	virtual void resizeEvent(QResizeEvent* event);
	
	// Mouse input
	virtual void mousePressEvent(QMouseEvent* event);
	virtual void mouseMoveEvent(QMouseEvent* event);
	virtual void mouseReleaseEvent(QMouseEvent* event);
	virtual void mouseDoubleClickEvent(QMouseEvent* event);
	virtual void wheelEvent(QWheelEvent* event);
    virtual void leaveEvent(QEvent* event);
	
	// Key input (see also slots)
    virtual void focusOutEvent(QFocusEvent* event);
	
private:
	bool containsVisibleTemplate(int first_template, int last_template);
	void updateTemplateCache(QImage*& cache, QRect& dirty_rect, int first_template, int last_template, bool use_background);
	void updateMapCache(bool use_background);
	
	QRect calculateViewportBoundingBox(QRectF map_rect, int pixel_border);
	void setDynamicBoundingBox(QRectF map_rect, int pixel_border, QRect& dirty_rect_old, QRectF& dirty_rect_new, int& dirty_rect_new_border, bool do_update);
	void clearDynamicBoundingBox(QRect& dirty_rect_old, QRectF& dirty_rect_new, int& dirty_rect_new_border);
	
	void zoomDirtyRect(QRect& dirty_rect, qreal zoom_factor);
	void zoomDirtyRect(QRectF& dirty_rect, qreal zoom_factor);
	void moveDirtyRect(QRect& dirty_rect, qreal x, qreal y);
	void moveDirtyRect(QRectF& dirty_rect, qreal x, qreal y);
	
	void showHelpMessage(QPainter* painter, const QString& text);
	
	void updateZoomLabel();
	void updateCursorposLabel(MapCoordF pos);
	
	MapView* view;
	MapEditorTool* tool;
	MapEditorActivity* activity;
	
	QLabel* zoom_label;
	QLabel* cursorpos_label;
	
	bool show_help;
	bool use_antialiasing;
	
	// Panning
	bool dragging;
	QPoint drag_start_pos;
	QPoint drag_offset;
	QCursor normal_cursor;	// cursor used when not dragging
	
	// Template caches
	QImage* below_template_cache;			// cache for templates below map layer
	QRect below_template_cache_dirty_rect;
	QImage* above_template_cache;			// cache for templates above map layer
	QRect above_template_cache_dirty_rect;
	
	// Map cache
	QImage* map_cache;
	QRect map_cache_dirty_rect;
	
	// Dirty regions for drawings (tools) and activities
	QRect drawing_dirty_rect_old;			// dirty rect for dynamic display which has been drawn (and will have to be erased by the next draw operation)
	QRectF drawing_dirty_rect_new;			// dirty rect for dynamic display which has maybe not been drawn yet, but must be drawn by the next draw operation
	int drawing_dirty_rect_new_border;
	
	QRect activity_dirty_rect_old;
	QRectF activity_dirty_rect_new;
	int activity_dirty_rect_new_border;
};

#endif
