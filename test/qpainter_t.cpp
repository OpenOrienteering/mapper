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


#include "qpainter_t.h"

#include <Qt>
#include <QtTest>
#include <QRgb>

#include "core/image_transparency_fixup.h"

using namespace OpenOrienteering;


QPainterTest::QPainterTest(QObject* parent)
: QObject(parent),
  white_img(makeImage(Qt::white)),
  black_img(makeImage(Qt::black)),
  trans_img(makeImage(Qt::transparent))
{
	// nothing
}

void QPainterTest::initTestCase()
{
	QCOMPARE(white_img.pixel(0,0), qRgba(255, 255, 255, 255));
	QCOMPARE(trans_img.pixel(0,0), qRgba(0, 0, 0, 0));
	QCOMPARE(black_img.pixel(0,0), qRgba(0, 0, 0, 255));
}

void QPainterTest::sourceOverCompostion()
{
	const QPainter::CompositionMode source_over = QPainter::CompositionMode_SourceOver;
	
	QCOMPARE(compose(white_img, black_img, source_over).pixel(0,0), qRgba(255, 255, 255, 255));
	QCOMPARE(compose(black_img, white_img, source_over).pixel(0,0), qRgba(0, 0, 0, 255));
	
	QCOMPARE(compose(white_img, trans_img, source_over).pixel(0,0), qRgba(255, 255, 255, 255));
	QCOMPARE(compose(trans_img, white_img, source_over).pixel(0,0), qRgba(255, 255, 255, 255));
	
	QCOMPARE(compose(black_img, trans_img, source_over).pixel(0,0), qRgba(0, 0, 0, 255));
	QCOMPARE(compose(trans_img, black_img, source_over).pixel(0,0), qRgba(0, 0, 0, 255));
	
	QCOMPARE(compose(white_img, white_img, source_over).pixel(0,0), qRgba(255, 255, 255, 255));
	QCOMPARE(compose(black_img, black_img, source_over).pixel(0,0), qRgba(0, 0, 0, 255));
	
	QCOMPARE(compose(trans_img, trans_img, source_over).pixel(0,0), qRgba(0, 0, 0, 0));
}

void QPainterTest::multiplyComposition()
{
	const QPainter::CompositionMode multiply = QPainter::CompositionMode_Multiply;
	
	QCOMPARE(compose(white_img, black_img, multiply).pixel(0,0), qRgba(0, 0, 0, 255));
	QCOMPARE(compose(black_img, white_img, multiply).pixel(0,0), qRgba(0, 0, 0, 255));
	
	QCOMPARE(compose(white_img, trans_img, multiply).pixel(0,0), qRgba(255, 255, 255, 255));
	QCOMPARE(compose(trans_img, white_img, multiply).pixel(0,0), qRgba(255, 255, 255, 255));
	
	QCOMPARE(compose(black_img, trans_img, multiply).pixel(0,0), qRgba(0, 0, 0, 255));
	QCOMPARE(compose(trans_img, black_img, multiply).pixel(0,0), qRgba(0, 0, 0, 255));
	
	QCOMPARE(compose(white_img, white_img, multiply).pixel(0,0), qRgba(255, 255, 255, 255));
	QCOMPARE(compose(black_img, black_img, multiply).pixel(0,0), qRgba(0, 0, 0, 255));
	
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 9) || (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0) && QT_VERSION < QT_VERSION_CHECK(6, 2, 4))
	QEXPECT_FAIL("", "CompositionMode_Multiply incorrectly composes full transparency.", Continue);
#endif
	QCOMPARE(compose(trans_img, trans_img, multiply).pixel(0,0), qRgba(0, 0, 0, 0));
	
	// ImageTransparencyFixup fixes the particular issue.
	QImage result = compose(trans_img, trans_img, multiply);
	ImageTransparencyFixup fixup(&result);
	fixup();
	QCOMPARE(result.pixel(0,0), qRgba(0, 0, 0, 0)); // Now correct!
}

void QPainterTest::darkenComposition()
{
	const QPainter::CompositionMode darken = QPainter::CompositionMode_Darken;
	
	QCOMPARE(compose(white_img, black_img, darken).pixel(0,0), qRgba(0, 0, 0, 255));
	QCOMPARE(compose(black_img, white_img, darken).pixel(0,0), qRgba(0, 0, 0, 255));
	
	QCOMPARE(compose(white_img, trans_img, darken).pixel(0,0), qRgba(255, 255, 255, 255));
	QCOMPARE(compose(trans_img, white_img, darken).pixel(0,0), qRgba(255, 255, 255, 255));
	
	QCOMPARE(compose(black_img, trans_img, darken).pixel(0,0), qRgba(0, 0, 0, 255));
	QCOMPARE(compose(trans_img, black_img, darken).pixel(0,0), qRgba(0, 0, 0, 255));
	
	QCOMPARE(compose(white_img, white_img, darken).pixel(0,0), qRgba(255, 255, 255, 255));
	QCOMPARE(compose(black_img, black_img, darken).pixel(0,0), qRgba(0, 0, 0, 255));
	
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 9) || (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0) && QT_VERSION < QT_VERSION_CHECK(6, 2, 4))
	QEXPECT_FAIL("", "CompositionMode_Darken incorrectly composes full transparency.", Continue);
#endif
	QCOMPARE(compose(trans_img, trans_img, darken).pixel(0,0), qRgba(0, 0, 0, 0));
	
	// ImageTransparencyFixup fixes the particular issue.
	QImage result = compose(trans_img, trans_img, darken);
	ImageTransparencyFixup fixup(&result);
	fixup();
	QCOMPARE(result.pixel(0,0), qRgba(0, 0, 0, 0)); // Now correct!
}

template <typename ColorT>
QImage QPainterTest::makeImage(ColorT color) const
{
	QImage image(1, 1, QImage::Format_ARGB32_Premultiplied);
	image.fill(color);
	return image;
}

QImage QPainterTest::compose(const QImage& source, const QImage& dest, QPainter::CompositionMode mode)
{
	QImage result = dest;
	QPainter painter(&result);
	painter.setCompositionMode(mode);
	painter.drawImage(0, 0, source);
	painter.end();
	return result;
}


QTEST_GUILESS_MAIN(QPainterTest)
