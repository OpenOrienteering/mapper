/*
 *    Copyright 2020, 2026 Kai Pastor
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


#include <QtGlobal>
#include <QtTest>
#include <QColor>
#include <QDir>
#include <QFileInfo>
#include <QObject>
#include <QPainter>
#include <QPoint>
#include <QPrinterInfo>
#include <QString>

#include <global.h>
#include <core/map.h>
#include <core/map_printer.h>

#include <test_config.h>
#include "image-difference.h"

using namespace OpenOrienteering;


/**
 * @test Tests printing and export features
 */
class MapPrinterTest : public QObject
{
Q_OBJECT
private slots:
	void initTestCase()
	{
		QDir::addSearchPath(QStringLiteral("data"), QDir(QString::fromUtf8(MAPPER_TEST_SOURCE_DIR)).absoluteFilePath(QStringLiteral("..")));
		QDir::addSearchPath(QStringLiteral("testdata"), QDir(QString::fromUtf8(MAPPER_TEST_SOURCE_DIR)).absoluteFilePath(QStringLiteral("data")));
		doStaticInitializations();
	}
	
	void isPrinterTest()
	{
		QPrinterInfo printer_info;
		QCOMPARE(MapPrinter::isPrinter(&printer_info), true);
		QCOMPARE(MapPrinter::isPrinter(MapPrinter::imageTarget()), false);
		QCOMPARE(MapPrinter::isPrinter(MapPrinter::kmzTarget()), false);
		QCOMPARE(MapPrinter::isPrinter(MapPrinter::pdfTarget()), false);
	}
	
	/**
	 * (Old versions of) Qt do not correctly forward plain QRectF clip rects
	 * but round them to integers in broken ways. MapPrinter must fix this.
	 */
	void clipPathQuirkTest()
	{
		QString const map_filepath = QStringLiteral("testdata:/export/issue-1024-clipping.xmap");
		
		Map map;
		QVERIFY(map.loadFrom(map_filepath));
		
		MapPrinter printer(map, nullptr);
		printer.setTarget(MapPrinter::imageTarget());
		
		qreal pixel_per_mm = printer.getOptions().resolution / 25.4;
		int print_width = qRound(printer.getPrintAreaPaperSize().width() * pixel_per_mm);
		int print_height = qRound(printer.getPrintAreaPaperSize().height() * pixel_per_mm);
		QImage image(print_width, print_height, QImage::Format_ARGB32_Premultiplied);
		QVERIFY(!image.isNull());
		
		image.fill(QColor(Qt::white));
		{
			QPainter p(&image);
			printer.drawPage(&p, printer.getPrintArea(), &image);
		}
		
		auto image_filepath = QFileInfo{map_filepath}.absoluteFilePath();
		image_filepath.chop(4);
		image_filepath.append(QLatin1String{"png"});
		
#ifdef MAPPER_DEVELOPMENT_BUILD
		auto out_filename = image_filepath;
		out_filename.insert(image_filepath.length() - 4, QLatin1String(".tmp"));
		QVERIFY(image.save(out_filename, "PNG", 1));
#endif
		
		QVERIFY(QFileInfo::exists(image_filepath));
		
		QImage expected_image;
		QVERIFY(expected_image.load(image_filepath));
		
		image = image.convertToFormat(expected_image.format());
		QCOMPARE(fuzzyDifference(image, expected_image), QPoint(-1,-1));
	}
};



/*
 * We don't need a real GUI window.
 * 
 * But we discovered QTBUG-58768 macOS: Crash when using QPrinter
 * while running with "minimal" platform plugin.
 */
#ifndef Q_OS_MACOS
namespace  {
	auto Q_DECL_UNUSED qpa_selected = qputenv("QT_QPA_PLATFORM", "offscreen");  // clazy:exclude=non-pod-global-static
}
#endif


QTEST_MAIN(MapPrinterTest)
#include "map_printer_t.moc"  // IWYU pragma: keep
