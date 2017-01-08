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


#ifndef _OPENORIENTEERING_TEMPLATE_IMAGE_H_
#define _OPENORIENTEERING_TEMPLATE_IMAGE_H_

#include "template.h"

#include <QDialog>
#include <QImage>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QRadioButton;
class QXmlStreamReader;
class QXmlStreamWriter;
QT_END_NAMESPACE

class Georeferencing;

/**
 * Template showing a raster image.
 * Can be georeferenced or non-georeferenced.
 */
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
	
	/**
	 * Returns the filename extensions supported by this template class.
	 */
	static const std::vector<QByteArray>& supportedExtensions();
	
	TemplateImage(const QString& path, Map* map);
    virtual ~TemplateImage();
	virtual const char* getTemplateType() const {return "TemplateImage";}
	virtual bool isRasterGraphics() const {return true;}

	virtual bool saveTemplateFile() const;
	virtual bool loadTypeSpecificTemplateConfiguration(QIODevice* stream, int version);
	virtual void saveTypeSpecificTemplateConfiguration(QXmlStreamWriter& xml) const;
	virtual bool loadTypeSpecificTemplateConfiguration(QXmlStreamReader& xml);

	virtual bool loadTemplateFileImpl(bool configuring);
	virtual bool postLoadConfiguration(QWidget* dialog_parent, bool& out_center_in_view);
	virtual void unloadTemplateFileImpl();
	
    virtual void drawTemplate(QPainter* painter, QRectF& clip_rect, double scale, bool on_screen, float opacity) const;
	virtual QRectF getTemplateExtent() const;
	virtual bool canBeDrawnOnto() const {return true;}

	/**
	 * Calculates the image's center of gravity in template coordinates by
	 * iterating over all pixels, leaving out the pixels with background_color.
	 */
	QPointF calcCenterOfGravity(QRgb background_color);
	
	/** Returns the internal QImage. */
	inline const QImage& getImage() const {return image;}
	
	/**
	 * Returns which georeferencing method (if any) is available.
	 * (This does not mean that the image is in georeferenced mode)
	 */
	inline GeoreferencingType getAvailableGeoreferencing() const {return available_georef;}
	
public slots:
	void updateGeoreferencing();
	
protected:
	/** Holds a pixel-to-world transform loaded from a world file. */
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
		bool tryToLoadForImage(const QString& image_path);
	};
	
	/** Information about an undo step for the paint-on-template functionality. */
	struct DrawOnImageUndoStep
	{
		/** Copy of previous image part */
		QImage image;
		
		/** X position of image part origin */
		int x;
		
		/** Y position of image part origin */
		int y;
	};
	
	virtual Template* duplicateImpl() const;
	virtual void drawOntoTemplateImpl(MapCoordF* coords, int num_coords, QColor color, float width);
	virtual void drawOntoTemplateUndo(bool redo);
	void addUndoStep(const DrawOnImageUndoStep& new_step);
	void calculateGeoreferencing();
	void updatePosFromGeoreferencing();

	QImage image;
	
	std::vector< DrawOnImageUndoStep > undo_steps;
	/// Current index in undo_steps, where 0 means before the first item.
	int undo_index;
	
	GeoreferencingType available_georef;
	QScopedPointer<Georeferencing> georef;
	// Temporary storage for crs spec. Use georef instead.
	QString temp_crs_spec;
};

/**
 * Initial setting dialog when opening a raster image as template,
 * asking for how to position the image.
 * 
 * \todo Move this class to separate files.
 */
class TemplateImageOpenDialog : public QDialog
{
Q_OBJECT
public:
	TemplateImageOpenDialog(TemplateImage* templ, QWidget* parent);
	
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
	
	TemplateImage* templ;
};

#endif
