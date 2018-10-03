/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2018 Kai Pastor
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
#include <iterator>
#include <limits>
#include <memory>
#include <numeric>
#include <stdexcept>

#include <Qt>
#include <QtGlobal>
#include <QtTest>
#include <QBuffer>
#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QIODevice>
#include <QLatin1String>
#include <QPoint>
#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QString>
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
#include "core/symbols/symbol.h"
#include "fileformats/file_format.h"
#include "fileformats/file_format_registry.h"
#include "fileformats/file_import_export.h"
#include "fileformats/ocd_file_export.h"
#include "fileformats/ocd_file_format.h"
#include "fileformats/ocd_types_v8.h"
#include "fileformats/ocd_types_v9.h"
#include "fileformats/xml_file_format.h"
#include "templates/template.h"
#include "undo/undo.h"
#include "undo/undo_manager.h"
#include "util/backports.h"

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
		ba += MapPrinter::paperSizeNames()[page_format.paper_size];
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
}

#endif



namespace
{
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
	
	void compareMaps(const Map& actual, const Map& expected)
	{
		// TODO: This does not compare everything - yet ...
		
		// Miscellaneous
		QCOMPARE(actual.getScaleDenominator(), expected.getScaleDenominator());
		QCOMPARE(actual.getMapNotes(), expected.getMapNotes());
		
		const auto& actual_georef = actual.getGeoreferencing();
		const auto& expected_georef = expected.getGeoreferencing();
		QCOMPARE(actual_georef.getScaleDenominator(), expected_georef.getScaleDenominator());
		QCOMPARE(actual_georef.isLocal(), expected_georef.isLocal());
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
			QCOMPARE(*actual.getColor(i), *expected.getColor(i));
		}
		
		// Symbols
		if (actual.getNumSymbols() != expected.getNumSymbols())
		{
			QCOMPARE(actual.getNumSymbols(), expected.getNumSymbols());
		}
		else for (int i = 0; i < actual.getNumSymbols(); ++i)
		{
			if (!actual.getSymbol(i)->equals(expected.getSymbol(i), Qt::CaseSensitive))
				qDebug("%s vs %s", qPrintable(actual.getSymbol(i)->getNumberAsString()),
				                   qPrintable(expected.getSymbol(i)->getNumberAsString()));
			QVERIFY(actual.getSymbol(i)->equals(expected.getSymbol(i), Qt::CaseSensitive));
			QVERIFY(actual.getSymbol(i)->stateEquals(expected.getSymbol(i)));
		}
		
		// Parts and objects
		QCOMPARE(actual.getCurrentPartIndex(), expected.getCurrentPartIndex());
		if (actual.getNumParts() != expected.getNumParts())
		{
			QCOMPARE(actual.getNumParts(), expected.getNumParts());
		}
		else for (auto p = 0u; p < actual.getNumParts(); ++p)
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
	void fuzzyCompareMaps(const Map& actual, const Map& expected)
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
		auto actual_point = actual_georef.toProjectedCoords(ref_point);;
		auto expected_point = expected_georef.toProjectedCoords(ref_point);
		QVERIFY2(qAbs(actual_point.x() - expected_point.x()) < 1.0, qPrintable(test_label.arg(actual_point.x()).arg(expected_point.x())));
		QVERIFY2(qAbs(actual_point.y() - expected_point.y()) < 1.0, qPrintable(test_label.arg(actual_point.y()).arg(expected_point.y())));
		ref_point += MapCoord{500, 500};
		actual_point = actual_georef.toProjectedCoords(expected_georef.getMapRefPoint());;
		expected_point = expected_georef.toProjectedCoords(expected_georef.getMapRefPoint());
		QVERIFY2(qAbs(actual_point.x() - expected_point.x()) < 1.0, qPrintable(test_label.arg(actual_point.x()).arg(expected_point.x())));
		QVERIFY2(qAbs(actual_point.y() - expected_point.y()) < 1.0, qPrintable(test_label.arg(actual_point.y()).arg(expected_point.y())));
		
		if (!actual_georef.isLocal())
		{
			QCOMPARE(actual_georef.isLocal(), expected_georef.isLocal());
			QCOMPARE(actual_georef.toGeographicCoords(actual_point), expected_georef.toGeographicCoords(actual_point));
		}
		
