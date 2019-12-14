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

#include "gdal_image_reader.h"

#include <initializer_list>

#include <Qt>
#include <QtGlobal>
#include <QCoreApplication>
#include <QImage>
#include <QImageReader>
#include <QSize>
#include <QString>
#include <QVarLengthArray>

#include <gdal.h>

#include "gdal/gdal_manager.h"


namespace OpenOrienteering {

GdalImageReader::GdalImageReader(const QString& path)
: path(path)
{
	GdalManager();
	CPLErrorReset();
	dataset = GDALOpen(path.toUtf8(), GA_ReadOnly);
	if (dataset)
		raster_count = GDALGetRasterCount(dataset);
	if (!canRead())
		error_string = tr("Failed to read image data: %s").arg(QString::fromUtf8(CPLGetLastErrorMsg()));
}

GdalImageReader::~GdalImageReader()
{
	if (dataset)
		GDALClose(dataset);
}

bool GdalImageReader::canRead() const
{
	return raster_count > 0;
}

QImageReader::ImageReaderError GdalImageReader::error() const
{
	return err;
}

QString GdalImageReader::errorString() const
{
	return error_string;
}

QString GdalImageReader::filename() const
{
	return path;
}

QByteArray GdalImageReader::format() const
{
	auto driver = GDALGetDatasetDriver(dataset);
	return { GDALGetDriverShortName(driver) };
}

QImage GdalImageReader::read()
{
	QImage image;
	if (!read(&image))
		image = {};
	return image;
}

bool GdalImageReader::read(QImage* image)
{
	Q_ASSERT(image);
	if (!image)
	{
		err = QImageReader::UnknownError;
		return false;
	}
	
	auto raster = readRasterInfo();
	if (raster.image_format == QImage::Format_Invalid)
	{
		err = QImageReader::UnsupportedFormatError;
		error_string = tr("Unsupported image format");
		return false;
	}
	
	if (image->format() != raster.image_format || image->size() != raster.size)
	{
		*image = QImage(raster.size, raster.image_format);
	}
	if (image->isNull())
	{
		err = QImageReader::UnknownError;
		error_string = QCoreApplication::translate(
		                   "OpenOrienteering::TemplateImage",
		                   "Not enough free memory (image size: %1x%2 pixels)")
		               .arg(raster.size.width()).arg(raster.size.height());
		return false;
	}
	
	image->fill(Qt::white);
	auto const width = raster.size.width();
	auto const height = raster.size.height();
	CPLErrorReset();
	auto result = GDALDatasetRasterIO(dataset, GF_Read, 
	                                  0, 0, width, height,
	                                  image->bits() + raster.band_offset, width, height,
	                                  GDT_Byte, raster.bands.count(), raster.bands.data(),
	                                  raster.pixel_space, image->bytesPerLine(), raster.band_space);
	if (result >= CE_Warning)
	{
		err = QImageReader::InvalidDataError;
		error_string = tr("Failed to read image data: %s").arg(QString::fromUtf8(CPLGetLastErrorMsg()));
	}
	
	return true;
}

int GdalImageReader::findRasterBand(GDALColorInterp color_interpretation) const
{
	auto band = raster_count;
	while (band > 0)
	{
		auto hBand = GDALGetRasterBand(dataset, band);
		if (GDALGetRasterColorInterpretation(hBand) == color_interpretation)
			break;
		--band;
	}
	return band;
}

GdalImageReader::RasterInfo GdalImageReader::readRasterInfo() const
{
	RasterInfo raster;
	raster.size = { GDALGetRasterXSize(dataset), GDALGetRasterYSize(dataset) };
	
	for (auto color_interpretation : { GCI_BlueBand, GCI_GreenBand, GCI_RedBand })
	{
		if (auto found = findRasterBand(color_interpretation))
			raster.bands.push_back(found);
	}
	if (raster.bands.count() == 3)
	{
		raster.pixel_space = 4;
		if (auto alpha_band = findRasterBand(GCI_AlphaBand))
		{
			raster.bands.push_back(alpha_band);
			raster.image_format = QImage::Format_ARGB32;
		}
		else
		{
			raster.image_format = QImage::Format_RGB32;
		}
	}
	
	return raster;
}


}  // namespace OpenOrienteering
