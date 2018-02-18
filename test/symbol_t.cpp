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

#include <memory>

#include <Qt>
#include <QtGlobal>
#include <QtTest>
#include <QColor>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QLatin1String>
#include <QObject>
#include <QPainter>
#include <QPoint>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QString>

#include "global.h"
#include "test_config.h"
#include "core/map.h"
#include "core/map_color.h"
#include "core/renderables/renderable.h"
#include "core/symbols/symbol.h"

using namespace OpenOrienteering;


namespace
{

static const auto example_files = {
    "data:/examples/complete map.omap",
    "data:/examples/forest sample.omap",
    "data:/examples/overprinting.omap",
};


}  // namespace


/**
 * @test Tests symbols.
 */
class SymbolTest : public QObject
{
	Q_OBJECT
	
private slots:
	void initTestCase()
	{
		QDir::addSearchPath(QStringLiteral("data"), QDir(QString::fromUtf8(MAPPER_TEST_SOURCE_DIR)).absoluteFilePath(QStringLiteral("..")));
		QDir::addSearchPath(QStringLiteral("testdata"), QDir(QString::fromUtf8(MAPPER_TEST_SOURCE_DIR)).absoluteFilePath(QStringLiteral("data")));
		doStaticInitializations();
	}
	
	
	void numberEqualsTest_data()
	{
		QTest::addColumn<int>("a0");
		QTest::addColumn<int>("a1");
		QTest::addColumn<int>("a2");
		QTest::addColumn<int>("b0");
		QTest::addColumn<int>("b1");
		QTest::addColumn<int>("b2");
		QTest::addColumn<bool>("result_strict");
		QTest::addColumn<bool>("result_relaxed");
		
		QTest::newRow("2     | 0")     << 2 << -1 << -1    << 0 << -1 << -1    << false << false;
		QTest::newRow("2     | 2")     << 2 << -1 << -1    << 2 << -1 << -1    << true  << true;
		QTest::newRow("2     | 4")     << 2 << -1 << -1    << 4 << -1 << -1    << false << false;
		QTest::newRow("2     | 0.0")   << 2 << -1 << -1    << 0 <<  0 << -1    << false << false;
		QTest::newRow("2     | 0.2")   << 2 << -1 << -1    << 0 <<  2 << -1    << false << false;
		QTest::newRow("2     | 2.0")   << 2 << -1 << -1    << 2 <<  0 << -1    << false << true;
		QTest::newRow("2     | 2.2")   << 2 << -1 << -1    << 2 <<  2 << -1    << false << false;
		QTest::newRow("2     | 2.0.0") << 2 << -1 << -1    << 2 <<  0 <<  0    << false << true;
		QTest::newRow("2     | 2.0.2") << 2 << -1 << -1    << 2 <<  0 <<  2    << false << false;
		QTest::newRow("2.0   | 0.0")   << 2 <<  0 << -1    << 0 <<  0 << -1    << false << false;
		QTest::newRow("2.0   | 2.0")   << 2 <<  0 << -1    << 2 <<  0 << -1    << true  << true;
		QTest::newRow("2.0   | 2.0.0") << 2 <<  0 << -1    << 2 <<  0 <<  0    << false << true;
		QTest::newRow("2.0   | 2.0.2") << 2 <<  0 << -1    << 2 <<  0 <<  2    << false << false;
		QTest::newRow("2.2   | 0.0")   << 2 <<  2 << -1    << 0 <<  0 << -1    << false << false;
		QTest::newRow("2.2   | 0.2")   << 2 <<  2 << -1    << 0 <<  2 << -1    << false << false;
		QTest::newRow("2.2   | 2.0")   << 2 <<  2 << -1    << 2 <<  0 << -1    << false << false;
		QTest::newRow("2.2   | 2.2")   << 2 <<  2 << -1    << 2 <<  2 << -1    << true  << true;
		QTest::newRow("2.2   | 2.4")   << 2 <<  2 << -1    << 2 <<  4 << -1    << false << false;
		QTest::newRow("2.2   | 2.0.0") << 2 <<  2 << -1    << 2 <<  0 <<  0    << false << false;
		QTest::newRow("2.2   | 2.0.2") << 2 <<  2 << -1    << 2 <<  0 <<  2    << false << false;
		QTest::newRow("2.2   | 2.2.0") << 2 <<  2 << -1    << 2 <<  2 <<  0    << false << true;
		QTest::newRow("2.2   | 2.2.2") << 2 <<  2 << -1    << 2 <<  2 <<  2    << false << false;
		QTest::newRow("2.2.0 | 2.2.0") << 2 <<  2 <<  0    << 2 <<  2 <<  0    << true  << true;
		QTest::newRow("2.2.0 | 2.2.2") << 2 <<  2 <<  0    << 2 <<  2 <<  2    << false << false;
		QTest::newRow("2.2.2 | 2.2.2") << 2 <<  2 <<  2    << 2 <<  2 <<  2    << true  << true;
	}
	
