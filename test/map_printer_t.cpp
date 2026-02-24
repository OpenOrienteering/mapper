/*
 *    Copyright 2020 Kai Pastor
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
#include <QtTest>
#include <QObject>
#include <QPrinterInfo>
#include <QString>

#include "core/map_printer.h"

using namespace OpenOrienteering;


/**
 * @test Tests printing and export features
 */
class MapPrinterTest : public QObject
{
Q_OBJECT
private slots:
	void isPrinterTest()
	{
		QPrinterInfo printer_info;
		QCOMPARE(MapPrinter::isPrinter(&printer_info), true);
		QCOMPARE(MapPrinter::isPrinter(MapPrinter::imageTarget()), false);
		QCOMPARE(MapPrinter::isPrinter(MapPrinter::kmzTarget()), false);
		QCOMPARE(MapPrinter::isPrinter(MapPrinter::pdfTarget()), false);
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


QTEST_MAIN(MapPrinterTest)
#include "map_printer_t.moc"  // IWYU pragma: keep
