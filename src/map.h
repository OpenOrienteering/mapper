/*
 *    Copyright 2011 Thomas Schöps
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


#ifndef _OPENORIENTEERING_MAP_H_
#define _OPENORIENTEERING_MAP_H_

#include <vector>

#include <QString>
#include <QColor>
#include <QRect>
#include <QHash>

#include "matrix.h"

QT_BEGIN_NAMESPACE
class QFile;
class QPainter;
QT_END_NAMESPACE

class MapWidget;
class MapCoord;
class MapCoordF;
class Template;

/// Central class for an OpenOrienteering map
class Map : public QObject
{
Q_OBJECT
public:
	struct Color
	{
		void updateFromCMYK();
		void updateFromRGB();
		
		QString name;
		int priority;
		
		// values are in range [0; 1]
		float c, m, y, k;
		float r, g, b;
		float opacity;
		
		QColor color;
	};
	
	/// Creates a new, empty map
	Map();
	~Map();
	
	/// Attempts to save the map to the given file.
	bool saveTo(const QString& path);
	/// Attempts to load the map from the specified path. Returns true on success.
	bool loadFrom(const QString& path);
	
	/// Must be called to notify the map of new widgets displaying it. Useful to notify the widgets about which parts of the map have changed and need to be redrawn
	void addMapWidget(MapWidget* widget);
	void removeMapWidget(MapWidget* widget);
	/// Redraws all map widgets completely - that can be slow!
	void updateAllMapWidgets();
	
	// Current drawing
	
	/// Sets the rect (given in map coordinates) as dirty rect for every map widget, enlarged by the given pixel border
	void setDrawingBoundingBox(QRectF map_coords_rect, int pixel_border, bool do_update = true);
	void clearDrawingBoundingBox();
	
	void setActivityBoundingBox(QRectF map_coords_rect, int pixel_border, bool do_update = true);
	void clearActivityBoundingBox();
	
	void updateDrawing(QRectF map_coords_rect, int pixel_border);	// updates all dynamic drawing: tool & activity drawings
	
	// Colors
	
	inline int getNumColors() const {return (int)colors.size();}
	inline Color* getColor(int i) {return colors[i];}
	void setColor(Color* color, int pos);
	Color* addColor(int pos);
	void addColor(Color* color, int pos);
	void deleteColor(int pos);
	void setColorsDirty();
	
	// Templates
	
	inline int getNumTemplates() const {return templates.size();}
	inline Template* getTemplate(int i) {return templates[i];}
	inline void setFirstFrontTemplate(int pos) {first_front_template = pos;}
	inline int getFirstFrontTemplate() const {return first_front_template;}
	void setTemplate(Template* temp, int pos);
	void addTemplate(Template* temp, int pos);									// NOTE: adjust first_front_template manually!
	void deleteTemplate(int pos);												// NOTE: adjust first_front_template manually!
	void setTemplateAreaDirty(Template* temp, QRectF area, int pixel_border);	// marks the respecive regions in the template caches as dirty; area is given in map coords (mm). Does nothing if the template is not visible in a widget! So make sure to call this and showing/hiding a template in the correct order!
	void setTemplateAreaDirty(int i);											// this does nothing for i == -1
	void setTemplatesDirty();
	
	// Other settings
	
	inline void setScaleDenominator(int value) {scale_denominator = value;}
	inline int getScaleDenominator() const {return scale_denominator;}
	
signals:
	void gotUnsavedChanges();
	
	void templateAdded(Template* temp);
	void templateDeleted(Template* temp);
	
private:
	typedef std::vector<Color*> ColorVector;
	typedef std::vector<Template*> TemplateVector;
	typedef std::vector<MapWidget*> WidgetVector;
	
	void checkIfFirstColorAdded();
	void checkIfFirstTemplateAdded();
	void saveString(QFile* file, const QString& str);
	void loadString(QFile* file, QString& str);
	
	ColorVector colors;
	TemplateVector templates;
	int first_front_template;	// index of the first template in templates which should be drawn in front of the map
	WidgetVector widgets;
	
	bool colors_dirty;		// are there unsaved changes for the colors?
	bool templates_dirty;	//    ... for the templates?
	bool unsaved_changes;	// are there unsaved changes for any component?
	
	int scale_denominator;	// this is the number x if the scale is written as 1:x
};

/// Coordinates of a point in a map.
/// Saved as 64bit integers, where in addition some flags about the type of the point can be stored in the lowest 4 bits.
class MapCoord
{
public:
	/// Create coordinates from position given in millimeters
	inline MapCoord(double x, double y)
	{
		this->x = qRound64(x * 1000) << 4;
		this->y = qRound64(y * 1000) << 4;
	}
	
	// Get the coordinates as doubles
	inline double xd() {return (x >> 4) / 1000.0;}
	inline double yd() {return (y >> 4) / 1000.0;}
	
private:
	qint64 x;
	qint64 y;
};

/// Floating point map coordinates, only for rendering.
class MapCoordF
{
public:
	
	inline MapCoordF() {}
	inline MapCoordF(double x, double y) {setX(x); setY(y);}
	inline MapCoordF(const MapCoordF& copy) {x = copy.x; y = copy.y;}
	
    inline void setX(double x) {this->x = x;};
	double getX() const {return x;}
	
	inline void setY(double y) {this->y = y;};
	double getY() const {return y;}
	
	inline void normalize()
	{
		double length = sqrt(getX()*getX() + getY()*getY());
		if (length < 1e-08)
			return;
		
		setX(getX() / length);
		setY(getY() / length);
	}
	
	inline double length() const {return sqrt(x*x + y*y);}
	inline double lengthSquared() const {return x*x + y*y;}
	
	inline double lengthTo(const MapCoordF& to)
	{
		double dx = to.getX() - getX();
		double dy = to.getY() - getY();
		return sqrt(dx*dx + dy*dy);
	}
	
	inline double getAngle()
	{
		return atan2(y, x);
	}
	
	/// Get a perpendicular vector pointing to the right
	inline void perpRight()
	{
		double x = getX();
		setX(-getY());
		setY(x);
	}
	
	MapCoord toMapCoord()
	{
		return MapCoord(x, y);
	}
	
	inline bool operator== (const MapCoordF& other) const
	{
		return (other.x == x) && (other.y == y);
	}
	
    inline QPointF toQPointF()
	{
		return QPointF(x, y);
	}
	
protected:
	
	double x;
	double y;
};

/// Contains all visibility information for a template. This is stored in the MapViews
struct TemplateVisibility
{
	float opacity;	// 0 to 1
	bool visible;
	
	inline TemplateVisibility()
	{
		opacity = 1;
		visible = false;
	}
};

/// Stores view position, zoom, rotation and template visibilities to define a view onto a map
class MapView
{
public:
	/// Creates a default view looking at the origin
	MapView(Map* map);
	~MapView();
	
	/// Must be called to notify the map view of new widgets displaying it. Useful to notify the widgets which need to be redrawn
	void addMapWidget(MapWidget* widget);
	void removeMapWidget(MapWidget* widget);
	/// Redraws all map widgets completely - that can be slow!
	void updateAllMapWidgets();
	
	/// Converts x, y (with origin at the center of the view) to map coordinates
	inline MapCoord viewToMap(double x, double y)
	{
		return MapCoord(view_to_map.get(0, 0) * x + view_to_map.get(0, 1) * y + view_to_map.get(0, 2), view_to_map.get(1, 0) * x + view_to_map.get(1, 1) * y + view_to_map.get(1, 2));
	}
	inline MapCoord viewToMap(QPointF point)
	{
		return viewToMap(point.x(), point.y());
	}
	inline MapCoordF viewToMapF(double x, double y)
	{
		return MapCoordF(view_to_map.get(0, 0) * x + view_to_map.get(0, 1) * y + view_to_map.get(0, 2), view_to_map.get(1, 0) * x + view_to_map.get(1, 1) * y + view_to_map.get(1, 2));
	}
	inline MapCoordF viewToMapF(QPointF point)
	{
		return viewToMapF(point.x(), point.y());
	}
	/// Converts map coordinates to view coordinates (with origin at the center of the view)
	inline void mapToView(MapCoord coords, double& out_x, double& out_y)
	{
		out_x = map_to_view.get(0, 0) * coords.xd() + map_to_view.get(0, 1) * coords.yd() + map_to_view.get(0, 2);
		out_y = map_to_view.get(1, 0) * coords.xd() + map_to_view.get(1, 1) * coords.yd() + map_to_view.get(1, 2);
	}
	inline QPointF mapToView(MapCoord coords)
	{
		return QPointF(map_to_view.get(0, 0) * coords.xd() + map_to_view.get(0, 1) * coords.yd() + map_to_view.get(0, 2),
					   map_to_view.get(1, 0) * coords.xd() + map_to_view.get(1, 1) * coords.yd() + map_to_view.get(1, 2));
	}
	inline QPointF mapToView(MapCoordF coords)
	{
		return QPointF(map_to_view.get(0, 0) * coords.getX() + map_to_view.get(0, 1) * coords.getY() + map_to_view.get(0, 2),
					   map_to_view.get(1, 0) * coords.getX() + map_to_view.get(1, 1) * coords.getY() + map_to_view.get(1, 2));
	}
	
	/// Convert lengths between map and pixel display
	double lengthToPixel(qint64 length);
	qint64 pixelToLength(double pixel);
	
	/// Calculates the bounding box of the map coordinates which can be viewed using the given view coordinates rect
	QRectF calculateViewedRect(QRectF view_rect);
	/// Calculates the bounding box in view coordinates of the given map coordinates rect
	QRectF calculateViewBoundingBox(QRectF map_rect);
	
	/// Applies the view transform to the given painter.
	/// Note 1: The transform is combined with the painter's existing tranfsorm.
	/// Note 2: If you want to use the transform to draw something, you will probably also want to make sure that
	///         the view center is in the center of your viewport. Because the view does not know the viewport size,
	///         it cannot do that. So this offset must be applied before calling this method.
	void applyTransform(QPainter* painter);
	
	// Dragging
	void setDragOffset(QPoint offset);
	void completeDragging(QPoint offset);
	
	inline Map* getMap() const {return map;}
	
	inline float getZoom() const {return zoom;}
	void setZoom(float value);
	inline float getRotation() const {return rotation;}
	inline void setRotation(float value) {rotation = value; update();}
	inline qint64 getPositionX() const {return position_x;}
	void setPositionX(qint64 value);
	inline qint64 getPositionY() const {return position_y;}
	void setPositionY(qint64 value);
	inline int getViewX() const {return view_x;}
	inline void setViewX(int value) {view_x = value; update();}
	inline int getViewY() const {return view_y;}
	inline void setViewY(int value) {view_y = value; update();}
	inline QPoint getDragOffset() const {return drag_offset;}
	
	// Template visibilities
	bool isTemplateVisible(Template* temp);						// checks if the template is visible without creating a template visibility object if none exists
	TemplateVisibility* getTemplateVisibility(Template* temp);	// returns the template visibility object, creates one if not there yet with the default settings (invisible)
	void deleteTemplateVisibility(Template* temp);				// call this when a template is deleted to destroy the template visibility object
	
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
	
	QHash<Template*, TemplateVisibility*> template_visibilities;
	
	WidgetVector widgets;
};

#endif
