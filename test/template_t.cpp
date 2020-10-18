/*
 *    Copyright 2017-2020 Kai Pastor
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


#include <cmath>
#include <iosfwd>
#include <memory>
#include <vector>

#include <QtGlobal>
#include <QtMath>
#include <QtTest>
#include <QBuffer>
#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileDevice>
#include <QFileInfo>
#include <QIODevice>
#include <QLineF>
#include <QMetaObject>
#include <QObject>
#include <QPainter>
#include <QPicture>
#include <QPointF>
#include <QRectF>
#include <QSignalSpy>  // IWYU pragma: keep
#include <QString>
#include <QStringList>
#include <QTransform>

#include "test_config.h"

#include "global.h"
#include "core/georeferencing.h"
#include "core/latlon.h"
#include "core/map.h"
#include "core/map_coord.h"
#include "core/map_view.h"
#include "fileformats/xml_file_format_p.h"
#include "gdal/ogr_template.h"
#include "gdal/gdal_manager.h"
#include "templates/template.h"
#include "templates/template_table_model.h"
#include "templates/template_track.h"
#include "templates/world_file.h"

using namespace OpenOrienteering;

namespace
{

QPointF center(const TemplateTrack* temp)
{
	QPicture picture;
	QPainter painter(&picture);
	temp->drawTracks(&painter, false);
	painter.end();
	return QRectF(picture.boundingRect()).center();
}

QPointF center(const Template* temp)
{
	if (auto* template_track = qobject_cast<const TemplateTrack*>(temp))
		return center(template_track);
	return temp->templateToMap(temp->getTemplateExtent().center());
}

}  // namespace


/**
 * @test Tests template classes.
 */
class TemplateTest : public QObject
{
Q_OBJECT
private slots:
	void initTestCase()
	{
		// Use distinct QSettings
		QCoreApplication::setOrganizationName(QString::fromLatin1("OpenOrienteering.org"));
		QCoreApplication::setApplicationName(QString::fromLatin1(metaObject()->className()));
		QVERIFY2(QDir::home().exists(), "The home dir must be writable in order to use QSettings.");
		
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
		QVERIFY(temp->isTemplateGeoreferenced());
		
		QVERIFY(temp->getTemplateState() != Template::Invalid);
		QCOMPARE(temp->getTemplateRelativePath(), QStringLiteral("world-file.png"));
		QCOMPARE(temp->getTemplateRelativePath(nullptr), QStringLiteral("world-file.png"));
		auto base_dir = QDir(QStringLiteral("testdata:templates"));
		QCOMPARE(temp->getTemplateRelativePath(&base_dir), QStringLiteral("world-file.png"));
		base_dir.cdUp();
		QCOMPARE(temp->getTemplateRelativePath(&base_dir), QStringLiteral("templates/world-file.png"));
		
		map.loadTemplateFiles(view);
		QCOMPARE(temp->getTemplateState(), Template::Loaded);
		auto rotation_template = 0.01 * qRound(100 * qRadiansToDegrees(temp->getTemplateRotation()));
		auto rotation_map = 0.01 * qRound(100 * georef.getGrivation());
		QCOMPARE(rotation_template, rotation_map);
		
		// Test boundingRect for TemplateImage
		QCOMPARE(temp->boundingRect().center(), center(temp));
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
		
		QCOMPARE(temp->getTemplateRelativePath(nullptr), QStringLiteral("world-file.png"));
		auto base_dir = QDir(QStringLiteral("testdata:templates"));
		QCOMPARE(temp->getTemplateRelativePath(&base_dir), QStringLiteral("world-file.png"));
		base_dir.cdUp();
		QCOMPARE(temp->getTemplateRelativePath(&base_dir), QStringLiteral("world-file.png"));
		
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
		QVERIFY(temp->isTemplateGeoreferenced());
		
		map.loadTemplateFiles(view);
		QCOMPARE(temp->getTemplateState(), Template::Loaded);
		auto rotation_template = 0.01 * qRound(100 * qRadiansToDegrees(temp->getTemplateRotation()));
		auto rotation_map = 0.01 * qRound(100 * georef.getGrivation());
		QCOMPARE(rotation_template, rotation_map);
#endif
	}
	
	void templateTrackTest_data()
	{
		QTest::addColumn<QString>("map_file");
		QTest::addColumn<int>("template_index");

		QTest::newRow("TemplateTrack georef")         << QStringLiteral("testdata:templates/template-track.xmap") << 0;
		QTest::newRow("TemplateTrack non-georef")     << QStringLiteral("testdata:templates/template-track.xmap") << 1;
		QTest::newRow("TemplateTrack OGR georef")     << QStringLiteral("testdata:templates/template-track.xmap") << 2;
		QTest::newRow("TemplateTrack OGR non-georef") << QStringLiteral("testdata:templates/template-track.xmap") << 3;
	}
	
