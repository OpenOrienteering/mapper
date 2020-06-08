/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2020 Kai Pastor
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

#include <memory>
#include <vector>

#include <QtGlobal>
#include <QByteArray>
#include <QColor>
#include <QImage>
#include <QObject>
#include <QPointF>
#include <QRectF>
#include <QRgb>
#include <QString>
#include <QTransform>

#include "templates/template.h"

class QPainter;
class QPointF;
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
	/**
	 * Information fragment about template image georeferencing.
	 * 
	 * A GeoreferencingOption may carry information about the CRS and/or the
	 * pixel-to-world transform for a template image. The crs_spec is empty
	 * if the CRS is unknown. The transform's source is empty if the
	 * pixel-to-world transformation is unknown.
	 */
	struct GeoreferencingOption
	{
		struct Transform
		{
			QTransform pixel_to_world = {};     ///< The transformation from pixel space to CRS space.
			QByteArray source = {};             ///< The source of the pixel-to-world transform.
		};

		QString crs_spec = {};              ///< The specification of the CRS uses for georeferencing.
		Transform transform = {};           ///< The transformation from pixel space to CRS space.
	};
	
	/**
	 * A collection of alternative and/or complementary georeferencing information fragments.
	 */
	struct GeoreferencingOptions
	{
		GeoreferencingOption effective;     ///< The effective option, if CRS and transform are defined.
		GeoreferencingOption world_file;    ///< A pixel-to-world transform from a world file.
		GeoreferencingOption template_file; ///< CRS and/or pixel-to-world transform from the template file.
	};
	
	/**
	 * Returns the filename extensions supported by this template class.
	 */
	static const std::vector<QByteArray>& supportedExtensions();
	
	TemplateImage(const QString& path, Map* map);
protected:
	TemplateImage(const TemplateImage& proto);
public:
    ~TemplateImage() override;
	
	TemplateImage* duplicate() const override;
	
	const char* getTemplateType() const override {return "TemplateImage";}
	bool isRasterGraphics() const override {return true;}

	bool saveTemplateFile() const override;
	void saveTypeSpecificTemplateConfiguration(QXmlStreamWriter& xml) const override;
	bool loadTypeSpecificTemplateConfiguration(QXmlStreamReader& xml) override;

	bool loadTemplateFileImpl(bool configuring) override;
	bool postLoadConfiguration(QWidget* dialog_parent, bool& out_center_in_view) override;
	void unloadTemplateFileImpl() override;
	
    void drawTemplate(QPainter* painter, const QRectF& clip_rect, double scale, bool on_screen, qreal opacity) const override;
	QRectF getTemplateExtent() const override;
	bool canBeDrawnOnto() const override { return drawable; }

	/**
	 * Calculates the image's center of gravity in template coordinates by
	 * iterating over all pixels, leaving out the pixels with background_color.
	 */
	QPointF calcCenterOfGravity(QRgb background_color);
	
	/** Returns the internal QImage. */
	inline const QImage& getImage() const {return image;}
	
	/**
	 * Returns which georeferencing methods are known to be available.
	 * 
	 * (This does not imply that the image is in georeferenced mode.)
	 * 
	 * Invariant: The list is never empty. It always contains at least an entry
	 * of type Georeferencing_None. This entry is always the last one.
	 */
	const GeoreferencingOptions& availableGeoreferencing() const { return available_georef; }
	
	bool canChangeTemplateGeoreferenced() override;
	bool trySetTemplateGeoreferenced(bool value, QWidget* dialog_parent) override;
	
	
public slots:
	void updateGeoreferencing();
	
protected:
	/**
	 * Collects available georeferencing information.
	 * 
	 * This function is meant to be used from loadTemplateFileImpl() to set the
	 * available_georef member. The parameter template_file_option is used to
	 * efficiently provide georeferencing information from the template file
	 * itself.
	 */
	GeoreferencingOptions findAvailableGeoreferencing(GeoreferencingOption template_file_option) const;
	
	/**
	 * Tests if the available georeferencing options can be used with the current map.
	 * 
	 * To be usable, the effective option must provide a transformation, and
	 * if the map's CRS spec is not empty (local georeferencing), the effective
	 * option's CRS spec must not be empty.
	 * 
	 * Note that changing the state to georeferenced may still fail for invalid
	 * CRS specs.
	 */
	bool isGeoreferencingUsable() const;
	
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
	
	void drawOntoTemplateImpl(MapCoordF* coords, int num_coords, const QColor& color, qreal width, ScribbleOptions mode) override;
	void drawOntoTemplateUndo(bool redo) override;
	void addUndoStep(const DrawOnImageUndoStep& new_step);
	void calculateGeoreferencing();
	void updatePosFromGeoreferencing();

	QImage image;
	
	std::vector< DrawOnImageUndoStep > undo_steps;
	/// Current index in undo_steps, where 0 means before the first item.
	int undo_index = 0;
	/// A flag indicating that this template can be drawn onto.
	bool drawable = false;
	
	GeoreferencingOptions available_georef;
	std::unique_ptr<Georeferencing> georef;
};


}  // namespace OpenOrienteering

#endif
