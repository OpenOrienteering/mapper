/*
 *    Copyright 2017 Kai Pastor
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
#include "templates/template.h"
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
		WorldFile world_file;
		QVERIFY(!world_file.loaded);
		QCOMPARE(world_file.pixel_to_world, QTransform{});
		
		QString world_file_path = QStringLiteral("testdata:templates/world-file.pgw");
		QVERIFY(world_file.load(world_file_path));
		QVERIFY(world_file.loaded);
		QCOMPARE(world_file.pixel_to_world.m11(), 0.15);
		QCOMPARE(world_file.pixel_to_world.m12(), 0.0);
		QCOMPARE(world_file.pixel_to_world.m21(), 0.0);
		QCOMPARE(world_file.pixel_to_world.m22(), -0.15);
		QCOMPARE(world_file.pixel_to_world.dx(), 649078.0);
		QCOMPARE(world_file.pixel_to_world.dy(), 394159.0);
		
		QString image_path = QStringLiteral("testdata:templates/world-file.png");
		WorldFile world_file_from_image;
		QVERIFY(world_file_from_image.tryToLoadForImage(image_path));
		QCOMPARE(world_file_from_image.pixel_to_world, world_file.pixel_to_world);
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


QTEST_MAIN(TemplateTest)
#include "template_t.moc"  // IWYU pragma: keep
