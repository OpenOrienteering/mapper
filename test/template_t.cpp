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

#include "global.h"
#include "core/georeferencing.h"
#include "core/map.h"
#include "core/map_view.h"
#include "templates/template.h"
#include "templates/world_file.h"


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
		QDir::addSearchPath(QStringLiteral("testdata"), QFileInfo(QString::fromUtf8(__FILE__)).dir().absoluteFilePath(QStringLiteral("data")));
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
		QVERIFY(map.loadFrom(QStringLiteral("testdata:templates/world-file.xmap"), nullptr, &view, false, false));
		
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
	
};



/*
 * We don't need a real GUI window.
 * 
 * But we discovered QTBUG-58768 macOS: Crash when using QPrinter
 * while running with "minimal" platform plugin.
 */
#ifndef Q_OS_MACOS
namespace  {
	auto qpa_selected = qputenv("QT_QPA_PLATFORM", "minimal");
}
#endif


QTEST_MAIN(TemplateTest)
#include "template_t.moc"  // IWYU pragma: keep
