/*
 *    Copyright 2019-2020 Kai Pastor
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

#ifndef OPENORIENTEERING_GDAL_TEMPLATE_H
#define OPENORIENTEERING_GDAL_TEMPLATE_H

#include <vector>

#include <QString>

#include "templates/template_image.h"

class QByteArray;

namespace OpenOrienteering {

class Map;


/**
 * Support for geospatial raster data.
 */
class GdalTemplate : public TemplateImage
{
public:
	static bool canRead(const QString& path);
	
	static const std::vector<QByteArray>& supportedExtensions();
	
	GdalTemplate(const QString& path, Map* map);
	~GdalTemplate() override;
	
protected:
	GdalTemplate(const GdalTemplate& proto);
	GdalTemplate* duplicate() const override;
	
public:
	const char* getTemplateType() const override;
	
protected:
	bool loadTemplateFileImpl(bool configuring) override;
	
};


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_GDAL_TEMPLATE_H
