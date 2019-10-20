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

#include <cpl_conv.h>
#include <gdal.h>
#include <ogr_api.h>
#include <ogr_srs_api.h>

#include <QString>


namespace OpenOrienteering {


// ## GdalTemplate::RasterGeoreferencing

// static
GdalTemplate::RasterGeoreferencing GdalTemplate::RasterGeoreferencing::fromGDALDataset(GDALDatasetH dataset)
{
	RasterGeoreferencing raster_georef;
	if (dataset != nullptr)
	{
		auto driver = GDALGetDatasetDriver(dataset);
		raster_georef.driver = GDALGetDriverShortName(driver);
		raster_georef.spec = GDALGetProjectionRef(dataset);
		auto const result = GDALGetGeoTransform(dataset, raster_georef.geo_transform.data());
		raster_georef.valid = result == CE_None;
	}
	return raster_georef;
}

GdalTemplate::RasterGeoreferencing::operator QTransform() const
{
	return { geo_transform[1], geo_transform[2], geo_transform[4], geo_transform[5], geo_transform[0], geo_transform[3] };
}

// static
QByteArray GdalTemplate::RasterGeoreferencing::toProjSpec(const QByteArray& gdal_spec)
{
	auto const spatial_reference = OSRNewSpatialReference(gdal_spec);
	char* proj_spec_cstring;
	auto const ogr_error = OSRExportToProj4(spatial_reference, &proj_spec_cstring);
	auto const result = QByteArray(ogr_error == OGRERR_NONE ? proj_spec_cstring : nullptr);
	CPLFree(proj_spec_cstring);
	OSRDestroySpatialReference(spatial_reference);
	return result;
}



// ## GdalTemplate

// static
GdalTemplate::RasterGeoreferencing GdalTemplate::tryReadProjection(const QString& filepath)
{
	if (auto dataset = GDALOpen(filepath.toUtf8(), GA_ReadOnly))
	{
		auto raster_georef = RasterGeoreferencing::fromGDALDataset(dataset);
		GDALClose(dataset);
		return raster_georef;
	}
	return {};
}


}  // namespace OpenOrienteering
