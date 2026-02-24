/*
 *    Copyright 2018-2020, 2022, 2024, 2025 Kai Pastor
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

#include <initializer_list>
#include <memory>

#include <Qt>
#include <QtGlobal>
#include <QtTest>
#include <QByteArray>
#include <QColor>
#include <QDir>
#include <QFile>  // IWYU pragma: keep
#include <QFileInfo>
#include <QImage>
#include <QLatin1String>
#include <QObject>
#include <QPainter>
#include <QPoint>
#include <QRect>
#include <QRectF>
#include <QRgb>
#include <QSize>
#include <QString>

#include "global.h"
#include "test_config.h"
#include "core/map.h"
#include "core/map_color.h"
#include "core/renderables/renderable.h"
#include "core/symbols/area_symbol.h"
#include "core/symbols/combined_symbol.h"
#include "core/symbols/line_symbol.h"
#include "core/symbols/point_symbol.h"
#include "core/symbols/symbol.h"

using namespace OpenOrienteering;


// Uncomment to enable tests which produce unstable results,
// depending on architecture, compiler, Qt.
// #define ENABLE_VOLATILE_RENDER_TESTS

namespace
{

static const auto example_files = {
    "data:/examples/complete map.omap",
    "data:/examples/forest sample.omap",
    "data:/examples/overprinting.omap",
};


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
		struct
		{
			const char* path;
			qreal pixel_per_mm;
		} const render_test_files[] = {
		    { "testdata:symbols/area-symbol-line-pattern.omap", 80 },
		    { "testdata:symbols/line-symbol-border-variants.omap", 50 },
		    { "testdata:symbols/line-symbol-cap-variants.omap", 50 },
		    { "testdata:symbols/line-symbol-start-end-symbol.omap", 50 },
		    { "testdata:symbols/line-symbol-mid-symbol-variants.omap", 50 },
		    { "testdata:symbols/line-symbol-mid-symbol-at-gap.omap", 30 },
		    { "testdata:symbols/dashed-line-with-mid-symbols.omap", 80 },
#ifdef ENABLE_VOLATILE_RENDER_TESTS
		    { "data:examples/complete map.omap", 5 },
		    { "data:examples/forest sample.omap", 10 },
#endif  // ENABLE_VOLATILE_RENDER_TESTS
		};
		QTest::addColumn<QString>("map_filename");
		QTest::addColumn<qreal>("pixel_per_mm");
		for (auto test_file : render_test_files)
		{
			QTest::newRow(test_file.path) << QString::fromUtf8(test_file.path) << test_file.pixel_per_mm;
		}
	}
	
	void renderTest()
	{
		QFETCH(QString, map_filename);
		QFETCH(qreal, pixel_per_mm);
		
		Map map;
		QVERIFY(map.loadFrom(map_filename));
		
		// Don' test text symbols: text rendering requires more Qt GUI,
		// and it depends on fonts and platforms.
		for (auto i = 0; i < map.getNumSymbols(); ++i)
		{
			auto* symbol = map.getSymbol(i);
			if (symbol && symbol->getType() == Symbol::Text)
				symbol->setIsHelperSymbol(true);
		}
		
		const auto extent = map.calculateExtent().toAlignedRect().adjusted(-1, -1, +1, +1);
		
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
		QCOMPARE(fuzzyDifference(image, expected_image), QPoint(-1,-1));
		
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
		QVERIFY(map.loadFrom(map_filename));
		
		for (int i = 0; i < map.getNumSymbols(); ++i)
		{
			const auto* symbol = map.getSymbol(i);
			
			MapColor color;
			QVERIFY(!symbol->containsColor(&color));
			
			QVERIFY(!symbol->containsSymbol(symbol));
			
		}
	}
	
	void lineSymbolTest()
	{
		MapColor color;
		LineSymbol l;
		l.setColor(&color);
		l.setLineWidth(1.0);
		
		l.ensurePointSymbols(QStringLiteral("Start"), QStringLiteral("Mid"), QStringLiteral("End"), QStringLiteral("4"));
		QVERIFY(l.getStartSymbol());
		QVERIFY(l.getMidSymbol());
		QVERIFY(l.getEndSymbol());
		QVERIFY(l.getDashSymbol());
		
		l.cleanupPointSymbols();
		QVERIFY(!l.getStartSymbol());
		QVERIFY(!l.getMidSymbol());
		QVERIFY(!l.getEndSymbol());
		QVERIFY(!l.getDashSymbol());
		
		auto clone = duplicate(l);
		QVERIFY(clone->equals(&l));
		
		l.setName(QStringLiteral("A"));
		QVERIFY(!clone->equals(&l));
		
		clone->setName(QStringLiteral("a"));
		QVERIFY(!clone->equals(&l));
		QVERIFY(clone->equals(&l, Qt::CaseInsensitive));
		
		clone->setName(QStringLiteral("A"));
		QVERIFY(clone->equals(&l));
		
		clone->setStartSymbol(new PointSymbol());
		QVERIFY(clone->getStartSymbol()->isEmpty());
		QVERIFY(clone->equals(&l));
		clone->cleanupPointSymbols();
		QVERIFY(clone->equals(&l));
		
		l.setEndSymbol(new PointSymbol());
		QVERIFY(l.getEndSymbol()->isEmpty());
		QVERIFY(clone->equals(&l));
		l.cleanupPointSymbols();
		QVERIFY(clone->equals(&l));
		
		clone->setMidSymbol(new PointSymbol());
		QVERIFY(clone->getMidSymbol()->isEmpty());
		QVERIFY(clone->equals(&l));
		clone->cleanupPointSymbols();
		QVERIFY(clone->equals(&l));
		
		l.setDashSymbol(new PointSymbol());
		QVERIFY(l.getDashSymbol()->isEmpty());
		QVERIFY(clone->equals(&l));
		l.cleanupPointSymbols();
		QVERIFY(clone->equals(&l));
	}
	
	void CombinedSymbolTest()
	{
		CombinedSymbol c;
		c.setNumParts(10);
		
		Symbol& s = c;
		QCOMPARE(s.getMinimumLength(), 0);
		QCOMPARE(s.getMinimumArea(), 0);
		
		auto make_line_symbol = [](int len) {
			auto* l = new LineSymbol();
			l->setMinimumLength(len);
			return l;
		};
		c.setPart(1, make_line_symbol(18), true);
		c.setPart(2, make_line_symbol(20), true);
		c.setPart(3, make_line_symbol(19), true);
		
		auto make_area_symbol = [](int area) {
			auto* a = new AreaSymbol();
			a->setMinimumArea(area);
			return a;
		};
		c.setPart(5, make_area_symbol(28), true);
		c.setPart(6, make_area_symbol(30), true);
		c.setPart(7, make_area_symbol(29), true);
		
		QCOMPARE(s.getMinimumLength(), 20);
		QCOMPARE(s.getMinimumArea(), 30);
	}
	
	void lessByColorTest_data()
	{
		QTest::addColumn<QString>("map_filename");
		for (auto raw_path : example_files)
		{
			QTest::newRow(raw_path) << QString::fromUtf8(raw_path);
		}
	}
	
	void lessByColorTest()
	{
		QFETCH(QString, map_filename);
		Map map {};
		QVERIFY(map.loadFrom(map_filename));
		
		auto failed_compare = QString{};
		
		auto const less_by_color = Symbol::lessByColor(&map);
		auto const last = map.getNumSymbols();
		for (int i = 0; i < last; ++i)
		{
			const auto* a = map.getSymbol(i);
			for (int j = 0; j < last; ++j)
			{
				const auto* b = map.getSymbol(j);
				if (less_by_color(a, b) && less_by_color(b, a))
					failed_compare += a->getNumberAsString() + QLatin1String("<=>") + b->getNumberAsString() + QLatin1String("  ");
			}
		}
		QCOMPARE(failed_compare, QString{});
	}
	
	
	void getNumberAndPlainTextName_data()
	{
		QTest::addColumn<QString>("input");
		QTest::addColumn<QString>("number_and_name");
		QTest::newRow("simple") << QString::fromLatin1("Symbol 1") << QString::fromLatin1("1 Symbol 1");
		QTest::newRow("html tag") << QString::fromLatin1("<b>Symbol</b> 2") << QString::fromLatin1("1 Symbol 2");
		QTest::newRow("invalid html") << QString::fromLatin1("<b>Symbol 3") << QString::fromLatin1("1 Symbol 3");
		QTest::newRow("html entity")  << QString::fromLatin1("Symbol &nbsp; 4") << QString::fromLatin1("1 Symbol < 4");
	}
	
	void getNumberAndPlainTextName()
	{
		QFETCH(QString, input);
		QFETCH(QString, number_and_name);
		PointSymbol s;
		s.setNumberComponent(0, 1);
		s.setName(input);
		QEXPECT_FAIL("html entity", "HTML entities are not converted", Continue);
		QCOMPARE(s.getNumberAndPlainTextName(), number_and_name);
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


QTEST_MAIN(SymbolTest)
#include "symbol_t.moc"  // IWYU pragma: keep
