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


#include <QtTest/QtTest>

#include <QDir>

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
};



QTEST_APPLESS_MAIN(TemplateTest)
#include "template_t.moc"
