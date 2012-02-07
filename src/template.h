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


#ifndef _OPENORIENTEERING_TEMPLATE_H_
#define _OPENORIENTEERING_TEMPLATE_H_

#include <QDialog>
#include <QRectF>

#include "map.h"

QT_BEGIN_NAMESPACE
class QPixmap;
class QPainter;
class QLineEdit;
class QFile;
QT_END_NAMESPACE

class Map;
class MapView;
class MapWidget;

/// Base class for templates
class Template : public QObject
{
Q_OBJECT
public:
	struct TemplateTransform
	{
		TemplateTransform();
		
		void save(QFile* file);
		void load(QFile* file);
		
		qint64 template_x;			// in 1/1000 mm
		qint64 template_y;
		double template_scale_x;
		double template_scale_y;
		double template_rotation;	// 0 - 2*M_PI
	};
	struct PassPoint
	{
		void save(QFile* file);
		void load(QFile* file);
		
		MapCoordF src_coords_template;		// start position specified by the user, in template coordinates
		MapCoordF src_coords_map;			// start position specified by the user
		MapCoordF dest_coords_map;			// end position specified by the user
		MapCoordF calculated_coords_map;	// position where the point really ended up
		double error;						// distance between dest_coords_map and calculated_coords_map; negative if not calculated yet
	};
	
	Template(const QString& filename, Map* map);
	Template(const Template& other);
	virtual ~Template();
	
	/// Must create a duplicate of the template
	virtual Template* duplicate() = 0;
	
	/// Returns a string which should identify the type of the template uniquely: the class name. Very simple RTTI feature.
	virtual const QString getTemplateType() = 0;
	
	/// Saves parameters such as transformation, georeferencing, etc.
	void saveTemplateParameters(QFile* file);
	void loadTemplateParameters(QFile* file);
	/// Saves the template itself, returns if successful. This is called when saving the map and the template hasUnsavedChanges() returns true
	virtual bool saveTemplateFile() {return false;}
	
	/// Changes a template's file without changing the parameters. Useful when a template file has been moved. Returns if successful.
	bool changeTemplateFile(const QString& filename);
	
	/// Is called when the template is opened by the user. Can show an initial configuration dialog
	/// and possibly adjust the template position to the main view. If this returns false, the template is closed again.
	/// Note: this should call updateTransformationMatrices(). TODO: would it be better to always call this automatically after calling open()?
	virtual bool open(QWidget* dialog_parent, MapView* main_view) {updateTransformationMatrices(); return true;}
	
	/// Applies the transformation to the given painter. The map scale denominator may be used to calculate the final template scale.
	void applyTemplateTransform(QPainter* painter);
	
	/// Must draw the template using the given painter with the given opacity. The clip rect is in template coordinates,
	/// the scale is the combined view & template scale, which can be used to give a minimum size to elements.
	/// The painter transformation is set to use template coordinates.
	virtual void drawTemplate(QPainter* painter, QRectF& clip_rect, double scale, float opacity) = 0;
	
	/// Can draw untransformed parts of the template (like texts) in viewport coordinates.
	virtual void drawTemplateUntransformed(QPainter* painter, const QRect& clip_rect, MapWidget* widget) {}
	
	/// Marks the whole area of the template as "to be redrawn". Use this before and after modifications to the template transformation.
	void setTemplateAreaDirty();
	
	/// Returns the bounding box of the template in map coordinates (mm) after transformations applied
	QRectF calculateBoundingBox();
	
	/// Must return if freehand drawing onto the template is possible
	virtual bool canBeDrawnOnto() {return false;}
	
	/// Draws onto the template. coords is an array of points with which the drawn line is defined and must contain at least 2 points.
	/// map_bbox can be an invalid rect, then the method will calculate it itself.
	void drawOntoTemplate(MapCoordF* coords, int num_coords, QColor color, float width, QRectF map_bbox);
	
	/// Must return the extent of the template around the origin (in template coordinates, without applying any transformations)
	virtual QRectF getExtent() = 0;
	
	// Coordinate transformations between template coordinates and map coordinates
	
	inline MapCoordF mapToTemplate(MapCoordF coords)
	{
		return MapCoordF(map_to_template.get(0, 0) * coords.getX() + map_to_template.get(0, 1) * coords.getY() + map_to_template.get(0, 2),
						 map_to_template.get(1, 0) * coords.getX() + map_to_template.get(1, 1) * coords.getY() + map_to_template.get(1, 2));
	}
	inline QPointF mapToTemplateQPoint(MapCoordF coords)
	{
		return QPointF(map_to_template.get(0, 0) * coords.getX() + map_to_template.get(0, 1) * coords.getY() + map_to_template.get(0, 2),
						 map_to_template.get(1, 0) * coords.getX() + map_to_template.get(1, 1) * coords.getY() + map_to_template.get(1, 2));
	}
	inline MapCoordF templateToMap(MapCoordF coords)
	{
		return MapCoordF(template_to_map.get(0, 0) * coords.getX() + template_to_map.get(0, 1) * coords.getY() + template_to_map.get(0, 2),
						 template_to_map.get(1, 0) * coords.getX() + template_to_map.get(1, 1) * coords.getY() + template_to_map.get(1, 2));
	}
	inline MapCoordF templateToMap(QPointF coords)
	{
		return MapCoordF(template_to_map.get(0, 0) * coords.x() + template_to_map.get(0, 1) * coords.y() + template_to_map.get(0, 2),
						 template_to_map.get(1, 0) * coords.x() + template_to_map.get(1, 1) * coords.y() + template_to_map.get(1, 2));
	}
	inline MapCoordF templateToMapOther(MapCoordF coords)	// normally not needed - this uses the other transformation parameters
	{
		return MapCoordF(template_to_map_other.get(0, 0) * coords.getX() + template_to_map_other.get(0, 1) * coords.getY() + template_to_map_other.get(0, 2),
						 template_to_map_other.get(1, 0) * coords.getX() + template_to_map_other.get(1, 1) * coords.getY() + template_to_map_other.get(1, 2));
	}
	