	void templateTrackTest()
	{
#ifdef MAPPER_USE_GDAL
		GdalManager().setFormatEnabled(GdalManager::GPX, false);
#endif
		
		QFETCH(QString, map_file);
		QFETCH(int, template_index);
		
		Map map;
		MapView view{ &map };
		QVERIFY(map.loadFrom(map_file, &view));
		
		QVERIFY(map.getNumTemplates() > template_index);
		auto const* temp = map.getTemplate(template_index);
		QCOMPARE(temp->getTemplateType(), "TemplateTrack");
		QCOMPARE(temp->getTemplateState(), Template::Unloaded);
		QVERIFY(map.getTemplate(template_index)->loadTemplateFile());
		QCOMPARE(temp->getTemplateState(), Template::Loaded);

		auto const expected_center = map.calculateExtent().center();
		if (QLineF(center(temp), expected_center).length() > 0.5)
			QCOMPARE(center(temp), expected_center);
		else
			QVERIFY2(true, "Centers do match");
	}
	
#ifdef MAPPER_USE_GDAL
	void ogrTemplateTest_data()
	{
		QTest::addColumn<QString>("map_file");
		QTest::addColumn<int>("template_index");

		QTest::newRow("TemplateTrack georef")         << QStringLiteral("testdata:templates/template-track.xmap") << 0;
		QTest::newRow("TemplateTrack non-georef")     << QStringLiteral("testdata:templates/template-track.xmap") << 1;
		QTest::newRow("TemplateTrack OGR georef")     << QStringLiteral("testdata:templates/template-track.xmap") << 2;
		QTest::newRow("TemplateTrack OGR non-georef") << QStringLiteral("testdata:templates/template-track.xmap") << 3;
		QTest::newRow("OgrTemplate basic")            << QStringLiteral("testdata:templates/ogr-template.xmap")   << 0;
		QTest::newRow("OgrTemplate compatibility")    << QStringLiteral("testdata:templates/ogr-template.xmap")   << 1;
	}
	
	void ogrTemplateTest()
	{
		GdalManager().setFormatEnabled(GdalManager::GPX, true);
		
		QFETCH(QString, map_file);
		QFETCH(int, template_index);
		
		Map map;
		MapView view{ &map };
		QVERIFY(map.loadFrom(map_file, &view));
		
		QVERIFY(map.getNumTemplates() > template_index);
		auto const* temp = map.getTemplate(template_index);
		QCOMPARE(temp->getTemplateType(), "OgrTemplate");
		QCOMPARE(temp->getTemplateState(), Template::Unloaded);
		QVERIFY(map.getTemplate(template_index)->loadTemplateFile());
		QCOMPARE(temp->getTemplateState(), Template::Loaded);

		auto const expected_center = map.calculateExtent().center();
		if (QLineF(center(temp), expected_center).length() > 0.5)
			QCOMPARE(center(temp), expected_center);
		else
			QVERIFY2(true, "Centers do match");
		
		// Test boundingRect for OgrTemplate
		QCOMPARE(temp->boundingRect().center(), center(temp));
	}
	
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
	
	void templateRotationTest_data()
	{
		QTest::addColumn<QString>("map_file");
		QTest::addColumn<int>("template_index");

		QTest::newRow("TemplateImage world file")  << QStringLiteral("testdata:templates/world-file.xmap")     << 0;
		QTest::newRow("TemplateTrack georef")      << QStringLiteral("testdata:templates/template-track.xmap") << 0;
		QTest::newRow("TemplateTrack non-georef")  << QStringLiteral("testdata:templates/template-track.xmap") << 1;
#ifdef MAPPER_USE_GDAL
		QTest::newRow("GdalImage geotiff")         << QStringLiteral("testdata:templates/geotiff.xmap")        << 0;
		QTest::newRow("OgrTemplate basic")         << QStringLiteral("testdata:templates/ogr-template.xmap")   << 0;
		QTest::newRow("OgrTemplate compatibility") << QStringLiteral("testdata:templates/ogr-template.xmap")   << 1;
#endif
	}
	
