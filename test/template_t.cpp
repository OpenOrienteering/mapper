/*
 *    Copyright 2017-2019 Kai Pastor
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
#include <QtMath>
#include <QtTest>
#include <QDir>
#include <QFileInfo>
#include <QObject>
#include <QString>
#include <QTransform>

#include "test_config.h"

#include "global.h"
#include "core/georeferencing.h"
#include "core/map.h"
#include "core/map_view.h"
#include "fileformats/xml_file_format_p.h"
#include "gdal/ogr_template.h"
#include "templates/template.h"
#include "templates/template_table_model.h"
#include "templates/world_file.h"

using namespace OpenOrienteering;


/**
 * @test Tests template classes.
 */
class TemplateTest : public QObject
{
Q_OBJECT
private slots:
	void initTestCase()
	{
		Q_INIT_RESOURCE(resources);
		doStaticInitializations();
		// Static map initializations
		Map map;
		QDir::addSearchPath(QStringLiteral("testdata"), QDir(QString::fromUtf8(MAPPER_TEST_SOURCE_DIR)).absoluteFilePath(QStringLiteral("data")));
	}
	
	void worldFilePathTest()
	{
		QCOMPARE(WorldFile::pathForImage(QStringLiteral("dir.ext/file-noext")), QStringLiteral("dir.ext/file-noext.wld"));
		QCOMPARE(WorldFile::pathForImage(QStringLiteral("dir/file.")), QStringLiteral("dir/file.wld"));
		QCOMPARE(WorldFile::pathForImage(QStringLiteral("dir/file.j")), QStringLiteral("dir/file.jw"));
		QCOMPARE(WorldFile::pathForImage(QStringLiteral("dir/file.jp")), QStringLiteral("dir/file.jpw"));
		QCOMPARE(WorldFile::pathForImage(QStringLiteral("dir/file.jpg")), QStringLiteral("dir/file.jgw"));
		QCOMPARE(WorldFile::pathForImage(QStringLiteral("dir/file.jpeg")), QStringLiteral("dir/file.jpegw"));
		QCOMPARE(WorldFile::pathForImage(QStringLiteral("dir/file.png.jpeg")), QStringLiteral("dir/file.png.jpegw"));
	}
	
	void worldFileUnitTest()
	{
		auto world_file = WorldFile { 1, 2, 3, 4, 5, 6 };
		QCOMPARE(world_file.parameters[0], 1.0);
		QCOMPARE(world_file.parameters[1], 2.0);
		QCOMPARE(world_file.parameters[2], 3.0);
		QCOMPARE(world_file.parameters[3], 4.0);
		QCOMPARE(world_file.parameters[4], 5.0);
		QCOMPARE(world_file.parameters[5], 6.0);
		
		auto const transform = QTransform(world_file);
		QCOMPARE(transform.m11(), 1.0);
		QCOMPARE(transform.m12(), 2.0);
		QCOMPARE(transform.m21(), 3.0);
		QCOMPARE(transform.m22(), 4.0);
		QCOMPARE(transform.map(QPointF(0.5, 0.5)), QPointF(5, 6));
		
		WorldFile world_file_2;
		QCOMPARE(QTransform(world_file_2), QTransform{});
		
		world_file = world_file_2;
		QCOMPARE(QTransform(world_file), QTransform{});
		
		auto const verify_parameters = [](auto const & parameters) {
			QCOMPARE(parameters[0], 0.15);
			QCOMPARE(parameters[1], 0.0);
			QCOMPARE(parameters[2], 0.0);
			QCOMPARE(parameters[3], -0.15);
			QCOMPARE(parameters[4], 649078.0);
			QCOMPARE(parameters[5], 394159.0);
		};
		
		QString world_file_path = QStringLiteral("testdata:templates/world-file.pgw");
		QVERIFY(world_file.load(world_file_path));
		verify_parameters(world_file.parameters);
		
		QString image_path = QStringLiteral("testdata:templates/world-file.png");
		QVERIFY(world_file_2.tryToLoadForImage(image_path));
		verify_parameters(world_file_2.parameters);
		
		// Try reading garbage
		QVERIFY(!world_file.load(image_path));
	}
	
