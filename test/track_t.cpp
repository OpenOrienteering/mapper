/*
 *    Copyright 2018, 2024 Kai Pastor
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

#include <Qt>
#include <QtGlobal>
#include <QtTest>
#include <QByteArray>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QLatin1String>
#include <QMetaObject>
#include <QObject>
#include <QString>

#include "global.h"
#include "test_config.h"
#include "core/track.h"

using namespace OpenOrienteering;


/**
 * @test Tests GPX tracks.
 */
class TrackTest : public QObject
{
	Q_OBJECT
	
	QDateTime base_datetime = QDateTime::fromMSecsSinceEpoch(0, Qt::UTC).addYears(40);
	
private slots:
	void initTestCase()
	{
		// Use distinct QSettings
		QCoreApplication::setOrganizationName(QString::fromLatin1("OpenOrienteering.org"));
		QCoreApplication::setApplicationName(QString::fromLatin1(metaObject()->className()));
		QVERIFY2(QDir::home().exists(), "The home dir must be writable in order to use QSettings.");
		
		QDir::addSearchPath(QStringLiteral("testdata"), QDir(QString::fromUtf8(MAPPER_TEST_SOURCE_DIR)).absoluteFilePath(QStringLiteral("data")));
		doStaticInitializations();
	}
	
	
	void gpxTest_data()
	{
		struct
		{
			const char* path_out;  // expected output
			const char* path_in;   // input to be preloaded
			const int offset;      // seconds to be added to new timestamps
		} const track_test_files[] = {
			{ "testdata:track/track-0.gpx", nullptr, 0 },
			{ "testdata:track/track-1.gpx", "testdata:track/track-0.gpx", 60 },
		};
		
		QTest::addColumn<QString>("filename_out");
		QTest::addColumn<QString>("filename_in");
		QTest::addColumn<int>("offset");
		for (auto test_file : track_test_files)
		{
			QTest::newRow(test_file.path_out) << QString::fromUtf8(test_file.path_out)
			                                  << QString::fromUtf8(test_file.path_in)
			                                  << test_file.offset;
		}
	}
	
	void gpxTest()
	{
		QFETCH(QString, filename_out);
		QFETCH(QString, filename_in);
		QFETCH(int, offset);
		
		auto actual_track = Track{};
		if (!filename_in.isEmpty())
			QVERIFY(actual_track.loadFrom(filename_in, true));
		
		const auto waypoint_name = filename_out.right(11);
		const auto wp0 = TrackPoint{ {50.0, 7.1}, base_datetime.addSecs(offset + 0), 105, 24};
		actual_track.appendWaypoint(wp0, waypoint_name);
		
		const auto tp0 = TrackPoint{ {50.0, 7.0}, base_datetime.addSecs(offset + 1), 100};
		actual_track.appendTrackPoint(tp0);
		const auto tp1 = TrackPoint{ {50.1, 7.0}, base_datetime.addSecs(offset + 2), 110, 28 };
		actual_track.appendTrackPoint(tp1);
		const auto tp2 = TrackPoint{ {50.1, 7.1}, base_datetime.addSecs(offset + 3), NAN, 32 };
		actual_track.appendTrackPoint(tp2);
		actual_track.finishCurrentSegment();
		
		const auto tp3 = TrackPoint{ {50.0, 7.1}, base_datetime.addSecs(offset + 9), 105 };
		actual_track.appendTrackPoint(tp3);
		actual_track.finishCurrentSegment();
		
		auto filename_tmp = QFileInfo(filename_out).fileName();
		filename_tmp.insert(filename_tmp.length() - 4, QLatin1String(".tmp"));
		QVERIFY(actual_track.saveTo(filename_tmp));
		
		auto readAll = [](QFile&& file) -> QByteArray {
			file.open(QIODevice::ReadOnly | QIODevice::Text);
			return file.readAll();
		};
		const auto raw_tmp = readAll(QFile(filename_tmp));
		QVERIFY(!raw_tmp.isEmpty());
		const auto raw_expected = readAll(QFile(filename_out));
		QVERIFY(!raw_expected.isEmpty());
		QCOMPARE(raw_tmp, raw_expected);
		
		auto expected_track = Track{};
		QVERIFY(expected_track.loadFrom(filename_out, true));
		QCOMPARE(actual_track, expected_track);
		
		QVERIFY(QFile::remove(filename_tmp));
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


QTEST_MAIN(TrackTest)
#include "track_t.moc"  // IWYU pragma: keep
