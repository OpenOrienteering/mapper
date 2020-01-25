/*
 * Copyright (c) 2005-2019 Libor Pecháček.
 * Copyright 2020 Kai Pastor
 *
 * This file is part of CoVe 
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ParallelImageProcessing.h"

#include <algorithm>

#include <QtGlobal>
#include <QImage>
#include <QRgb>


namespace cove {

// ### InplaceImage ###

InplaceImage::InplaceImage(const QImage& original, uchar* bits, int width, int height, int bytes_per_line, QImage::Format format)
    : QImage(bits, width, height, bytes_per_line, format)
{
	if (original.colorCount() > 0)
		setColorTable(original.colorTable());
}

InplaceImage::InplaceImage(const InplaceImage& source)
    : InplaceImage(source, const_cast<InplaceImage&>(source).bits(), source.width(), source.height(), source.bytesPerLine(), source.format())
{}

InplaceImage::InplaceImage(QImage& source)
    : InplaceImage(source, source.bits(), source.width(), source.height(), source.bytesPerLine(), source.format())
{}

InplaceImage& InplaceImage::operator=(const InplaceImage& source)
{
	*this = InplaceImage(source);
	return *this;
}

InplaceImage::~InplaceImage() = default;



// ### HorizontalStripes ###

QImage HorizontalStripes::makeStripe(const QImage& original, int scanline, int stripe_height)
{
	QImage result(original.constScanLine(scanline),		        
	              original.width(),
	              std::min(scanline + stripe_height, original.height()) - scanline,
	              original.bytesPerLine(),
	              original.format() );
	if (original.colorCount() > 0)
		result.setColorTable(original.colorTable());
	return result;
}

InplaceImage HorizontalStripes::makeStripe(QImage& original, int scanline, int stripe_height)
{
	InplaceImage result(original,
	                    original.scanLine(scanline),
	                    original.width(),
	                    std::min(scanline + stripe_height, original.height()) - scanline,
	                    original.bytesPerLine(),
	                    original.format() );
	return result;
}


}  // namespace cove

