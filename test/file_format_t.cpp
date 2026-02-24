/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2021, 2024, 2025 Kai Pastor
 *    Copyright 2025 Matthias Kühlewein
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

#include "file_format_t.h"

#include <algorithm>
// IWYU pragma: no_include <array>
#include <cstddef>
// IWYU pragma: no_include <initializer_list>
#include <limits>
#include <memory>
#include <stdexcept>
// IWYU pragma: no_include <type_traits>
#include <utility>
#include <vector>

#include <Qt>
#include <QtGlobal>
#include <QtTest>
#include <QBuffer>
#include <QByteArray>
#include <QByteRef>
#include <QChar>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFlags>
#include <QIODevice>
#include <QLatin1Char>
#include <QLatin1String>
#include <QMetaObject>
#include <QMetaType>
#include <QPageSize>
#include <QPoint>
#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QStringList>
#include <QStringRef>
#include <QTemporaryDir>
#include <QVariant>

#include "global.h"
#include "test_config.h"
#include "core/georeferencing.h"
#include "core/latlon.h"
#include "core/map.h"
#include "core/map_color.h"
#include "core/map_coord.h"
#include "core/map_grid.h"
#include "core/map_part.h"
#include "core/map_printer.h"
#include "core/objects/object.h"
#include "core/objects/text_object.h"
#include "core/symbols/area_symbol.h"
#include "core/symbols/line_symbol.h"
#include "core/symbols/symbol.h"
#include "core/symbols/text_symbol.h"
#include "fileformats/file_format.h"
#include "fileformats/file_format_registry.h"
#include "fileformats/file_import_export.h"
#include "fileformats/iof_course_export.h"
#include "fileformats/kml_course_export.h"
#include "fileformats/ocd_file_export.h"
#include "fileformats/ocd_file_format.h"
#include "fileformats/ocd_file_import.h"
#include "fileformats/ocd_types.h"
#include "fileformats/ocd_types_v8.h"
#include "fileformats/ocd_types_v12.h"
#include "fileformats/simple_course_export.h"
#include "fileformats/xml_file_format.h"
#include "templates/template.h"
#include "undo/undo.h"
#include "undo/undo_manager.h"
#include "util/backports.h"  // IWYU pragma: keep

#ifdef MAPPER_USE_GDAL
#  include "gdal/gdal_manager.h"
#endif

using namespace OpenOrienteering;


#ifdef QT_PRINTSUPPORT_LIB

namespace QTest
{
	/*
	 * This debug helper must use the parameter name 't' in order to avoid
	 * a warning about a difference between declaration and definition.
	 */
	template<>
	char* toString(const MapPrinterPageFormat& t)
	{
		const auto& page_format = t;
		QByteArray ba = "";
		ba += qPrintable(QPageSize::key(page_format.page_size));
		ba += (page_format.orientation == MapPrinterPageFormat::Landscape) ? " landscape (" : " portrait (";
		ba += QByteArray::number(page_format.paper_dimensions.width(), 'f', 2) + "x";
		ba += QByteArray::number(page_format.paper_dimensions.height(), 'f', 2) + "), ";
		ba += QByteArray::number(page_format.page_rect.left(), 'f', 2) + ",";
		ba += QByteArray::number(page_format.page_rect.top(), 'f', 2) + "+";
		ba += QByteArray::number(page_format.page_rect.width(), 'f', 2) + "x";
		ba += QByteArray::number(page_format.page_rect.height(), 'f', 2) + ", overlap ";
		ba += QByteArray::number(page_format.h_overlap, 'f', 2) + ",";
		ba += QByteArray::number(page_format.v_overlap, 'f', 2);
		return qstrdup(ba.data());
	}
	
	/*
	 * This debug helper must use the parameter name 't' in order to avoid
	 * a warning about a difference between declaration and definition.
	 */
	template<>
	char* toString(const MapPrinterOptions& t)
	{
		const auto& options = t;
		QByteArray ba = "";
		ba += "1:" + QByteArray::number(options.scale) + ", ";
		ba += QByteArray::number(options.resolution) + " dpi, ";
		if (options.show_templates)
			ba += ", templates";
		if (options.show_grid)
			ba += ", grid";
		if (options.simulate_overprinting)
			ba += ", overprinting";
		return qstrdup(ba.data());
	}
}  // namespace QTest

#endif