	void numberEqualsTest()
	{
		auto symbol_a = Symbol::makeSymbolForType(Symbol::Point);
		QFETCH(int, a0);
		symbol_a->setNumberComponent(0, a0);
		QFETCH(int, a1);
		symbol_a->setNumberComponent(1, a1);
		QFETCH(int, a2);
		symbol_a->setNumberComponent(2, a2);
		
		auto symbol_b = Symbol::makeSymbolForType(Symbol::Point);
		QFETCH(int, b0);
		symbol_b->setNumberComponent(0, b0);
		QFETCH(int, b1);
		symbol_b->setNumberComponent(1, b1);
		QFETCH(int, b2);
		symbol_b->setNumberComponent(2, b2);
		
		QFETCH(bool, result_strict);
		QCOMPARE(symbol_a->numberEquals(symbol_b.get()), result_strict);
		QCOMPARE(symbol_b->numberEquals(symbol_a.get()), result_strict);
		QFETCH(bool, result_relaxed);
		QCOMPARE(symbol_a->numberEqualsRelaxed(symbol_b.get()), result_relaxed);
		QCOMPARE(symbol_b->numberEqualsRelaxed(symbol_a.get()), result_relaxed);
	}
	
	
	void renderTest_data()
	{
		const auto render_test_files = {
		    "testdata:symbols/line-symbol-border-variants.omap",
		    "testdata:symbols/line-symbol-start-end-symbol.omap",
		    "testdata:symbols/line-symbol-mid-symbol-variants.omap",
		};
		QTest::addColumn<QString>("map_filename");
		for (auto raw_path : render_test_files)
		{
			QTest::newRow(raw_path) << QString::fromUtf8(raw_path);
		}
	}
	
	void renderTest()
	{
		QFETCH(QString, map_filename);
		
		Map map;
		QVERIFY(map.loadFrom(map_filename, nullptr, nullptr, false, false));
		
		const auto extent = map.calculateExtent().toAlignedRect().adjusted(-1, -1, +1, +1);
		constexpr auto pixel_per_mm = 50;
		
		auto image = QImage{pixel_per_mm * extent.size(), QImage::Format_ARGB32_Premultiplied};
		image.fill(QColor(Qt::white));
		
		QPainter painter{&image};
		painter.setRenderHint(QPainter::Antialiasing, false);
		painter.scale(pixel_per_mm, pixel_per_mm);
		painter.translate(-extent.topLeft());
		painter.setClipRect(extent);
		map.draw(&painter, RenderConfig{map, extent, pixel_per_mm, RenderConfig::DisableAntialiasing, 1});
		painter.end();
		
		auto image_filename = QFileInfo{map_filename}.absoluteFilePath();
		image_filename.chop(4);
		image_filename.append(QLatin1String{"png"});
		
#ifdef MAPPER_DEVELOPMENT_BUILD
		auto out_filename = image_filename;
		out_filename.insert(image_filename.length() - 4, QLatin1String(".tmp"));
		QVERIFY(image.save(out_filename, "PNG", 1));
#endif
		
		QVERIFY(QFileInfo::exists(image_filename));
		QImage expected_image;
		QVERIFY(expected_image.load(image_filename));
		image = image.convertToFormat(expected_image.format());
		QCOMPARE(image, expected_image);
		
#ifdef MAPPER_DEVELOPMENT_BUILD
		QVERIFY(QFile::remove(out_filename));
#endif
	}
	
	
	void invariantTest_data()
	{
		QTest::addColumn<QString>("map_filename");
		for (auto raw_path : example_files)
		{
			QTest::newRow(raw_path) << QString::fromUtf8(raw_path);
		}
	}
	
	void invariantTest()
	{
		QFETCH(QString, map_filename);
		Map map {};
		QVERIFY(map.loadFrom(map_filename, nullptr, nullptr, false, false));
		
		for (int i = 0; i < map.getNumSymbols(); ++i)
		{
			const auto symbol = map.getSymbol(i);
			
			MapColor color;
			QVERIFY(!symbol->containsColor(&color));
			
			QVERIFY(!symbol->containsSymbol(symbol));
			
		}
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
	auto qpa_selected = qputenv("QT_QPA_PLATFORM", "minimal");  // clazy:exclude=non-pod-global-static
}
#endif


QTEST_MAIN(SymbolTest)
#include "symbol_t.moc"  // IWYU pragma: keep