	void templateRotationTest()
	{
#ifdef MAPPER_USE_GDAL
		GdalManager().setFormatEnabled(GdalManager::GPX, false);
#endif
		
		QFETCH(QString, map_file);
		QFETCH(int, template_index);
		
		Map map;
		MapView view{ &map };
		QVERIFY(map.loadFrom(map_file, &view));
		
		auto const& georef = map.getGeoreferencing();
		QVERIFY(georef.isValid());
		QVERIFY(std::abs(georef.getGrivation()) >= 0.01);
		
		QVERIFY(map.getNumTemplates() > template_index);
		auto const* temp = map.getTemplate(template_index);
		QCOMPARE(temp->getTemplateState(), Template::Unloaded);
		
		// A clone of temp which is loaded before map rotation
		auto* loaded_clone = temp->duplicate();
		map.addTemplate(map.getNumTemplates(), std::unique_ptr<Template>(loaded_clone));
		if (loaded_clone->getTemplateState() != Template::Loaded)
			QVERIFY(loaded_clone->loadTemplateFile());
		QCOMPARE(loaded_clone->getTemplateState(), Template::Loaded);
		
		// A clone of temp which is loaded and unloaded before map rotation
		auto* reloaded_clone = temp->duplicate();
		map.addTemplate(map.getNumTemplates(), std::unique_ptr<Template>(reloaded_clone));
		if (reloaded_clone->getTemplateState() != Template::Loaded)
			QVERIFY(reloaded_clone->loadTemplateFile());
		reloaded_clone->unloadTemplateFile();
		QCOMPARE(reloaded_clone->getTemplateState(), Template::Unloaded);
		
		auto initially_georeferenced = temp->isTemplateGeoreferenced();
		auto initial_rotation = temp->getTemplateRotation();  // radians
		if (initially_georeferenced)
			QCOMPARE(initial_rotation, 0.0);
		
		// ROTATION
		auto const ref_point = map.getGeoreferencing().getMapRefPoint();
		map.rotateMap(0.1, ref_point, true, true, true);  // radians
		
		if (!initially_georeferenced)
			QCOMPARE(temp->getTemplateRotation(), initial_rotation + 0.1);  // radians
		
		// TEMPLATE LOADING
		QVERIFY(map.getTemplate(template_index)->loadTemplateFile());
		QCOMPARE(temp->getTemplateState(), Template::Loaded);
		
		QVERIFY(reloaded_clone->loadTemplateFile());
		QCOMPARE(reloaded_clone->getTemplateState(), Template::Loaded);
		
		// The template rectangle is in the interior of the map figure.
		auto const expected_center = map.calculateExtent().center();
		// Template instance loaded only after configuration
		if (QLineF(center(temp), expected_center).length() > 0.5)
			QCOMPARE(center(temp), expected_center);
		else
			QVERIFY2(true, "Centers do match.");
		
		// Template instance loaded before rotation
		if (QLineF(center(loaded_clone), expected_center).length() > 0.5)
			QCOMPARE(center(loaded_clone), expected_center);
		else
			QVERIFY2(true, "Centers do match.");
		
		// Template instance loaded+unloaded before, and loaded again after, rotation
		if (QLineF(center(reloaded_clone), expected_center).length() > 0.5)
			QCOMPARE(center(reloaded_clone), expected_center);
		else
			QVERIFY2(true, "Centers do match.");
	}
	
	void asyncLoadingTest_data()
	{
		QTest::addColumn<QString>("map_file");
		QTest::addColumn<int>("template_index");
		
		QTest::newRow("TemplateImage")             << QStringLiteral("testdata:templates/world-file.xmap")     << 0;
		QTest::newRow("TemplateTrack georef")      << QStringLiteral("testdata:templates/template-track.xmap") << 0;
		QTest::newRow("TemplateTrack non-georef")  << QStringLiteral("testdata:templates/template-track.xmap") << 1;
#ifdef MAPPER_USE_GDAL
		QTest::newRow("OgrTemplate basic")         << QStringLiteral("testdata:templates/ogr-template.xmap")   << 0;
		QTest::newRow("OgrTemplate compatibility") << QStringLiteral("testdata:templates/ogr-template.xmap")   << 1;
#endif
	}
	
	void asyncLoadingTest()
	{
#ifdef MAPPER_USE_GDAL
		GdalManager().setFormatEnabled(GdalManager::GPX, false);
#endif
		
		QFETCH(QString, map_file);
		QFETCH(int, template_index);
		
		Map map;
		MapView view{ &map };
		QVERIFY(map.loadFrom(map_file, &view));
		QVERIFY(map.getNumTemplates() > template_index);
		
		auto* temp = map.getTemplate(template_index);
		QCOMPARE(temp->getTemplateState(), Template::Unloaded);
		view.setTemplateVisibility(temp, TemplateVisibility{1, true});
		
		QStringList messages;
		messages.reserve(10);
		map.loadTemplateFilesAsync(view, [&messages](const QString& msg){
			messages.push_back(msg);
		});
		QSignalSpy(temp, &Template::templateStateChanged).wait();
		QCOMPARE(temp->getTemplateState(), Template::Loaded);
		QVERIFY(messages.size() >= 2);
		QVERIFY(!messages.front().isEmpty());
		QVERIFY(messages.back().isEmpty());
	}
	
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
