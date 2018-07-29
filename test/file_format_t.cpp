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
#include <limits>
#include <memory>

#include <Qt>
#include <QtGlobal>
#include <QtTest>
#include <QBuffer>
#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
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

#include "global.h"
#include "test_config.h"
#include "settings.h"
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
#include "fileformats/ocad8_file_format.h"
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
	
	bool compareMaps(const Map& a, const Map& b, QString& error)
	{
		// TODO: This does not compare everything - yet ...
		
		// Miscellaneous
		
		if (a.getScaleDenominator() != b.getScaleDenominator())
		{
			error = QString::fromLatin1("The map scales differ.");
			return false;
		}
		if (a.getMapNotes().compare(b.getMapNotes()) != 0)
		{
			error = QString::fromLatin1("The map notes differ.");
			return false;
		}
		
		const Georeferencing& a_geo = a.getGeoreferencing();
		const Georeferencing& b_geo = b.getGeoreferencing();
		if (a_geo.isLocal() != b_geo.isLocal() ||
			a_geo.getScaleDenominator() != b_geo.getScaleDenominator() ||
			a_geo.getDeclination() != b_geo.getDeclination() ||
			a_geo.getGrivation() != b_geo.getGrivation() ||
			a_geo.getMapRefPoint() != b_geo.getMapRefPoint() ||
			a_geo.getProjectedRefPoint() != b_geo.getProjectedRefPoint() ||
			a_geo.getProjectedCRSId() != b_geo.getProjectedCRSId() ||
			a_geo.getProjectedCRSName() != b_geo.getProjectedCRSName() ||
			a_geo.getProjectedCoordinatesName() != b_geo.getProjectedCoordinatesName() ||
			a_geo.getProjectedCRSSpec() != b_geo.getProjectedCRSSpec() ||
			qAbs(a_geo.getGeographicRefPoint().latitude() - b_geo.getGeographicRefPoint().latitude()) > 0.5e-8 ||
			qAbs(a_geo.getGeographicRefPoint().longitude() - b_geo.getGeographicRefPoint().longitude())  > 0.5e-8)
		{
			error = QString::fromLatin1("The georeferencing differs.");
			return false;
		}
		
		const MapGrid& a_grid = a.getGrid();
		const MapGrid& b_grid = b.getGrid();
		
		if (a_grid != b_grid || !(a_grid == b_grid))
		{
			error = QString::fromLatin1("The map grid differs.");
			return false;
		}
		
		if (a.isAreaHatchingEnabled() != b.isAreaHatchingEnabled() ||
			a.isBaselineViewEnabled() != b.isBaselineViewEnabled())
		{
			error = QString::fromLatin1("The view mode differs.");
			return false;
		}
		
		// Colors
		if (a.getNumColors() != b.getNumColors())
		{
			error = QString::fromLatin1("The number of colors differs.");
			return false;
		}
		for (int i = 0; i < a.getNumColors(); ++i)
		{
			if (!a.getColor(i)->equals(*b.getColor(i), true))
			{
				error = QString::fromLatin1("Color #%1 (%2) differs.").arg(i).arg(a.getColor(i)->getName());
				return false;
			}
		}
		
		// Symbols
		if (a.getNumSymbols() != b.getNumSymbols())
		{
			error = QString::fromLatin1("The number of symbols differs.");
			return false;
		}
		for (int i = 0; i < a.getNumSymbols(); ++i)
		{
			const Symbol* a_symbol = a.getSymbol(i);
			if (!a_symbol->equals(b.getSymbol(i), Qt::CaseSensitive))
			{
				error = QString::fromLatin1("Symbol #%1 (%2) differs.").arg(i).arg(a_symbol->getName());
				return false;
			}
			if (!a_symbol->stateEquals(b.getSymbol(i)))
			{
				error = QString::fromLatin1("State of symbol #%1 (%2) differs.").arg(i).arg(a_symbol->getName());
				return false;
			}
		}
		
		// Parts and objects
		// TODO: Only a single part is compared here! Add comparison for parts ...
		if (a.getNumParts() != b.getNumParts())
		{
			error = QString::fromLatin1("The number of parts differs.");
			return false;
		}
		if (a.getCurrentPartIndex() != b.getCurrentPartIndex())
		{
			error = QString::fromLatin1("The current part differs.");
			return false;
		}
		for (int part = 0; part < a.getNumParts(); ++part)
		{
			MapPart* a_part = a.getPart(part);
			MapPart* b_part = b.getPart(part);
			if (a_part->getName().compare(b_part->getName(), Qt::CaseSensitive) != 0)
			{
				error = QString::fromLatin1("The names of part #%1 differ (%2 <-> %3).")
				        .arg(QString::number(part), a_part->getName(), b_part->getName());
				return false;
			}
			if (a_part->getNumObjects() != b_part->getNumObjects())
			{
				error = QString::fromLatin1("The number of objects differs.");
				return false;
			}
			for (int i = 0; i < a_part->getNumObjects(); ++i)
			{
				if (!a_part->getObject(i)->equals(b_part->getObject(i), true))
				{
					error = QString::fromLatin1("Object #%1 (with symbol %2) in part #%3 differs.")
					        .arg(QString::number(i), a_part->getObject(i)->getSymbol()->getName(), QString::number(part));
					return false;
				}
			}
		}
		
		// Object selection
		if (a.getNumSelectedObjects() != b.getNumSelectedObjects())
		{
			error = QString::fromLatin1("The number of selected objects differs.");
			return false;
		}
		if ((a.getFirstSelectedObject() == nullptr && b.getFirstSelectedObject() != nullptr) || (a.getFirstSelectedObject() != nullptr && b.getFirstSelectedObject() == nullptr) ||
			(a.getFirstSelectedObject() != nullptr && b.getFirstSelectedObject() != nullptr &&
			 a.getCurrentPart()->findObjectIndex(a.getFirstSelectedObject()) != b.getCurrentPart()->findObjectIndex(b.getFirstSelectedObject())))
		{
			error = QString::fromLatin1("The first selected object differs.");
			return false;
		}
		for (auto object_index : qAsConst(a.selectedObjects()))
		{
			if (!b.isObjectSelected(b.getCurrentPart()->getObject(a.getCurrentPart()->findObjectIndex(object_index))))
			{
				error = QString::fromLatin1("The selected objects differ.");
				return false;
			}
		}
		
		// Undo steps
		// TODO: Currently only the number of steps is compared here.
		if (a.undoManager().undoStepCount() != b.undoManager().undoStepCount() ||
			a.undoManager().redoStepCount() != b.undoManager().redoStepCount())
		{
			error = QString::fromLatin1("The number of undo / redo steps differs.");
			return false;
		}
		if (a.undoManager().canUndo() &&
		    a.undoManager().nextUndoStep()->getType() != b.undoManager().nextUndoStep()->getType())
		{
			error = QString::fromLatin1("The type of the first undo step is different.");
			return false;
		}
		if (a.undoManager().canRedo() &&
		    a.undoManager().nextRedoStep()->getType() != b.undoManager().nextRedoStep()->getType())
		{
			error = QString::fromLatin1("The type of the first redo step is different.");
			return false;
		}
		
		// Templates
		if (a.getNumTemplates() != b.getNumTemplates())
		{
			error = QString::fromLatin1("The number of templates differs.");
			return false;
		}
		if (a.getFirstFrontTemplate() != b.getFirstFrontTemplate())
		{
			error = QString::fromLatin1("The division into front / back templates differs.");
			return false;
		}
		for (int i = 0; i < a.getNumTemplates(); ++i)
		{
			// TODO: only template filenames are compared
			if (a.getTemplate(i)->getTemplateFilename().compare(b.getTemplate(i)->getTemplateFilename()) != 0)
			{
				error = QString::fromLatin1("Template filename #%1 differs.").arg(i);
				return false;
			}
		}
		
		return true;
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
	
	static const auto test_files = {
	  "data:issue-513-coords-outside-printable.xmap",
	  "data:issue-513-coords-outside-printable.omap",
	  "data:issue-513-coords-outside-qint32.omap",
	  "data:legacy_map.omap",
	  "data:/examples/complete map.omap",
	  "data:/examples/forest sample.omap",
	  "data:/examples/overprinting.omap",
	};
	
}  // namespace