		// Colors
		QVERIFY2(actual.getNumColors() >= expected.getNumColors(), qPrintable(test_label.arg(actual.getNumColors()).arg(expected.getNumColors())));
		QVERIFY2(actual.getNumColors() <= 2 * expected.getNumColors(), qPrintable(test_label.arg(actual.getNumColors()).arg(expected.getNumColors())));
		
		// Symbols
		// Combined symbols may be dropped on export
		QVERIFY2(2 * actual.getNumSymbols() >= expected.getNumSymbols(), qPrintable(test_label.arg(actual.getNumSymbols()).arg(expected.getNumSymbols())));
		QVERIFY2(actual.getNumSymbols() <= 2 * expected.getNumSymbols(), qPrintable(test_label.arg(actual.getNumSymbols()).arg(expected.getNumSymbols())));
		
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
	
	static const auto issue_513_files = {
	  "data:issue-513-coords-outside-printable.xmap",
	  "data:issue-513-coords-outside-printable.omap",
	  "data:issue-513-coords-outside-qint32.omap",
	};
	
	static const auto example_files = {
	  "data:/examples/complete map.omap",
	  "data:/examples/forest sample.omap",
	  "data:/examples/overprinting.omap",
	  "testdata:templates/world-file.xmap",
	};
	
}  // namespace



void FileFormatTest::initTestCase()
{
	QCoreApplication::setOrganizationName(QString::fromLatin1("OpenOrienteering.org"));
	QCoreApplication::setApplicationName(QString::fromLatin1("FileFormatTest"));
	
	doStaticInitializations();
	
	const auto prefix = QString::fromLatin1("data");
	QDir::addSearchPath(prefix, QDir(QString::fromUtf8(MAPPER_TEST_SOURCE_DIR)).absoluteFilePath(prefix));
	QDir::addSearchPath(prefix, QDir(QString::fromUtf8(MAPPER_TEST_SOURCE_DIR)).absoluteFilePath(QStringLiteral("..")));
	QDir::addSearchPath(QStringLiteral("testdata"), QDir(QString::fromUtf8(MAPPER_TEST_SOURCE_DIR)).absoluteFilePath(QStringLiteral("data")));
}



void FileFormatTest::mapCoordtoString()
{
	QCOMPARE(MapCoord().toString(), QLatin1String("0 0;"));
	
	// Verify toString for native coordinates at the numeric limits.
	auto native_x = MapCoord().nativeX();
	using bounds = std::numeric_limits<decltype(native_x)>;
	static_assert(sizeof(decltype(native_x)) == sizeof(qint32), "This test assumes qint32 native coordinates");
	QCOMPARE(MapCoord::fromNative(bounds::max(), bounds::max(), MapCoord::Flags{MapCoord::Flags::Int(8)}).toString(), QString::fromLatin1("2147483647 2147483647 8;"));
	QCOMPARE(MapCoord::fromNative(bounds::min(), bounds::min(), MapCoord::Flags{MapCoord::Flags::Int(1)}).toString(), QString::fromLatin1("-2147483648 -2147483648 1;"));
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
	QTest::addColumn<QString>("filename");
	
	for (auto raw_path : issue_513_files)
	{
		auto path = QString::fromUtf8(raw_path);
		QVERIFY(QFileInfo::exists(path));
		QTest::newRow(raw_path) << QString::fromUtf8(raw_path);
	}
}

void FileFormatTest::issue_513_high_coordinates()
{
	QFETCH(QString, filename);
	
	// Load the test map
	Map map {};
	QVERIFY(map.loadFrom(filename));
	
	// The map's single object must exist. Otherwise it may have been deleted
	// for being irregular, indicating failure to handle high coordinates.
	QCOMPARE(map.getNumObjects(), 1);
	
	for (int i = 0; i < map.getNumParts(); ++i)
	{
		auto part = map.getPart(i);
		auto extent = part->calculateExtent(true);
		QVERIFY2(extent.top()    <  1000000.0, "extent.top() outside printable range");
		QVERIFY2(extent.left()   > -1000000.0, "extent.left() outside printable range");
		QVERIFY2(extent.bottom() > -1000000.0, "extent.bottom() outside printable range");
		QVERIFY2(extent.right()  <  1000000.0, "extent.right() outside printable range");
	}
	
	auto print_area = map.printerConfig().print_area;
	QVERIFY2(print_area.top()    <  1000000.0, "extent.top() outside printable range");
	QVERIFY2(print_area.left()   > -1000000.0, "extent.left() outside printable range");
	QVERIFY2(print_area.bottom() > -1000000.0, "extent.bottom() outside printable range");
	QVERIFY2(print_area.right()  <  1000000.0, "extent.right() outside printable range");
}



