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


#include "image-difference.h"

#include <QtTest>
#include <QImage>
#include <QPoint>
#include <QString>


namespace OpenOrienteering {

/**
 * Tests if any pixel in the image region given by x0, y0, x1, y1
 * approximately matches the expected value.
 */
bool fuzzyComparePixel(const QImage& image, int x0, int y0, int x1, int y1, const quint32 expected)
{
	for (int y = y0; y < y1; ++y)
	{
		const auto* scanline = reinterpret_cast<const quint32*>(image.scanLine(y));
		for (int x = x0; x < x1; ++x)
		{
			if (scanline[x] == expected)
				return true;
		}
	}
	for (int y = y0; y < y1; ++y)
	{
		const auto* scanline = reinterpret_cast<const quint32*>(image.scanLine(y));
		for (int x = x0; x < x1; ++x)
		{
			const auto actual = scanline[x];
			if (qAbs(qRed(actual) - qRed(expected)) <= 1
			    && qAbs(qGreen(actual) - qGreen(expected)) <= 1
			    && qAbs(qBlue(actual)  - qBlue(expected))  <= 1
			    && qAbs(qAlpha(actual) - qAlpha(expected)) <= 1)
				return true;
		}
	}
	return false;
}


/**
 * Tests if the images are different, ignoring edges which differ in location by one pixel.
 * 
 * Returns { -1, -1 } when the images are equal,
 * the minimum width and height when the images have different sizes,
 * or the coordinates of the first pixel which does not match (fuzzily).
 * In the second case, it uses QCOMPARE to output the sizes.
 * In the last case, it uses QCOMPARE to output the pixel values.
 */
QPoint fuzzyDifference(const QImage& lhs, const QImage& rhs)
{
	if (lhs.size() != rhs.size())
	{
		auto actual_image = lhs.size();
		auto expected_image = rhs.size();
		[actual_image, expected_image](){ QCOMPARE(actual_image, expected_image); }();
		return { qMin(lhs.width(), rhs.width()), qMin(lhs.height(), rhs.height()) };
	}
	
	for (auto y = 0; y < lhs.height(); ++y)
	{
		const auto* left = reinterpret_cast<const quint32*>(lhs.scanLine(y));
		const auto* right = reinterpret_cast<const quint32*>(rhs.scanLine(y));
		for (auto x = 0; x < lhs.width(); ++x)
		{
			if (Q_UNLIKELY(left[x] != right[x])
			    && !fuzzyComparePixel(lhs, qMax(x-1, 0), qMax(y-1, 0), qMin(x+2, lhs.width()), qMin(y+2, lhs.height()), right[x]))
			{
				auto actual = QString::number(left[x], 16).toLatin1();
				auto actual_rgb = actual.constData();
				auto expected = QString::number(right[x], 16).toLatin1();
				auto expected_rgb = expected.constData();
				[actual_rgb, expected_rgb](){ QCOMPARE(actual_rgb, expected_rgb); }();
				return { x, y };
			}
		}
	}
	return { -1, -1 };
}

}  // namespace OpenOrienteering
