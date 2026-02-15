/*
 *    Copyright 2019, 2020 Kai Pastor
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

#include <algorithm>
#include <array>
#include <initializer_list>

#include <Qt>
#include <QtGlobal>
#include <QCoreApplication>
#include <QImage>
#include <QImageReader>
#include <QRgb>
#include <QSize>
#include <QString>
#include <QTransform>
#include <QVarLengthArray>

#include <cpl_conv.h>
#include <gdal.h>
#include <ogr_api.h>
#include <ogr_srs_api.h>

#include "gdal/gdal_manager.h"


namespace {

QString toWkt(OGRSpatialReferenceH srs)
{
	QString wkt;
	char* wkt_cstring = nullptr;
	if (srs && OSRExportToWkt(srs, &wkt_cstring) == OGRERR_NONE)
		wkt = QString::fromUtf8(wkt_cstring);
	CPLFree(wkt_cstring);
	return wkt;
}

}


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
		error_string = tr("Failed to read image data: %1").arg(QString::fromUtf8(CPLGetLastErrorMsg()));
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
		error_string = tr("Unsupported raster data: %1")
		               .arg(QString::fromUtf8(rasterBandsAsText()));
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
		error_string = tr("Failed to read image data: %1").arg(QString::fromUtf8(CPLGetLastErrorMsg()));
		return false;
	}
	
	raster.postprocessing(*image);
	return true;
}

QByteArray GdalImageReader::rasterBandsAsText() const
{
	QByteArray text;
	text.reserve(raster_count * 15);
	for (int i = 1; i <= raster_count; ++i)
	{
		auto const raster_band = GDALGetRasterBand(dataset, i);
		auto const ci = GDALGetRasterColorInterpretation(raster_band);
		text += GDALGetColorInterpretationName(ci);
		text += GDALGetDataTypeName(GDALGetRasterDataType(raster_band));
		if (i < raster_count)
			text += ", ";
	}
	return text;
}

int GdalImageReader::findRasterBand(GDALColorInterp color_interpretation, GDALDataType data_type) const
{
	for (auto i = raster_count; i > 0; --i)
	{
		if (getColorInterpretationWithType(i, data_type) == color_interpretation)
			return i;
	}
	return 0;
}

GDALColorInterp GdalImageReader::getColorInterpretationWithType(int index, GDALDataType data_type) const
{
	auto raster_band = GDALGetRasterBand(dataset, index);
	return (GDALGetRasterDataType(raster_band) == data_type)
	       ? GDALGetRasterColorInterpretation(raster_band)
	       : GCI_Undefined;
}

GdalImageReader::RasterInfo GdalImageReader::readRasterInfo() const
{
	RasterInfo raster;
	raster.size = { GDALGetRasterXSize(dataset), GDALGetRasterYSize(dataset) };
	
	qDebug("GdalTemplate raster bands: %s", rasterBandsAsText().constData());
	
	auto alpha_band = findRasterBand(GCI_AlphaBand, GDT_Byte);
	if (raster_count == 1)
	{
		auto const color_band = 1;
		auto const color_interpretation = getColorInterpretationWithType(color_band, GDT_Byte);
		switch (color_interpretation)
		{
		case GCI_GrayIndex:
			raster.image_format = QImage::Format_Grayscale8;
			raster.pixel_space = 1;
			raster.bands.push_back(color_band);
			break;
		case GCI_PaletteIndex:
			raster.image_format = QImage::Format_Indexed8;
			raster.pixel_space = 1;
			raster.bands.push_back(color_band);
			raster.postprocessing = [this, color_band](QImage& image) {
				image.setColorTable(readColorTable(color_band));
			};
			break;
		default:
			// unsupported
			break;
		}
	}
	else if (raster_count == 2 && alpha_band)
	{
		auto color_band = 3 - alpha_band;
		auto const color_interpretation = getColorInterpretationWithType(color_band, GDT_Byte);
		switch (color_interpretation)
		{
		case GCI_GrayIndex:
			raster.image_format = QImage::Format_ARGB32_Premultiplied;
			raster.postprocessing = GdalImageReader::premultiplyGray8;
			raster.pixel_space = 4;
			raster.band_offset = 2;  // store gray to red, alpha to alpha
			raster.bands.push_back(color_band);
			raster.bands.push_back(alpha_band);
			break;
		default:
			// unsupported
			break;
		}
	}
	else if (raster_count >= 3)
	{
		for (auto color_interpretation : { GCI_BlueBand, GCI_GreenBand, GCI_RedBand })
		{
			if (auto color_band = findRasterBand(color_interpretation, GDT_Byte))
				raster.bands.push_back(color_band);
		}
		if (raster.bands.count() == 3)
		{
			raster.pixel_space = 4;
			if (alpha_band)
			{
				raster.bands.push_back(alpha_band);
				raster.image_format = QImage::Format_ARGB32_Premultiplied;
				raster.postprocessing = GdalImageReader::premultiplyARGB32;
			}
			else
			{
				raster.image_format = QImage::Format_RGB32;
			}
		}
	}
	
	return raster;
}

QVector<QRgb> GdalImageReader::readColorTable(int band) const
{
	QVector<QRgb> palette;
	auto const raster_band = GDALGetRasterBand(dataset, band);
	auto const color_table = GDALGetRasterColorTable(raster_band);
	auto count = GDALGetColorEntryCount(color_table);
	palette.reserve(count);
	for (int i = 0; i < count; ++i)
	{
		GDALColorEntry entry;
		GDALGetColorEntryAsRGB(color_table, i, &entry);
		palette.push_back(qRgb(entry.c1, entry.c2, entry.c3));
	}
	return palette;
}

TemplateImage::GeoreferencingOption GdalImageReader::readGeoTransform()
{
	auto georef = TemplateImage::GeoreferencingOption {};
	if (dataset != nullptr)
	{
		auto geo_transform = std::array<double, 6> {};
		auto const result = GDALGetGeoTransform(dataset, geo_transform.data());
		if (result == CE_None)
		{
#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(3,0,0) && !defined(ACCEPT_USE_OF_DEPRECATED_PROJ_API_H)
			georef.crs_spec = toWkt(GDALGetSpatialRef(dataset));
#else
			georef.crs_spec = toProjSpec(GDALGetProjectionRef(dataset));
#endif
			georef.transform.source = GDALGetDriverShortName(GDALGetDatasetDriver(dataset));
			georef.transform.pixel_to_world = { geo_transform[1], geo_transform[2],
			                                    geo_transform[4], geo_transform[5],
			                                    geo_transform[0], geo_transform[3] };
		}
	}
	return georef;
}


// static
QString GdalImageReader::toProjSpec(const QByteArray& gdal_spec)
{
	auto const spatial_reference = OSRNewSpatialReference(gdal_spec);
	char* proj_spec_cstring;
	auto const ogr_error = OSRExportToProj4(spatial_reference, &proj_spec_cstring);
	auto result = QByteArray(ogr_error == OGRERR_NONE ? proj_spec_cstring : nullptr);
	CPLFree(proj_spec_cstring);
	OSRDestroySpatialReference(spatial_reference);
	result.replace("+k=0 ", "+k=1 ");  // https://github.com/OSGeo/PROJ/issues/1700
	return QString::fromUtf8(result);
}

// static
void GdalImageReader::noop(QImage& /*image*/)
{}

// static
void GdalImageReader::premultiplyARGB32(QImage &image)
{
	if (image.depth() != 32)
		return;
	
	auto const first = reinterpret_cast<QRgb*>(image.bits());
	auto const last = first + image.width() * image.height();
	std::transform(first, last, first, [](auto qrgb) { return qPremultiply(qrgb); });
}

// static
void GdalImageReader::premultiplyGray8(QImage &image)
{
	if (image.depth() != 32)
		return;
	
	auto const first = reinterpret_cast<QRgb*>(image.bits());
	auto const last = first + image.width() * image.height();
	std::transform(first, last, first, [](auto qrgb) {
		auto const gray = qRed(qrgb);
		return qPremultiply(qRgba(gray, gray, gray, qAlpha(qrgb))); 
	});
}



// ### Free function ###

TemplateImage::GeoreferencingOption readGdalGeoTransform(const QString& filepath)
{
	return GdalImageReader(filepath).readGeoTransform();
}


}  // namespace OpenOrienteering
