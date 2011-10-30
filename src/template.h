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


#ifndef _OPENORIENTEERING_TEMPLATE_H_
#define _OPENORIENTEERING_TEMPLATE_H_

#include <QDialog>
#include <QRectF>

QT_BEGIN_NAMESPACE
class QPixmap;
class QPainter;
class QLineEdit;
QT_END_NAMESPACE

class Map;
class MapView;

/// Base class for templates
class Template
{
public:
	Template(const QString& filename, Map* map);
	Template(const Template& other);
	virtual ~Template();
	
	/// Must create a duplicate of the template
	virtual Template* duplicate() = 0;
	
	/// Is called when the template is opened by the user. Can show an initial configuration dialog
	/// and possibly adjust the template position to the main view. If this returns false, the template is closed again.
	virtual bool open(QWidget* dialog_parent, MapView* main_view) {return true;}
	
	/// Applies the transformation to the given painter. The map scale denominator may be used to calculate the final template scale.
	void applyTemplateTransform(QPainter* painter);
	
	/// Must draw the template using the given painter. The clip rect is in template coordinates,
	/// the scale is the combined view & template scale, which can be used to give a minimum size to elements
	virtual void drawTemplate(QPainter* painter, QRectF& clip_rect, double scale) = 0;
	
	/// Marks the whole area of the template as "to be redrawn". Use this before and after modifications to the template transformation.
	void setTemplateAreaDirty();
	
	/// Returns the bounding box of the template in map coordinates (mm) after transformations applied
	QRectF calculateBoundingBox();
	
	/// Must return the extent of the template around the origin (before applying any transformations)
	virtual QRectF getExtent() = 0;
	
	// Getters / Setters
	inline bool isTemplateValid() const {return template_valid;}
	inline const QString& getTemplateFilename() const {return template_file;}
	inline const QString& getTemplatePath() const {return template_path;}
	
	inline void setTemplateFilename(const QString& new_filename) {template_file = new_filename;}
	
	inline qint64 getTemplateX() const {return template_x;}
	inline void setTemplateX(qint64 x) {template_x = x;}
	
	inline qint64 getTemplateY() const {return template_y;}
	inline void setTemplateY(qint64 y) {template_y = y;}
	
	// These are the scale values for display; the scale values used for scale calculation can be
	// different and are returned by getTemplateFinalScaleX/Y. For example, this can be used to
	// display the scale in a different unit.
	inline double getTemplateScaleX() const {return template_scale_x;}
	inline void setTemplateScaleX(double scale_x) {template_scale_x = scale_x;}
	inline double getTemplateScaleY() const {return template_scale_y;}
	inline void setTemplateScaleY(double scale_y) {template_scale_y = scale_y;}
	virtual double getTemplateFinalScaleX() const {return getTemplateScaleX();}
	virtual double getTemplateFinalScaleY() const {return getTemplateScaleY();}
	
	inline double getTemplateRotation() const {return template_rotation;}
	inline void setTemplateRotation(double rotation) {template_rotation = rotation;}
	
	inline int getTemplateGroup() const {return template_group;}
	inline void setTemplateGroup(int value) {template_group = value;}
	
	/// Tries to find a matching template subclass for the given path by looking at the file extension
	static Template* templateForFile(const QString& path, Map* map);
	
protected:
	QString template_file;
	QString template_path;
	
	qint64 template_x;			// in 1/1000 mm
	qint64 template_y;
	double template_scale_x;
	double template_scale_y;
	double template_rotation;	// 0 - 2*M_PI
	
	int template_group;			// -1: no group
	
	// to be filled by the derived class
	//float templateCenterX;
	//float templateCenterY;
	bool template_valid;		// if the file cannot be found or loaded, the template is invalid
	
	Map* map;					// the map which contains this template
};

class TemplateImage : public Template
{
public:
	TemplateImage(const QString& filename, Map* map);
	TemplateImage(const TemplateImage& other);
    virtual ~TemplateImage();
    virtual Template* duplicate();
	
    virtual bool open(QWidget* dialog_parent, MapView* main_view);
    virtual void drawTemplate(QPainter* painter, QRectF& clip_rect, double scale);
    virtual QRectF getExtent();
	
	virtual double getTemplateFinalScaleX() const;
	virtual double getTemplateFinalScaleY() const;
	
protected:
	QPixmap* pixmap;	// TODO: Change that to QImage?
};
class TemplateImageOpenDialog : public QDialog
{
Q_OBJECT
public:
	TemplateImageOpenDialog(QWidget* parent);
	
	inline double getMpp() const {return mpp;}
	
protected slots:
	void mppChanged(const QString& new_text);
	
private:
	double mpp;
	
	QLineEdit* mpp_edit;
	QPushButton* open_button;
};

#endif
