/*
 *    Copyright 2019-2020 Kai Pastor
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

#include <QtTest>
#include <QFile>          // IWYU pragma: keep
#include <QObject>
#include <QSignalSpy>     // IWYU pragma: keep
#include <QStandardPaths> // IWYU pragma: keep
#include <QStaticPlugin>  // IWYU pragma: keep
#include <QTemporaryFile> // IWYU pragma: keep

#include "test_config.h"  // IWYU pragma: keep

#include "util/backports.h"  // IWYU pragma: keep

namespace OpenOrienteering {};
using namespace OpenOrienteering;


#ifdef QT_POSITIONING_LIB
#include <QGeoPositionInfoSource>
#endif

#ifdef MAPPER_USE_NMEA_POSITION_PLUGIN
#include <QNmeaPositionInfoSource>  // IWYU pragma: keep
#include "sensors/nmea_position_plugin.h"
Q_IMPORT_PLUGIN(NmeaPositionPlugin)
#endif

#ifdef MAPPER_USE_POWERSHELL_POSITION_PLUGIN
#include "sensors/powershell_position_source.h"
#endif


class SensorsTest : public QObject
{
	Q_OBJECT
	
private slots:
	
#if defined(MAPPER_USE_NMEA_POSITION_PLUGIN)
#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
	void nmeaPositionSourceTest()
	{
		auto nmea_position_source = QString::fromLatin1("NMEA (OpenOrienteering)");
		auto sources = QGeoPositionInfoSource::availableSources();
		QVERIFY(sources.contains(nmea_position_source));  // JSON file properties
		
		auto* source = QGeoPositionInfoSource::createSource(nmea_position_source, this);
		QVERIFY(source);
		QVERIFY(qobject_cast<QNmeaPositionInfoSource*>(source));
		QCOMPARE(int(source->error()), int(QGeoPositionInfoSource::NoError));
		
		QSignalSpy source_spy(source, &QGeoPositionInfoSource::positionUpdated);
		QVERIFY(source_spy.isValid());
		
		auto* test_file = MAPPER_TEST_SOURCE_DIR "/data/nmea.txt";
		QVERIFY(QFile::exists(QString::fromUtf8(test_file)));
		qputenv("QT_NMEA_SERIAL_PORT", test_file);
		source->startUpdates();
		QCOMPARE(int(source->error()), int(QGeoPositionInfoSource::NoError));
		QVERIFY(source_spy.wait());
		
		auto last = source->lastKnownPosition(true);
		QVERIFY(last.isValid());
		QCOMPARE(int(last.coordinate().latitude()), -30);
		QCOMPARE(int(last.coordinate().longitude()), 139);
		
		source->stopUpdates();
		
		QTemporaryFile unreadable_file;
		QVERIFY(unreadable_file.open());
		unreadable_file.close();
		unreadable_file.setPermissions({});
		qputenv("QT_NMEA_SERIAL_PORT", unreadable_file.fileName().toUtf8());
		source->startUpdates();
		QCOMPARE(int(source->error()), int(QGeoPositionInfoSource::AccessError));
		
		delete source;
	}

#else
	void nmeaPositionSourceUnsupportedTest()
	{
		auto sources = QGeoPositionInfoSource::availableSources();
		QVERIFY(sources.contains(QStringLiteral("NMEA (OpenOrienteering)")));  // JSON file properties
		
		auto* source = QGeoPositionInfoSource::createSource(QStringLiteral("NMEA (OpenOrienteering)"), this);
		QVERIFY(!source || source->error() == QGeoPositionInfoSource::UnknownSourceError);
	}
#endif
#endif  // MAPPER_USE_NMEA_POSITION_PLUGIN
	
#if defined(MAPPER_USE_POWERSHELL_POSITION_PLUGIN)
#ifdef Q_OS_WIN
	void powershellPositionSourceWindowsTest()
	{
		QVERIFY(!PowershellPositionSource::defaultScript().isEmpty());
		
		PowershellPositionSource source;
		QVERIFY(!source.script().isEmpty());
		if (QStandardPaths::findExecutable(QLatin1String("powershell.exe")).isEmpty())
			QCOMPARE(source.error(), QGeoPositionInfoSource::UnknownSourceError);
		else
			QCOMPARE(source.error(), QGeoPositionInfoSource::NoError);
	}

#else
	void powershellPositionSourceOtherTest()
	{
		// The resource file shall not be linked.
		QVERIFY(PowershellPositionSource::defaultScript().isEmpty());
		
		PowershellPositionSource source;
		QVERIFY(source.script().isEmpty());
		QCOMPARE(source.error(), QGeoPositionInfoSource::UnknownSourceError);
		source.startUpdates();
		QCOMPARE(source.error(), QGeoPositionInfoSource::UnknownSourceError);
	}
#endif
#endif  // MAPPER_USE_POWERSHELL_POSITION_PLUGIN
	
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


QTEST_MAIN(SensorsTest)
#include "sensors_t.moc"  // IWYU pragma: keep
