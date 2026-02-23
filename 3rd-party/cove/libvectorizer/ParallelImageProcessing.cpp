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

// ### HorizontalStripes ###

namespace {

QImage withoutColorTable(const QImage& original, int scanline, int stripe_height)
{
	return QImage(original.constScanLine(scanline),
	              original.width(),
	              std::min(scanline + stripe_height, original.height()) - scanline,
	              original.bytesPerLine(),
	              original.format());
}

QImage withColorTable(const QImage& original, int scanline, int stripe_height)
{
	// Cf. https://bugreports.qt.io/browse/QTBUG-81674 (fixed in Qt 6.4)
	// Setting the color table would cause detaching if we use the
	// const char* taking constructor, so we const_cast<char*> to select
	// the other constructor.
	// But now we need a way to enforce detaching on non-const access to the
	// returned image. We achieve this by creating a shared copy of the
	// returned image carried in the cleanup data, and thus bound to its
	// lifetime.
	auto* cleanup_data = new QImage();
	auto cleanup_function = [](void* data) { delete static_cast<QImage*>(data); };
	QImage stripe(const_cast<uchar*>(original.constScanLine(scanline)),
	              original.width(),
	              std::min(scanline + stripe_height, original.height()) - scanline,
	              original.bytesPerLine(),
	              original.format(),
	              cleanup_function,
	              cleanup_data);
	stripe.setColorTable(original.colorTable());
	*cleanup_data = stripe;  // increase reference count -> force detach on write.
	return stripe;
};

}  // namespace

QImage HorizontalStripes::makeStripe(const QImage& original, int scanline, int stripe_height)
{
	auto const & delegate = original.colorCount() == 0 ? withoutColorTable : withColorTable;
	return delegate(original, scanline, stripe_height);
}

QImage HorizontalStripes::makeStripe(QImage& original, int scanline, int stripe_height)
{
	QImage result(original.scanLine(scanline),
	              original.width(),
	              std::min(scanline + stripe_height, original.height()) - scanline,
	              original.bytesPerLine(),
	              original.format() );
	if (original.colorCount() > 0)
		result.setColorTable(original.colorTable());
	return result;
}


}  // namespace cove