void FileFormatTest::compressedOcdIconTest()
{
	using std::begin;
	using std::end;
	
	{
		Ocd::IconV9 icon;
		
		// White background
		std::fill(begin(icon.bits), end(icon.bits), 124);
		QCOMPARE(icon.compress().uncompress(), icon);
		
		// Black bar on white background
		std::fill(begin(icon.bits)+44, begin(icon.bits)+88, 0);
		QCOMPARE(icon.compress().uncompress(), icon);
		
		// 250 non-repeating bytes of input, followed by white
		// => cannot compress to less than 256 bytes
		std::iota(begin(icon.bits), begin(icon.bits)+125, 0);
		std::transform(begin(icon.bits), begin(icon.bits)+63, begin(icon.bits)+125, [](auto value) {
			return value * 2;
		});
		std::transform(begin(icon.bits), begin(icon.bits)+62, begin(icon.bits)+188, [](auto value) {
			return value * 2 + 1;
		});
		QVERIFY_EXCEPTION_THROWN(icon.compress(), std::length_error);
		
		// 197 non-repeating bytes of input, followed by white
		// => cannot compress to less than 256 bytes
		std::fill(begin(icon.bits)+197, end(icon.bits), 124);
		QVERIFY_EXCEPTION_THROWN(icon.compress(), std::length_error);
		
		// 196 non-repeating bytes of input, followed by white
		std::fill(begin(icon.bits)+196, end(icon.bits), 124);
		QCOMPARE(icon.compress().uncompress(), icon);
		
		QCOMPARE(int(*std::max_element(begin(icon.bits), end(icon.bits))), 124);
	}		
	{
		Ocd::IconV8 icon {};
		QVERIFY_EXCEPTION_THROWN(icon.uncompress(), std::domain_error);
	}
}



void FileFormatTest::saveAndLoad_data()
{
	// Add all file formats which support import and export
	QTest::addColumn<QByteArray>("id"); // memory management for test data tag
	QTest::addColumn<QByteArray>("format_id");
	QTest::addColumn<int>("format_version");
	QTest::addColumn<QString>("map_filename");
	
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
			QVERIFY(QFileInfo::exists(path));
			
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
				ocd_id = id;
				ocd_id.append("-legacy");
				QTest::newRow(ocd_id) << ocd_id << QByteArray{format_id}+"-legacy" << 8 << path;
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
	QFETCH(QString, map_filename);
	
	// Find the file format and verify that it exists
	const FileFormat* format = FileFormats.findFormat(format_id);
	QVERIFY(format);
	
	// Load the test map
	auto original = std::make_unique<Map>();
	QVERIFY(original->loadFrom(map_filename));
	
	// Fix precision of grid rotation
	MapGrid grid = original->getGrid();
	grid.setAdditionalRotation(Georeferencing::roundDeclination(grid.getAdditionalRotation()));
	original->setGrid(grid);
	
	// Manipulate some data
	auto printer_config = original->printerConfig();
	printer_config.page_format.h_overlap += 2.0;
	printer_config.page_format.v_overlap += 4.0;
	original->setPrinterConfig(printer_config);
	
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
		fuzzyCompareMaps(*new_map, *original);
		
		original = std::move(new_map);
		new_map = saveAndLoadMap(*original, format);
		QVERIFY2(new_map, "Exception while importing / exporting.");
	}
	
	compareMaps(*new_map, *original);
	comparePrinterConfig(new_map->printerConfig(), original->printerConfig());
}



void FileFormatTest::pristineMapTest()
{
	auto spot_color = std::make_unique<MapColor>(QString::fromLatin1("spot color"), 0);
	spot_color->setSpotColorName(QString::fromLatin1("SPOTCOLOR"));
	spot_color->setCmyk({0.1f, 0.2f, 0.3f, 0.4f});
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


QTEST_MAIN(FileFormatTest)