namespace
{
	/**
	 * Provides QCOMPARE-style symbol property comparison.
	 * 
	 * This macro reports the symbol, but avoids expensive string operations
	 * when the properties match.
	 */
	#define COMPARE_SYMBOL_PROPERTY(a, b, symbol) \
	{ \
		if ((a) == (b)) \
		{ \
			QVERIFY(true);  /* for QEXPECT_FAIL etc. */ \
		} \
		else \
		{ \
			auto const diff = qstrlen(#b) - qstrlen(#a); \
			auto const fill_a = QString().fill(QChar::Space, +diff); \
			auto const fill_b = QString().fill(QChar::Space, -diff); \
			QFAIL(QString::fromLatin1( \
			   "Compared values are not the same (%1)\n   Actual   (%2)%3: %6\n   Expected (%4)%5: %7") \
			  .arg((symbol).getNumberAndPlainTextName(), \
			       QString::fromUtf8(#a), fill_a, \
			       QString::fromUtf8(#b), fill_b) \
			  .arg(a).arg(b) \
			  .toUtf8()); \
		} \
	}
	
	/**
	 * Provides QVERIFY-style symbol property verification.
	 * 
	 * This macro reports the symbol on failure, but
	 * avoids expensive string operations when the properties match.
	 */
	#define VERIFY_SYMBOL_PROPERTY(cond, symbol) \
	{ \
		if (cond) \
		{ \
			QVERIFY(true);  /* for QEXPECT_FAIL etc. */ \
		} \
		else \
		{ \
			QVERIFY2(cond, QByteArray((symbol).getNumberAndPlainTextName().toUtf8())); \
		} \
	}
	
	
	void comparePrinterConfig(const MapPrinterConfig& copy, const MapPrinterConfig& orig)
	{
		QCOMPARE(copy.center_print_area, orig.center_print_area);
		QCOMPARE(copy.options, orig.options);
		QCOMPARE(copy.page_format, orig.page_format);
		// Compare print area with reasonable precision, here: 0.1 mm
		QCOMPARE((copy.print_area.topLeft()*10.0).toPoint(), (orig.print_area.topLeft()*10.0).toPoint());
		QCOMPARE((copy.print_area.size()*10.0).toSize(), (orig.print_area.size()*10.0).toSize());
		QCOMPARE(copy.printer_name, orig.printer_name);
		QCOMPARE(copy.single_page_print_area, orig.single_page_print_area);
	}
	
	void compareSymbol(const Symbol& actual, const Symbol& expected)
	{
		COMPARE_SYMBOL_PROPERTY(actual.isHidden(), expected.isHidden(), expected);
		COMPARE_SYMBOL_PROPERTY(actual.isProtected(), expected.isProtected(), expected);
		VERIFY_SYMBOL_PROPERTY(actual.stateEquals(&expected), expected);
		VERIFY_SYMBOL_PROPERTY(actual.equals(&expected, Qt::CaseInsensitive), expected);
	}
	
#define COMPARE_AREA_SYMBOL_PATTERN_PROPERTY(property) \
	COMPARE_SYMBOL_PROPERTY(actual_pattern.property, expected_pattern.property, symbol)
	
	void compareAreaSymbolPattern(const AreaSymbol::FillPattern& actual_pattern, const AreaSymbol::FillPattern& expected_pattern, const AreaSymbol& symbol)
	{
		// cf. AreaSymbol::FillPattern::equals
		COMPARE_AREA_SYMBOL_PATTERN_PROPERTY(type);
		COMPARE_AREA_SYMBOL_PATTERN_PROPERTY(line_spacing);
		COMPARE_AREA_SYMBOL_PATTERN_PROPERTY(line_offset);
		
		// OCD treats all patterns as rotatable.
		COMPARE_SYMBOL_PROPERTY(actual_pattern.flags,
		                        expected_pattern.flags | AreaSymbol::FillPattern::Rotatable,
		                        symbol
		)
		
		switch(expected_pattern.type)
		{
		case AreaSymbol::FillPattern::PointPattern:
			COMPARE_AREA_SYMBOL_PATTERN_PROPERTY(offset_along_line);
			COMPARE_AREA_SYMBOL_PATTERN_PROPERTY(point_distance);
			COMPARE_SYMBOL_PROPERTY(bool(actual_pattern.point), bool(expected_pattern.point), symbol);
			if(actual_pattern.point)
				QVERIFY(actual_pattern.point->equals(expected_pattern.point));
			break;
		case AreaSymbol::FillPattern::LinePattern:
			COMPARE_SYMBOL_PROPERTY(actual_pattern.line_color->operator QRgb(), expected_pattern.line_color->operator QRgb(), symbol);
			COMPARE_AREA_SYMBOL_PATTERN_PROPERTY(line_width);
			break;
		}
	}
	
	void compareMaps(const Map& actual, const Map& expected)
	{
		// TODO: This does not compare everything - yet ...
		
		// Miscellaneous
		QCOMPARE(actual.getScaleDenominator(), expected.getScaleDenominator());
		QCOMPARE(actual.getMapNotes(), expected.getMapNotes());
		
		const auto& actual_georef = actual.getGeoreferencing();
		const auto& expected_georef = expected.getGeoreferencing();
		QCOMPARE(actual_georef.getScaleDenominator(), expected_georef.getScaleDenominator());
		QCOMPARE(actual_georef.getState(), expected_georef.getState());
		QCOMPARE(actual_georef.getCombinedScaleFactor(), expected_georef.getCombinedScaleFactor());
		QCOMPARE(actual_georef.getAuxiliaryScaleFactor(), expected_georef.getAuxiliaryScaleFactor());
		QCOMPARE(actual_georef.getDeclination(), expected_georef.getDeclination());
		QCOMPARE(actual_georef.getGrivation(), expected_georef.getGrivation());
		QCOMPARE(actual_georef.getMapRefPoint(), expected_georef.getMapRefPoint());
		QCOMPARE(actual_georef.getProjectedRefPoint(), expected_georef.getProjectedRefPoint());
		QCOMPARE(actual_georef.getProjectedCRSId(), expected_georef.getProjectedCRSId());
		QCOMPARE(actual_georef.getProjectedCRSName(), expected_georef.getProjectedCRSName());
		QCOMPARE(actual_georef.getProjectedCoordinatesName(), expected_georef.getProjectedCoordinatesName());
		QCOMPARE(actual_georef.getProjectedCRSSpec(), expected_georef.getProjectedCRSSpec());
		QVERIFY(qAbs(actual_georef.getGeographicRefPoint().latitude() - expected_georef.getGeographicRefPoint().latitude()) < 0.5e-8);
		QVERIFY(qAbs(actual_georef.getGeographicRefPoint().longitude() - expected_georef.getGeographicRefPoint().longitude()) < 0.5e-8);
		
		QCOMPARE(actual.getGrid(), expected.getGrid());
		QCOMPARE(actual.isAreaHatchingEnabled(), expected.isAreaHatchingEnabled());
		QCOMPARE(actual.isBaselineViewEnabled(), expected.isBaselineViewEnabled());
		
		// Colors
		if (actual.getNumColors() != expected.getNumColors())
		{
			QCOMPARE(actual.getNumColors(), expected.getNumColors());
		}
		else for (int i = 0; i < actual.getNumColors(); ++i)
		{
			const auto& actual_color = *actual.getColor(i);
			const auto& expected_color = *expected.getColor(i);
			QCOMPARE(actual_color, expected_color);
			if (expected_color.getSpotColorMethod() == MapColor::SpotColor)
			{
				QCOMPARE(actual_color.getScreenAngle(), expected_color.getScreenAngle());
				QCOMPARE(actual_color.getScreenFrequency(), expected_color.getScreenFrequency());
			}
		}
		
		// Symbols
		if (actual.getNumSymbols() != expected.getNumSymbols())
		{
			QCOMPARE(actual.getNumSymbols(), expected.getNumSymbols());
		}
		else for (int i = 0; i < actual.getNumSymbols(); ++i)
		{
			compareSymbol(*actual.getSymbol(i), *expected.getSymbol(i));
		}
		
		// Parts and objects
		QCOMPARE(actual.getCurrentPartIndex(), expected.getCurrentPartIndex());
		if (actual.getNumParts() != expected.getNumParts())
		{
			QCOMPARE(actual.getNumParts(), expected.getNumParts());
		}
		else for (auto p = 0u; p < static_cast<decltype(p)>(actual.getNumParts()); ++p)
		{
			const auto& actual_part = *actual.getPart(p);
			const auto& expected_part = *expected.getPart(p);
			QVERIFY(actual_part.getName().compare(expected_part.getName(), Qt::CaseSensitive) == 0);
			if (actual_part.getNumObjects() != expected_part.getNumObjects())
			{
				QCOMPARE(actual_part.getNumObjects(), expected_part.getNumObjects());
			}
			else for (int i = 0; i < actual_part.getNumObjects(); ++i)
			{
				QVERIFY(actual_part.getObject(i)->equals(expected_part.getObject(i), true));
			}
		}
		
		// Object selection
		QCOMPARE(actual.getNumSelectedObjects(), expected.getNumSelectedObjects());
		if (actual.getFirstSelectedObject() == nullptr)
		{
			QCOMPARE(actual.getFirstSelectedObject(), expected.getFirstSelectedObject());
		}
		else
		{
			QCOMPARE(actual.getCurrentPart()->findObjectIndex(actual.getFirstSelectedObject()),
			         expected.getCurrentPart()->findObjectIndex(expected.getFirstSelectedObject()));
			if (actual.getCurrentPart()->getNumObjects() != expected.getCurrentPart()->getNumObjects())
			{
				for (auto object_index : qAsConst(actual.selectedObjects()))
				{
					QVERIFY(actual.isObjectSelected(actual.getCurrentPart()->getObject(expected.getCurrentPart()->findObjectIndex(object_index))));
				}
			}
		}
		
		// Undo steps
		// TODO: Currently only the number of steps is compared here.
		QCOMPARE(actual.undoManager().undoStepCount(), expected.undoManager().undoStepCount());
		QCOMPARE(actual.undoManager().redoStepCount(), expected.undoManager().redoStepCount());
		if (actual.undoManager().canUndo())
			QCOMPARE(actual.undoManager().nextUndoStep()->getType(), expected.undoManager().nextUndoStep()->getType());
		if (actual.undoManager().canRedo())
			QCOMPARE(actual.undoManager().nextRedoStep()->getType(), expected.undoManager().nextRedoStep()->getType());
		
		// Templates
		QCOMPARE(actual.getFirstFrontTemplate(), expected.getFirstFrontTemplate());
		if (actual.getNumTemplates() != expected.getNumTemplates())
		{
			QCOMPARE(actual.getNumTemplates(), expected.getNumTemplates());
		}
		else for (int i = 0; i < actual.getNumTemplates(); ++i)
		{
			// TODO: only template filenames are compared
			QCOMPARE(actual.getTemplate(i)->getTemplateFilename(), expected.getTemplate(i)->getTemplateFilename());
		}
	}
	
	
	void fuzzyCompareSymbol(const AreaSymbol& actual, const AreaSymbol& expected, const QByteArray& format_id)
	{
		auto pattern_rotable = format_id.startsWith("OCD") ? true : false;
		for (auto i = 0; i < expected.getNumFillPatterns(); ++i)
			pattern_rotable |= expected.getFillPattern(i).rotatable();
		for (auto i = 0; i < actual.getNumFillPatterns(); ++i)
			COMPARE_SYMBOL_PROPERTY(actual.getFillPattern(i).rotatable(), pattern_rotable, expected);
	}
	
	void fuzzyCompareSymbol(const Symbol& actual, const Symbol& expected, const QByteArray& format_id)
	{
		COMPARE_SYMBOL_PROPERTY(actual.isHidden(), expected.isHidden(), expected);
		COMPARE_SYMBOL_PROPERTY(actual.isProtected(), expected.isProtected(), expected);
		COMPARE_SYMBOL_PROPERTY(actual.isRotatable(), expected.isRotatable(), expected);
		
		/// \todo Ideally, the dominant color would be preserved.
#ifdef EXPORT_PRESERVES_DOMINANT_COLOR
		COMPARE_SYMBOL_PROPERTY(actual.guessDominantColor()->getName(), expected.guessDominantColor()->getName(), expected);
#endif
		if (actual.getType() != expected.getType())
			return;  // The following tests assume the same type.
		
		switch (actual.getType())
		{
		case Symbol::Area:
			fuzzyCompareSymbol(static_cast<AreaSymbol const&>(actual), static_cast<AreaSymbol const&>(expected), format_id);
			break;
			
		default:
			;  /// \todo Extend fuzzy testing
		}
		
	}
	
	void fuzzyMatchSymbol(const Map& map, const QByteArray& format_id, const Symbol* expected)
	{
		Symbol const* actual = nullptr;
		for (auto i = 0; !actual && i < map.getNumSymbols(); ++i)
		{
			auto const* symbol = map.getSymbol(i);
			if (symbol->getType() != expected->getType()
			    && symbol->getType() != Symbol::Combined)
			{
				continue;
			}
			
			if (format_id.startsWith("OCD"))
			{
				// In OCD format, export may have incremented second and or first component.
				if (symbol->getNumberComponent(0) < expected->getNumberComponent(0)
				    || symbol->getNumberComponent(0) > expected->getNumberComponent(0) + 1)
				{
					continue;
				}
			}
			else if (expected->getNumberAsString() != symbol->getNumberAsString())
			{
				continue;
			}
			
			if (!expected->getPlainTextName().startsWith(symbol->getPlainTextName()))
				continue;
			
			actual = symbol;
		}
		if (format_id == "OCD8")
		{
			// Don't elaborate testing for these legacy formats.
			switch (expected->getType())
			{
			case Symbol::Combined:
				// Expected symbol may be entirely dropped. (Its parts live independently.)
				if (!actual)
					return;
				break;
			case Symbol::Line:
				// Expected symbol may be entirely turned into combined symbol.
				break;
			default:
				;  // nothing
			}
		}
		if (actual)
			fuzzyCompareSymbol(*actual, *expected, format_id);
		else
			QFAIL(qPrintable(QString::fromLatin1("Missing symbol: %1").arg(expected->getNumberAndPlainTextName())));
	}
	
	/**
	 * Compares map features in a way that works for lossy exporters and importers.
	 * 
	 * A lossy exporter may create extra colors, symbols and objects in order to
	 * maintain the map's appearance. It may also merge all map parts into a
	 * single part.
	 * 
	 * A lossy importer might skip some elements, but it is fair to require that
	 * it imports everything which is exported by the corresponding exporter.
	 */
	void fuzzyCompareMaps(const Map& actual, const Map& expected, const QByteArray& format_id)
	{
		// Miscellaneous
		QCOMPARE(actual.getScaleDenominator(), expected.getScaleDenominator());
		QCOMPARE(actual.getMapNotes(), expected.getMapNotes());
		
		// Georeferencing
		const auto& actual_georef = actual.getGeoreferencing();
		const auto& expected_georef = expected.getGeoreferencing();
		QCOMPARE(actual_georef.getScaleDenominator(), expected_georef.getScaleDenominator());
		QCOMPARE(actual_georef.getGrivation(), expected_georef.getGrivation());
		
		auto test_label = QString::fromLatin1("actual: %1, expected: %2");
		auto ref_point = actual_georef.getMapRefPoint();
		auto actual_point = actual_georef.toProjectedCoords(ref_point);
		auto expected_point = expected_georef.toProjectedCoords(ref_point);
		QVERIFY2(qAbs(actual_point.x() - expected_point.x()) < 1.0, qPrintable(test_label.arg(actual_point.x()).arg(expected_point.x())));
		QVERIFY2(qAbs(actual_point.y() - expected_point.y()) < 1.0, qPrintable(test_label.arg(actual_point.y()).arg(expected_point.y())));
		ref_point += MapCoord{500, 500};
		actual_point = actual_georef.toProjectedCoords(expected_georef.getMapRefPoint());
		expected_point = expected_georef.toProjectedCoords(expected_georef.getMapRefPoint());
		QVERIFY2(qAbs(actual_point.x() - expected_point.x()) < 1.0, qPrintable(test_label.arg(actual_point.x()).arg(expected_point.x())));
		QVERIFY2(qAbs(actual_point.y() - expected_point.y()) < 1.0, qPrintable(test_label.arg(actual_point.y()).arg(expected_point.y())));
		
		if (format_id != "OCD8")
			QCOMPARE(int(actual_georef.getState()), int(expected_georef.getState()));
		
		if (actual_georef.getState() == Georeferencing::Geospatial)
			QCOMPARE(actual_georef.toGeographicCoords(actual_point), expected_georef.toGeographicCoords(actual_point));
		
		// Colors
		QVERIFY2(actual.getNumColors() >= expected.getNumColors(), qPrintable(test_label.arg(actual.getNumColors()).arg(expected.getNumColors())));
		QVERIFY2(actual.getNumColors() <= 2 * expected.getNumColors(), qPrintable(test_label.arg(actual.getNumColors()).arg(expected.getNumColors())));
		
		// Symbols
		// Combined symbols may be dropped (split) on export.
		// Text symbols may be duplicated on export.
		QVERIFY2(2 * actual.getNumSymbols() >= expected.getNumSymbols(), qPrintable(test_label.arg(actual.getNumSymbols()).arg(expected.getNumSymbols())));
		QVERIFY2(actual.getNumSymbols() <= 2 * expected.getNumSymbols(), qPrintable(test_label.arg(actual.getNumSymbols()).arg(expected.getNumSymbols())));
		for (auto i = 0; i < expected.getNumSymbols(); ++i)
		{
			fuzzyMatchSymbol(actual, format_id, expected.getSymbol(i));
		}
		
		// Objects
		QVERIFY2(actual.getNumObjects() >= expected.getNumObjects(), qPrintable(test_label.arg(actual.getNumObjects()).arg(expected.getNumObjects())));
		QVERIFY2(actual.getNumObjects() <= 2 * expected.getNumObjects(), qPrintable(test_label.arg(actual.getNumObjects()).arg(expected.getNumObjects())));
		
		// Parts
		if (actual.getNumParts() > 1)
		{
			QCOMPARE(actual.getNumParts(), expected.getNumParts());
		}
	}
	
	
	std::unique_ptr<Map> saveAndLoadMap(const Map& input, const FileFormat* format)
	{
		auto out = std::make_unique<Map>();
		auto exporter = format->makeExporter({}, &input, nullptr);
		auto importer = format->makeImporter({}, out.get(), nullptr);
		if (exporter && importer)
		{
			QBuffer buffer;
			exporter->setDevice(&buffer);
			importer->setDevice(&buffer);
			if (buffer.open(QIODevice::ReadWrite)
			    && exporter->doExport()
			    && buffer.seek(0)
			    && importer->doImport())
			{
				return out;  // success
			}
		}
		out.reset();  // failure
		return out;
	}
	
	auto const issue_513_files = {
	  "testdata:issue-513-coords-outside-printable.xmap",
	  "testdata:issue-513-coords-outside-printable.omap",
	  "testdata:issue-513-coords-outside-qint32.omap",
	};
	
	auto const example_files = {
	  "data:/examples/complete map.omap",
	  "data:/examples/forest sample.omap",
	  "data:/examples/overprinting.omap",
	  "testdata:templates/world-file.xmap",
	  "data:undefined-objects.omap",
	};
	
	auto const xml_test_files = {
	  "data:barrier.omap",
	  "data:rotated.omap",
	  "data:tags.omap",
	  "data:text-object.omap",
	  "data:undo.omap",
	};
	
}  // namespace



void FileFormatTest::initTestCase()
{
	// Use distinct QSettings
	QCoreApplication::setOrganizationName(QString::fromLatin1("OpenOrienteering.org"));
	QCoreApplication::setApplicationName(QString::fromLatin1(metaObject()->className()));
	QVERIFY2(QDir::home().exists(), "The home dir must be writable in order to use QSettings.");
	
	doStaticInitializations();
	
	const auto prefix = QString::fromLatin1("data");
	QDir::addSearchPath(prefix, QDir(QString::fromUtf8(MAPPER_TEST_SOURCE_DIR)).absoluteFilePath(prefix));
	QDir::addSearchPath(prefix, QDir(QString::fromUtf8(MAPPER_TEST_SOURCE_DIR)).absoluteFilePath(QStringLiteral("..")));
	QDir::addSearchPath(QStringLiteral("testdata"), QDir(QString::fromUtf8(MAPPER_TEST_SOURCE_DIR)).absoluteFilePath(QStringLiteral("data")));
}



void FileFormatTest::mapCoordtoString_data()
{
	using native_int = decltype(MapCoord().nativeX());
	QTest::addColumn<native_int>("x");
	QTest::addColumn<native_int>("y");
	QTest::addColumn<int>("flags");
	QTest::addColumn<QByteArray>("expected");
	
	// Verify toString() especially for native coordinates at the numeric limits.
	using bounds = std::numeric_limits<native_int>;
	QTest::newRow("max,-1/0") << bounds::max() << -1 << 0 << QByteArray("2147483647 -1;");
	QTest::newRow("-2,max/1") << -2 << bounds::max() << 1 << QByteArray("-2 2147483647 1;");
	QTest::newRow("min,min/255") << bounds::min() << bounds::min() << 255 << QByteArray("-2147483648 -2147483648 255;");
}

void FileFormatTest::mapCoordtoString()
{
	using native_int = decltype(MapCoord().nativeX());
	QFETCH(native_int, x);
	QFETCH(native_int, y);
	QFETCH(int, flags);
	QFETCH(QByteArray, expected);
	
	auto const coord = MapCoord::fromNative(x, y, MapCoord::Flags(flags));
	QCOMPARE(coord.toString(), QString::fromLatin1(expected));
	
	MapCoord::StringBuffer<char> buffer;
	QCOMPARE(coord.toUtf8(buffer), expected);
}



void FileFormatTest::mapCoordFromString_data()
{
	using native_int = decltype(MapCoord().nativeX());
	using flags_type = decltype(MapCoord().flags());
	QTest::addColumn<QString>("input");
	QTest::addColumn<native_int>("x");
	QTest::addColumn<native_int>("y");
	QTest::addColumn<flags_type>("flags");
	
	QTest::newRow("plain")     << QString::fromLatin1("-12 -23 255;")    << -12 << -23 << flags_type(255);
	QTest::newRow("multi ' '") << QString::fromLatin1("-12  23    255;") << -12 <<  23 << flags_type(255);
	QTest::newRow("early \\n") << QString::fromLatin1("-12\n\n 23 255;") << -12 <<  23 << flags_type(255);
	QTest::newRow("late \\r")  << QString::fromLatin1("12 -23 \r\r255;") <<  12 << -23 << flags_type(255);
}

void FileFormatTest::mapCoordFromString()
{
	using native_int = decltype(MapCoord().nativeX());
	using flags_type = decltype(MapCoord().flags());
	QFETCH(QString, input);
	QFETCH(native_int, x);
	QFETCH(native_int, y);
	QFETCH(flags_type, flags);
	
	bool no_exception = true;
	auto ref = QStringRef{&input};
	MapCoord coord;
	try {
		coord = MapCoord(ref);
	}  catch (std::invalid_argument const& e) {
		no_exception = false;
	}
	QVERIFY(no_exception);
	QCOMPARE(coord.nativeX(), x);
	QCOMPARE(coord.nativeY(), y);
	QCOMPARE(coord.flags(), flags);
}



void FileFormatTest::fixupExtensionTest_data()
{
	QTest::addColumn<QByteArray>("format_id");
	QTest::addColumn<QString>("expected");
	
	QTest::newRow("file.omap")   << QByteArray("XML") << QStringLiteral("file.omap");
	QTest::newRow("file")        << QByteArray("XML") << QStringLiteral("file.omap");
	QTest::newRow("file.")       << QByteArray("XML") << QStringLiteral("file.omap");
	QTest::newRow("file_omap")   << QByteArray("XML") << QStringLiteral("file_omap.omap");
	QTest::newRow("file.xmap")   << QByteArray("XML") << QStringLiteral("file.xmap");
	QTest::newRow("f.omap.zip")  << QByteArray("XML") << QStringLiteral("f.omap.zip.omap");
}

void FileFormatTest::fixupExtensionTest()
{
	QFETCH(QByteArray, format_id);
	QFETCH(QString, expected);
	
	auto const* format = FileFormats.findFormat(format_id);
	QVERIFY(format);
	
	auto const filename = QString::fromUtf8(QTest::currentDataTag());
	QCOMPARE(format->fixupExtension(filename), expected);
}



void FileFormatTest::understandsTest_data()
{
	quint8 ocd_start_raw[2] = { 0xAD, 0x0C };
	auto ocd_start   = QByteArray::fromRawData(reinterpret_cast<const char*>(ocd_start_raw), 2).append("random data");
	auto omap_start  = QByteArray("OMAP plus random data");
	auto xml_start   = QByteArray("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
	auto xml_legacy  = QByteArray(xml_start + "\r\n<map xmlns=\"http://oorienteering.sourceforge.net/mapper/xml/v2\">");
	auto xml_regular = QByteArray(xml_start + "\n<map xmlns=\"http://openorienteering.org/apps/mapper/xml/v2\" version=\"7\">");
	auto xml_gpx     = QByteArray(xml_start + "\n<gpx>");
	
	// Add all file formats which support import and export
	QTest::addColumn<QByteArray>("format_id");
	QTest::addColumn<QByteArray>("data");
	QTest::addColumn<int>("support");
	
	QTest::newRow("XML < xml legacy")       << QByteArray("XML") << xml_legacy        << int(FileFormat::FullySupported);
	QTest::newRow("XML < xml regular")      << QByteArray("XML") << xml_regular       << int(FileFormat::FullySupported);
	QTest::newRow("XML < xml start")        << QByteArray("XML") << xml_start         << int(FileFormat::Unknown);
	QTest::newRow("XML < xml incomplete 1") << QByteArray("XML") << xml_regular.left(xml_start.length() - 10) << int(FileFormat::Unknown);
	QTest::newRow("XML < xml incomplete 2") << QByteArray("XML") << xml_regular.left(xml_start.length() + 10) << int(FileFormat::Unknown);
	QTest::newRow("XML < ''")               << QByteArray("XML") << QByteArray()      << int(FileFormat::Unknown);
	QTest::newRow("XML < xml other")        << QByteArray("XML") << xml_gpx           << int(FileFormat::NotSupported);
	QTest::newRow("XML < 'OMAPxxx'")        << QByteArray("XML") << omap_start        << int(FileFormat::FullySupported);
	QTest::newRow("XML < 0x0CADxxx")        << QByteArray("XML") << ocd_start         << int(FileFormat::NotSupported);
	
	QTest::newRow("OCD < 0x0CADxxx")        << QByteArray("OCD") << ocd_start         << int(FileFormat::FullySupported);
	QTest::newRow("OCD < 0x0CAD")           << QByteArray("OCD") << ocd_start.left(2) << int(FileFormat::FullySupported);
	QTest::newRow("OCD < 0x0c")             << QByteArray("OCD") << ocd_start.left(1) << int(FileFormat::Unknown);
	QTest::newRow("OCD < ''")               << QByteArray("OCD") << QByteArray()      << int(FileFormat::Unknown);
	QTest::newRow("OCD < 'OMAPxxx'")        << QByteArray("OCD") << omap_start        << int(FileFormat::NotSupported);
	QTest::newRow("OCD < xml start")        << QByteArray("OCD") << xml_start         << int(FileFormat::NotSupported);
	
	/// \todo Test OgrFileFormat (ID "OGR")
}

void FileFormatTest::understandsTest()
{
	QFETCH(QByteArray, format_id);
	QFETCH(QByteArray, data);
	QFETCH(int, support);
	
	auto format = FileFormats.findFormat(format_id);
#ifdef MAPPER_BIG_ENDIAN
	if (format_id.startsWith("OCD"))
		QEXPECT_FAIL("", "OCD format not support on big endian systems", Abort);
#endif
	QVERIFY(format);
	QVERIFY(format->supportsReading());
	QCOMPARE(int(format->understands(data.constData(), data.length())), support);
}



void FileFormatTest::formatForDataTest_data()
{
	understandsTest_data();
}

void FileFormatTest::formatForDataTest()
{
	QFETCH(QByteArray, format_id); Q_UNUSED(format_id)
	QFETCH(QByteArray, data);
	QFETCH(int, support);
	
#ifdef MAPPER_BIG_ENDIAN
	if (format_id.startsWith("OCD"))
		return;
#endif
	
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	
	auto path = QDir(dir.path()).absoluteFilePath(QStringLiteral("testfile"));
	QFile out_file(path);
	QVERIFY(out_file.open(QIODevice::WriteOnly));
	QVERIFY(out_file.write(data) == data.size());
	out_file.close();
	QVERIFY(!out_file.error());
	
	auto result = FileFormats.findFormatForData(path, FileFormat::AllFiles);
	switch (support)
	{
	case FileFormat::NotSupported:
		QVERIFY(!result || result->understands(data.constData(), data.length()) != FileFormat::NotSupported);
		break;
	case FileFormat::Unknown:
		QVERIFY(result);
		QVERIFY(result->understands(data.constData(), data.length()) != FileFormat::NotSupported);
		break;
	case FileFormat::FullySupported:
		QVERIFY(result);
		QVERIFY(result->understands(data.constData(), data.length()) == FileFormat::FullySupported);
		break;
	}
}



void FileFormatTest::issue_513_high_coordinates_data()
{
	QTest::addColumn<QString>("filepath");
	
	for (auto const* raw_path : issue_513_files)
	{
		QTest::newRow(raw_path) << QString::fromUtf8(raw_path);
	}
}

void FileFormatTest::issue_513_high_coordinates()
{
	QFETCH(QString, filepath);
	
	QVERIFY(QFileInfo::exists(filepath));
	
	// Load the test map
	Map map {};
	QVERIFY(map.loadFrom(filepath));
	
	// The map's two objects must exist. Otherwise one may have been deleted
	// for being irregular, indicating failure to handle high coordinates.
	QCOMPARE(map.getNumObjects(), 2);
	
	// The map's two undo steps must exist. Otherwise one may have been deleted
	// for being irregular, indicating failure to handle high coordinates.
	QCOMPARE(map.undoManager().undoStepCount(), 2);
	
	QCOMPARE(map.getNumParts(), 1);
	{
		auto const* part = map.getPart(0);
		auto extent = part->calculateExtent(true);
		QVERIFY2(extent.top()    <  1000000.0, "extent.top() outside printable range");
		QVERIFY2(extent.left()   > -1000000.0, "extent.left() outside printable range");
		QVERIFY2(extent.bottom() > -1000000.0, "extent.bottom() outside printable range");
		QVERIFY2(extent.right()  <  1000000.0, "extent.right() outside printable range");
		
		QCOMPARE(part->getNumObjects(), 2);
		auto const* object = part->getObject(1);
		QCOMPARE(object->getType(), Object::Text);
		auto const* text = static_cast<TextObject const*>(object);
		QVERIFY(!text->hasSingleAnchor());
		QCOMPARE(text->getBoxSize(), MapCoord::fromNative(16000, 10000));
	}
	
	auto print_area = map.printerConfig().print_area;
	QVERIFY2(print_area.top()    <  1000000.0, "print_area.top() outside printable range");
	QVERIFY2(print_area.left()   > -1000000.0, "print_area.left() outside printable range");
	QVERIFY2(print_area.bottom() > -1000000.0, "print_area.bottom() outside printable range");
	QVERIFY2(print_area.right()  <  1000000.0, "print_area.right() outside printable range");
}



void FileFormatTest::saveAndLoad_data()
{
	QTest::addColumn<QByteArray>("id"); // memory management for test data tag
	QTest::addColumn<QByteArray>("format_id");
	QTest::addColumn<int>("format_version");
	QTest::addColumn<QString>("filepath");
	
	// Tests for particular feature of the XML file format.
	for (auto const* raw_path : xml_test_files)
	{
		auto const* format_id = "XML";
		auto id = QByteArray{};
		id.reserve(int(qstrlen(raw_path) + qstrlen(format_id) + 5u));
		id.append(raw_path).append(" <> ").append(format_id);
		
		QTest::newRow(id) << id << QByteArray{format_id} << 0 << QString::fromLatin1(raw_path);
	}
	
	// Add all file formats which support import and export
	static const auto format_ids = {
	    "XML",
#ifndef MAPPER_BIG_ENDIAN
	    "OCD",
#endif
	};
	
	for (auto format_id : format_ids)
	{
		auto format = FileFormats.findFormat(format_id);
		QVERIFY(format);
		QCOMPARE(format->fileType(), FileFormat::MapFile);
		QVERIFY(format->supportsReading());
		QVERIFY(format->supportsWriting());
		
		for (auto raw_path : example_files)
		{
			auto id = QByteArray{};
			id.reserve(int(qstrlen(raw_path) + qstrlen(format_id) + 5u));
			id.append(raw_path).append(" <> ").append(format_id);
			auto path = QString::fromUtf8(raw_path);
			
			if (qstrcmp(format_id, "OCD") == 0)
			{
				for (auto i : { 8, 9, 10, 11, 12 })
				{
					auto ocd_id = id;
					ocd_id.append(QByteArray::number(i));
					QTest::newRow(ocd_id) << ocd_id << QByteArray{format_id}+QByteArray::number(i) << i << path;
				}
				auto ocd_id = id;
				ocd_id.append(" (default)");
				QTest::newRow(ocd_id) << ocd_id << QByteArray{format_id} << int(OcdFileExport::default_version) << path;
			}
			else
			{
				QTest::newRow(id) << id << QByteArray{format_id} << 0 << path;
			}
		}
	}
}

void FileFormatTest::saveAndLoad()
{
	QFETCH(QByteArray, format_id);
	QFETCH(int, format_version);
	QFETCH(QString, filepath);
	
	QVERIFY(QFileInfo::exists(filepath));
	
	// Find the file format and verify that it exists
	const FileFormat* format = FileFormats.findFormat(format_id);
	QVERIFY(format);
	
	// Load the test map
	auto original = std::make_unique<Map>();
	QVERIFY(original->loadFrom(filepath));
	
	// Fix precision of grid rotation
	MapGrid grid = original->getGrid();
	grid.setAdditionalRotation(Georeferencing::roundDeclination(grid.getAdditionalRotation()));
	original->setGrid(grid);
	
	// Manipulate some data
	auto printer_config = original->printerConfig();
	printer_config.page_format.h_overlap += 2.0;
	printer_config.page_format.v_overlap += 4.0;
	original->setPrinterConfig(printer_config);
	
	// Enforce a unique prefix for symbol names, allowing for reliable
	// recognition even when truncated. However, this doesn't help with
	// symbols which are duplicated or dropped on export/import.
	for (auto i = 0; i < original->getNumSymbols(); ++i)
	{
		auto* symbol = original->getSymbol(i);
		symbol->setName(QLatin1Char('#') + QString::number(i) + QChar::Space + symbol->getPlainTextName());
	}
	
	// Save and load the map
	auto new_map = saveAndLoadMap(*original, format);
	QVERIFY2(new_map, "Exception while importing / exporting.");
	
	if (format_version)
	{
		if (qstrncmp(format_id, "OCD", 3) == 0)
		{
			QCOMPARE(new_map->property(OcdFileFormat::versionProperty()).toInt(), format_version);
		}
	}
	
	// If the export is lossy, do an extra export / import cycle in order to
	// be independent of information which cannot be exported into this format
	if (new_map && format->isWritingLossy())
	{
		fuzzyCompareMaps(*new_map, *original, format_id);
		
		original = std::move(new_map);
		new_map = saveAndLoadMap(*original, format);
		QVERIFY2(new_map, "Exception while importing / exporting.");
	}
	
	compareMaps(*new_map, *original);
	comparePrinterConfig(new_map->printerConfig(), original->printerConfig());
	
	if (filepath.endsWith(QStringLiteral("text-object.omap")))
	{
		QCOMPARE(new_map->getNumParts(), 1);
		auto const* part = new_map->getPart(0);
		QCOMPARE(part->getNumObjects(), 4);
		for (int i = 0; i < 3; ++i)
		{
			auto const* object = part->getObject(i);
			QCOMPARE(object->getType(), Object::Text);
			QCOMPARE(static_cast<TextObject const*>(object)->getBoxSize(), MapCoord::fromNative(16000, 10000));
		}
	}
}



void FileFormatTest::pristineMapTest()
{
	auto spot_color = std::make_unique<MapColor>(QString::fromLatin1("spot color"), 0);
	spot_color->setSpotColorName(QString::fromLatin1("SPOTCOLOR"));
	spot_color->setCmyk({0.1f, 0.2f, 0.3f, 0.4f});
	spot_color->setScreenFrequency(55.0);
	spot_color->setScreenAngle(73.0);
	spot_color->setRgbFromCmyk();
	
	auto mixed_color = std::make_unique<MapColor>(QString::fromLatin1("mixed color"), 1);
	mixed_color->setSpotColorComposition({ {&*spot_color, 0.5f} });
	mixed_color->setCmykFromSpotColors();
	mixed_color->setRgbFromCmyk();
	
	auto custom_color = std::make_unique<MapColor>(QString::fromLatin1("custom color"), 2);
	custom_color->setSpotColorComposition({ {&*spot_color, 0.5f} });
	custom_color->setCmyk({0.1f, 0.2f, 0.3f, 0.4f});
	custom_color->setRgb({0.5f, 0.6f, 0.7f});
	
	Map map {};
	map.addColor(spot_color.release(), 0);
	map.addColor(mixed_color.release(), 1);
	map.addColor(custom_color.release(), 2);
	
	XMLFileFormat format;
	auto reloaded_map = saveAndLoadMap(map, &format);
	QVERIFY(bool(reloaded_map));
	compareMaps(*reloaded_map, map);
}



void FileFormatTest::ogrExportTest_data()
{
	QTest::addColumn<QString>("map_filepath");
	QTest::addColumn<QByteArray>("ogr_format_id");
	QTest::addColumn<QString>("ogr_extension");
	QTest::addColumn<int>("latitude");
	QTest::addColumn<int>("longitude");
	
	QTest::newRow("complete map") << QString::fromLatin1("data:/examples/complete map.omap")
	                              << QByteArray("OGR-export-GPX") << QString::fromLatin1("gpx")
	                              << 48 << 12;
}

void FileFormatTest::ogrExportTest()
{
#ifdef MAPPER_USE_GDAL
	QFETCH(QString, map_filepath);
	QFETCH(QByteArray, ogr_format_id);
	QFETCH(QString, ogr_extension);
	QFETCH(int, latitude);
	QFETCH(int, longitude);
	
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	auto const ogr_filepath = QString {dir.path() + QLatin1String("/ogrexport.") + ogr_extension};
	
	{
		Map map;
		QVERIFY(map.loadFrom(map_filepath));
		QCOMPARE(map.getGeoreferencing().getState(), Georeferencing::Geospatial);
		
		auto const exported_latlon = map.getGeoreferencing().getGeographicRefPoint();
		QCOMPARE(qRound(exported_latlon.latitude()), latitude);
		QCOMPARE(qRound(exported_latlon.longitude()), longitude);
		
		auto const* format = FileFormats.findFormat(ogr_format_id);
		QVERIFY(format);
		
		auto exporter = format->makeExporter(ogr_filepath, &map, nullptr);
		QVERIFY(bool(exporter));
		QVERIFY(exporter->doExport());
	}
	
	{
		Map map;
		
		auto const* format = FileFormats.findFormat("OGR");
		QVERIFY(format);
		
		auto importer = format->makeImporter(ogr_filepath, &map, nullptr);
		QVERIFY(bool(importer));
		QVERIFY(importer->doImport());
		QCOMPARE(map.getGeoreferencing().getState(), Georeferencing::Geospatial);
		
		auto const imported_latlon = map.getGeoreferencing().getGeographicRefPoint();
		QCOMPARE(qRound(imported_latlon.latitude()), latitude);
		QCOMPARE(qRound(imported_latlon.longitude()), longitude);
	}
#endif  // MAPPER_USE_GDAL
}


void FileFormatTest::kmlCourseExportTest()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	auto const filepath = QDir(dir.path()).absoluteFilePath(QStringLiteral("test.kml"));
	{
		Georeferencing georef;
		georef.setScaleDenominator(100000);
		georef.setProjectedCRS({}, QStringLiteral("+proj=utm +zone=32 +datum=WGS84 +no_defs"));
		georef.setGeographicRefPoint({50, 9});
		QCOMPARE(georef.getState(), Georeferencing::Geospatial);
		
		Map map;
		map.setGeoreferencing(georef);
		
		SimpleCourseExport simple_export{map};
		QVERIFY(!simple_export.canExport());  // empty map
		
		auto* path_object = new PathObject(Map::getUndefinedLine());
		path_object->addCoordinate(georef.toMapCoords(LatLon{50.001, 9.000}));  // start
		path_object->addCoordinate(georef.toMapCoords(LatLon{50.001, 9.001}));  // 1
		path_object->addCoordinate(georef.toMapCoords(LatLon{50.000, 9.001}));  // 2
		path_object->addCoordinate(georef.toMapCoords(LatLon{50.000, 9.000}));  // finish
		QVERIFY(simple_export.canExport(path_object));
		
		map.getPart(0)->addObject(path_object);
		QVERIFY(simple_export.canExport());
		
		KmlCourseExport exporter{filepath, &map, nullptr};
		QVERIFY(exporter.doExport());
	}
#ifdef MAPPER_USE_GDAL
	{
		auto const* format = FileFormats.findFormatForFilename(filepath, &FileFormat::supportsReading);
		QVERIFY(format);
		
		Map map;
		auto importer = format->makeImporter(QFileInfo(filepath).canonicalFilePath(), &map, nullptr);
		QVERIFY(bool(importer));
		QVERIFY(importer->doImport());
		
		auto const* part = map.getPart(0);
		struct {
			int start = 0;
			int control = 0;
			int finish = 0;
			int other = 0;
		} count;
		for (int i = 0; i < part->getNumObjects(); ++i)
		{
			auto const* object = part->getObject(i);
			auto const name = object->getTag(QStringLiteral("Name"));
			if (object->getType() != Object::Point)
				++count.other;
			else if (name == QLatin1String("S1"))
				++count.start;
			else if (name == QLatin1String("F1"))
				++count.finish;
			else if ([](auto name) { bool ok; void(name.toInt(&ok)); return ok; }(name))
				++count.control;
			else
				++count.other;
		}
		QCOMPARE(count.start, 1);
		QCOMPARE(count.control, 2);
		QCOMPARE(count.finish, 1);
		QCOMPARE(count.other, 0);
		
		auto const center_map = MapCoordF(map.calculateExtent().center());
		auto const center = map.getGeoreferencing().toGeographicCoords(center_map);
		QCOMPARE(int(center.latitude()), 50);
		QCOMPARE(int(center.longitude()), 9);
	}
#endif
}


void FileFormatTest::iofCourseExportTest()
{
	QString const map_filepath = QStringLiteral("testdata:/export/single-line.xmap");
	
	Map map;
	QVERIFY(map.loadFrom(map_filepath));
	
	SimpleCourseExport simple_export{map};
	QVERIFY(simple_export.canExport());
	
	simple_export.setProperties(map, QStringLiteral("Test event"), QStringLiteral("Test course"), 101);
	
	IofCourseExport exporter{{}, &map, nullptr};
	
	QBuffer exported;
	exporter.setDevice(&exported);
	QVERIFY(exporter.doExport());
	QVERIFY(!exported.data().isEmpty());
	
	QString const expected_filepath = QStringLiteral("testdata:/export/iof-3.0-course.xml");
	QFile expected_file = {expected_filepath};
	expected_file.open(QIODevice::ReadOnly | QIODevice::Text);
	auto const expected_data = expected_file.readAll();
	QVERIFY(!expected_data.isEmpty());
	
	// Ignore creator and timestamp
	auto stable_exported = exported.data().indexOf("<Event");
	QVERIFY(stable_exported > 0);
	auto stable_expected = expected_data.indexOf("<Event");
	QVERIFY(stable_expected > 0);
	QCOMPARE(exported.data().mid(stable_exported), expected_data.mid(stable_expected));
}


void FileFormatTest::importTemplateTest_data()
{
	QTest::addColumn<QString>("filepath");
	
	QTest::newRow("Course Design") << QStringLiteral("data:symbol sets/10000/Course_Design_10000.omap");
#ifdef MAPPER_USE_GDAL
	if (GdalManager::isDriverEnabled("LIBKML"))
		QTest::newRow("KMZ") << QStringLiteral("testdata:templates/vsi-test.kmz");  // needs libkml
#endif  // MAPPER_USE_GDAL
}

void FileFormatTest::importTemplateTest()
{
	QFETCH(QString, filepath);
	QVERIFY(QFileInfo::exists(filepath));
	
	{
		Map map;
		
		auto const* format = FileFormats.findFormatForFilename(filepath, &FileFormat::supportsReading);
		QVERIFY(format);
		
		auto importer = format->makeImporter(QFileInfo(filepath).canonicalFilePath(), &map, nullptr);
		QVERIFY(bool(importer));
		QVERIFY(importer->doImport());
		
		QVERIFY(map.getNumTemplates() >= 1);
		auto* temp = map.getTemplate(0);
		temp->loadTemplateFile();
		QCOMPARE(temp->getTemplateState(), Template::Loaded);
	}
}


struct TestOcdFileExport : public OcdFileExport
{
	explicit TestOcdFileExport(const QString& path)
	: OcdFileExport(path, nullptr, nullptr, 12)
	{}

	using OcdFileExport::exportTextData;
};

void FileFormatTest::ocdTextExportTest_data()
{
	QTest::addColumn<QString>("input");
	QTest::addColumn<int>("expected_len");
	QTest::addColumn<int>("first_null");
	
	// Between two chunks (size: 8)
	QTest::newRow("0 chars") << ""                            <<  8 <<  0;
	QTest::newRow("3 chars") << "123"                         <<  8 <<  6;
	QTest::newRow("4 chars") << "1234"                        << 16 <<  8;
	
	// End of last chunk (max chunks: 2)
	QTest::newRow("7 chars") << "1234567"                     << 16 << 14;
	QTest::newRow("8 chars") << "12345678"                    << 16 << 14;
	
	// Trailing surrogate pair at end of last chunk
	QTest::newRow("5+Yee")   << QString::fromUtf8("12345𐐷")   << 16 << 14;
	QTest::newRow("6+Yee")   << QString::fromUtf8("123456𐐷")  << 16 << 12;
	QTest::newRow("7+Yee")   << QString::fromUtf8("1234567𐐷") << 16 << 14;
}

void FileFormatTest::ocdTextExportTest()
{
	QFETCH(QString, input);
	QFETCH(int, expected_len);
	QFETCH(int, first_null);

	TestOcdFileExport ocd_export{{}};

	TextObject object;
	object.setText(input);
	auto exported = ocd_export.exportTextData(&object, /* chunk_size */ 8, /* max_chunks */ 2);
	QCOMPARE(exported.length(), expected_len);
	QCOMPARE(exported[first_null], '\0');
	QCOMPARE(exported[first_null+1], '\0');
	if(first_null > 0)
		QVERIFY(exported[first_null-2] != '\0');
	QCOMPARE(exported.back(), '\0');
}


struct TestOcdFileImport : public OcdFileImport
{
	explicit TestOcdFileImport(int ocd_version)
		: OcdFileImport({}, nullptr, nullptr)
	{
		this->ocd_version = ocd_version;
	}
	
	using OcdFileImport::getObjectText;
	using OcdFileImport::fillPathCoords;
	using OcdFileImport::OcdImportedPathObject;
};

void FileFormatTest::ocdTextImportTest_data()
{
	QTest::addColumn<QString>("string");
	QTest::addColumn<int>("num_chars");
	QTest::addColumn<QString>("expected");
	
	const QString string1 = QLatin1String("123456789abcdef") + QChar::Null + QLatin1String("789ABCDEF");
	QTest::newRow("0 from 0")   << string1.left(0)  <<  0 << string1.left(0);
	QTest::newRow("0 from 5")   << string1.left(5)  <<  0 << string1.left(0);
	QTest::newRow("13 from 13") << string1.left(13) << 13 << string1.left(13);
	QTest::newRow("15 from 20") << string1.left(20) << 15 << string1.left(15);
	QTest::newRow("16 from 20") << string1.left(20) << 16 << string1.left(15);
	QTest::newRow("20 from 20") << string1.left(20) << 20 << string1.left(15);
	
	const QString string2 = QLatin1String("\r\n123");
	QTest::newRow("\\r\\n")     << string2.left(2)  <<  0 <<  string2.mid(2, 0);
	QTest::newRow("\\r\\n123")  << string2.left(5)  <<  5 <<  string2.mid(2, 3);
}

void FileFormatTest::ocdTextImportTest()
{
	QFETCH(QString, string);
	QFETCH(int, num_chars);
	QFETCH(QString, expected);
	
	{
		TestOcdFileImport ocd_v8_import{8};
		Ocd::FormatV8::Object buffer[8]; // large enough to hold more than the string
		auto& ocd_object = buffer[0];
		ocd_object.num_items = 4; // arbitrary offset, here: 32 bytes
		ocd_object.num_text = (num_chars * sizeof(QChar) + sizeof(Ocd::OcdPoint32) - 1) / sizeof(Ocd::OcdPoint32);
		ocd_object.unicode = 1;
		
		auto* first = reinterpret_cast<QChar*>(reinterpret_cast<Ocd::OcdPoint32*>(ocd_object.coords) + ocd_object.num_items);
		auto* tail = std::copy(string.begin(), string.end(), first);
		
		// Must not read behind the reserved space.
		auto num_reserved = int(ocd_object.num_text * sizeof(Ocd::OcdPoint32) / sizeof(QChar));
		std::fill(tail, first + std::max(num_reserved, string.length()) + 1, QChar::Space);
		QVERIFY(ocd_v8_import.getObjectText(ocd_object).length() <= num_reserved);
		
		// With zero at the end of the reserved space, the output must begin with the expected text.
		*(first + num_reserved - 1) = QChar::Null;
		QVERIFY(ocd_v8_import.getObjectText(ocd_object).startsWith(expected));
		
		// With zero at the end of the input text, the output must match the expected text.
		*tail = QChar::Null;
		QCOMPARE(ocd_v8_import.getObjectText(ocd_object), expected);
	}
	
	{
		TestOcdFileImport ocd_v12_import{12};
		Ocd::FormatV12::Object buffer[8]; // large enough to hold more than the string
		auto& ocd_object = buffer[0];
		ocd_object.num_items = 4; // arbitrary offset, here: 32 bytes
		ocd_object.num_text = (num_chars * sizeof(QChar) + sizeof(Ocd::OcdPoint32) - 1) / sizeof(Ocd::OcdPoint32);
		
		auto* first = reinterpret_cast<QChar*>(reinterpret_cast<Ocd::OcdPoint32*>(ocd_object.coords) + ocd_object.num_items);
		auto* tail = std::copy(string.begin(), string.end(), first);
		
		// Must not read behind the reserved space.
		auto num_reserved = int(ocd_object.num_text * sizeof(Ocd::OcdPoint32) / sizeof(QChar));
		std::fill(tail, first + std::max(num_reserved, string.length()) + 1, QChar::Space);
		QVERIFY(ocd_v12_import.getObjectText(ocd_object).length() <= num_reserved);
		
		// With zero at the end of the reserved space, the output must begin with the expected text.
		*(first + num_reserved - 1) = QChar::Null;
		QVERIFY(ocd_v12_import.getObjectText(ocd_object).startsWith(expected));
		
		// With zero at the end of the input text, the output must match the expected text.
		*tail = QChar::Null;
		QCOMPARE(ocd_v12_import.getObjectText(ocd_object), expected);
	}
}


struct OcdPointsView {
	const Ocd::OcdPoint32* data = nullptr;
	int size = 0;
	
	OcdPointsView() = default;
	
	template <class T, std::size_t n>
	explicit OcdPointsView(T(& t)[n])
	: data { t }
	, size { int(n) }
	{}
};
Q_DECLARE_METATYPE(OcdPointsView)

struct FlagsView {
	const int* data = nullptr;
	int size = 0;
	
	FlagsView() = default;
	
	template <class T, std::size_t n>
	explicit FlagsView(T(& t)[n], int size)
	: data { t }
	, size { size }
	{}
	
	template <class T, std::size_t n>
	explicit FlagsView(T(& t)[n])
	: data { t }
	, size { int(n) }
	{}
};
Q_DECLARE_METATYPE(FlagsView)

enum OcdPathPersonality {
	Line = 0,
	Area = 1
};
Q_DECLARE_METATYPE(OcdPathPersonality)

void FileFormatTest::ocdPathImportTest_data()
{
	#define C(x) ((int)((unsigned int)(x)<<8))  // FIXME: Not the same as in export
	constexpr auto ocd_flag_gap = 8;  // TODO: implement as Ocd::OcdPoint32::FlagGap
	
	QTest::addColumn<OcdPointsView>("points");
	QTest::addColumn<OcdPathPersonality>("personality");
	QTest::addColumn<FlagsView>("expected");
	
	{
		// bezier curve
		static Ocd::OcdPoint32 ocd_points[] = {
		    { C(-1109), C(212) },
		    { C(-1035) | Ocd::OcdPoint32::FlagCtl1, C(302) },
		    { C(-1008) | Ocd::OcdPoint32::FlagCtl2, C(519) },
		    { C(-926), C(437) }  // different from first point
		};
		static int expected_flags[] = {
		    MapCoord::CurveStart,
		    0,
		    0,
		    0,
		    MapCoord::ClosePoint  // injected by Mapper
		};
		QTest::newRow("bezier, area") << OcdPointsView(ocd_points) << Area << FlagsView(expected_flags);
		QTest::newRow("bezier, line") << OcdPointsView(ocd_points) << Line << FlagsView(expected_flags, 4);
	}
	
	{
		// straight segments, corner flag
		static Ocd::OcdPoint32 ocd_points[] = {
		    { C(-589), C(432) },
		    { C(-269), C(845) | Ocd::OcdPoint32::FlagCorner },
		    { C(267), C(279) },  // different from first point
		};
		static int expected_flags[] = {
		    0,
		    MapCoord::DashPoint,
		    0,
		    MapCoord::ClosePoint  // injected by Mapper
		};
		QTest::newRow("straight, area") << OcdPointsView(ocd_points) << Area << FlagsView(expected_flags);
		QTest::newRow("straight, line") << OcdPointsView(ocd_points) << Line << FlagsView(expected_flags, 3);
	}
	
	{
		// straight segments, virtual gap
		static Ocd::OcdPoint32 ocd_points[] = {
		    { C(-972), C(-264) },
		    { C(-836) | Ocd::OcdPoint32::FlagLeft | ocd_flag_gap, C(-151) | Ocd::OcdPoint32::FlagRight },
		    { C(-677), C(-19) },
		    { C(-518), C(112) }  // different from first point
		};
		static int expected_flags[] = {
		    0,
		    0,
		    0,
		    0,
		    MapCoord::ClosePoint  // injected by Mapper
		};
		QTest::newRow("virtual gap, area") << OcdPointsView(ocd_points) << Area << FlagsView(expected_flags);
		QTest::newRow("virtual gap, line") << OcdPointsView(ocd_points) << Line << FlagsView(expected_flags, 4);
	}
	
	{
		// straight segments, one hole
		static Ocd::OcdPoint32 ocd_points[] = {
		    { C(100), C(-250) },
		    { C(150), C(-260) },
		    { C(100), C(-250) },  // same as first point
		    { C(200), C(-350) | Ocd::OcdPoint32::FlagHole },
		    { C(220), C(-400) },
		    { C(200), C(-350) }  // same as first point of hole
		};
		static int expected_flags_area[] = {
		    0,
		    0,
		    MapCoord::ClosePoint | MapCoord::HolePoint,
		    0,
		    0,
		    MapCoord::ClosePoint
		};
		QTest::newRow("hole, area") << OcdPointsView(ocd_points) << Area << FlagsView(expected_flags_area);
		static int expected_flags_line[6] = {};
		QTest::newRow("hole, line") << OcdPointsView(ocd_points) << Line << FlagsView(expected_flags_line);
	}
	
	{
		// straight segments, with an "empty" hole
		static Ocd::OcdPoint32 ocd_points[] = {
		    { C(100), C(-250) },
		    { C(150), C(-260) },
		    { C(100), C(-250) },  // same as first point
		    { C(120), C(-200) | Ocd::OcdPoint32::FlagHole },
		    { C(200), C(-350) | Ocd::OcdPoint32::FlagHole },
		    { C(220), C(-400) },
		    { C(200), C(-350) }  // same as second FlagHole point
		};
		static int expected_flags_area[] = {
		    0,
		    0,
		    MapCoord::ClosePoint | MapCoord::HolePoint,
		    MapCoord::ClosePoint | MapCoord::HolePoint,
		    0,
		    0,
		    MapCoord::ClosePoint
		};
		QTest::newRow("empty hole, area") << OcdPointsView(ocd_points) << Area << FlagsView(expected_flags_area);
		static int expected_flags_line[7] = {};
		QTest::newRow("empty hole, line") << OcdPointsView(ocd_points) << Line << FlagsView(expected_flags_line);
	}
	
	{
		// straight segments, with two "empty" holes
		static Ocd::OcdPoint32 ocd_points[] = {
		    { C(100), C(-250) },
		    { C(150), C(-260) },
		    { C(100), C(-250) },  // same as first point
		    { C(120), C(-200) | Ocd::OcdPoint32::FlagHole },
		    { C(140), C(-300) | Ocd::OcdPoint32::FlagHole },
		    { C(200), C(-350) | Ocd::OcdPoint32::FlagHole },
		    { C(220), C(-400) },
		    { C(200), C(-350) },  // same as third FlagHole point
		};
		static int expected_flags_area[] = {
		    0,
		    0,
		    MapCoord::ClosePoint | MapCoord::HolePoint,
		    MapCoord::ClosePoint | MapCoord::HolePoint,
		    MapCoord::ClosePoint | MapCoord::HolePoint,
		    0,
		    0,
		    MapCoord::ClosePoint
		};
		QTest::newRow("two empty holes, area") << OcdPointsView(ocd_points) << Area << FlagsView(expected_flags_area);
		static int expected_flags_line[8] = {};
		QTest::newRow("two empty holes, line") << OcdPointsView(ocd_points) << Line << FlagsView(expected_flags_line);
	}
	
	{
		// straight segments, one hole, not actual areas
		static Ocd::OcdPoint32 ocd_points[] = {
		    { C(100), C(-250) },
		    { C(150), C(-260) },
		    { C(200), C(-350) | Ocd::OcdPoint32::FlagHole },
		    { C(220), C(-400) }
		};
		static int expected_flags_area[] = {
		    0,
		    0,
		    MapCoord::ClosePoint | MapCoord::HolePoint,  // injected by Mapper
		    0,
		    0,
		    MapCoord::ClosePoint  // injected by Mapper
		};
		QTest::newRow("open areas with hole, area") << OcdPointsView(ocd_points) << Area << FlagsView(expected_flags_area);
		static int expected_flags_line[4] = {};
		QTest::newRow("open areas with hole, line") << OcdPointsView(ocd_points) << Line << FlagsView(expected_flags_line);
	}
	
	{
		// area with hole in hole
		static Ocd::OcdPoint32 ocd_points[] = {
			{ C(-405), C(-167) },
		    { C(-348) | Ocd::OcdPoint32::FlagCtl1, C(22) },
		    { C(-113) | Ocd::OcdPoint32::FlagCtl2, C(667) },
		    { C(54), C(687) },
		    { C(184) | Ocd::OcdPoint32::FlagCtl1, C(702) },
		    { C(836) | Ocd::OcdPoint32::FlagCtl2, C(418) },
		    { C(889), C(298) },
		    { C(599), C(117) },
		    { C(137), C(93) | Ocd::OcdPoint32::FlagDash},  // different from first point
		    { C(-25), C(79) | Ocd::OcdPoint32::FlagHole},
		    { C(-208) | Ocd::OcdPoint32::FlagCtl1, C(259) },
		    { C(90) | Ocd::OcdPoint32::FlagCtl2, C(652) },
		    { C(559), C(322) },  // different from first point of hole
		    { C(78), C(326) | Ocd::OcdPoint32::FlagHole},
		    { C(100) | Ocd::OcdPoint32::FlagCtl1, C(341) },
		    { C(157) | Ocd::OcdPoint32::FlagCtl2, C(354) },
		    { C(198), C(339) | Ocd::OcdPoint32::FlagCorner },
		    { C(227) | Ocd::OcdPoint32::FlagCtl1, C(329) },
		    { C(247) | Ocd::OcdPoint32::FlagCtl2, C(304) },
		    { C(242), C(256) },
		    { C(144), C(243) },  // different from first point of hole
		};
		static int expected_flags_area[] = {
		    MapCoord::CurveStart,
		    0,
		    0,
		    MapCoord::CurveStart,
		    0,
		    0,
		    0,
		    0,
		    MapCoord::DashPoint,
		    MapCoord::ClosePoint | MapCoord::HolePoint,  // injected by Mapper
		    MapCoord::CurveStart,
		    0,
		    0,
		    0,
		    MapCoord::ClosePoint | MapCoord::HolePoint,  // injected by Mapper
		    MapCoord::CurveStart,
		    0,
		    0,
		    MapCoord::DashPoint | MapCoord::CurveStart,
		    0,
		    0,
		    0,
		    0,
		    MapCoord::ClosePoint  // injected by Mapper
		};
		QTest::newRow("area with nested holes, area") << OcdPointsView(ocd_points) << Area << FlagsView(expected_flags_area);
		static int expected_flags_line[] = {
		    MapCoord::CurveStart,
		    0,
		    0,
		    MapCoord::CurveStart,
		    0,
		    0,
		    0,
		    0,
		    MapCoord::DashPoint,
		    MapCoord::CurveStart,
		    0,
		    0,
		    0,
		    MapCoord::CurveStart,
		    0,
		    0,
		    MapCoord::DashPoint | MapCoord::CurveStart,
		    0,
		    0,
		    0,
		    0
		};
		QTest::newRow("area with nested holes, line") << OcdPointsView(ocd_points) << Line << FlagsView(expected_flags_line);
	}
}

void FileFormatTest::ocdPathImportTest()
{
	QFETCH(OcdPointsView, points);
	QFETCH(OcdPathPersonality, personality);
	QFETCH(FlagsView, expected);
	
	TestOcdFileImport ocd_v12_import{12};
	
	TestOcdFileImport::OcdImportedPathObject path_object;
	ocd_v12_import.fillPathCoords(&path_object, personality == Area, points.size, points.data);
	QVERIFY(path_object.getRawCoordinateVector().size() > 0);
	
	QCOMPARE(path_object.getRawCoordinateVector().size(), expected.size);
	for (int i = 0; i < expected.size; ++i)
	{
		// Provide the current index when failing.
		if (path_object.getCoordinate(i).flags() != MapCoord::Flags(expected.data[i]))
		{
			auto err = QString::fromLatin1("Compared flags are not the same at index %1\n"
			                               "   Actual  : %2\n"
			                               "   Expected: %3"
			                               ).arg(QString::number(i),
			                                     QString::number(path_object.getCoordinate(i).flags()),
			                                     QString::number(expected.data[i])
			                               );
			QFAIL(qPrintable(err));
		}
	}
}


void FileFormatTest::ocdAreaSymbolTest_data()
{
	QTest::addColumn<int>("format_version");
	
#ifndef MAPPER_BIG_ENDIAN
	static struct { int version; const char* id; } const tests[] = {
	    { 8, "OCD8" },
	    { 9, "OCD9" },
	    { 10, "OCD10" },
	    { 11, "OCD11" },
	    { 12, "OCD12" },
	};
	for (auto& t : tests)
	{
		QTest::newRow(t.id) << t.version;
	}
#endif
}

void FileFormatTest::ocdAreaSymbolTest()
{
	QFETCH(int, format_version);
	
	auto* format_id = QTest::currentDataTag();
	const auto* format = FileFormats.findFormat(format_id);
	QVERIFY(format);
	
	static const auto filepath = QString::fromLatin1("data:rotated-pattern.omap");
	QVERIFY(QFileInfo::exists(filepath));
	
	// Load the test map
	auto expected = std::make_unique<Map>();
	QVERIFY(expected->loadFrom(filepath));
	
	// Save and load the map
	auto actual = saveAndLoadMap(*expected, format);
	QVERIFY2(actual, "Exception while importing / exporting.");
	QCOMPARE(actual->property(OcdFileFormat::versionProperty()), format_version);
	
	// Symbols
	QCOMPARE(actual->getNumSymbols(), expected->getNumSymbols());
	for (int i = 0; i < actual->getNumSymbols(); ++i)
	{
		auto* expected_symbol = expected->getSymbol(i);
		if (expected_symbol->getType() != Symbol::Area)
			continue;
		
		auto* actual_symbol = actual->getSymbol(i);
		QCOMPARE(actual_symbol->getType(), Symbol::Area);
		
		COMPARE_SYMBOL_PROPERTY(actual_symbol->getNumberComponent(0), expected_symbol->getNumberComponent(0), *expected_symbol);
		// OCD limitation: always two number components
		expected_symbol->setNumberComponent(2, -1);
		if (expected_symbol->getNumberComponent(1) == -1)
			expected_symbol->setNumberComponent(1, actual_symbol->getNumberComponent(1));
		
		auto* expected_area_symbol = expected_symbol->asArea();
		auto* actual_area_symbol = actual_symbol->asArea();
		COMPARE_SYMBOL_PROPERTY(bool(actual_area_symbol->getColor()), bool(expected_area_symbol->getColor()), *expected_area_symbol);
		if (expected_area_symbol->getColor())
			COMPARE_SYMBOL_PROPERTY(actual_area_symbol->getColor()->operator QRgb(), expected_area_symbol->getColor()->operator QRgb(), *expected_area_symbol);
		COMPARE_SYMBOL_PROPERTY(actual_area_symbol->isRotatable(), expected_area_symbol->isRotatable(), *expected_area_symbol);
		
		COMPARE_SYMBOL_PROPERTY(actual_area_symbol->getNumFillPatterns(), expected_area_symbol->getNumFillPatterns(), *expected_area_symbol);
		for (auto j = 0; j < expected_area_symbol->getNumFillPatterns(); ++j)
		{
			auto expected_pattern = expected_area_symbol->getFillPattern(j);
			auto actual_pattern = actual_area_symbol->getFillPattern(j);
			
			// OCD limitation: Fill pattern always rotatable
			expected_pattern.setRotatable(true);
			
			compareAreaSymbolPattern(actual_pattern, expected_pattern, *expected_area_symbol);
		}
	}
	
	// Objects
	QCOMPARE(actual->getNumParts(), 1);
	QCOMPARE(expected->getNumParts(), 1);
	
	const auto& actual_part = *actual->getPart(0);
	const auto& expected_part = *expected->getPart(0);
	QCOMPARE(actual_part.getNumObjects(), expected_part.getNumObjects());
	for (int i = 0; i < actual_part.getNumObjects(); ++i)
	{
		auto const& expected_object = *expected_part.getObject(i);
		auto const& expected_symbol = *expected_object.getSymbol();
		if (expected_symbol.getType() != Symbol::Area)
			continue;
		
		auto const& actual_object = *actual_part.getObject(i);
		
		// Adopt expected object properties to OCD format capabilites.
		{
			auto& expected_path = *const_cast<Object&>(expected_object).asPath();
			
			// ATM the last import coord never carries "HolePoint".
			auto& last_coord = expected_path.getCoordinateRef(expected_path.getCoordinateCount()-1);
			last_coord.setHolePoint(false);
			
			// OC*D uses tenths of a degree, counterclockwise.
			auto delta_rotation = actual_object.getRotation() - expected_path.getRotation();
			if (delta_rotation > M_PI)
				delta_rotation -= 2*M_PI;
			else if (delta_rotation < -M_PI)
				delta_rotation += 2*M_PI;
			if (qAbs(delta_rotation) < qDegreesToRadians(0.2))
				expected_path.setRotation(actual_object.getRotation());
			
			// OC*D doesn't have pattern origin.
			expected_path.setPatternOrigin({});
			
			// Mapper may store rotation even if there are no rotatable patterns (left).
			if (!expected_symbol.hasRotatableFillPattern())
				expected_path.setRotation(0);
		}
		
		// Verifying object property; symbol is for tracing
		VERIFY_SYMBOL_PROPERTY(actual_object.equals(&expected_object, false), expected_symbol);
	}
}

void FileFormatTest::ocdTextSymbolTest_data()
{
	QTest::addColumn<int>("format_version");
	
#ifndef MAPPER_BIG_ENDIAN
	static struct { int version; const char* id; } const tests[] = {
	    { 8, "OCD8" },
	    { 9, "OCD9" },
	    { 10, "OCD10" },
	    { 11, "OCD11" },
	    { 12, "OCD12" },
	};
	for (auto& t : tests)
	{
		QTest::newRow(t.id) << t.version;
	}
#endif
}

void FileFormatTest::ocdTextSymbolTest()
{
	QFETCH(int, format_version);
	
	auto* format_id = QTest::currentDataTag();
	const auto* format = FileFormats.findFormat(format_id);
	QVERIFY(format);
	
	static const auto filepath = QString::fromLatin1("data:text-symbol.omap");
	QVERIFY(QFileInfo::exists(filepath));
	
	// Load the test map
	auto expected = std::make_unique<Map>();
	QVERIFY(expected->loadFrom(filepath));
	
	// Save and load the map
	auto actual = saveAndLoadMap(*expected, format);
	QVERIFY2(actual, "Exception while importing / exporting.");
	QCOMPARE(actual->property(OcdFileFormat::versionProperty()), format_version);
	
	// Symbols
	QCOMPARE(actual->getNumSymbols(), expected->getNumSymbols());
	for (int i = 0; i < actual->getNumSymbols(); ++i)
	{
		auto* expected_symbol = expected->getSymbol(i);
		if (expected_symbol->getType() != Symbol::Text)	// actually redundant for the given test data
			continue;
		
		auto* actual_symbol = actual->getSymbol(i);
		QCOMPARE(actual_symbol->getType(), Symbol::Text);
		
		COMPARE_SYMBOL_PROPERTY(actual_symbol->getNumberComponent(0), expected_symbol->getNumberComponent(0), *expected_symbol);
		// OCD limitation: always two number components
		if (expected_symbol->getNumberComponent(1) != -1)
			COMPARE_SYMBOL_PROPERTY(actual_symbol->getNumberComponent(1), expected_symbol->getNumberComponent(1), *expected_symbol);
		
		QVERIFY(expected_symbol->stateEquals(actual_symbol));
		
		auto* expected_text_symbol = expected_symbol->asText();
		auto* actual_text_symbol = actual_symbol->asText();
		
		COMPARE_SYMBOL_PROPERTY(bool(actual_text_symbol->getColor()), bool(expected_text_symbol->getColor()), *expected_text_symbol);
		if (expected_text_symbol->getColor())
			COMPARE_SYMBOL_PROPERTY(actual_text_symbol->getColor()->operator QRgb(), expected_text_symbol->getColor()->operator QRgb(), *expected_text_symbol);
		COMPARE_SYMBOL_PROPERTY(bool(actual_text_symbol->getFramingColor()), bool(expected_text_symbol->getFramingColor()), *expected_text_symbol);
		if (expected_text_symbol->getFramingColor())
			COMPARE_SYMBOL_PROPERTY(actual_text_symbol->getFramingColor()->operator QRgb(), expected_text_symbol->getFramingColor()->operator QRgb(), *expected_text_symbol);
		COMPARE_SYMBOL_PROPERTY(bool(actual_text_symbol->getLineBelowColor()), bool(expected_text_symbol->getLineBelowColor()), *expected_text_symbol);
		if (expected_text_symbol->getLineBelowColor())
			COMPARE_SYMBOL_PROPERTY(actual_text_symbol->getLineBelowColor()->operator QRgb(), expected_text_symbol->getLineBelowColor()->operator QRgb(), *expected_text_symbol);
		
		COMPARE_SYMBOL_PROPERTY(actual_text_symbol->getFramingMode(), expected_text_symbol->getFramingMode(), *expected_text_symbol);
		if (expected_text_symbol->getFramingMode() == TextSymbol::FramingMode::ShadowFraming)
		{
			COMPARE_SYMBOL_PROPERTY(actual_text_symbol->getFramingShadowYOffset(), expected_text_symbol->getFramingShadowYOffset(), *expected_text_symbol);
			COMPARE_SYMBOL_PROPERTY(actual_text_symbol->getFramingShadowXOffset(), expected_text_symbol->getFramingShadowXOffset(), *expected_text_symbol);
		}
		if (expected_text_symbol->getFramingMode() == TextSymbol::FramingMode::LineFraming)
		{
			COMPARE_SYMBOL_PROPERTY(actual_text_symbol->getFramingLineHalfWidth(), expected_text_symbol->getFramingLineHalfWidth(), *expected_text_symbol);
		}

		if (expected_text_symbol->hasLineBelow())
		{
			COMPARE_SYMBOL_PROPERTY(actual_text_symbol->getLineBelowWidth(), expected_text_symbol->getLineBelowWidth(), *expected_text_symbol);
			COMPARE_SYMBOL_PROPERTY(actual_text_symbol->getLineBelowDistance(), expected_text_symbol->getLineBelowDistance(), *expected_text_symbol);
		}
		
		COMPARE_SYMBOL_PROPERTY(actual_text_symbol->getNumCustomTabs(), expected_text_symbol->getNumCustomTabs(), *expected_text_symbol);
		if (expected_text_symbol->getNumCustomTabs())
		{
			for (int i = 0; i < expected_text_symbol->getNumCustomTabs(); ++i)
				COMPARE_SYMBOL_PROPERTY(actual_text_symbol->getCustomTab(i), expected_text_symbol->getCustomTab(i), *expected_text_symbol);
		}
	}
}

void FileFormatTest::dxfExportTest_data()
{
	QTest::addColumn<QString>("map_filepath");
	QTest::addColumn<QByteArray>("ogr_format_id");
	QTest::addColumn<QString>("ogr_extension");
	QTest::addColumn<bool>("export_one_layer_per_symbol");
	
	QTest::newRow("export_one_layer_per_symbol = false") << QString::fromLatin1("data:helper-hidden-symbols.omap")
	                              << QByteArray("OGR-export-DXF") << QString::fromLatin1("dxf")
	                              << false;
	QTest::newRow("export_one_layer_per_symbol = true") << QString::fromLatin1("data:helper-hidden-symbols.omap")
	                              << QByteArray("OGR-export-DXF") << QString::fromLatin1("dxf")
	                              << true;
}

void FileFormatTest::dxfExportTest()
{
#ifdef MAPPER_USE_GDAL
	QFETCH(QString, map_filepath);
	QFETCH(QByteArray, ogr_format_id);
	QFETCH(QString, ogr_extension);
	QFETCH(bool, export_one_layer_per_symbol);
	
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	auto const ogr_filepath = QString {dir.path() + QLatin1String("/dxfexport.") + ogr_extension};
	
	GdalManager manager;
	auto prev_export_one_layer_per_symbol = manager.isExportOptionEnabled(GdalManager::OneLayerPerSymbol);
	if (prev_export_one_layer_per_symbol != export_one_layer_per_symbol)
		manager.setExportOptionEnabled(GdalManager::OneLayerPerSymbol, export_one_layer_per_symbol);
	
	Map map;
	{
		QVERIFY(map.loadFrom(map_filepath));
		
		auto const* format = FileFormats.findFormat(ogr_format_id);
		QVERIFY(format);
		
		auto exporter = format->makeExporter(ogr_filepath, &map, nullptr);
		QVERIFY(bool(exporter));
		QVERIFY(exporter->doExport());
	}
	
	if (prev_export_one_layer_per_symbol != export_one_layer_per_symbol)
		manager.setExportOptionEnabled(GdalManager::OneLayerPerSymbol, prev_export_one_layer_per_symbol);
	
	Map imported_map;
	{
		auto const* format = FileFormats.findFormat("OGR");
		QVERIFY(format);
		
		auto importer = format->makeImporter(ogr_filepath, &imported_map, nullptr);
		QVERIFY(bool(importer));
		QVERIFY(importer->doImport());
	}
	
	// Symbols
	QCOMPARE(imported_map.getNumSymbols(), 8);	// 4 default symbols and 4 symbols for the imported objects
	
	// Objects
	const auto& actual_part = *map.getPart(0);
	const auto& imported_part = *imported_map.getPart(0);
	QCOMPARE(actual_part.getNumObjects(), 12);
	QCOMPARE(imported_part.getNumObjects(), 4);
#endif  // MAPPER_USE_GDAL
}


void FileFormatTest::shpExportTest_data()
{
	QTest::addColumn<QString>("map_filepath");
	QTest::addColumn<QByteArray>("ogr_format_id");
	QTest::addColumn<QString>("ogr_extension");
	QTest::addColumn<bool>("export_one_layer_per_symbol");
	QTest::addColumn<QString>("name_extensions");
	QTest::addColumn<std::vector<int>>("number_symbols");
	QTest::addColumn<std::vector<int>>("number_objects");
	
	
	QTest::newRow("export_one_layer_per_symbol = false") << QString::fromLatin1("data:helper-hidden-symbols.omap")
	                              << QByteArray("OGR-export-ESRI Shapefile") << QString::fromLatin1("shp")
	                              << false
	                              << QString::fromLatin1("areas lines")
	                              << std::vector<int>{4, 4}
 	                              << std::vector<int>{1, 2};	// the single line is exported as two lines
	QTest::newRow("export_one_layer_per_symbol = true") << QString::fromLatin1("data:helper-hidden-symbols.omap")
	                              << QByteArray("OGR-export-ESRI Shapefile") << QString::fromLatin1("shp")
	                              << true
	                              << QString::fromLatin1("Area Line Text")
	                              << std::vector<int>{4, 4, 4}
 	                              << std::vector<int>{1, 2, 0};	// the single line is exported as two lines
}

void FileFormatTest::shpExportTest()
{
#ifdef MAPPER_USE_GDAL
	QFETCH(QString, map_filepath);
	QFETCH(QByteArray, ogr_format_id);
	QFETCH(QString, ogr_extension);
	QFETCH(bool, export_one_layer_per_symbol);
	QFETCH(QString, name_extensions);
	QFETCH(std::vector<int>, number_symbols);
	QFETCH(std::vector<int>, number_objects);
	
	const auto name_extension_list = name_extensions.split(QLatin1String(" "));
	QVERIFY(name_extension_list.size() == (int)number_symbols.size());
	QVERIFY(number_objects.size() == number_symbols.size());
	
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	auto const ogr_filepath = QString {dir.path() + QLatin1String("/shpexport.") + ogr_extension};
	
	GdalManager manager;
	auto prev_export_one_layer_per_symbol = manager.isExportOptionEnabled(GdalManager::OneLayerPerSymbol);
	if (prev_export_one_layer_per_symbol != export_one_layer_per_symbol)
		manager.setExportOptionEnabled(GdalManager::OneLayerPerSymbol, export_one_layer_per_symbol);
	
	Map map;
	{
		QVERIFY(map.loadFrom(map_filepath));
		
		auto const* format = FileFormats.findFormat(ogr_format_id);
		QVERIFY(format);
		
		auto exporter = format->makeExporter(ogr_filepath, &map, nullptr);
		QVERIFY(bool(exporter));
		QVERIFY(exporter->doExport());
	}
	
	if (prev_export_one_layer_per_symbol != export_one_layer_per_symbol)
		manager.setExportOptionEnabled(GdalManager::OneLayerPerSymbol, prev_export_one_layer_per_symbol);
	
	int i = 0;
	for (const auto& name_extension : name_extension_list)
	{
		Map imported_map;
		{
			auto const* format = FileFormats.findFormat("OGR");
			QVERIFY(format);
			
			auto const ogr_filepath = QString {dir.path() + QLatin1String("/shpexport_") + name_extension + QLatin1String(".") + ogr_extension};
			auto importer = format->makeImporter(ogr_filepath, &imported_map, nullptr);
			QVERIFY(bool(importer));
			QVERIFY(importer->doImport());
		}
	
		// Symbols
		QCOMPARE(imported_map.getNumSymbols(), number_symbols.at(i));	// 4 default symbols and 4 symbols for the imported objects
		
		// Objects
		const auto& actual_part = *map.getPart(0);
		const auto& imported_part = *imported_map.getPart(0);
		QCOMPARE(actual_part.getNumObjects(), 12);
		QCOMPARE(imported_part.getNumObjects(), number_objects.at(i));
		++i;
	}
#endif  // MAPPER_USE_GDAL
}

/*
 * We don't need a real GUI window.
 * 
 * But we discovered QTBUG-58768 macOS: Crash when using QPrinter
 * while running with "minimal" platform plugin.
 */
#ifndef Q_OS_MACOS
namespace  {
	auto const Q_DECL_UNUSED qpa_selected = qputenv("QT_QPA_PLATFORM", "offscreen");  // clazy:exclude=non-pod-global-static
}
#endif


QTEST_MAIN(FileFormatTest)
