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

#ifndef OPENORIENTEERING_GDAL_TEMPLATE_H
#define OPENORIENTEERING_GDAL_TEMPLATE_H

#include <array>

#include <QByteArray>
#include <QString>
#include <QTransform>

using GDALDatasetH = void*;

namespace OpenOrienteering {


/**
 * Support for geospatial raster data.
 */
class GdalTemplate
{
public:
	/**
	 * Raster data georeferencing information
	 */
	struct RasterGeoreferencing
	{
		QByteArray driver;                                             ///< @see GDALGetDriverShortName()
		QByteArray spec;                                               ///< @see GDALGetProjectionRef()
		std::array<double, 6> geo_transform = {{ 0, 1, 0, 0, 0, 1 }};  ///< @see GDALGetGeoTransform()
		bool valid = false;
		
		/**
		 * Returns RasterGeoreferencing for the data in the given dataset.
		 */
		static RasterGeoreferencing fromGDALDataset(GDALDatasetH dataset);
		
		/**
		 * Returns a pixel-to-world transformation from GDAL's geo_transform.
		 */
		operator QTransform() const;
		
		/**
		 * Return the given CRS specification converted to PROJ format, if possible.
		 * 
		 * @see OSRExportToProj4()
		 */
		static QByteArray toProjSpec(const QByteArray& gdal_spec);
	};
	
	/**
	 * Returns raster georeferencing for the given file.
	 * 
	 * In case of errors, the data's `valid` member is set to false.
	 */
	static RasterGeoreferencing tryReadProjection(const QString& filepath);
	
};


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_GDAL_TEMPLATE_H
