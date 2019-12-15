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

#ifndef OPENORIENTEERING_GDAL_IMAGE_READER_H
#define OPENORIENTEERING_GDAL_IMAGE_READER_H

#include <QByteArray>
#include <QCoreApplication>
#include <QImage>
#include <QImageReader>
#include <QSize>
#include <QString>
#include <QVarLengthArray>

#include <gdal.h>

#include "templates/template_image.h"

namespace OpenOrienteering {


/**
 * A GDAL image reader modeled like QImageReader.
 */
class GdalImageReader
{
public:
	GdalImageReader() = delete;
	GdalImageReader(const GdalImageReader&) = delete;
	GdalImageReader(GdalImageReader&&) = delete;
	GdalImageReader& operator=(const GdalImageReader&) = delete;
	GdalImageReader& operator=(GdalImageReader&&) = delete;
	
	// QImageReader related API
	
	explicit GdalImageReader(const QString& path);
	
	~GdalImageReader();
	
	bool canRead() const;
	
	QImageReader::ImageReaderError error() const;
	
	QString errorString() const;
	
	QString filename() const;
	
	QByteArray format() const;
	
	QImage read();
	
	bool read(QImage* image);
	
	
	// GDAL related API
	
	int findRasterBand(GDALColorInterp color_interpretation) const;
	
	struct RasterInfo
	{
		QVarLengthArray<int, 4> bands;
		QSize size;
		QImage::Format image_format = QImage::Format_Invalid;
		int pixel_space = 1;   ///< The byte offset from one pixel to the next one.
		int band_space  = 1;   ///< The in-pixel byte offset from one band value to the next one.
		int band_offset = 0;   ///< The in-pixel byte offset of the first band value.
	};
	
	RasterInfo readRasterInfo() const;
	
	/**
	 * Returns the file's geotransform in a type suitable for TemplateImage.
	 * 
	 * For valid data, the result's type member is set to Georeferencing_GDAL.
	 */
	TemplateImage::GeoreferencingOption readGeoTransform();
	
	static QString toProjSpec(const QByteArray& gdal_spec);
	
	
	// Translation
	
	Q_DECLARE_TR_FUNCTIONS(GdalImageReader)
	
	
private:
	QString path;
	QImageReader::ImageReaderError err = QImageReader::UnknownError;
	QString error_string;
	GDALDatasetH dataset = nullptr;
	int raster_count = 0;
	
};


/**
 * Returns the file's geotransform via GDAL in a type suitable for TemplateImage.
 * 
 * This is a convienence function which can beforward-declared to avoid the
 * dependency on GDAL header files which results from including this header.
 * 
 * \see GdalImageReader::readGeoTransform()
 */
TemplateImage::GeoreferencingOption readGdalGeoTransform(const QString& filepath);


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_GDAL_IMAGE_READER_H