void FileFormatTest::initTestCase()
{
	QCoreApplication::setOrganizationName(QString::fromLatin1("OpenOrienteering.org"));
	QCoreApplication::setApplicationName(QString::fromLatin1("FileFormatTest"));
	Settings::getInstance().setSetting(Settings::General_NewOcd8Implementation, true);
	
	doStaticInitializations();
	if (!FileFormats.findFormat("OCAD78"))
		FileFormats.registerFormat(new OCAD8FileFormat());
	
	const auto prefix = QString::fromLatin1("data");
	QDir::addSearchPath(prefix, QDir(QString::fromUtf8(MAPPER_TEST_SOURCE_DIR)).absoluteFilePath(prefix));
	QDir::addSearchPath(prefix, QDir(QString::fromUtf8(MAPPER_TEST_SOURCE_DIR)).absoluteFilePath(QStringLiteral("..")));
	
	for (auto raw_path : test_files)
	{
		auto path = QString::fromUtf8(raw_path);
		QVERIFY(QFileInfo::exists(path));
	}
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
	QVERIFY(format->supportsImport());
	QCOMPARE(int(format->understands(data.constData(), data.length())), support);
}



void FileFormatTest::issue_513_high_coordinates_data()
{
	QTest::addColumn<QString>("filename");
	
	for (auto raw_path : test_files)
	{
		auto path = QString::fromUtf8(raw_path);
		if (path.contains(QString::fromLatin1("issue-513")))
			QTest::newRow(raw_path) << QString::fromUtf8(raw_path);
	}
}

