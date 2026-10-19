/*
 *    Copyright 2018 Kai Pastor
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


#ifndef OPENORIENTEERING_IMAGE_DIFFERENCE_H
#define OPENORIENTEERING_IMAGE_DIFFERENCE_H

#include <QtGlobal>

class QImage;
class QPoint;


namespace OpenOrienteering {

/**
 * Tests if any pixel in the image region given by x0, y0, x1, y1
 * approximately matches the expected value.
 */
bool fuzzyComparePixel(const QImage& image, int x0, int y0, int x1, int y1, const quint32 expected);

/**
 * Tests if the images are different, ignoring edges which differ in location by one pixel.
 *
 * Returns { -1, -1 } when the images are equal,
 * the minimum width and height when the images have different sizes,
 * or the coordinates of the first pixel which does not match (fuzzily).
 * In the second case, it uses QCOMPARE to output the sizes.
 * In the last case, it uses QCOMPARE to output the pixel values.
 */
QPoint fuzzyDifference(const QImage& lhs, const QImage& rhs);

}  // namespace OpenOrienteering

#endif  // OPENORIENTEERING_IMAGE_DIFFERENCE_H