	void worldFileTemplateTest()
	{
		Map map;
		MapView view{ &map };
		QVERIFY(map.loadFrom(QStringLiteral("testdata:templates/world-file.xmap"), &view));
		
		const auto& georef = map.getGeoreferencing();
		QVERIFY(georef.isValid());
		
		QCOMPARE(map.getNumTemplates(), 1);
		auto temp = map.getTemplate(0);
		QCOMPARE(temp->getTemplateType(), "TemplateImage");
		QCOMPARE(temp->getTemplateFilename(), QString::fromLatin1("world-file.png"));
		QCOMPARE(temp->getTemplateState(), Template::Loaded);
		QVERIFY(temp->isTemplateGeoreferenced());
		auto rotation_template = 0.01 * qRound(100 * qRadiansToDegrees(temp->getTemplateRotation()));
		auto rotation_map = 0.01 * qRound(100 * georef.getGrivation());
		QCOMPARE(rotation_template, rotation_map);
	}
	
	
	void templatePathTest()
	{
		QFile file{ QStringLiteral("testdata:templates/world-file.xmap") };
		QVERIFY(file.open(QIODevice::ReadOnly));
		auto original_data = file.readAll();
		QCOMPARE(file.error(), QFileDevice::NoError);
		file.close();
		
		Map map;
		MapView view{ &map };
		
		// The buffer has no path, so the template cannot be loaded.
		// This results in a warning.
		QBuffer buffer{ &original_data };
		buffer.open(QIODevice::ReadOnly);
		XMLFileImporter importer{ {}, &map, &view };
		importer.setDevice(&buffer);
		QVERIFY(importer.doImport());
		QCOMPARE(importer.warnings().size(), std::size_t(1));
		
		// The image is in Invalid state, but path attributes are intact.
		QCOMPARE(map.getNumTemplates(), 1);
		auto temp = map.getTemplate(0);
		QCOMPARE(temp->getTemplateType(), "TemplateImage");
		QCOMPARE(temp->getTemplateFilename(), QStringLiteral("world-file.png"));
		QCOMPARE(temp->getTemplatePath(), QStringLiteral("world-file.png"));
		QCOMPARE(temp->getTemplateRelativePath(), QStringLiteral("world-file.png"));
		QCOMPARE(temp->getTemplateState(), Template::Invalid);
		
		QBuffer out_buffer;
		QVERIFY(out_buffer.open(QIODevice::WriteOnly));
		XMLFileExporter exporter{ {}, &map, &view };
		exporter.setOption(QStringLiteral("autoFormatting"), true);
		exporter.setDevice(&out_buffer);
		exporter.doExport();
		out_buffer.close();
		QCOMPARE(exporter.warnings().size(), std::size_t(0));
		
		// The exported data matches the original data.
		QCOMPARE(out_buffer.buffer(), original_data);
	}
	
	void geoTiffTemplateTest()
	{
		Map map;
		MapView view{ &map };
		QVERIFY(map.loadFrom(QStringLiteral("testdata:templates/geotiff.xmap"), &view));
		
		const auto& georef = map.getGeoreferencing();
		QVERIFY(georef.isValid());
		
		QCOMPARE(map.getNumTemplates(), 1);
		auto temp = map.getTemplate(0);
		QCOMPARE(temp->getTemplateFilename(), QString::fromUtf8("\u0433\u0435\u043E.tiff"));
#ifndef MAPPER_USE_GDAL
		QCOMPARE(temp->getTemplateType(), "TemplateImage");
#else
		QCOMPARE(temp->getTemplateType(), "GdalTemplate");
#ifdef Q_OS_UNIX
		if (QFile::encodeName(temp->getTemplateFilename()) != temp->getTemplateFilename().toUtf8())
			QEXPECT_FAIL("", "Local 8 bit encoding is not UTF-8", Abort);
#endif
		QCOMPARE(temp->getTemplateState(), Template::Loaded);
		QVERIFY(temp->isTemplateGeoreferenced());
		auto rotation_template = 0.01 * qRound(100 * qRadiansToDegrees(temp->getTemplateRotation()));
		auto rotation_map = 0.01 * qRound(100 * georef.getGrivation());
		QCOMPARE(rotation_template, rotation_map);
#endif
	}
	
#ifdef MAPPER_USE_GDAL
	void ogrTemplateGeoreferencingTest()
	{
		auto const osm_fileinfo = QFileInfo(QStringLiteral("testdata:templates/map.osm"));
		QVERIFY(osm_fileinfo.exists());
		auto georef = OgrTemplate::getDataGeoreferencing(osm_fileinfo.absoluteFilePath(), Georeferencing{});
		QVERIFY(bool(georef));
		auto latlon = georef->getGeographicRefPoint();
		QCOMPARE(qRound(latlon.latitude()), 50);
		QCOMPARE(qRound(latlon.longitude()), 8);
	}
#endif
	
	void templateTableModelTest()
	{
		Map map;
		MapView view{ &map };
		QVERIFY(map.loadFrom(QStringLiteral("testdata:templates/world-file.xmap"), &view));
		QCOMPARE(map.getNumTemplates(), 1);
		
		TemplateTableModel model(map, view);
		QCOMPARE(model.rowCount(), 2);
		
		// Single template, in background
		QCOMPARE(map.getFirstFrontTemplate(), 1);
		QCOMPARE(model.mapRow(map.getFirstFrontTemplate()), 0);
		QCOMPARE(model.posFromRow(0), -1);
		QCOMPARE(model.rowFromPos(0), 1);
		QCOMPARE(model.posFromRow(1), 0);
		// Three possible calls in onTemplateAboutToBeAdded
		QCOMPARE(model.insertionRowFromPos(0, true), 2);
		QCOMPARE(model.insertionRowFromPos(1, true), 1);
		QCOMPARE(model.insertionRowFromPos(1, false), 0);
		
		// Single template, in foreground
		map.setFirstFrontTemplate(0);
		QCOMPARE(model.mapRow(map.getFirstFrontTemplate()), 1);
		QCOMPARE(model.posFromRow(1), -1);
		QCOMPARE(model.rowFromPos(0), 0);
		QCOMPARE(model.posFromRow(0), 0);
		// Three possible calls in onTemplateAboutToBeAdded
		QCOMPARE(model.insertionRowFromPos(0, true), 2);
		QCOMPARE(model.insertionRowFromPos(0, false), 1);
		QCOMPARE(model.insertionRowFromPos(1, false), 0);
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
	auto Q_DECL_UNUSED qpa_selected = qputenv("QT_QPA_PLATFORM", "minimal");  // clazy:exclude=non-pod-global-static
}
#endif


QTEST_MAIN(TemplateTest)
#include "template_t.moc"  // IWYU pragma: keep
