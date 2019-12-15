/*
 *    Copyright 2019 Kai Pastor
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

#include "gdal_template.h"

#include <QtGlobal>
#include <QByteArray>
#include <QString>

#include "gdal/gdal_image_reader.h"
#include "gdal/gdal_manager.h"

namespace OpenOrienteering {

// static
const std::vector<QByteArray>& GdalTemplate::supportedExtensions()
{
	return GdalManager().supportedRasterExtensions();
}


GdalTemplate::GdalTemplate(const QString& path, Map* map)
: TemplateImage(path, map)
{}

GdalTemplate::GdalTemplate(const GdalTemplate& proto) = default;

GdalTemplate::~GdalTemplate() = default;

GdalTemplate* GdalTemplate::duplicate() const
{
	return new GdalTemplate(*this);
}

const char* GdalTemplate::getTemplateType() const
{
	return "GdalTemplate";
}

bool GdalTemplate::loadTemplateFileImpl(bool configuring)
{
	GdalImageReader reader(template_path);
	if (!reader.canRead())
	{
		setErrorString(reader.errorString());
		return false;
	}
	
	qDebug("GdalTemplate: Using GDAL driver '%s'", reader.format().constData());
	
	if (!reader.read(&image))
	{
		setErrorString(reader.errorString());
		return false;
	}
	
	// Duplicated from TemplateImage, for compatibility
	available_georef = findAvailableGeoreferencing(reader.readGeoTransform());
	if (!configuring && is_georeferenced)
	{
		if (available_georef.front().type == Georeferencing_None)
		{
			// Image was georeferenced, but georeferencing info is gone -> deny to load template
			setErrorString(::OpenOrienteering::TemplateImage::tr("Georeferencing not found"));
			return false;
		}
		
		calculateGeoreferencing();
	}
	
	return true;
}


}  // namespace OpenOrienteering
