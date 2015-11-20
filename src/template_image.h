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
class QXmlStreamReader;
class QXmlStreamWriter;
QT_END_NAMESPACE

class Georeferencing;

/// A raster image used as template
class TemplateImage : public Template
{
Q_OBJECT
public:
	enum GeoreferencingType
	{
		Georeferencing_None = 0,
		Georeferencing_WorldFile,
		Georeferencing_GeoTiff
	};
	
	TemplateImage(const QString& path, Map* map);
	virtual ~TemplateImage();
	virtual const QString getTemplateType() {return "TemplateImage";}

	virtual bool saveTemplateFile();
	virtual void saveTypeSpecificTemplateConfiguration(QIODevice* stream);
	virtual bool loadTypeSpecificTemplateConfiguration(QIODevice* stream, int version);
	virtual void saveTypeSpecificTemplateConfiguration(QXmlStreamWriter& xml);
	virtual bool loadTypeSpecificTemplateConfiguration(QXmlStreamReader& xml);

	virtual bool loadTemplateFileImpl(bool configuring);
	virtual bool postLoadConfiguration(QWidget* dialog_parent);
	virtual void unloadTemplateFileImpl();
	
	virtual void drawTemplate(QPainter* painter, QRectF& clip_rect, double scale, float opacity);
	virtual QRectF getTemplateExtent();
	virtual bool canBeDrawnOnto() {return true;}

	/// Calculates the image's center of gravity in template coordinates by iterating over all pixels, leaving out the pixels with background_color.
	QPointF calcCenterOfGravity(QRgb background_color);
	
	/// Returns the internal QImage
	inline QImage* getQImage() const {return image;}
	
	/// Returns which georeferencing method (if any) is available.
	/// (This does not mean that the image is in georeferenced mode)
	inline GeoreferencingType getAvailableGeoreferencing() const {return available_georef;}
	
public slots:
	void updateGeoreferencing();
	
protected:
	struct WorldFile
	{
		bool loaded;
		QTransform pixel_to_world;
		
		/// Creates an unloaded world file
		WorldFile();
		
		/// Tries to load the given path as world file.
		/// Returns true on success and sets loaded to true or false.
		bool load(const QString& path);
		
		/// Tries to find and load a world file for the given image path.
		bool tryToLoadForImage(const QString image_path);
	};
	
	virtual Template* duplicateImpl();
	virtual void drawOntoTemplateImpl(MapCoordF* coords, int num_coords, QColor color, float width);
	void calculateGeoreferencing();
	void updatePosFromGeoreferencing();

	QImage* image;
	
	GeoreferencingType available_georef;
	QScopedPointer<Georeferencing> georef;
	// Temporary storage for crs spec. Use georef instead.
	QString temp_crs_spec;
};

/// Initial setting dialog when opening a raster image as template, asking for the scale
class TemplateImageOpenDialog : public QDialog
{
Q_OBJECT
public:
	TemplateImageOpenDialog(TemplateImage* image, QWidget* parent);
	
	double getMpp() const;
	bool isGeorefRadioChecked() const;
	
protected slots:
	void radioClicked();
	void setOpenEnabled();
	void doAccept();
	
private:
	QRadioButton* georef_radio;
	QRadioButton* mpp_radio;
	QRadioButton* dpi_radio;
	QLineEdit* mpp_edit;
	QLineEdit* dpi_edit;
	QLineEdit* scale_edit;
	QPushButton* open_button;
	
	TemplateImage* image;
};

#endif
