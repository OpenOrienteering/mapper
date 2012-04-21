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


#ifndef _OPENORIENTEERING_TEMPLATE_IMAGE_H_
#define _OPENORIENTEERING_TEMPLATE_IMAGE_H_

#include "template.h"

QT_BEGIN_NAMESPACE
class QCheckBox;
class QRadioButton;
QT_END_NAMESPACE

/// A raster image used as template
class TemplateImage : public Template
{
Q_OBJECT
public:
	TemplateImage(const QString& filename, Map* map);
	TemplateImage(const TemplateImage& other);
    virtual ~TemplateImage();
    virtual Template* duplicate();
	virtual const QString getTemplateType() {return "TemplateImage";}
    virtual bool saveTemplateFile();
	
    virtual bool open(QWidget* dialog_parent, MapView* main_view);
    virtual void drawTemplate(QPainter* painter, QRectF& clip_rect, double scale, float opacity);
    virtual QRectF getExtent();
	virtual bool canBeDrawnOnto() {return true;}
	
	/// Calculates the image's center of gravity in template coordinates by iterating over all pixels, leaving out the pixels with background_color.
	QPointF calcCenterOfGravity(QRgb background_color);
	
	virtual double getTemplateFinalScaleX() const;
	virtual double getTemplateFinalScaleY() const;
	
protected:
    virtual void drawOntoTemplateImpl(QPointF* points, int num_points, QColor color, float width);
    virtual bool changeTemplateFileImpl(const QString& filename);
	
	QImage* image;
};
/// Initial setting dialog when opening a raster image as template, asking for the meters per pixel
class TemplateImageOpenDialog : public QDialog
{
Q_OBJECT
public:
	TemplateImageOpenDialog(Map* map, QWidget* parent);
	
	inline double getMpp() const;
	
protected slots:
	void radioClicked();
	void setOpenEnabled();
	void doAccept();
	
private:
	QRadioButton* mpp_radio;
	QRadioButton* dpi_radio;
	QLineEdit* mpp_edit;
	QLineEdit* dpi_edit;
	QLineEdit* scale_edit;
	QPushButton* open_button;
	
	Map* map;
};

#endif
