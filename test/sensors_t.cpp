/*
 *    Copyright 2018 Kai Pastor
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
#include <QObject>
#include <QStandardPaths> // IWYU pragma: keep

namespace OpenOrienteering {};
using namespace OpenOrienteering;


#ifdef MAPPER_USE_POWERSHELL_POSITION_PLUGIN
#include <QGeoPositionInfoSource>  // IWYU pragma: keep
#include "sensors/powershell_position_source.h"
#endif


class SensorsTest : public QObject
{
	Q_OBJECT
	
private slots:
	
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