	// Pass points & georeferencing
	
	inline int getNumPassPoints() const {return passpoints.size();}
	inline PassPoint* getPassPoint(int i) {return &passpoints[i];}
	void addPassPoint(const PassPoint& point, int pos);
	void deletePassPoint(int pos);
	void clearPassPoints();
	
	void switchTransforms();	// change from georeferenced into original state or the other way round
	void getTransform(TemplateTransform& out);
	void setTransform(const TemplateTransform& transform);
	void getOtherTransform(TemplateTransform& out);
	void setOtherTransform(const TemplateTransform& transform);
	
	// Getters / Setters
	
	inline bool isTemplateValid() const {return template_valid;}
	inline const QString& getTemplateFilename() const {return template_file;}
	inline const QString& getTemplatePath() const {return template_path;}
	
	inline void setTemplateFilename(const QString& new_filename) {template_file = new_filename;}
	
	inline qint64 getTemplateX() const {return cur_trans.template_x;}
	inline void setTemplateX(qint64 x) {cur_trans.template_x = x; updateTransformationMatrices();}
	
	inline qint64 getTemplateY() const {return cur_trans.template_y;}
	inline void setTemplateY(qint64 y) {cur_trans.template_y = y; updateTransformationMatrices();}
	
	// These are the scale values for display (as text); the scale values used for the real scale calculation can be
	// different and are returned by getTemplateFinalScaleX/Y. For example, this can be used to
	// display the scale in a different unit.
	inline double getTemplateScaleX() const {return cur_trans.template_scale_x;}
	inline void setTemplateScaleX(double scale_x) {cur_trans.template_scale_x = scale_x; updateTransformationMatrices();}
	inline double getTemplateScaleY() const {return cur_trans.template_scale_y;}
	inline void setTemplateScaleY(double scale_y) {cur_trans.template_scale_y = scale_y; updateTransformationMatrices();}
	virtual double getTemplateFinalScaleX() const {return getTemplateScaleX();}
	virtual double getTemplateFinalScaleY() const {return getTemplateScaleY();}
	
	inline double getTemplateRotation() const {return cur_trans.template_rotation;}
	inline void setTemplateRotation(double rotation) {cur_trans.template_rotation = rotation; updateTransformationMatrices();}
	
	inline bool isGeoreferencingApplied() const {return georeferenced;}
	inline bool isGeoreferencingDirty() const {return georeferencing_dirty;}
	inline void setGeoreferencingDirty(bool value) {georeferencing_dirty = value; if (value) map->setTemplatesDirty();}
	
	inline int getTemplateGroup() const {return template_group;}
	inline void setTemplateGroup(int value) {template_group = value;}
	
	inline bool hasUnsavedChanges() const {return has_unsaved_changes;}
	inline void setHasUnsavedChanges(bool value) {has_unsaved_changes = value; if (value) map->setTemplatesDirty();}
	
	inline Map* getMap() const {return map;}
	
	/// Tries to find a matching template subclass for the given path by looking at the file extension
	static Template* templateForFile(const QString& path, Map* map);
	
protected:
	/// Must be implemented to draw the polyline given by the points onto the template if canBeDrawnOnto() returns true
	virtual void drawOntoTemplateImpl(QPointF* points, int num_points, QColor color, float width) {}
	
	/// Must be implemented to make changing the template file possible
	virtual bool changeTemplateFileImpl(const QString& filename) {return false;}
	
	void updateTransformationMatrices();
	
	QString template_file;
	QString template_path;
	bool template_valid;		// if the file cannot be found or loaded, the template is invalid (to be filled by the derived class)
	
	bool has_unsaved_changes;	// does the template itself (not its transformation) have unsaved changes (e.g. GPS track has changed, image has been painted on)
	
	// Transformation parameters & georeferencing; NOTE: call updateTransformationMatrices() after making direct changes to the transforms!
	TemplateTransform cur_trans;
	TemplateTransform other_trans;
	bool georeferenced;			// if true, cur_trans is the georeferenced transformation, otherwise it is the original one
	bool georeferencing_dirty;	// if true, the georeferenced transformation has to be recalculated
	std::vector< PassPoint > passpoints;
	
	// Transformation matrices
	Matrix map_to_template;
	Matrix template_to_map;
	Matrix template_to_map_other;
	
	int template_group;			// -1: no group
	
	Map* map;					// the map which contains this template
};

#endif
