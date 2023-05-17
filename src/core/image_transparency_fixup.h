/*
 *    Copyright 2012, 2013 Kai Pastor
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

#ifndef OPENORIENTEERING_IMAGE_TRANSPARENCY_FIXUP_H
#define OPENORIENTEERING_IMAGE_TRANSPARENCY_FIXUP_H

#include <QImage>

namespace OpenOrienteering {


/**
 * ImageTransparencyFixup repairs a particular issue with composing
 * transparent pixels.
 * 
 * QPainter::CompositionMode_Multiply and QPainter::CompositionMode_Darken
 * on a QImage of Format_ARGB32_Premultiplied calculate the resulting alpha
 * channel in a very efficient but not accurate way. A particular case is the
 * composition of two fully transparent pixels which should in theory give a
 * fully transparent pixel. Qt yields an alpha value of 1 (in 0..255) instead.
 * The error accumulates with further compositions.
 * 
 * This class may be used as a functor on a particular image, providing a
 * comfortable way to fix the described case after each composition.
 *
 * Synopsis:
 *
 * QImage image(100, 100, QImage::Format_ARGB32_Premultiplied);
 * ImageTransparencyFixup fixup(&image);
 * QPainter p(&image);
 * p.setCompositionMode(QPainter::CompositionMode_Multiply);
 * p.drawImage(another_image);
 * p.end();
 * fixup();
 */
class ImageTransparencyFixup
{
public:
	/**
	 * Create a fixup functor for the given image.
	 * 
	 * The image must be of QImage::Format_ARGB32_Premultiplied.
	 * It may be null.
	 *
	 * This fixup is needed for Qt5 < 5.15.9 and Qt6 < 6.2.4 which are
	 * affected by https://bugreports.qt.io/browse/QTBUG-100327.
	 */
	inline ImageTransparencyFixup(QImage* image)
	: dest(0), dest_end(0)
	{
		// NOTE: Here we may add a check for a setting which disables the 
		//       fixup (for better application performance)
		if (image)
		{
			dest = (QRgb*)image->bits();
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
			dest_end = dest + image->sizeInBytes() / sizeof(QRgb);
#else
			dest_end = dest + image->byteCount() / sizeof(QRgb);
#endif
		}
	}
	
	/**
	 * Checks all pixels of the image for the known wrong result of composing
	 * fully transparent pixels, and replaces them with a fully transparent 
	 * pixel.
	 */
	inline void operator()() const
	{
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 9) || (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0) && QT_VERSION < QT_VERSION_CHECK(6, 2, 4))
		for (QRgb* px = dest; px < dest_end; px++)
		{
			if (*px == 0x01000000) /* qRgba(0, 0, 0, 1) */
				*px = 0x00000000;  /* qRgba(0, 0, 0, 0) */
		}
#endif
	}
	
protected:
	QRgb* dest;
	QRgb* dest_end;
};


}  // namespace OpenOrienteering

#endif
