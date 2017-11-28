/*
*    Copyright 2017 Libor Pecháček
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

#include "georef_ocd_mapping_t.h"

#include <list>

#include <QBuffer>
#include <QString>
#include <QTest>

#include "core/georeferencing.h"
#include "fileformats/ocd_georef.h"
#include "fileformats/xml_file_format.h"

// mock stuff to satisfy link-time dependencies
int XMLFileFormat::active_version = 6;

void GeoreferenceMappingTest::initTestCase()
{
    // empty
}


void GeoreferenceMappingTest::testOcdToMapper_data()
{
	QTest::addColumn<QString>("si_ScalePar");
	QTest::addColumn<int>("ocd_version"); // <Version><3 digit Subversion>
	QTest::addColumn<QString>("georef_result");
	QTest::addColumn<bool>("warnings_expected");

	QTest::newRow("Sibelius Monumentti - Finland, TM35FIN, ETRS89/ETRS-TM35FIN")
	        << QStringLiteral("\tm15000\tg50.0000\tr1\tx384268\ty6673512\ta10.70000000\td1000.000000\ti6005\tb0.00\tc0.00")
	        << 11006
	        << QStringLiteral("1;10.7;10.7;60.18202642,24.91341508;EPSG:3067")
	        << false;
	QTest::newRow("CERN - Switzerland, CH1903")
	        << QStringLiteral("\tm15000\tg50.0000\tr1\tx495133\ty129609\ta2.72000000\td1000.000000\ti14001\tb0.00\tc0.00")
	        << 11006
	        << QStringLiteral("1;2.72;2.72;46.30967927,6.07729237;EPSG:21781")
	        << false;
	QTest::newRow("Aldasin - Czech Republic, S-JTSK/Krovak")
	        << QStringLiteral("\tm10000\tg1000\tr1\tx-716000\ty-1061000\ta10.82000000\td1000\ti46001")
	        << 10003
	        << QStringLiteral("1;10.82;10.82;0.00000000,0.00000000;Local:")
	        << true;
	QTest::newRow("Launch Pad 39A - USA, Florida, WGS 84/UTM zone 17")
	        << QStringLiteral("\tm15000\tg50.0000\tr1\tx538705\ty3164567\ta-7.06000000\td1000.000000\ti2017\tb0.00\tc0.00")
	        << 11006
	        << QStringLiteral("1;-7.06;-7.06;28.60751559,-80.60410576;UTM:17")
	        << false;
	QTest::newRow("Brandenburger Tor - Germany, DHDN/3-degree Gauss-Kruger zone 4")
	        << QStringLiteral("\tm15000\tg50.0000\tr1\tx4571442\ty5807888\ta2.84\td1000\ti8004\tb0.00\tc0.00")
	        << 11006
	        << QStringLiteral("1;2.84;2.84;52.39964048,13.04810920;Gauss-Krueger, datum: Potsdam:4")
	        << false;
	QTest::newRow("Brandenburger Tor - Germany, WGS 84/UTM zone 33N")
	        << QStringLiteral("\tm15000\tg50.0000\tr1\tx367205\ty5807281\ta5.22000000\td1000.000000\ti2033\tb0.00\tc0.00")
	        << 11006
	        << QStringLiteral("1;5.22;5.22;52.39963782,13.04811373;UTM:33")
	        << false;
}


void GeoreferenceMappingTest::testOcdToMapper()
{
	QFETCH(QString, si_ScalePar);
	QFETCH(QString, georef_result);
	QFETCH(bool, warnings_expected);

	std::list<QString> warnings;
	auto gref = OcdGeoref::georefFromString(si_ScalePar, warnings);
	QVERIFY(bool(warnings.size()) == warnings_expected);

	QStringList crs_params;
	for (auto s : gref.getProjectedCRSParameters())
		crs_params.push_back(s);

	auto georef_string = QString(QStringLiteral("%1;%2;%3;%4,%5;%6:%7"))
	                     .arg(gref.getGridScaleFactor())
	                     .arg(gref.getDeclination())
	                     .arg(gref.getGrivation())
	                     .arg(gref.getGeographicRefPoint().latitude(), 0, 'f', 8)
	                     .arg(gref.getGeographicRefPoint().longitude(), 0, 'f', 8)
	                     .arg(gref.getProjectedCRSId())
	                     .arg(crs_params.join(QLatin1Char(',')));

	//std::cout << georef_string;
	QCOMPARE(georef_string, georef_result);
}

QTEST_GUILESS_MAIN(GeoreferenceMappingTest)