void FileFormatTest::issue_513_high_coordinates()
{
	QFETCH(QString, filename);
	
	// Load the test map
	Map map {};
	QVERIFY(map.loadFrom(filename, nullptr, nullptr, false, false));
	
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



void FileFormatTest::saveAndLoad_data()
{
	// Add all file formats which support import and export
	QTest::addColumn<QByteArray>("id"); // memory management for test data tag
	QTest::addColumn<QByteArray>("format_id");
	QTest::addColumn<QString>("map_filename");
	
	for (auto format : FileFormats.formats())
	{
		if (format->supportsExport() && format->supportsImport())
		{
			for (auto raw_path : test_files)
			{
				auto format_id = format->id();
				auto id = QByteArray{};
				id.reserve(int(qstrlen(raw_path) + qstrlen(format_id) + 5u));
				id.append(raw_path).append(" <> ").append(format_id);
				auto path = QString::fromUtf8(raw_path);
				QTest::newRow(id) << id << QByteArray{format_id} << path;
			}
		}
	}
}

void FileFormatTest::saveAndLoad()
{
	QFETCH(QByteArray, format_id);
	QFETCH(QString, map_filename);
	
	// Find the file format and verify that it exists
	const FileFormat* format = FileFormats.findFormat(format_id);
	QVERIFY(format);
	
	if (map_filename.contains(QLatin1String("legacy")))
		QEXPECT_FAIL(QTest::currentDataTag(), "Unsupported legacy binary format", Abort);
	
	// Load the test map
	auto original = std::make_unique<Map>();
	QVERIFY(original->loadFrom(map_filename, nullptr, nullptr, false, false));
	QVERIFY(!original->hasUnsavedChanges());
	
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
	
	// If the export is lossy, do an extra export / import cycle in order to
	// be independent of information which cannot be exported into this format
	if (new_map && format->isExportLossy())
	{
		original = std::move(new_map);
		new_map = saveAndLoadMap(*original, format);
	}
	
	QVERIFY2(new_map, "Exception while importing / exporting.");
	
	QString error;
	bool equal = compareMaps(*original, *new_map, error);
	if (!equal)
		QFAIL(QString::fromLatin1("Loaded map does not equal saved map, error: %1").arg(error).toLocal8Bit());
	
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
	
	QString error;
	if (!compareMaps(map, *reloaded_map, error))
		QFAIL(QString::fromLatin1("Loaded map does not equal saved map, error: %1").arg(error).toLocal8Bit());
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
