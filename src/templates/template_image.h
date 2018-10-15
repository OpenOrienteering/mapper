/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2017 Kai Pastor
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


#ifndef OPENORIENTEERING_TEMPLATE_IMAGE_H
#define OPENORIENTEERING_TEMPLATE_IMAGE_H

#include <vector>

#include <QColor>
#include <QDialog>
#include <QImage>
#include <QObject>
#include <QPointF>
#include <QRectF>
#include <QRgb>
#include <QScopedPointer>
#include <QString>

#include "templates/template.h"

class QByteArray;
class QLineEdit;
class QPainter;
class QPointF;
class QPushButton;
class QRadioButton;
class QRectF;
class QWidget;
class QXmlStreamReader;
class QXmlStreamWriter;

namespace OpenOrienteering {

class Georeferencing;
class Map;
class MapCoordF;


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
    ~TemplateImage() override;
	const char* getTemplateType() const override {return "TemplateImage";}
	bool isRasterGraphics() const override {return true;}

	bool saveTemplateFile() const override;
	void saveTypeSpecificTemplateConfiguration(QXmlStreamWriter& xml) const override;
	bool loadTypeSpecificTemplateConfiguration(QXmlStreamReader& xml) override;

	bool loadTemplateFileImpl(bool configuring) override;
	bool postLoadConfiguration(QWidget* dialog_parent, bool& out_center_in_view) override;
	void unloadTemplateFileImpl() override;
	
    void drawTemplate(QPainter* painter, const QRectF& clip_rect, double scale, bool on_screen, float opacity) const override;
	QRectF getTemplateExtent() const override;
	bool canBeDrawnOnto() const override {return true;}

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
	
	bool canChangeTemplateGeoreferenced() override;
	bool trySetTemplateGeoreferenced(bool value, QWidget* dialog_parent) override;
	
	
public slots:
	void updateGeoreferencing();
	
protected:
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
	
	Template* duplicateImpl() const override;
	void drawOntoTemplateImpl(MapCoordF* coords, int num_coords, QColor color, float width) override;
	void drawOntoTemplateUndo(bool redo) override;
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


}  // namespace OpenOrienteering

#endif
